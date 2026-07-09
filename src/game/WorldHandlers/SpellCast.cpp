/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file SpellCast.cpp
 * @brief Cohesion split of Spell.cpp -- cast pipeline (prepare/cast/finish).
 *        Same `Spell` class; no behaviour change.
 */

#include "Spell.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Pet.h"
#include "Unit.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "Policies/Singleton.h"
#include "SharedDefines.h"
#include "LootMgr.h"
#include "VMapFactory.h"
#include "BattleGround/BattleGround.h"
#include "Util.h"
#include "Chat.h"
#include "Vehicle.h"
#include "TemporarySummon.h"
#include "SQLStorages.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /*ENABLE_ELUNA*/

/**
 * @brief Cancels the spell and sends the appropriate interruption notifications.
 */
void Spell::cancel()
{
    if (m_spellState == SPELL_STATE_FINISHED)
    {
        return;
    }

    // channeled spells don't display interrupted message even if they are interrupted, possible other cases with no "Interrupted" message
    bool sendInterrupt = IsChanneledSpell(m_spellInfo) ? false : true;

    m_autoRepeat = false;
    switch (m_spellState)
    {
        case SPELL_STATE_PREPARING:
            CancelGlobalCooldown();

            //(no break)
        case SPELL_STATE_DELAYED:
        {
            SendInterrupted(0);

            if (sendInterrupt)
            {
                SendCastResult(SPELL_FAILED_INTERRUPTED);
            }
            break;
        }
        case SPELL_STATE_CASTING:
        {
            for (TargetList::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            {
                if (ihit->missCondition == SPELL_MISS_NONE)
                {
                    Unit* unit = m_caster->GetObjectGuid() == (*ihit).targetGUID ? m_caster : sObjectAccessor.GetUnit(*m_caster, ihit->targetGUID);
                    if (unit && unit->IsAlive())
                    {
                        unit->RemoveAurasByCasterSpell(m_spellInfo->ID, m_caster->GetObjectGuid());
                        return;
                    }

                    // prevent other effects applying if spell is already interrupted
                    // i.e. if effects have different targets and it was interrupted on one of them when
                    // haven't yet applied to another
                    ihit->processed = true;
                }
            }

            SendChannelUpdate(0);
            SendInterrupted(0);

            if (sendInterrupt)
            {
                SendCastResult(SPELL_FAILED_INTERRUPTED);
            }
            break;
        }
        default:
        {
            break;
        }
    }

    finish(false);
    m_caster->RemoveDynObject(m_spellInfo->ID);
    m_caster->RemoveGameObject(m_spellInfo->ID, true);
}

/**
 * @brief Executes the spell cast after preparation has completed.
 *
 * @param skipCheck True to skip the second cast-condition validation.
 */
void Spell::cast(bool skipCheck)
{
    SetExecutedCurrently(true);

    if (!m_caster->CheckAndIncreaseCastCounter())
    {
        if (m_triggeredByAuraSpell)
        {
            sLog.outError("Spell %u triggered by aura spell %u too deep in cast chain for cast. Cast not allowed for prevent overflow stack crash.", m_spellInfo->ID, m_triggeredByAuraSpell->ID);
        }
        else
        {
            sLog.outError("Spell %u too deep in cast chain for cast. Cast not allowed for prevent overflow stack crash.", m_spellInfo->ID);
        }

        SendCastResult(SPELL_FAILED_ERROR);
        finish(false);
        SetExecutedCurrently(false);
        return;
    }

    // update pointers base at GUIDs to prevent access to already nonexistent object
    UpdatePointers();

    // cancel at lost main target unit
    if (!m_targets.getUnitTarget() && m_targets.getUnitTargetGuid() && m_targets.getUnitTargetGuid() != m_caster->GetObjectGuid())
    {
        cancel();
        m_caster->DecreaseCastCounter();
        SetExecutedCurrently(false);
        return;
    }

    if (m_caster->GetTypeId() != TYPEID_PLAYER && m_targets.getUnitTarget() && m_targets.getUnitTarget() != m_caster)
    {
        m_caster->SetInFront(m_targets.getUnitTarget());
    }

    SpellCastResult castResult = CheckPower();
    if (castResult != SPELL_CAST_OK)
    {
        SendCastResult(castResult);
        finish(false);
        m_caster->DecreaseCastCounter();
        SetExecutedCurrently(false);
        return;
    }

    // triggered cast called from Spell::prepare where it was already checked
    if (!skipCheck)
    {
        castResult = CheckCast(false);
        if (castResult != SPELL_CAST_OK)
        {
            SendCastResult(castResult);
            finish(false);
            m_caster->DecreaseCastCounter();
            SetExecutedCurrently(false);
            return;
        }
    }

    // different triggered (for caster and main target after main cast) and pre-cast (casted before apply effect to each target) cases
    switch (m_spellInfo->SpellClassSet)
    {
        case SPELLFAMILY_GENERIC:
        {
            // Bandages
            if (m_spellInfo->Mechanic == MECHANIC_BANDAGE)
            {
                AddPrecastSpell(11196);                      // Recently Bandaged
            }
            // Stoneskin
            else if (m_spellInfo->ID == 20594)
                AddTriggeredSpell(65116);                   // Stoneskin - armor 10% for 8 sec
            // Chaos Bane strength buff
            else if (m_spellInfo->ID == 71904)
            {
                AddTriggeredSpell(73422);
            }
            // Weak Alcohol
            else if (m_spellInfo->SpellIconID == 1306 && m_spellInfo->SpellVisualID[0] == 11359)
            {
                AddTriggeredSpell(51655);                   // BOTM - Create Empty Brew Bottle
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // Ice Block
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000008000000000))
            {
                AddPrecastSpell(41425);                     // Hypothermia
            }
            // Icy Veins
            else if (m_spellInfo->ID == 12472)
            {
                if (m_caster->HasAura(56374))               // Glyph of Icy Veins
                {
                    // not exist spell do it so apply directly
                    m_caster->RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
                    m_caster->RemoveSpellsCausingAura(SPELL_AURA_HASTE_SPELLS);
                }
            }
            // Fingers of Frost
            else if (m_spellInfo->ID == 44544)
            {
                AddPrecastSpell(74396);                     // Fingers of Frost
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Shield Slam
            if ((m_spellInfo->SpellClassMask & UI64LIT(0x0000020000000000)) && m_spellInfo->Category == 1209)
            {
                if (m_caster->HasAura(58375))               // Glyph of Blocking
                {
                    AddTriggeredSpell(58374);               // Glyph of Blocking
                }
            }
            // Bloodrage
            if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000000100))
            {
                if (m_caster->HasAura(70844))               // Item - Warrior T10 Protection 4P Bonus
                {
                    AddTriggeredSpell(70845);               // Stoicism
                }
            }
            // Bloodsurge (triggered), Sudden Death (triggered)
            else if (m_spellInfo->ID == 46916 || m_spellInfo->ID == 52437)
            {
                // Item - Warrior T10 Melee 4P Bonus
                if (Aura* aur = m_caster->GetAura(70847, EFFECT_INDEX_0))
                {
                    if (roll_chance_i(aur->GetModifier()->m_amount))
                    {
                        AddTriggeredSpell(70849);           // Extra Charge!
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Power Word: Shield
            if (m_spellInfo->Mechanic == MECHANIC_SHIELD &&
                    (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000000001)))
            {
                AddPrecastSpell(6788);                      // Weakened Soul
            }
            // Prayer of Mending (jump animation), we need formal caster instead original for correct animation
            else if (m_spellInfo->SpellClassMask & UI64LIT(0x0000002000000000))
            {
                AddTriggeredSpell(41637);
            }

            switch (m_spellInfo->ID)
            {
                case 15237: AddTriggeredSpell(23455); break;// Holy Nova, rank 1
                case 15430: AddTriggeredSpell(23458); break;// Holy Nova, rank 2
                case 15431: AddTriggeredSpell(23459); break;// Holy Nova, rank 3
                case 27799: AddTriggeredSpell(27803); break;// Holy Nova, rank 4
                case 27800: AddTriggeredSpell(27804); break;// Holy Nova, rank 5
                case 27801: AddTriggeredSpell(27805); break;// Holy Nova, rank 6
                case 25331: AddTriggeredSpell(25329); break;// Holy Nova, rank 7
                case 48077: AddTriggeredSpell(48075); break;// Holy Nova, rank 8
                case 48078: AddTriggeredSpell(48076); break;// Holy Nova, rank 9
                default: break;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Faerie Fire (Feral)
            if (m_spellInfo->ID == 16857 && m_caster->GetShapeshiftForm() != FORM_CAT)
            {
                AddTriggeredSpell(60089);
            }
            // Clearcasting
            else if (m_spellInfo->ID == 16870)
            {
                if (m_caster->HasAura(70718))               // Item - Druid T10 Balance 2P Bonus
                {
                    AddPrecastSpell(70721);                 // Omen of Doom
                }
            }
            // Berserk (Bear Mangle part)
            else if (m_spellInfo->ID == 50334)
            {
                AddTriggeredSpell(58923);
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
            // Fan of Knives (main hand)
            if (m_spellInfo->ID == 51723 && m_caster->GetTypeId() == TYPEID_PLAYER &&
                    ((Player*)m_caster)->haveOffhandWeapon())
            {
                AddTriggeredSpell(52874);                   // Fan of Knives (offhand)
            }
            break;
        case SPELLFAMILY_HUNTER:
        {
            // Deterrence
            if (m_spellInfo->ID == 19263)
            {
                AddPrecastSpell(67801);
            }
            // Kill Command
            else if (m_spellInfo->ID == 34026)
            {
                if (m_caster->HasAura(37483))               // Improved Kill Command - Item set bonus
                {
                    m_caster->CastSpell(m_caster, 37482, true);// Exploited Weakness
                }
            }
            // Lock and Load
            else if (m_spellInfo->ID == 56453)
            {
                AddPrecastSpell(67544);                     // Lock and Load Marker
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Divine Illumination
            if (m_spellInfo->ID == 31842)
            {
                if (m_caster->HasAura(70755))               // Item - Paladin T10 Holy 2P Bonus
                {
                    AddPrecastSpell(71166);                 // Divine Illumination
                }
            }
            // Hand of Reckoning
            else if (m_spellInfo->ID == 62124)
            {
                if (!m_targets.getUnitTarget() || m_targets.getUnitTarget()->GetTargetGuid() != m_caster->GetObjectGuid())
                {
                    AddPrecastSpell(67485);                 // Hand of Rekoning (no typos in name ;) )
                }
            }
            // Divine Shield, Divine Protection or Hand of Protection
            else if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000400080))
            {
                AddPrecastSpell(25771);                     // Forbearance
                AddPrecastSpell(61987);                     // Avenging Wrath Marker
            }
            // Lay on Hands
            else if (m_spellInfo->SpellClassMask & UI64LIT(0x0000000000008000))
            {
                // only for self cast
                if (m_caster == m_targets.getUnitTarget())
                {
                    AddPrecastSpell(25771);                 // Forbearance
                }
            }
            // Avenging Wrath
            else if (m_spellInfo->SpellClassMask & UI64LIT(0x0000200000000000))
            {
                AddPrecastSpell(61987);                     // Avenging Wrath Marker
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Bloodlust
            if (m_spellInfo->ID == 2825)
            {
                AddPrecastSpell(57724);                     // Sated
            }
            // Heroism
            else if (m_spellInfo->ID == 32182)
            {
                AddPrecastSpell(57723);                     // Exhaustion
            }
            // Spirit Walk
            else if (m_spellInfo->ID == 58875)
            {
                AddPrecastSpell(58876);
            }
            // Totem of Wrath
            else if (m_spellInfo->Effect[EFFECT_INDEX_0] == SPELL_EFFECT_APPLY_AREA_AURA_RAID && m_spellInfo->SpellClassMask & UI64LIT(0x0000000004000000))
            {
                // only for main totem spell cast
                AddTriggeredSpell(30708);                   // Totem of Wrath
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Chains of Ice
            if (m_spellInfo->ID == 45524)
            {
                AddTriggeredSpell(55095);                   // Frost Fever
            }
            break;
        }
        default:
            break;
    }

    // traded items have trade slot instead of guid in m_itemTargetGUID
    // set to real guid to be sent later to the client
    m_targets.updateTradeSlotItem();

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (!m_IsTriggeredSpell && m_CastItem)
        {
            ((Player*)m_caster)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM, m_CastItem->GetEntry());
        }

        ((Player*)m_caster)->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL, m_spellInfo->ID);
    }

#ifdef ENABLE_ELUNA
    if (Eluna* e = m_caster->GetEluna())
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            e->OnSpellCast(m_caster->ToPlayer(), this, skipCheck);
        }
    }
#endif /* ENABLE_ELUNA */

    FillTargetMap();

    if (m_spellState == SPELL_STATE_FINISHED)               // stop cast if spell marked as finish somewhere in FillTargetMap
    {
        m_caster->DecreaseCastCounter();
        SetExecutedCurrently(false);
        return;
    }

    // CAST SPELL
    SendSpellCooldown();

    TakePower();
    TakeReagents();                                         // we must remove reagents before HandleEffects to allow place crafted item in same slot
    TakeAmmo();

    SendCastResult(castResult);
    SendSpellGo();                                          // we must send smsg_spell_go packet before m_castItem delete in TakeCastItem()...

    InitializeDamageMultipliers();

    Unit* procTarget = m_targets.getUnitTarget();
    if (!procTarget)
    {
        procTarget = m_caster;
    }

    // Okay, everything is prepared. Now we need to distinguish between immediate and evented delayed spells
    float speed = m_spellInfo->Speed == 0.0f && m_triggeredBySpellInfo ? m_triggeredBySpellInfo->Speed : m_spellInfo->Speed;
    if (speed > 0.0f)
    {
        // Remove used for cast item if need (it can be already NULL after TakeReagents call
        // in case delayed spell remove item at cast delay start
        TakeCastItem();

        // fill initial spell damage from caster for delayed casted spells
        for (TargetList::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
        {
            HandleDelayedSpellLaunch(&(*ihit));
        }

        // Okay, maps created, now prepare flags
        m_immediateHandled = false;
        m_spellState = SPELL_STATE_DELAYED;
        SetDelayStart(0);

        // on spell cast end proc,
        // critical hit related part is currently done on hit so proc there,
        // 0 damage since any damage based procs should be on hit
        // 0 victim proc since there is no victim proc dependent on successfull cast for caster
        m_caster->ProcDamageAndSpell(procTarget, m_procAttacker, 0, PROC_EX_CAST_END, 0, m_attackType, m_spellInfo);
    }
    else
    {
        m_caster->ProcDamageAndSpell(procTarget, m_procAttacker, 0, PROC_EX_CAST_END, 0, m_attackType, m_spellInfo);

        // Immediate spell, no big deal
        handle_immediate();
    }

    m_caster->DecreaseCastCounter();
    SetExecutedCurrently(false);
}

/**
 * @brief Handles the full execution path for an immediate spell.
 */
void Spell::handle_immediate()
{
    // process immediate effects (items, ground, etc.) also initialize some variables
    _handle_immediate_phase();

    // start channeling if applicable (after _handle_immediate_phase for get persistent effect dynamic object for channel target
    if (IsChanneledSpell(m_spellInfo) && m_duration)
    {
        m_spellState = SPELL_STATE_CASTING;
        SendChannelStart(m_duration);
    }

    for (TargetList::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        DoAllEffectOnTarget(&(*ihit));
    }

    for (GOTargetList::iterator ihit = m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
    {
        DoAllEffectOnTarget(&(*ihit));
    }

    // spell is finished, perform some last features of the spell here
    _handle_finish_phase();

    // Remove used for cast item if need (it can be already NULL after TakeReagents call
    TakeCastItem();

    if (m_spellState != SPELL_STATE_CASTING)
    {
        finish(true);                                        // successfully finish spell cast (not last in case autorepeat or channel spell)
    }
}

/**
 * @brief Processes delayed spell impacts that are due at the current offset.
 *
 * @param t_offset The elapsed delay offset in milliseconds.
 * @return The next pending delay time, or zero when finished.
 */
uint64 Spell::handle_delayed(uint64 t_offset)
{
    uint64 next_time = 0;

    if (!m_immediateHandled)
    {
        _handle_immediate_phase();
        m_immediateHandled = true;
    }

    // now recheck units targeting correctness (need before any effects apply to prevent adding immunity at first effect not allow apply second spell effect and similar cases)
    for (TargetList::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (!ihit->processed)
        {
            if (ihit->timeDelay <= t_offset)
            {
                DoAllEffectOnTarget(&(*ihit));
            }
            else if (next_time == 0 || ihit->timeDelay < next_time)
            {
                next_time = ihit->timeDelay;
            }
        }
    }

    // now recheck gameobject targeting correctness
    for (GOTargetList::iterator ighit = m_UniqueGOTargetInfo.begin(); ighit != m_UniqueGOTargetInfo.end(); ++ighit)
    {
        if (!ighit->processed)
        {
            if (ighit->timeDelay <= t_offset)
            {
                DoAllEffectOnTarget(&(*ighit));
            }
            else if (next_time == 0 || ighit->timeDelay < next_time)
            {
                next_time = ighit->timeDelay;
            }
        }
    }
    // All targets passed - need finish phase
    if (next_time == 0)
    {
        // spell is finished, perform some last features of the spell here
        _handle_finish_phase();

        finish(true);                                       // successfully finish spell cast

        // return zero, spell is finished now
        return 0;
    }
    else
    {
        // spell is unfinished, return next execution time
        return next_time;
    }
}

/**
 * @brief Performs the immediate pre-impact phase shared by instant and delayed spells.
 */
void Spell::_handle_immediate_phase()
{
    // handle some immediate features of the spell here
    HandleThreatSpells();

    m_needSpellLog = IsNeedSendToClient();
    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        if (m_spellInfo->Effect[j] == 0)
        {
            continue;
        }

        // apply Send Event effect to ground in case empty target lists
        if (m_spellInfo->Effect[j] == SPELL_EFFECT_SEND_EVENT && !HaveTargetsForEffect(SpellEffectIndex(j)))
        {
            HandleEffects(NULL, NULL, NULL, SpellEffectIndex(j));
            continue;
        }

        // Don't do spell log, if is school damage spell
        if (m_spellInfo->Effect[j] == SPELL_EFFECT_SCHOOL_DAMAGE || m_spellInfo->Effect[j] == 0)
        {
            m_needSpellLog = false;
        }
    }

    // initialize Diminishing Returns Data
    m_diminishLevel = DIMINISHING_LEVEL_1;
    m_diminishGroup = DIMINISHING_NONE;

    // process items
    for (ItemTargetList::iterator ihit = m_UniqueItemInfo.begin(); ihit != m_UniqueItemInfo.end(); ++ihit)
    {
        DoAllEffectOnTarget(&(*ihit));
    }

    // process ground
    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        // persistent area auras target only the ground
        if (m_spellInfo->Effect[j] == SPELL_EFFECT_PERSISTENT_AREA_AURA)
        {
            HandleEffects(NULL, NULL, NULL, SpellEffectIndex(j));
        }
    }
}

/**
 * @brief Performs post-impact finishing logic before the spell completes.
 */
void Spell::_handle_finish_phase()
{
    // spell log
    if (m_needSpellLog)
    {
        SendLogExecute();
    }
}

/**
 * @brief Applies and sends cooldown data for player casts when appropriate.
 */
void Spell::SendSpellCooldown()
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* _player = (Player*)m_caster;

    // mana/health/etc potions, disabled by client (until combat out as declarate)
    if (m_CastItem && m_CastItem->IsPotion())
    {
        // need in some way provided data for Spell::finish SendCooldownEvent
        _player->SetLastPotionId(m_CastItem->GetEntry());
        return;
    }

    // (1) have infinity cooldown but set at aura apply, (2) passive cooldown at triggering
    if (m_spellInfo->HasAttribute(SPELL_ATTR_DISABLED_WHILE_ACTIVE) || m_spellInfo->HasAttribute(SPELL_ATTR_PASSIVE))
    {
        return;
    }

    _player->AddSpellAndCategoryCooldowns(m_spellInfo, m_CastItem ? m_CastItem->GetEntry() : 0, this);
}

/**
 * @brief Updates the spell state machine during preparation or channeling.
 *
 * @param difftime The elapsed update time in milliseconds.
 */
void Spell::update(uint32 difftime)
{
    // update pointers based at it's GUIDs
    UpdatePointers();

    if (m_targets.getUnitTargetGuid() && !m_targets.getUnitTarget())
    {
        cancel();
        return;
    }

    // check if the player caster has moved before the spell finished
    if ((m_caster->GetTypeId() == TYPEID_PLAYER && m_timer != 0) &&
        (m_castPositionX != m_caster->GetPositionX() || m_castPositionY != m_caster->GetPositionY() || m_castPositionZ != m_caster->GetPositionZ()) &&
        (m_spellInfo->Effect[EFFECT_INDEX_0] != SPELL_EFFECT_STUCK || !((Player*)m_caster)->m_movementInfo.HasMovementFlag(MOVEFLAG_FALLINGFAR)))
    {
        // always cancel for channeled spells
        if (m_spellState == SPELL_STATE_CASTING)
        {
            cancel();
        }
        // don't cancel for melee, autorepeat, triggered and instant spells
        else if (!IsNextMeleeSwingSpell() && !IsAutoRepeat() && !m_IsTriggeredSpell && (m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_MOVEMENT))
        {
            cancel();
        }
    }

    switch (m_spellState)
    {
        case SPELL_STATE_PREPARING:
        {
            if (m_timer)
            {
                if (difftime >= m_timer)
                {
                    m_timer = 0;
                }
                else
                {
                    m_timer -= difftime;
                }
            }

            if (m_timer == 0 && !IsNextMeleeSwingSpell() && !IsAutoRepeat())
            {
                cast();
            }
        break;
        }
        case SPELL_STATE_CASTING:
        {
            if (m_timer > 0)
            {
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    // check if player has jumped before the channeling finished
                    if (((Player*)m_caster)->m_movementInfo.HasMovementFlag(MOVEFLAG_FALLING))
                    {
                        cancel();
                    }

                    // check for incapacitating player states
                    if (m_caster->hasUnitState(UNIT_STAT_CAN_NOT_REACT))
                    {
                        cancel();
                    }

                    // check if player has turned if flag is set
                    if (m_spellInfo->ChannelInterruptFlags & CHANNEL_FLAG_TURNING && m_castOrientation != m_caster->GetOrientation())
                    {
                        cancel();
                    }
                }

                // check if there are alive targets left
                if (!IsAliveUnitPresentInTargetList())
                {
                    SendChannelUpdate(0);
                    finish();
                }

                if (difftime >= m_timer)
                {
                    m_timer = 0;
                }
                else
                {
                    m_timer -= difftime;
                }
            }

            if (m_timer == 0)
            {
                SendChannelUpdate(0);

                // channeled spell processed independently for quest targeting
                // cast at creature (or GO) quest objectives update at successful cast channel finished
                // ignore autorepeat/melee casts for speed (not exist quest for spells (hm... )
                if (!IsAutoRepeat() && !IsNextMeleeSwingSpell())
                {
                    if (Player* p = m_caster->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        {
                            TargetInfo const& target = *ihit;
                            if (!target.targetGUID.IsCreatureOrVehicle())
                            {
                                continue;
                            }

                            Unit* unit = m_caster->GetObjectGuid() == target.targetGUID ? m_caster : sObjectAccessor.GetUnit(*m_caster, target.targetGUID);
                            if (unit == NULL)
                            {
                                continue;
                            }

                            p->RewardPlayerAndGroupAtCast(unit, m_spellInfo->ID);
                        }

                        for (GOTargetList::const_iterator ihit = m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
                        {
                            GOTargetInfo const& target = *ihit;

                            GameObject* go = m_caster->GetMap()->GetGameObject(target.targetGUID);
                            if (!go)
                            {
                                continue;
                            }

                            p->RewardPlayerAndGroupAtCast(go, m_spellInfo->ID);
                        }
                    }
                }

                finish();
            }
        break;
        }
        default:
        {
            break;
        }
    }
}

/**
 * @brief Finalizes the spell and performs successful-completion side effects.
 *
 * @param ok True when the spell completed successfully; false otherwise.
 */
void Spell::finish(bool ok)
{
    if (!m_caster)
    {
        return;
    }

    if (m_spellState == SPELL_STATE_FINISHED)
    {
        return;
    }

    m_spellState = SPELL_STATE_FINISHED;

    // other code related only to successfully finished spells
    if (!ok)
    {
        return;
    }

    // handle SPELL_AURA_ADD_TARGET_TRIGGER auras
    Unit::AuraList const& targetTriggers = m_caster->GetAurasByType(SPELL_AURA_ADD_TARGET_TRIGGER);
    for (Unit::AuraList::const_iterator i = targetTriggers.begin(); i != targetTriggers.end(); ++i)
    {
        if (!(*i)->isAffectedOnSpell(m_spellInfo))
        {
            continue;
        }
        for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
        {
            if (ihit->missCondition == SPELL_MISS_NONE)
            {
                // check m_caster->GetGUID() let load auras at login and speedup most often case
                Unit* unit = m_caster->GetObjectGuid() == ihit->targetGUID ? m_caster : sObjectAccessor.GetUnit(*m_caster, ihit->targetGUID);
                if (unit && unit->IsAlive())
                {
                    SpellEntry const* auraSpellInfo = (*i)->GetSpellProto();
                    SpellEffectIndex auraSpellIdx = (*i)->GetEffIndex();
                    // Calculate chance at that moment (can be depend for example from combo points)
                    int32 auraBasePoints = (*i)->GetBasePoints();
                    int32 chance = m_caster->CalculateSpellDamage(unit, auraSpellInfo, auraSpellIdx, &auraBasePoints);
                    if (roll_chance_i(chance))
                    {
                        m_caster->CastSpell(unit, auraSpellInfo->EffectTriggerSpell[auraSpellIdx], true, NULL, (*i));
                    }
                }
            }
        }
    }

    // Heal caster for all health leech from all targets
    if (m_healthLeech)
    {
        uint32 absorb = 0;
        m_caster->CalculateHealAbsorb(uint32(m_healthLeech), &absorb);
        m_caster->DealHeal(m_caster, uint32(m_healthLeech) - absorb, m_spellInfo, false, absorb);
    }

    if (IsMeleeAttackResetSpell())
    {
        m_caster->resetAttackTimer(BASE_ATTACK);
        if (m_caster->haveOffhandWeapon())
        {
            m_caster->resetAttackTimer(OFF_ATTACK);
        }
    }

    /*if (IsRangedAttackResetSpell())
        m_caster->resetAttackTimer(RANGED_ATTACK);*/

    // Clear combo at finish state
    if (m_caster->GetTypeId() == TYPEID_PLAYER && NeedsComboPoints(m_spellInfo))
    {
        // Not drop combopoints if negative spell and if any miss on enemy exist
        bool needDrop = true;
        if (!IsPositiveSpell(m_spellInfo->ID))
        {
            for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
            {
                if (ihit->missCondition != SPELL_MISS_NONE && ihit->targetGUID != m_caster->GetObjectGuid())
                {
                    needDrop = false;
                    break;
                }
            }
        }
        if (needDrop)
        {
            ((Player*)m_caster)->ClearComboPoints();
        }
    }

    // potions disabled by client, send event "not in combat" if need
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)m_caster)->UpdatePotionCooldown(this);
    }

    // call triggered spell only at successful cast (after clear combo points -> for add some if need)
    if (!m_TriggerSpells.empty())
    {
        CastTriggerSpells();
    }

    // Stop Attack for some spells
    if (m_spellInfo->HasAttribute(SPELL_ATTR_STOP_ATTACK_TARGET))
    {
        m_caster->AttackStop();
    }

    // update encounter state if needed
    // Caster may have left the world during the cast pipeline (e.g. a
    // DB-script-triggered cast targeting a unit that despawned mid-step);
    // without this guard GetMap() would assert on a NULL m_currMap.
    if (m_caster->IsInWorld())
    {
        Map* map = m_caster->GetMap();
        if (map->IsDungeon())
        {
            ((DungeonMap*)map)->GetPersistanceState()->UpdateEncounterState(ENCOUNTER_CREDIT_CAST_SPELL, m_spellInfo->ID);
        }
    }
}

/**
 * @brief Consumes ammunition or durability for ranged attacks.
 */
void Spell::TakeAmmo()
{
    if (m_attackType == RANGED_ATTACK && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Item* pItem = ((Player*)m_caster)->GetWeaponForAttack(RANGED_ATTACK, true, false);

        // wands don't have ammo
        if (!pItem || pItem->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_WAND)
        {
            return;
        }

        if (pItem->GetProto()->InventoryType == INVTYPE_THROWN)
        {
            if (pItem->GetMaxStackCount() == 1)
            {
                // decrease durability for non-stackable throw weapon
                ((Player*)m_caster)->DurabilityPointLossForEquipSlot(EQUIPMENT_SLOT_RANGED);
            }
            else
            {
                // decrease items amount for stackable throw weapon
                uint32 count = 1;
                ((Player*)m_caster)->DestroyItemCount(pItem, count, true);
            }
        }
        else if (uint32 ammo = ((Player*)m_caster)->GetUInt32Value(PLAYER_AMMO_ID))
        {
            ((Player*)m_caster)->DestroyItemCount(ammo, 1, true);
        }
    }
}
