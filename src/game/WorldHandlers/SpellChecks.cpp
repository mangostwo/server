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
 * @file SpellChecks.cpp
 * @brief Cohesion split of Spell.cpp -- pre-cast validation (CheckCast and friends).
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
 * @brief Validates whether the spell can currently be cast.
 *
 * @param strict True to perform full pre-cast validation including global cooldown checks.
 * @return The resulting cast status.
 */
SpellCastResult Spell::CheckCast(bool strict)
{
    // check cooldowns to prevent cheating (ignore passive spells, that client side visual only)
    if (m_caster->GetTypeId() == TYPEID_PLAYER && !m_spellInfo->HasAttribute(SPELL_ATTR_PASSIVE) &&
        ((Player*)m_caster)->HasSpellCooldown(m_spellInfo->Id))
    {
        if (m_triggeredByAuraSpell)
        {
            return SPELL_FAILED_DONT_REPORT;
        }
        else
        {
            return SPELL_FAILED_NOT_READY;
        }
    }

    // check global cooldown
    if (strict && !m_IsTriggeredSpell && HasGlobalCooldown())
    {
        return SPELL_FAILED_NOT_READY;
    }

    // only allow triggered spells if at an ended battleground
    if (!m_IsTriggeredSpell && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (BattleGround* bg = ((Player*)m_caster)->GetBattleGround())
        {
            if (bg->GetStatus() == STATUS_WAIT_LEAVE)
            {
                return SPELL_FAILED_DONT_REPORT;
            }
        }
    }

    if (!m_IsTriggeredSpell && IsNonCombatSpell(m_spellInfo) &&
        m_caster->IsInCombat() && !m_caster->IsIgnoreUnitState(m_spellInfo, IGNORE_UNIT_COMBAT_STATE))
    {
        return SPELL_FAILED_AFFECTING_COMBAT;
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER && !((Player*)m_caster)->isGameMaster() &&
        sWorld.getConfig(CONFIG_BOOL_VMAP_INDOOR_CHECK) &&
        VMAP::VMapFactory::createOrGetVMapManager()->isLineOfSightCalcEnabled())
    {
        if (m_spellInfo->HasAttribute(SPELL_ATTR_OUTDOORS_ONLY) &&
            !m_caster->GetTerrain()->IsOutdoors(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ()))
        {
            return SPELL_FAILED_ONLY_OUTDOORS;
        }

        if (m_spellInfo->HasAttribute(SPELL_ATTR_INDOORS_ONLY) &&
            m_caster->GetTerrain()->IsOutdoors(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ()))
        {
            return SPELL_FAILED_ONLY_INDOORS;
        }
    }
    // only check at first call, Stealth auras are already removed at second call
    // for now, ignore triggered spells
    if (strict && !m_IsTriggeredSpell)
    {
        // Ignore form req aura
        if (!m_caster->HasAffectedAura(SPELL_AURA_MOD_IGNORE_SHAPESHIFT, m_spellInfo))
        {
            // Cannot be used in this stance/form
            SpellCastResult shapeError = GetErrorAtShapeshiftedCast(m_spellInfo, m_caster->GetShapeshiftForm());
            if (shapeError != SPELL_CAST_OK)
            {
                return shapeError;
            }

            if (m_spellInfo->HasAttribute(SPELL_ATTR_ONLY_STEALTHED) && !(m_caster->HasStealthAura()))
            {
                return SPELL_FAILED_ONLY_STEALTHED;
            }
        }
    }

    // caster state requirements
    if (m_spellInfo->CasterAuraState && !m_caster->HasAuraState(AuraState(m_spellInfo->CasterAuraState)))
    {
        return SPELL_FAILED_CASTER_AURASTATE;
    }
    if (m_spellInfo->CasterAuraStateNot && m_caster->HasAuraState(AuraState(m_spellInfo->CasterAuraStateNot)))
    {
        return SPELL_FAILED_CASTER_AURASTATE;
    }

    // Caster aura req check if need
    if (m_spellInfo->casterAuraSpell && !m_caster->HasAura(m_spellInfo->casterAuraSpell))
    {
        return SPELL_FAILED_CASTER_AURASTATE;
    }
    if (m_spellInfo->excludeCasterAuraSpell)
    {
        // Special cases of non existing auras handling
        if (m_spellInfo->excludeCasterAuraSpell == 61988)
        {
            // Avenging Wrath Marker
            if (m_caster->HasAura(61987))
            {
                return SPELL_FAILED_CASTER_AURASTATE;
            }
        }
        else if (m_caster->HasAura(m_spellInfo->excludeCasterAuraSpell))
        {
            return SPELL_FAILED_CASTER_AURASTATE;
        }
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        // cancel autorepeat spells if cast start when moving
        // (not wand currently autorepeat cast delayed to moving stop anyway in spell update code)
        if (((Player*)m_caster)->isMoving())
        {
            // skip stuck spell to allow use it in falling case and apply spell limitations at movement
            if ((!((Player*)m_caster)->m_movementInfo.HasMovementFlag(MOVEFLAG_FALLINGFAR) || m_spellInfo->Effect[EFFECT_INDEX_0] != SPELL_EFFECT_STUCK) &&
                (IsAutoRepeat() || (m_spellInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED) != 0))
            {
                return SPELL_FAILED_MOVING;
            }
        }

        if (!m_IsTriggeredSpell && NeedsComboPoints(m_spellInfo) && !m_caster->IsIgnoreUnitState(m_spellInfo, IGNORE_UNIT_TARGET_STATE) &&
            (!m_targets.getUnitTarget() || m_targets.getUnitTarget()->GetObjectGuid() != ((Player*)m_caster)->GetComboTargetGuid()))
        {
            // warrior not have real combo-points at client side but use this way for mark allow Overpower use
            return m_caster->getClass() == CLASS_WARRIOR ? SPELL_FAILED_CASTER_AURASTATE : SPELL_FAILED_NO_COMBO_POINTS;
        }
    }

    // Spells like Disengage are allowed only in combat
    if (!m_caster->IsInCombat() && m_spellInfo->HasAttribute(SPELL_ATTR_STOP_ATTACK_TARGET) && m_spellInfo->HasAttribute(SPELL_ATTR_EX2_UNK26))
    {
        return SPELL_FAILED_CASTER_AURASTATE;
    }

    if (Unit* target = m_targets.getUnitTarget())
    {
        // target state requirements (not allowed state), apply to self also
        if (m_spellInfo->TargetAuraStateNot && target->HasAuraState(AuraState(m_spellInfo->TargetAuraStateNot)))
        {
            return SPELL_FAILED_TARGET_AURASTATE;
        }

        if (!m_IsTriggeredSpell && IsDeathOnlySpell(m_spellInfo) && target->IsAlive())
        {
            return SPELL_FAILED_TARGET_NOT_DEAD;
        }

        // Target aura req check if need
        if (m_spellInfo->targetAuraSpell && !target->HasAura(m_spellInfo->targetAuraSpell))
        {
            return SPELL_FAILED_CASTER_AURASTATE;
        }
        if (m_spellInfo->excludeTargetAuraSpell)
        {
            // Special cases of non existing auras handling
            if (m_spellInfo->excludeTargetAuraSpell == 61988)
            {
                // Avenging Wrath Marker
                if (target->HasAura(61987))
                {
                    return SPELL_FAILED_CASTER_AURASTATE;
                }
            }
            else if (target->HasAura(m_spellInfo->excludeTargetAuraSpell))
            {
                return SPELL_FAILED_CASTER_AURASTATE;
            }
        }

        // totem immunity for channeled spells(needs to be before spell cast)
        // spell attribs for player channeled spells
        if (m_spellInfo->HasAttribute(SPELL_ATTR_EX_CHANNEL_TRACK_TARGET)
            && m_spellInfo->HasAttribute(SPELL_ATTR_EX5_UNK13)
            && target->GetTypeId() == TYPEID_UNIT
            && ((Creature*)target)->IsTotem())
        {
            return SPELL_FAILED_IMMUNE;
        }

        bool non_caster_target = target != m_caster && !IsSpellWithCasterSourceTargetsOnly(m_spellInfo);

        if (non_caster_target)
        {
            // target state requirements (apply to non-self only), to allow cast affects to self like Dirty Deeds
            if (m_spellInfo->TargetAuraState && !target->HasAuraStateForCaster(AuraState(m_spellInfo->TargetAuraState), m_caster->GetObjectGuid()) &&
                    !m_caster->IsIgnoreUnitState(m_spellInfo, m_spellInfo->TargetAuraState == AURA_STATE_FROZEN ? IGNORE_UNIT_TARGET_NON_FROZEN : IGNORE_UNIT_TARGET_STATE))
            {
                return SPELL_FAILED_TARGET_AURASTATE;
            }

            // Not allow casting on flying player
            if (target->IsTaxiFlying())
            {
                switch (m_spellInfo->Id)
                {
                    // Except some spells from Taxi Flying cast
                    case 36573:                             // Vision Guide
                    case 42316:                             // Alcaz Survey Credit
                    case 42385:                             // Alcaz Survey Aura
                        break;
                    default:
                        return SPELL_FAILED_BAD_TARGETS;
                }
            }

            if (!m_IsTriggeredSpell && !DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, m_spellInfo->Id, NULL, SPELL_DISABLE_LOS) && VMAP::VMapFactory::checkSpellForLoS(m_spellInfo->Id) && !m_caster->IsWithinLOSInMap(target))
            {
                return SPELL_FAILED_LINE_OF_SIGHT;
            }

            // auto selection spell rank implemented in WorldSession::HandleCastSpellOpcode
            // this case can be triggered if rank not found (too low-level target for first rank)
            if (m_caster->GetTypeId() == TYPEID_PLAYER && !m_CastItem && !m_IsTriggeredSpell)
            {
                // spell expected to be auto-downranking in cast handle, so must be same
                if (m_spellInfo != sSpellMgr.SelectAuraRankForLevel(m_spellInfo, target->getLevel()))
                {
                    return SPELL_FAILED_LOWLEVEL;
                }
            }

            if (strict && m_spellInfo->HasAttribute(SPELL_ATTR_EX3_TARGET_ONLY_PLAYER) && target->GetTypeId() != TYPEID_PLAYER && !IsAreaOfEffectSpell(m_spellInfo))
            {
                return SPELL_FAILED_BAD_TARGETS;
            }
        }
        else if (m_caster == target)
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->IsInWorld())
            {
                // Additional check for some spells
                // If 0 spell effect empty - client not send target data (need use selection)
                // TODO: check it on next client version
                if (m_targets.m_targetMask == TARGET_FLAG_SELF &&
                    m_spellInfo->EffectImplicitTargetA[EFFECT_INDEX_1] == TARGET_CHAIN_DAMAGE)
                {
                    target = m_caster->GetMap()->GetUnit(((Player*)m_caster)->GetSelectionGuid());
                    if (!target)
                    {
                        return SPELL_FAILED_BAD_TARGETS;
                    }

                    m_targets.setUnitTarget(target);
                }
            }

            // Some special spells with non-caster only mode

            // Fire Shield
            if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK &&
                m_spellInfo->SpellIconID == 16)
            {
                return SPELL_FAILED_BAD_TARGETS;
            }

            // Focus Magic (main spell)
            if (m_spellInfo->Id == 54646)
            {
                return SPELL_FAILED_BAD_TARGETS;
            }

            // Lay on Hands (self cast)
            if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PALADIN &&
                    m_spellInfo->SpellFamilyFlags & UI64LIT(0x0000000000008000))
            {
                if (target->HasAura(25771))                 // Forbearance
                {
                    return SPELL_FAILED_CASTER_AURASTATE;
                }
                if (target->HasAura(61987))                 // Avenging Wrath Marker
                {
                    return SPELL_FAILED_CASTER_AURASTATE;
                }
            }
        }

        // check pet presents
        for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_PET)
            {
                Pet* pet = m_caster->GetPet();
                if (!pet)
                {
                    if (m_triggeredByAuraSpell)             // not report pet not existence for triggered spells
                    {
                        return SPELL_FAILED_DONT_REPORT;
                    }
                    else
                    {
                        return SPELL_FAILED_NO_PET;
                    }
                }
                else if (!pet->IsAlive())
                {
                    return SPELL_FAILED_TARGETS_DEAD;
                }
                break;
            }
        }

        // check creature type
        // ignore self casts (including area casts when caster selected as target)
        if (non_caster_target)
        {
            if (!CheckTargetCreatureType(target))
            {
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    return SPELL_FAILED_TARGET_IS_PLAYER;
                }
                else
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }
            }

            // simple cases
            bool explicit_target_mode = false;
            bool target_hostile = false;
            bool target_hostile_checked = false;
            bool target_friendly = false;
            bool target_friendly_checked = false;
            for (int k = 0; k < MAX_EFFECT_INDEX;  ++k)
            {
                if (IsExplicitPositiveTarget(m_spellInfo->EffectImplicitTargetA[k]))
                {
                    if (!target_hostile_checked)
                    {
                        target_hostile_checked = true;
                        target_hostile = m_caster->IsHostileTo(target);
                    }

                    if (target_hostile)
                    {
                        return SPELL_FAILED_BAD_TARGETS;
                    }

                    explicit_target_mode = true;
                }
                else if (IsExplicitNegativeTarget(m_spellInfo->EffectImplicitTargetA[k]))
                {
                    if (!target_friendly_checked)
                    {
                        target_friendly_checked = true;
                        target_friendly = m_caster->IsFriendlyTo(target);
                    }

                    if (target_friendly)
                    {
                        return SPELL_FAILED_BAD_TARGETS;
                    }

                    explicit_target_mode = true;
                }
            }
            // TODO: this check can be applied and for player to prevent cheating when IsPositiveSpell will return always correct result.
            // check target for pet/charmed casts (not self targeted), self targeted cast used for area effects and etc
            if (!explicit_target_mode && m_caster->GetTypeId() == TYPEID_UNIT && m_caster->GetCharmerOrOwnerGuid())
            {
                // check correctness positive/negative cast target (pet cast real check and cheating check)
                if (IsPositiveSpell(m_spellInfo->Id))
                {
                    if (!target_hostile_checked)
                    {
                        target_hostile_checked = true;
                        target_hostile = m_caster->IsHostileTo(target);
                    }

                    if (target_hostile)
                    {
                        return SPELL_FAILED_BAD_TARGETS;
                    }
                }
                else
                {
                    if (!target_friendly_checked)
                    {
                        target_friendly_checked = true;
                        target_friendly = m_caster->IsFriendlyTo(target);
                    }

                    if (target_friendly)
                    {
                        return SPELL_FAILED_BAD_TARGETS;
                    }
                }
            }
        }

        if (IsPositiveSpell(m_spellInfo->Id))
        {
            if (target->IsImmuneToSpell(m_spellInfo, target == m_caster))
            {
                return SPELL_FAILED_TARGET_AURASTATE;
            }
        }

        // Must be behind the target.
        if (m_spellInfo->AttributesEx2 == SPELL_ATTR_EX2_FACING_TARGETS_BACK && m_spellInfo->HasAttribute(SPELL_ATTR_EX_FACING_TARGET) && target->HasInArc(M_PI_F, m_caster))
        {
            // Exclusion for Pounce: Facing Limitation was removed in 2.0.1, but it still uses the same, old Ex-Flags
            // Exclusion for Mutilate:Facing Limitation was removed in 2.0.1 and 3.0.3, but they still use the same, old Ex-Flags
            // Exclusion for Throw: Facing limitation was added in 3.2.x, but that shouldn't be
            if (!m_spellInfo->IsFitToFamily(SPELLFAMILY_DRUID, UI64LIT(0x0000000000020000)) &&
                    !m_spellInfo->IsFitToFamily(SPELLFAMILY_ROGUE, UI64LIT(0x0020000000000000)) &&
                    m_spellInfo->Id != 2764)
            {
                SendInterrupted(2);
                return SPELL_FAILED_NOT_BEHIND;
            }
        }

        // Target must be facing you.
        if ((m_spellInfo->Attributes == (SPELL_ATTR_ABILITY | SPELL_ATTR_NOT_SHAPESHIFT | SPELL_ATTR_DONT_AFFECT_SHEATH_STATE | SPELL_ATTR_STOP_ATTACK_TARGET)) && !target->HasInArc(M_PI_F, m_caster))
        {
            SendInterrupted(2);
            return SPELL_FAILED_NOT_INFRONT;
        }

        // check if target is in combat
        if (non_caster_target && m_spellInfo->HasAttribute(SPELL_ATTR_EX_NOT_IN_COMBAT_TARGET) && target->IsInCombat())
        {
            return SPELL_FAILED_TARGET_AFFECTING_COMBAT;
        }
    }
    // zone check
    uint32 zone, area;
    m_caster->GetZoneAndAreaId(zone, area);

    SpellCastResult locRes = sSpellMgr.GetSpellAllowedInLocationError(m_spellInfo, m_caster->GetMapId(), zone, area,
                             m_caster->GetCharmerOrOwnerPlayerOrPlayerItself());
    if (locRes != SPELL_CAST_OK)
    {
        return locRes;
    }

    // not let players cast spells at mount (and let do it to creatures)
    if (m_caster->IsMounted() && m_caster->GetTypeId() == TYPEID_PLAYER && !m_IsTriggeredSpell &&
        !IsPassiveSpell(m_spellInfo) && !m_spellInfo->HasAttribute(SPELL_ATTR_CASTABLE_WHILE_MOUNTED))
    {
        if (m_caster->IsTaxiFlying())
        {
            return SPELL_FAILED_NOT_ON_TAXI;
        }
        else
        {
            return SPELL_FAILED_NOT_MOUNTED;
        }
    }

    // always (except passive spells) check items (focus object can be required for any type casts)
    if (!IsPassiveSpell(m_spellInfo))
    {
        SpellCastResult castResult = CheckItems();
        if (castResult != SPELL_CAST_OK)
        {
            return castResult;
        }
    }

    // check spell focus object
    if (m_spellInfo->RequiresSpellFocus)
    {
        GameObject* ok = NULL;
        MaNGOS::GameObjectFocusCheck go_check(m_caster, m_spellInfo->RequiresSpellFocus);
        MaNGOS::GameObjectSearcher<MaNGOS::GameObjectFocusCheck> checker(ok, go_check);
        Cell::VisitGridObjects(m_caster, checker, m_caster->GetMap()->GetVisibilityDistance());

        if (!ok)
        {
            return SPELL_FAILED_REQUIRES_SPELL_FOCUS;
        }

        focusObject = ok;                                   // game object found in range
    }

    // Database based targets from spell_target_script
    if (m_UniqueTargetInfo.empty())                         // skip second CheckCast apply (for delayed spells for example)
    {
        for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT ||
                    m_spellInfo->EffectImplicitTargetB[j] == TARGET_SCRIPT ||
                    m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT_COORDINATES ||
                    m_spellInfo->EffectImplicitTargetB[j] == TARGET_SCRIPT_COORDINATES ||
                    m_spellInfo->EffectImplicitTargetA[j] == TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT ||
                    m_spellInfo->EffectImplicitTargetB[j] == TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT)
            {
                SQLMultiStorage::SQLMSIteratorBounds<SpellTargetEntry> bounds = sSpellScriptTargetStorage.getBounds<SpellTargetEntry>(m_spellInfo->Id);

                if (bounds.first == bounds.second)
                {
                    if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT || m_spellInfo->EffectImplicitTargetB[j] == TARGET_SCRIPT)
                    {
                        sLog.outErrorDb("Spell entry %u, effect %i has EffectImplicitTargetA/EffectImplicitTargetB = TARGET_SCRIPT, but creature are not defined in `spell_script_target`", m_spellInfo->Id, j);
                    }

                    if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT_COORDINATES || m_spellInfo->EffectImplicitTargetB[j] == TARGET_SCRIPT_COORDINATES)
                    {
                        sLog.outErrorDb("Spell entry %u, effect %i has EffectImplicitTargetA/EffectImplicitTargetB = TARGET_SCRIPT_COORDINATES, but gameobject or creature are not defined in `spell_script_target`", m_spellInfo->Id, j);
                    }

                    if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT || m_spellInfo->EffectImplicitTargetB[j] == TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT)
                    {
                        sLog.outErrorDb("Spell entry %u, effect %i has EffectImplicitTargetA/EffectImplicitTargetB = TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT, but gameobject are not defined in `spell_script_target`", m_spellInfo->Id, j);
                    }
                }

                SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex);
                float range = GetSpellMaxRange(srange);

                Creature* targetExplicit = NULL;            // used for cases where a target is provided (by script for example)
                Creature* creatureScriptTarget = NULL;
                GameObject* goScriptTarget = NULL;

                for (SQLMultiStorage::SQLMultiSIterator<SpellTargetEntry> i_spellST = bounds.first; i_spellST != bounds.second; ++i_spellST)
                {
                    if (i_spellST->CanNotHitWithSpellEffect(SpellEffectIndex(j)))
                    {
                        continue;
                    }

                    switch (i_spellST->type)
                    {
                        case SPELL_TARGET_TYPE_GAMEOBJECT:
                        {
                            GameObject* p_GameObject = NULL;

                            if (i_spellST->targetEntry)
                            {
                                MaNGOS::NearestGameObjectEntryInObjectRangeCheck go_check(*m_caster, i_spellST->targetEntry, range);
                                MaNGOS::GameObjectLastSearcher<MaNGOS::NearestGameObjectEntryInObjectRangeCheck> checker(p_GameObject, go_check);
                                Cell::VisitGridObjects(m_caster, checker, range);

                                if (p_GameObject)
                                {
                                    // remember found target and range, next attempt will find more near target with another entry
                                    creatureScriptTarget = NULL;
                                    goScriptTarget = p_GameObject;
                                    range = go_check.GetLastRange();
                                }
                            }
                            else if (focusObject)           // Focus Object
                            {
                                float frange = m_caster->GetDistance(focusObject);
                                if (range >= frange)
                                {
                                    creatureScriptTarget = NULL;
                                    goScriptTarget = focusObject;
                                    range = frange;
                                }
                            }
                            break;
                        }
                        case SPELL_TARGET_TYPE_CREATURE:
                        case SPELL_TARGET_TYPE_DEAD:
                        default:
                        {
                            Creature* p_Creature = NULL;

                            // check if explicit target is provided and check it up against database valid target entry/state
                            if (Unit* pTarget = m_targets.getUnitTarget())
                            {
                                if (pTarget->GetTypeId() == TYPEID_UNIT && pTarget->GetEntry() == i_spellST->targetEntry)
                                {
                                    if (i_spellST->type == SPELL_TARGET_TYPE_DEAD && ((Creature*)pTarget)->IsCorpse())
                                    {
                                        // always use spellMaxRange, in case GetLastRange returned different in a previous pass
                                        if (pTarget->IsWithinDistInMap(m_caster, GetSpellMaxRange(srange)))
                                        {
                                            targetExplicit = (Creature*)pTarget;
                                        }
                                    }
                                    else if (i_spellST->type == SPELL_TARGET_TYPE_CREATURE && pTarget->IsAlive())
                                    {
                                        // always use spellMaxRange, in case GetLastRange returned different in a previous pass
                                        if (pTarget->IsWithinDistInMap(m_caster, GetSpellMaxRange(srange)))
                                        {
                                            targetExplicit = (Creature*)pTarget;
                                        }
                                    }
                                }
                            }

                            // no target provided or it was not valid, so use closest in range
                            if (!targetExplicit)
                            {
                                MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck u_check(*m_caster, i_spellST->targetEntry, i_spellST->type != SPELL_TARGET_TYPE_DEAD, i_spellST->type == SPELL_TARGET_TYPE_DEAD, range);
                                MaNGOS::CreatureLastSearcher<MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(p_Creature, u_check);

                                // Visit all, need to find also Pet* objects
                                Cell::VisitAllObjects(m_caster, searcher, range);

                                range = u_check.GetLastRange();
                            }

                            // always prefer provided target if it's valid
                            if (targetExplicit)
                            {
                                creatureScriptTarget = targetExplicit;
                            }
                            else if (p_Creature)
                            {
                                creatureScriptTarget = p_Creature;
                            }

                            if (creatureScriptTarget)
                            {
                                goScriptTarget = NULL;
                            }

                            break;
                        }
                    }
                }

                if (creatureScriptTarget)
                {
                    // store coordinates for TARGET_SCRIPT_COORDINATES
                    if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT_COORDINATES ||
                        m_spellInfo->EffectImplicitTargetB[j] == TARGET_SCRIPT_COORDINATES)
                    {
                        m_targets.setDestination(creatureScriptTarget->GetPositionX(), creatureScriptTarget->GetPositionY(), creatureScriptTarget->GetPositionZ());

                        if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT_COORDINATES && m_spellInfo->Effect[j] != SPELL_EFFECT_PERSISTENT_AREA_AURA)
                        {
                            AddUnitTarget(creatureScriptTarget, SpellEffectIndex(j));
                        }
                    }
                    // store explicit target for TARGET_SCRIPT
                    else
                    {
                        if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT ||
                            m_spellInfo->EffectImplicitTargetB[j] == TARGET_SCRIPT)
                        {
                            AddUnitTarget(creatureScriptTarget, SpellEffectIndex(j));
                        }
                    }
                }
                else if (goScriptTarget)
                {
                    // store coordinates for TARGET_SCRIPT_COORDINATES
                    if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT_COORDINATES ||
                        m_spellInfo->EffectImplicitTargetB[j] == TARGET_SCRIPT_COORDINATES)
                    {
                        m_targets.setDestination(goScriptTarget->GetPositionX(), goScriptTarget->GetPositionY(), goScriptTarget->GetPositionZ());

                        if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_SCRIPT_COORDINATES && m_spellInfo->Effect[j] != SPELL_EFFECT_PERSISTENT_AREA_AURA)
                        {
                            AddGOTarget(goScriptTarget, SpellEffectIndex(j));
                        }
                    }
                    // store explicit target for TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT
                    else
                    {
                        if (m_spellInfo->EffectImplicitTargetA[j] == TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT ||
                            m_spellInfo->EffectImplicitTargetB[j] == TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT)
                        {
                            AddGOTarget(goScriptTarget, SpellEffectIndex(j));
                        }
                    }
                }
                // Missing DB Entry or targets for this spellEffect.
                else
                {
                    /* For TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT makes DB targets optional not required for now
                     * TODO: Makes more research for this target type
                     */
                    if (m_spellInfo->EffectImplicitTargetA[j] != TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT)
                    {
                        // not report target not existence for triggered spells
                        if (m_triggeredByAuraSpell || m_IsTriggeredSpell)
                        {
                            return SPELL_FAILED_DONT_REPORT;
                        }
                        else
                        {
                            return SPELL_FAILED_BAD_TARGETS;
                        }
                    }
                }
            }
        }
    }

    if (!m_IsTriggeredSpell)
    {
        if (!m_triggeredByAuraSpell)
        {
            SpellCastResult castResult = CheckRange(strict);
            if (castResult != SPELL_CAST_OK)
            {
                return castResult;
            }
        }
    }

    if (!m_IsTriggeredSpell)                                // triggered spell does not use power
    {
        SpellCastResult castResult = CheckPower();
        if (castResult != SPELL_CAST_OK)
        {
            return castResult;
        }
    }

    if (!m_IsTriggeredSpell)                                // triggered spell not affected by stun/etc
    {
        SpellCastResult castResult = CheckCasterAuras();
        if (castResult != SPELL_CAST_OK)
        {
            return castResult;
        }
    }

    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        // for effects of spells that have only one target
        switch (m_spellInfo->Effect[i])
        {
            case SPELL_EFFECT_INSTAKILL:
                // Death Pact
                if (m_spellInfo->Id == 48743)
                {
                    if (m_caster->GetTypeId() != TYPEID_PLAYER)
                    {
                        return SPELL_FAILED_ERROR;
                    }

                    if (!((Player*)m_caster)->GetSelectionGuid())
                    {
                        return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                    }
                    Pet* target = m_caster->GetMap()->GetPet(((Player*)m_caster)->GetSelectionGuid());

                    // alive
                    if (!target || target->IsDead())
                    {
                        return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                    }
                    // undead
                    if (target->GetCreatureType() != CREATURE_TYPE_UNDEAD)
                    {
                        return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                    }
                    // owned
                    if (target->GetOwnerGuid() != m_caster->GetObjectGuid())
                    {
                        return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                    }

                    float dist = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[i]));
                    if (!target->IsWithinDistInMap(m_caster, dist))
                    {
                        return SPELL_FAILED_OUT_OF_RANGE;
                    }

                    // will set in target selection code
                }
                break;
            case SPELL_EFFECT_DUMMY:
            {
                // By Spell ID
                if (m_spellInfo->Id == 19938)               // Awaken Lazy Peon
                {
                    Unit* target = m_targets.getUnitTarget();
                    // 17743 = Lazy Peon Sleep | 10556 = Lazy Peon
                    if (!target || !target->HasAura(17743) || target->GetEntry() != 10556)
                    {
                        return SPELL_FAILED_BAD_TARGETS;
                    }
                }
                else if (m_spellInfo->Id == 51582)          // Rocket Boots Engaged
                {
                    if (m_caster->IsInWater())
                    {
                        return SPELL_FAILED_ONLY_ABOVEWATER;
                    }
                }
                else if (m_spellInfo->Id == 51690)          // Killing Spree
                {
                    UnitList targets;

                    float radius = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex));

                    MaNGOS::AnyUnfriendlyVisibleUnitInObjectRangeCheck unitCheck(m_caster, m_caster, radius);
                    MaNGOS::UnitListSearcher<MaNGOS::AnyUnfriendlyVisibleUnitInObjectRangeCheck> checker(targets, unitCheck);
                    Cell::VisitAllObjects(m_caster, checker, radius);

                    if (targets.empty())
                    {
                        return SPELL_FAILED_OUT_OF_RANGE;
                    }
                }
                else if (m_spellInfo->SpellIconID == 156)   // Holy Shock
                {
                    // spell different for friends and enemies
                    // hart version required facing
                    if (m_targets.getUnitTarget() && !m_caster->IsFriendlyTo(m_targets.getUnitTarget()) && !m_caster->HasInArc(M_PI_F, m_targets.getUnitTarget()))
                    {
                        return SPELL_FAILED_UNIT_NOT_INFRONT;
                    }
                }
                else if (m_spellInfo->Id == 49576) // Death Grip
                {
                    if (m_caster->m_movementInfo.HasMovementFlag(MovementFlags(MOVEFLAG_FALLING | MOVEFLAG_FALLINGFAR)))
                    {
                        return SPELL_FAILED_MOVING;
                    }
                }
                // Fire Nova
                if (m_spellInfo->SpellFamilyName == SPELLFAMILY_SHAMAN && m_spellInfo->SpellIconID == 33)
                {
                    // fire totems slot
                    if (!m_caster->GetTotemGuid(TOTEM_SLOT_FIRE))
                    {
                        return SPELL_FAILED_TOTEMS;
                    }
                }
                break;
            }
            case SPELL_EFFECT_DISTRACT:                     // All nearby enemies must not be in combat
            {
                if (m_targets.m_targetMask & (TARGET_FLAG_DEST_LOCATION | TARGET_FLAG_SOURCE_LOCATION))
                {
                    UnitList targetsCombat;
                    float radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[i]));

                    FillAreaTargets(targetsCombat, radius, PUSH_DEST_CENTER, SPELL_TARGETS_AOE_DAMAGE);

                    if (targetsCombat.empty())
                    {
                        break;
                    }

                    for (UnitList::iterator itr = targetsCombat.begin(); itr != targetsCombat.end(); ++itr)
                    {
                        if ((*itr)->IsInCombat())
                        {
                            return SPELL_FAILED_TARGET_IN_COMBAT;
                        }
                    }
                }
                break;
            }
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            {
                // Hammer of Wrath
                if (m_spellInfo->SpellVisual[0] == 7250)
                {
                    if (!m_targets.getUnitTarget())
                    {
                        return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                    }

                    if (m_targets.getUnitTarget()->GetHealth() > m_targets.getUnitTarget()->GetMaxHealth() * 0.2)
                    {
                        return SPELL_FAILED_BAD_TARGETS;
                    }
                }
                break;
            }
            case SPELL_EFFECT_TAMECREATURE:
            {
                // Spell can be triggered, we need to check original caster prior to caster
                Unit* caster = GetAffectiveCaster();
                if (!caster || caster->GetTypeId() != TYPEID_PLAYER ||
                    !m_targets.getUnitTarget() ||
                    m_targets.getUnitTarget()->GetTypeId() == TYPEID_PLAYER)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                Player* plrCaster = (Player*)caster;

                bool gmmode = m_triggeredBySpellInfo == NULL;

                if (gmmode && !ChatHandler(plrCaster).FindCommand("npc tame"))
                {
                    plrCaster->SendPetTameFailure(PETTAME_UNKNOWNERROR);
                    return SPELL_FAILED_DONT_REPORT;
                }

                if (plrCaster->getClass() != CLASS_HUNTER && !gmmode)
                {
                    plrCaster->SendPetTameFailure(PETTAME_UNITSCANTTAME);
                    return SPELL_FAILED_DONT_REPORT;
                }

                Creature* target = (Creature*)m_targets.getUnitTarget();

                if (target->IsPet() || target->IsCharmed())
                {
                    plrCaster->SendPetTameFailure(PETTAME_CREATUREALREADYOWNED);
                    return SPELL_FAILED_DONT_REPORT;
                }

                if (target->getLevel() > plrCaster->getLevel() && !gmmode)
                {
                    plrCaster->SendPetTameFailure(PETTAME_TOOHIGHLEVEL);
                    return SPELL_FAILED_DONT_REPORT;
                }

                if (target->GetCreatureInfo()->IsExotic() && !plrCaster->CanTameExoticPets() && !gmmode)
                {
                    plrCaster->SendPetTameFailure(PETTAME_CANTCONTROLEXOTIC);
                    return SPELL_FAILED_DONT_REPORT;
                }

                if (!target->GetCreatureInfo()->isTameable(plrCaster->CanTameExoticPets()))
                {
                    plrCaster->SendPetTameFailure(PETTAME_NOTTAMEABLE);
                    return SPELL_FAILED_DONT_REPORT;
                }

                if (plrCaster->GetPetGuid() || plrCaster->GetCharmGuid())
                {
                    plrCaster->SendPetTameFailure(PETTAME_ANOTHERSUMMONACTIVE);
                    return SPELL_FAILED_DONT_REPORT;
                }

                break;
            }
            case SPELL_EFFECT_LEARN_SPELL:
            {
                if (m_spellInfo->EffectImplicitTargetA[i] != TARGET_PET)
                {
                    break;
                }

                Pet* pet = m_caster->GetPet();

                if (!pet)
                {
                    return SPELL_FAILED_NO_PET;
                }

                SpellEntry const* learn_spellproto = sSpellStore.LookupEntry(m_spellInfo->EffectTriggerSpell[i]);

                if (!learn_spellproto)
                {
                    return SPELL_FAILED_NOT_KNOWN;
                }

                if (m_spellInfo->spellLevel > pet->getLevel())
                {
                    return SPELL_FAILED_LOWLEVEL;
                }

                break;
            }
            case SPELL_EFFECT_LEARN_PET_SPELL:
            {
                Pet* pet = m_caster->GetPet();

                if (!pet)
                {
                    return SPELL_FAILED_NO_PET;
                }

                SpellEntry const* learn_spellproto = sSpellStore.LookupEntry(m_spellInfo->EffectTriggerSpell[i]);

                if (!learn_spellproto)
                {
                    return SPELL_FAILED_NOT_KNOWN;
                }

                if (m_spellInfo->spellLevel > pet->getLevel())
                {
                    return SPELL_FAILED_LOWLEVEL;
                }

                break;
            }
            case SPELL_EFFECT_APPLY_GLYPH:
            {
                uint32 glyphId = m_spellInfo->EffectMiscValue[i];
                if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyphId))
                {
                    if (m_caster->HasAura(gp->SpellId))
                    {
                        return SPELL_FAILED_UNIQUE_GLYPH;
                    }
                }
                break;
            }
            case SPELL_EFFECT_FEED_PET:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                Item* foodItem = m_targets.getItemTarget();
                if (!foodItem)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                Pet* pet = m_caster->GetPet();

                if (!pet)
                {
                    return SPELL_FAILED_NO_PET;
                }

                if (!pet->HaveInDiet(foodItem->GetProto()))
                {
                    return SPELL_FAILED_WRONG_PET_FOOD;
                }

                if (!pet->GetCurrentFoodBenefitLevel(foodItem->GetProto()->ItemLevel))
                {
                    return SPELL_FAILED_FOOD_LOWLEVEL;
                }

                if (pet->IsInCombat())
                {
                    return SPELL_FAILED_AFFECTING_COMBAT;
                }

                break;
            }
            case SPELL_EFFECT_POWER_BURN:
            case SPELL_EFFECT_POWER_DRAIN:
            {
                // Can be area effect, Check only for players and not check if target - caster (spell can have multiply drain/burn effects)
                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    if (Unit* target = m_targets.getUnitTarget())
                    {
                        if (target != m_caster && int32(target->GetPowerType()) != m_spellInfo->EffectMiscValue[i])
                        {
                            return SPELL_FAILED_BAD_TARGETS;
                        }
                    }
                }
                break;
            }
            case SPELL_EFFECT_CHARGE:
            {
                if (m_caster->hasUnitState(UNIT_STAT_ROOT))
                {
                    return SPELL_FAILED_ROOTED;
                }

                break;
            }
            case SPELL_EFFECT_SKINNING:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER || !m_targets.getUnitTarget() || m_targets.getUnitTarget()->GetTypeId() != TYPEID_UNIT)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                if (!m_targets.getUnitTarget()->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE))
                {
                    return SPELL_FAILED_TARGET_UNSKINNABLE;
                }

                Creature* creature = (Creature*)m_targets.getUnitTarget();
                if (creature->GetCreatureType() != CREATURE_TYPE_CRITTER && (!creature->lootForBody || creature->lootForSkin || !creature->loot.empty()))
                {
                    return SPELL_FAILED_TARGET_NOT_LOOTED;
                }

                uint32 skill = creature->GetCreatureInfo()->GetRequiredLootSkill();

                int32 skillValue = ((Player*)m_caster)->GetSkillValue(skill);
                int32 TargetLevel = m_targets.getUnitTarget()->getLevel();
                int32 ReqValue = (skillValue < 100 ? (TargetLevel - 10) * 10 : TargetLevel * 5);
                if (ReqValue > skillValue)
                {
                    return SPELL_FAILED_LOW_CASTLEVEL;
                }

                // chance for fail at orange skinning attempt
                if ((m_selfContainer && (*m_selfContainer) == this) &&
                    skillValue < sWorld.GetConfigMaxSkillValue() &&
                    (ReqValue < 0 ? 0 : ReqValue) > irand(skillValue - 25, skillValue + 37))
                {
                    return SPELL_FAILED_TRY_AGAIN;
                }

                break;
            }
            case SPELL_EFFECT_OPEN_LOCK:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER) // only players can open locks, gather etc.
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                // we need a go target in case of TARGET_GAMEOBJECT (for other targets acceptable GO and items)
                if (m_spellInfo->EffectImplicitTargetA[i] == TARGET_GAMEOBJECT)
                {
                    if (!m_targets.getGOTarget())
                    {
                        return SPELL_FAILED_BAD_TARGETS;
                    }
                }

                // get the lock entry
                uint32 lockId = 0;
                if (GameObject* go = m_targets.getGOTarget())
                {
                    // In BattleGround players can use only flags and banners
                    if (((Player*)m_caster)->InBattleGround() &&
                        !((Player*)m_caster)->CanUseBattleGroundObject())
                    {
                        return SPELL_FAILED_TRY_AGAIN;
                    }

                    lockId = go->GetGOInfo()->GetLockId();
                    if (!lockId)
                    {
                        return SPELL_FAILED_ALREADY_OPEN;
                    }
                }
                else if (Item* item = m_targets.getItemTarget())
                {
                    // not own (trade?)
                    if (item->GetOwner() != m_caster)
                    {
                        return SPELL_FAILED_ITEM_GONE;
                    }

                    lockId = item->GetProto()->LockID;

                    // if already unlocked
                    if (!lockId || item->HasFlag(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_UNLOCKED))
                    {
                        return SPELL_FAILED_ALREADY_OPEN;
                    }
                }
                else
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                SkillType skillId = SKILL_NONE;
                int32 reqSkillValue = 0;
                int32 skillValue = 0;

                // check lock compatibility
                SpellCastResult res = CanOpenLock(SpellEffectIndex(i), lockId, skillId, reqSkillValue, skillValue);
                if (res != SPELL_CAST_OK)
                {
                    return res;
                }

                // chance for fail at orange mining/herb/LockPicking gathering attempt
                // second check prevent fail at rechecks
                // Check must be executed at the end of  the cast.
                if (m_spellState != SPELL_STATE_CREATED && skillId != SKILL_NONE && skillId != SKILL_HERBALISM && skillId != SKILL_MINING)
                {
                    bool canFailAtMax = skillId != SKILL_HERBALISM && skillId != SKILL_MINING;

                    // chance for failure in orange gather / lockpick (gathering skill can't fail at maxskill)
                    if ((canFailAtMax || skillValue < sWorld.GetConfigMaxSkillValue()) && reqSkillValue > irand(skillValue - 25, skillValue + 37))
                    {
                        return SPELL_FAILED_TRY_AGAIN;
                    }
                }
                break;
            }
            case SPELL_EFFECT_SUMMON_DEAD_PET:
            {
                Creature* pet = m_caster->GetPet();
                if (!pet)
                {
                    return SPELL_FAILED_NO_PET;
                }

                if (pet->IsAlive())
                {
                    return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                }

                break;
            }
            // This is generic summon effect now and don't make this check for summon types similar
            // SPELL_EFFECT_SUMMON_CRITTER, SPELL_EFFECT_SUMMON_WILD or SPELL_EFFECT_SUMMON_GUARDIAN.
            // These won't show up in m_caster->GetPetGUID()
            case SPELL_EFFECT_SUMMON:
            {
                if (SummonPropertiesEntry const* summon_prop = sSummonPropertiesStore.LookupEntry(m_spellInfo->EffectMiscValueB[i]))
                {
                    if (summon_prop->Group == SUMMON_PROP_GROUP_PETS)
                    {
                        if (m_caster->GetPetGuid())
                        {
                            return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                        }

                        if (m_caster->GetCharmGuid())
                        {
                            return SPELL_FAILED_ALREADY_HAVE_CHARM;
                        }
                    }
                }

                break;
            }
            case SPELL_EFFECT_SUMMON_PET:
            {
                if (m_caster->GetPetGuid())                 // let warlock do a replacement summon
                {
                    Pet* pet = ((Player*)m_caster)->GetPet();

                    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->getClass() == CLASS_WARLOCK)
                    {
                        if (strict)                         // Summoning Disorientation, trigger pet stun (cast by pet so it doesn't attack player)
                        {
                            pet->CastSpell(pet, 32752, true, NULL, NULL, pet->GetObjectGuid());
                        }
                    }
                    else
                    {
                        return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                    }
                }

                if (m_caster->GetCharmGuid())
                {
                    return SPELL_FAILED_ALREADY_HAVE_CHARM;
                }

                break;
            }
            case SPELL_EFFECT_SUMMON_PLAYER:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }
                if (!((Player*)m_caster)->GetSelectionGuid())
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                Player* target = sObjectMgr.GetPlayer(((Player*)m_caster)->GetSelectionGuid());
                if (!target || ((Player*)m_caster) == target || !target->IsInSameRaidWith((Player*)m_caster))
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                // check if our map is dungeon
                if (sMapStore.LookupEntry(m_caster->GetMapId())->IsDungeon())
                {
                    InstanceTemplate const* instance = ObjectMgr::GetInstanceTemplate(m_caster->GetMapId());
                    if (!instance)
                    {
                        return SPELL_FAILED_TARGET_NOT_IN_INSTANCE;
                    }
                    if (instance->levelMin > target->getLevel())
                    {
                        return SPELL_FAILED_LOWLEVEL;
                    }
                    if (instance->levelMax && instance->levelMax < target->getLevel())
                    {
                        return SPELL_FAILED_HIGHLEVEL;
                    }
                }
                break;
            }
            case SPELL_EFFECT_RESURRECT:
            case SPELL_EFFECT_RESURRECT_NEW:
            {
                if (m_targets.m_targetMask & (TARGET_FLAG_CORPSE | TARGET_FLAG_PVP_CORPSE))
                {
                    if (Corpse* corpse = m_caster->GetMap()->GetCorpse(m_targets.getCorpseTargetGuid()))
                    {
                        if (Player* owner = sObjectAccessor.FindPlayer(corpse->GetOwnerGuid()))
                        {
                            if (owner->HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
                            {
                                return SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED;
                            }
                        }
                        else
                        {
                            return SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED;
                        }
                    }
                    else
                    {
                        return SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED;
                    }
                }
                else if (m_targets.m_targetMask & TARGET_FLAG_UNIT)
                {
                    Unit* target = m_targets.getUnitTarget();
                    if (!target || target->HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
                    {
                        return SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED;
                    }
                }
                else
                {
                    return SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED;
                }

                break;
            }
            case SPELL_EFFECT_SELF_RESURRECT:
            {
                if (m_caster->HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
                {
                    return SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED;
                }

                break;
            }
            case SPELL_EFFECT_LEAP:
            case SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER:
            {
                if (!m_caster || m_caster->IsTaxiFlying())
                {
                    return SPELL_FAILED_NOT_ON_TAXI;
                }

                // Blink has leap first and then removing of auras with root effect
                // need further research with this
                if (m_spellInfo->Effect[i] != SPELL_EFFECT_LEAP)
                {
                    if (m_caster->hasUnitState(UNIT_STAT_ROOT))
                    {
                        return SPELL_FAILED_ROOTED;
                    }
                }

                if (m_caster->GetTypeId() == TYPEID_PLAYER)
                {
                    if (((Player*)m_caster)->HasMovementFlag(MOVEFLAG_ONTRANSPORT))
                    {
                        return SPELL_FAILED_NOT_ON_TRANSPORT;
                    }

                    // not allow use this effect at battleground until battleground start
                    if (BattleGround const* bg = ((Player*)m_caster)->GetBattleGround())
                    {
                        if (bg->GetStatus() != STATUS_IN_PROGRESS)
                        {
                            return SPELL_FAILED_TRY_AGAIN;
                        }
                    }
                }

                break;
            }
            case SPELL_EFFECT_STEAL_BENEFICIAL_BUFF:
            {
                if (m_targets.getUnitTarget() == m_caster)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }
                break;
            }
            default: break;
        }
    }

    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        // Do not check in case of junk in DBC
        if (!IsAuraApplyEffect(m_spellInfo, SpellEffectIndex(i)))
        {
            continue;
        }

        // Possible Unit-target for the spell
        Unit* expectedTarget = GetPrefilledUnitTargetOrUnitTarget(SpellEffectIndex(i));

        switch (m_spellInfo->EffectApplyAuraName[i])
        {
            case SPELL_AURA_DUMMY:
            {
                // custom check
                switch (m_spellInfo->Id)
                {
                    case 34026:                             // Kill Command
                        if (!m_caster->GetPet())
                        {
                            return SPELL_FAILED_NO_PET;
                        }
                        break;
                    case 61336:                             // Survival Instincts
                        if (m_caster->GetTypeId() != TYPEID_PLAYER || !((Player*)m_caster)->IsInFeralForm())
                        {
                            return SPELL_FAILED_ONLY_SHAPESHIFT;
                        }
                        break;
                    default:
                        break;
                }
                break;
            }
            case SPELL_AURA_MOD_POSSESS:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                {
                    return SPELL_FAILED_UNKNOWN;
                }

                if (expectedTarget == m_caster)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                if (m_caster->GetPetGuid())
                {
                    return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                }

                if (m_caster->GetCharmGuid())
                {
                    return SPELL_FAILED_ALREADY_HAVE_CHARM;
                }

                if (m_caster->GetCharmerGuid())
                {
                    return SPELL_FAILED_CHARMED;
                }

                if (!expectedTarget)
                {
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                }

                if (expectedTarget->GetCharmerGuid())
                {
                    return SPELL_FAILED_CHARMED;
                }

                if (int32(expectedTarget->getLevel()) > CalculateDamage(SpellEffectIndex(i), expectedTarget))
                {
                    return SPELL_FAILED_HIGHLEVEL;
                }

                break;
            }
            case SPELL_AURA_MOD_CHARM:
            {
                if (expectedTarget == m_caster)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                if (m_caster->GetPetGuid())
                {
                    return SPELL_FAILED_ALREADY_HAVE_SUMMON;
                }

                if (m_caster->GetCharmGuid())
                {
                    return SPELL_FAILED_ALREADY_HAVE_CHARM;
                }

                if (m_caster->GetCharmerGuid())
                {
                    return SPELL_FAILED_CHARMED;
                }

                if (!expectedTarget)
                {
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                }

                if (expectedTarget->GetCharmerGuid())
                {
                    return SPELL_FAILED_CHARMED;
                }

                if (int32(expectedTarget->getLevel()) > CalculateDamage(SpellEffectIndex(i), expectedTarget))
                {
                    return SPELL_FAILED_HIGHLEVEL;
                }

                break;
            }
            case SPELL_AURA_MOD_POSSESS_PET:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                {
                    return SPELL_FAILED_UNKNOWN;
                }

                if (m_caster->GetCharmGuid())
                {
                    return SPELL_FAILED_ALREADY_HAVE_CHARM;
                }

                if (m_caster->GetCharmerGuid())
                {
                    return SPELL_FAILED_CHARMED;
                }

                Pet* pet = m_caster->GetPet();
                if (!pet)
                {
                    return SPELL_FAILED_NO_PET;
                }

                if (pet->GetCharmerGuid())
                {
                    return SPELL_FAILED_CHARMED;
                }

                break;
            }
            case SPELL_AURA_MOUNTED:
            {
                if (m_caster->IsInWater())
                {
                    return SPELL_FAILED_ONLY_ABOVEWATER;
                }

                if (m_caster->GetTypeId() == TYPEID_PLAYER && ((Player*)m_caster)->GetTransport())
                {
                    return SPELL_FAILED_NO_MOUNTS_ALLOWED;
                }

                // Ignore map check if spell have AreaId. AreaId already checked and this prevent special mount spells
                if (m_caster->GetTypeId() == TYPEID_PLAYER && !sMapStore.LookupEntry(m_caster->GetMapId())->IsMountAllowed() && !m_IsTriggeredSpell && !m_spellInfo->AreaGroupId)
                {
                    return SPELL_FAILED_NO_MOUNTS_ALLOWED;
                }

                if (m_caster->IsInDisallowedMountForm())
                {
                    return SPELL_FAILED_NOT_SHAPESHIFT;
                }

                break;
            }
            case SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS:
            {
                if (!expectedTarget)
                {
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                }

                // can be casted at non-friendly unit or own pet/charm
                if (m_caster->IsFriendlyTo(expectedTarget))
                {
                    return SPELL_FAILED_TARGET_FRIENDLY;
                }

                break;
            }
            case SPELL_AURA_FLY:
            case SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED:
            {
                // not allow cast fly spells if not have req. skills  (all spells is self target)
                // allow always ghost flight spells
                if (m_caster->GetTypeId() == TYPEID_PLAYER && m_caster->IsAlive())
                {
                    if (!((Player*)m_caster)->CanStartFlyInArea(m_caster->GetMapId(), zone, area))
                    {
                        return m_IsTriggeredSpell ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_NOT_HERE;
                    }
                }
                break;
            }
            case SPELL_AURA_PERIODIC_MANA_LEECH:
            {
                if (!expectedTarget)
                {
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                }

                if (m_caster->GetTypeId() != TYPEID_PLAYER || m_CastItem)
                {
                    break;
                }

                if (expectedTarget->GetPowerType() != POWER_MANA)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                break;
            }
            case SPELL_AURA_CONTROL_VEHICLE:
            {
                if (m_caster->HasAuraType(SPELL_AURA_MOUNTED))
                {
                    return SPELL_FAILED_NOT_MOUNTED;
                }

                if (!expectedTarget || !expectedTarget->IsVehicle())
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                // It is possible to change between vehicles that are boarded on each other
                if (m_caster->IsBoarded() && m_caster->GetTransportInfo()->IsOnVehicle())
                {
                    // Check if trying to board a vehicle that is boarded on current transport
                    bool boardedOnEachOther = m_caster->GetTransportInfo()->HasOnBoard(expectedTarget);
                    // Check if trying to board a vehicle that has the current transport on board
                    if (!boardedOnEachOther)
                    {
                        boardedOnEachOther = expectedTarget->GetVehicleInfo()->HasOnBoard(m_caster);
                    }

                    if (!boardedOnEachOther)
                    {
                        return SPELL_FAILED_NOT_ON_TRANSPORT;
                    }
                }

                if (!expectedTarget->GetVehicleInfo()->CanBoard(m_caster))
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                break;
            }
            case SPELL_AURA_MIRROR_IMAGE:
            {
                if (!expectedTarget)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                // Target must be creature. TODO: Check if target can also be player
                if (expectedTarget->GetTypeId() != TYPEID_UNIT)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                if (expectedTarget == m_caster)             // Clone self can't be accepted
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                // It is assumed that target can not be cloned if already cloned by same or other clone auras
                if (expectedTarget->HasAuraType(SPELL_AURA_MIRROR_IMAGE))
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }

                break;
            }
            default:
                break;
        }
    }

    // check trade slot case (last, for allow catch any another cast problems)
    if (m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
    {
        if (m_caster->GetTypeId() != TYPEID_PLAYER)
        {
            return SPELL_FAILED_NOT_TRADING;
        }

        Player* pCaster = ((Player*)m_caster);
        TradeData* my_trade = pCaster->GetTradeData();

        if (!my_trade)
        {
            return SPELL_FAILED_NOT_TRADING;
        }

        TradeSlots slot = TradeSlots(m_targets.getItemTargetGuid().GetRawValue());
        if (slot != TRADE_SLOT_NONTRADED)
        {
            return SPELL_FAILED_ITEM_NOT_READY;
        }

        // if trade not complete then remember it in trade data
        if (!my_trade->IsInAcceptProcess())
        {
            // Spell will be casted at completing the trade. Silently ignore at this place
            my_trade->SetSpell(m_spellInfo->Id, m_CastItem);
            return SPELL_FAILED_DONT_REPORT;
        }
    }

    // all ok
    return SPELL_CAST_OK;
}

/**
 * @brief Validates whether a pet or charmed unit can cast the spell.
 *
 * @param target An optional explicit target override.
 * @return The resulting cast status.
 */
SpellCastResult Spell::CheckPetCast(Unit* target)
{
    if (!m_caster->IsAlive())
    {
        return SPELL_FAILED_CASTER_DEAD;
    }

    if (m_caster->IsNonMeleeSpellCasted(false))             // prevent spellcast interruption by another spellcast
    {
        return SPELL_FAILED_SPELL_IN_PROGRESS;
    }
    if (m_caster->IsInCombat() && IsNonCombatSpell(m_spellInfo))
    {
        return SPELL_FAILED_AFFECTING_COMBAT;
    }

    if (m_caster->GetTypeId() == TYPEID_UNIT && (((Creature*)m_caster)->IsPet() || m_caster->IsCharmed()))
    {
        // dead owner (pets still alive when owners ressed?)
        if (m_caster->GetCharmerOrOwner() && !m_caster->GetCharmerOrOwner()->IsAlive())
        {
            return SPELL_FAILED_CASTER_DEAD;
        }

        if (!target && m_targets.getUnitTarget())
        {
            target = m_targets.getUnitTarget();
        }

        bool need = false;
        for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (m_spellInfo->EffectImplicitTargetA[i] == TARGET_CHAIN_DAMAGE ||
                m_spellInfo->EffectImplicitTargetA[i] == TARGET_SINGLE_FRIEND ||
                m_spellInfo->EffectImplicitTargetA[i] == TARGET_SINGLE_FRIEND_2 ||
                m_spellInfo->EffectImplicitTargetA[i] == TARGET_DUELVSPLAYER ||
                m_spellInfo->EffectImplicitTargetA[i] == TARGET_SINGLE_PARTY ||
                m_spellInfo->EffectImplicitTargetA[i] == TARGET_CURRENT_ENEMY_COORDINATES)
            {
                need = true;
                if (!target)
                {
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;
                }
                break;
            }
        }
        if (need)
        {
            m_targets.setUnitTarget(target);
        }

        Unit* _target = m_targets.getUnitTarget();

        if (_target && m_targets.m_targetMask & TARGET_FLAG_UNIT)            // for target dead/target not valid
        {
            if (IsPositiveSpell(m_spellInfo->Id))
            {
                if (m_caster->IsHostileTo(_target))
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }
            }
            else
            {
                if (!_target->IsTargetableForAttack())
                {
                    return SPELL_FAILED_BAD_TARGETS;             // guessed error
                }

                bool duelvsplayertar = false;
                for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
                {
                    // TARGET_DUELVSPLAYER is positive AND negative
                    duelvsplayertar |= (m_spellInfo->EffectImplicitTargetA[j] == TARGET_DUELVSPLAYER);
                }
                if (m_caster->IsFriendlyTo(target) && !duelvsplayertar)
                {
                    return SPELL_FAILED_BAD_TARGETS;
                }
            }
        }
        // cooldown
        if (((Creature*)m_caster)->HasSpellCooldown(m_spellInfo->Id))
        {
            return SPELL_FAILED_NOT_READY;
        }
    }

    return CheckCast(true);
}

/**
 * @brief Checks whether active caster auras prevent this spell from being cast.
 *
 * @return The resulting cast status.
 */
SpellCastResult Spell::CheckCasterAuras() const
{
    // Flag drop spells totally immuned to caster auras
    // FIXME: find more nice check for all totally immuned spells
    // HasAttribute(SPELL_ATTR_EX3_UNK28) ?
    if (m_spellInfo->Id == 23336 ||                         // Alliance Flag Drop
        m_spellInfo->Id == 23334 ||                         // Horde Flag Drop
        m_spellInfo->Id == 34991)                           // Summon Netherstorm Flag
    {
        return SPELL_CAST_OK;
    }

    uint8 school_immune = 0;
    uint32 mechanic_immune = 0;
    uint32 dispel_immune = 0;

    // Check if the spell grants school or mechanic immunity.
    // We use bitmasks so the loop is done only once and not on every aura check below.
    if (m_spellInfo->HasAttribute(SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY))
    {
        for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            if (m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_SCHOOL_IMMUNITY)
            {
                school_immune |= uint32(m_spellInfo->EffectMiscValue[i]);
            }
            else if (m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MECHANIC_IMMUNITY)
            {
                mechanic_immune |= 1 << uint32(m_spellInfo->EffectMiscValue[i] - 1);
            }
            else if (m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_MECHANIC_IMMUNITY_MASK)
            {
                mechanic_immune |= uint32(m_spellInfo->EffectMiscValue[i]);
            }
            else if (m_spellInfo->EffectApplyAuraName[i] == SPELL_AURA_DISPEL_IMMUNITY)
            {
                dispel_immune |= GetDispellMask(DispelType(m_spellInfo->EffectMiscValue[i]));
            }
        }

        // immune movement impairment and loss of control (spell data have special structure for mark this case)
        if (IsSpellRemoveAllMovementAndControlLossEffects(m_spellInfo))
        {
            mechanic_immune = IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;
        }
    }

    // Check whether the cast should be prevented by any state you might have.
    SpellCastResult prevented_reason = SPELL_CAST_OK;
    bool spellUsableWhileStunned = m_spellInfo->HasAttribute(SPELL_ATTR_EX5_USABLE_WHILE_STUNNED);

    // Have to check if there is a stun aura. Otherwise will have problems with ghost aura apply while logging out
    uint32 unitflag = m_caster->GetUInt32Value(UNIT_FIELD_FLAGS);     // Get unit state
    if (unitflag & UNIT_FLAG_STUNNED)
    {
        // Pain Suppression (have SPELL_ATTR_EX5_USABLE_WHILE_STUNNED that must be used only with glyph)
        if (m_spellInfo->SpellFamilyName == SPELLFAMILY_PRIEST && m_spellInfo->SpellIconID == 2178)
        {
            if (!m_caster->HasAura(63248))                  // Glyph of Pain Suppression
            {
                spellUsableWhileStunned = false;
            }
        }

        // spell is usable while stunned, check if caster has only mechanic stun auras, another stun types must prevent cast spell
        if (spellUsableWhileStunned)
        {
            bool is_stun_mechanic = true;
            Unit::AuraList const& stunAuras = m_caster->GetAurasByType(SPELL_AURA_MOD_STUN);
            for (Unit::AuraList::const_iterator itr = stunAuras.begin(); itr != stunAuras.end(); ++itr)
            {
                if (!(*itr)->HasMechanic(MECHANIC_STUN))
                {
                    is_stun_mechanic = false;
                    break;
                }
            }
            if (!is_stun_mechanic)
            {
                prevented_reason = SPELL_FAILED_STUNNED;
            }
        }
        else
        {
            prevented_reason = SPELL_FAILED_STUNNED;
        }
    }
    else if (unitflag & UNIT_FLAG_CONFUSED && !m_spellInfo->HasAttribute(SPELL_ATTR_EX5_USABLE_WHILE_CONFUSED))
    {
        prevented_reason = SPELL_FAILED_CONFUSED;
    }
    else if (unitflag & UNIT_FLAG_FLEEING && !m_spellInfo->HasAttribute(SPELL_ATTR_EX5_USABLE_WHILE_FEARED))
    {
        prevented_reason = SPELL_FAILED_FLEEING;
    }
    else if (unitflag& UNIT_FLAG_SILENCED && m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
    {
        prevented_reason = SPELL_FAILED_SILENCED;
    }
    else if (unitflag & UNIT_FLAG_PACIFIED && m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY)
    {
        prevented_reason = SPELL_FAILED_PACIFIED;
    }
    else if (m_caster->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY))
    {
        Unit::AuraList const& casingLimit = m_caster->GetAurasByType(SPELL_AURA_ALLOW_ONLY_ABILITY);
        for (Unit::AuraList::const_iterator itr = casingLimit.begin(); itr != casingLimit.end(); ++itr)
        {
            if (!(*itr)->isAffectedOnSpell(m_spellInfo))
            {
                prevented_reason = SPELL_FAILED_CASTER_AURASTATE;
                break;
            }
        }
    }

    // Attr must make flag drop spell totally immune from all effects
    if (prevented_reason != SPELL_CAST_OK)
    {
        if (school_immune || mechanic_immune || dispel_immune)
        {
            // Checking auras is needed now, because you are prevented by some state but the spell grants immunity.
            Unit::SpellAuraHolderMap const& auras = m_caster->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                SpellAuraHolder* holder = itr->second;
                SpellEntry const* pEntry = holder->GetSpellProto();

                if ((GetSpellSchoolMask(pEntry) & school_immune) && !pEntry->HasAttribute(SPELL_ATTR_EX_UNAFFECTED_BY_SCHOOL_IMMUNE))
                {
                    continue;
                }
                if ((1 << (pEntry->Dispel)) & dispel_immune)
                {
                    continue;
                }

                for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
                {
                    Aura* aura = holder->GetAuraByEffectIndex(SpellEffectIndex(i));
                    if (!aura)
                    {
                        continue;
                    }

                    if (GetSpellMechanicMask(pEntry, 1 << i) & mechanic_immune)
                    {
                        continue;
                    }
                    // Make a second check for spell failed so the right SPELL_FAILED message is returned.
                    // That is needed when your casting is prevented by multiple states and you are only immune to some of them.
                    switch (aura->GetModifier()->m_auraname)
                    {
                        case SPELL_AURA_MOD_STUN:
                            if (!spellUsableWhileStunned || !aura->HasMechanic(MECHANIC_STUN))
                            {
                                return SPELL_FAILED_STUNNED;
                            }
                            break;
                        case SPELL_AURA_MOD_CONFUSE:
                            if (!m_spellInfo->HasAttribute(SPELL_ATTR_EX5_USABLE_WHILE_CONFUSED))
                            {
                                return SPELL_FAILED_CONFUSED;
                            }
                            break;
                        case SPELL_AURA_MOD_FEAR:
                            if (!m_spellInfo->HasAttribute(SPELL_ATTR_EX5_USABLE_WHILE_FEARED))
                            {
                                return SPELL_FAILED_FLEEING;
                            }
                            break;
                        case SPELL_AURA_MOD_SILENCE:
                        case SPELL_AURA_MOD_PACIFY:
                        case SPELL_AURA_MOD_PACIFY_SILENCE:
                            if (m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_PACIFY)
                            {
                                return SPELL_FAILED_PACIFIED;
                            }
                            else if (m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                            {
                                return SPELL_FAILED_SILENCED;
                            }
                            break;
                        default: break;
                    }
                }
            }
        }
        // You are prevented from casting and the spell casted does not grant immunity. Return a failed error.
        else
        {
            return prevented_reason;
        }
    }
    return SPELL_CAST_OK;
}

/**
 * @brief Checks whether the spell can be automatically cast on a target.
 *
 * @param target The target being evaluated.
 * @return True if automatic casting is allowed; otherwise, false.
 */
bool Spell::CanAutoCast(Unit* target)
{
    ObjectGuid targetguid = target->GetObjectGuid();

    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        if (m_spellInfo->Effect[j] == SPELL_EFFECT_APPLY_AURA)
        {
            if (m_spellInfo->StackAmount <= 1)
            {
                if (target->HasAura(m_spellInfo->Id, SpellEffectIndex(j)))
                {
                    return false;
                }
            }
            else
            {
                if (Aura* aura = target->GetAura(m_spellInfo->Id, SpellEffectIndex(j)))
                    if (aura->GetStackAmount() >= m_spellInfo->StackAmount)
                    {
                        return false;
                    }
            }
        }
        else if (IsAreaAuraEffect(m_spellInfo->Effect[j]))
        {
            if (target->HasAura(m_spellInfo->Id, SpellEffectIndex(j)))
            {
                return false;
            }
        }
    }

    SpellCastResult result = CheckPetCast(target);

    if (result == SPELL_CAST_OK || result == SPELL_FAILED_UNIT_NOT_INFRONT)
    {
        FillTargetMap();
        // check if among target units, our WANTED target is as well (->only self cast spells return false)
        for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
        {
            if (ihit->targetGUID == targetguid)
            {
                return true;
            }
        }
    }
    return false;                                           // target invalid
}

/**
 * @brief Validates spell range requirements for the current targets.
 *
 * @param strict True to use strict range validation.
 * @return The resulting cast status.
 */
SpellCastResult Spell::CheckRange(bool strict)
{
    Unit* target = m_targets.getUnitTarget();

    // special range cases
    switch (m_spellInfo->rangeIndex)
    {
        // self cast doesn't need range checking -- also for Starshards fix
        // spells that can be cast anywhere also need no check
        case SPELL_RANGE_IDX_SELF_ONLY:
        case SPELL_RANGE_IDX_ANYWHERE:
            return SPELL_CAST_OK;
        // combat range spells are treated differently
        case SPELL_RANGE_IDX_COMBAT:
        {
            if (target)
            {
                if (target == m_caster)
                {
                    return SPELL_CAST_OK;
                }

                float range_mod = strict ? 0.0f : 5.0f;
                if (Player* modOwner = m_caster->GetSpellModOwner())
                {
                    float base = ATTACK_DISTANCE;
                    range_mod += modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, base);
                }

                // with additional 5 dist for non stricted case (some melee spells have delay in apply
                return m_caster->CanReachWithMeleeAttack(target, range_mod) ? SPELL_CAST_OK : SPELL_FAILED_OUT_OF_RANGE;
            }
            break;                                          // let continue in generic way for no target
        }
    }

    // add radius of caster and ~5 yds "give" for non stricred (landing) check
    float range_mod = strict ? 1.25f : 6.25;

    SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex);
    bool friendly = target ? target->IsFriendlyTo(m_caster) : false;
    float max_range = GetSpellMaxRange(srange, friendly) + range_mod;
    float min_range = GetSpellMinRange(srange, friendly);

    if (Player* modOwner = m_caster->GetSpellModOwner())
    {
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RANGE, max_range);
    }

    if (target && target != m_caster)
    {
        // distance from target in checks
        float dist = m_caster->GetCombatDistance(target, m_spellInfo->rangeIndex == SPELL_RANGE_IDX_COMBAT);

        if (dist > max_range)
        {
            return SPELL_FAILED_OUT_OF_RANGE;
        }
        if (min_range && dist < min_range)
        {
            return SPELL_FAILED_TOO_CLOSE;
        }
        if (m_caster->GetTypeId() == TYPEID_PLAYER &&
                (m_spellInfo->FacingCasterFlags & SPELL_FACING_FLAG_INFRONT) && !m_caster->HasInArc(M_PI_F, target))
        {
            return SPELL_FAILED_UNIT_NOT_INFRONT;
        }
    }

    // TODO verify that such spells really use bounding radius
    if (m_targets.m_targetMask == TARGET_FLAG_DEST_LOCATION && m_targets.m_destX != 0 && m_targets.m_destY != 0 && m_targets.m_destZ != 0)
    {
        if (!m_caster->IsWithinDist3d(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, max_range))
        {
            return SPELL_FAILED_OUT_OF_RANGE;
        }
        if (min_range && m_caster->IsWithinDist3d(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, min_range))
        {
            return SPELL_FAILED_TOO_CLOSE;
        }
    }

    return SPELL_CAST_OK;
}

/**
 * @brief Calculates the final power cost for a spell cast.
 *
 * @param spellInfo The spell prototype being cast.
 * @param caster The casting unit.
 * @param spell The active spell instance, if available.
 * @param castItem The cast item, if the spell originates from an item.
 * @return The resulting power cost.
 */
uint32 Spell::CalculatePowerCost(SpellEntry const* spellInfo, Unit* caster, Spell const* spell, Item* castItem)
{
    // item cast not used power
    if (castItem)
    {
        return 0;
    }

    // Spell drain all exist power on cast (Only paladin lay of Hands)
    if (spellInfo->HasAttribute(SPELL_ATTR_EX_DRAIN_ALL_POWER))
    {
        // If power type - health drain all
        if (spellInfo->powerType == POWER_HEALTH)
        {
            return caster->GetHealth();
        }
        // Else drain all power
        if (spellInfo->powerType < MAX_POWERS)
        {
            return caster->GetPower(Powers(spellInfo->powerType));
        }
        sLog.outError("Spell::CalculateManaCost: Unknown power type '%d' in spell %d", spellInfo->powerType, spellInfo->Id);
        return 0;
    }

    // Base powerCost
    int32 powerCost = spellInfo->manaCost;
    // PCT cost from total amount
    if (spellInfo->ManaCostPercentage)
    {
        switch (spellInfo->powerType)
        {
                // health as power used
            case POWER_HEALTH:
                powerCost += spellInfo->ManaCostPercentage * caster->GetCreateHealth() / 100;
                break;
            case POWER_MANA:
                powerCost += spellInfo->ManaCostPercentage * caster->GetCreateMana() / 100;
                break;
            case POWER_RAGE:
            case POWER_FOCUS:
            case POWER_ENERGY:
            case POWER_HAPPINESS:
                powerCost += spellInfo->ManaCostPercentage * caster->GetMaxPower(Powers(spellInfo->powerType)) / 100;
                break;
            case POWER_RUNE:
            case POWER_RUNIC_POWER:
                DEBUG_LOG("Spell::CalculateManaCost: Not implemented yet!");
                break;
            default:
                sLog.outError("Spell::CalculateManaCost: Unknown power type '%d' in spell %d", spellInfo->powerType, spellInfo->Id);
                return 0;
        }
    }

    SpellSchools school = GetFirstSchoolInMask(spell ? spell->m_spellSchoolMask : GetSpellSchoolMask(spellInfo));
    // Flat mod from caster auras by spell school
    powerCost += caster->GetInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + school);
    // Shiv - costs 20 + weaponSpeed*10 energy (apply only to non-triggered spell with energy cost)
    if (spellInfo->HasAttribute(SPELL_ATTR_EX4_SPELL_VS_EXTEND_COST))
    {
        powerCost += caster->GetAttackTime(OFF_ATTACK) / 100;
    }
    // Apply cost mod by spell
    if (spell)
    {
        if (Player* modOwner = caster->GetSpellModOwner())
        {
            modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_COST, powerCost);
        }
    }

    if (spellInfo->HasAttribute(SPELL_ATTR_LEVEL_DAMAGE_CALCULATION))
    {
        powerCost = int32(powerCost / (1.117f * spellInfo->spellLevel / caster->getLevel() - 0.1327f));
    }

    // PCT mod from user auras by school
    powerCost = int32(powerCost * (1.0f + caster->GetFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + school)));
    if (powerCost < 0)
    {
        powerCost = 0;
    }
    return powerCost;
}

/**
 * @brief Checks whether the caster has enough power to cast the spell.
 *
 * @return The resulting cast status.
 */
SpellCastResult Spell::CheckPower()
{
    // item cast not used power
    if (m_CastItem)
    {
        return SPELL_CAST_OK;
    }

    // Do precise power regen on spell cast
    if (m_powerCost > 0 && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Player* playerCaster = (Player*)m_caster;
        uint32 diff = REGEN_TIME_FULL - m_caster->GetRegenTimer();
        if (diff >= REGEN_TIME_PRECISE)
        {
            playerCaster->RegenerateAll(diff);
        }
    }

    // health as power used - need check health amount
    if (m_spellInfo->powerType == POWER_HEALTH)
    {
        if (m_caster->GetHealth() <= m_powerCost)
        {
            return SPELL_FAILED_CASTER_AURASTATE;
        }
        return SPELL_CAST_OK;
    }

    // Check valid power type
    if (m_spellInfo->powerType >= MAX_POWERS)
    {
        sLog.outError("Spell::CheckMana: Unknown power type '%d'", m_spellInfo->powerType);
        return SPELL_FAILED_UNKNOWN;
    }

    // check rune cost only if a spell has PowerType == POWER_RUNE
    if (m_spellInfo->powerType == POWER_RUNE)
    {
        SpellCastResult failReason = CheckOrTakeRunePower(false);
        if (failReason != SPELL_CAST_OK)
        {
            return failReason;
        }
    }

    // Check power amount
    Powers powerType = Powers(m_spellInfo->powerType);
    if (m_caster->GetPower(powerType) < m_powerCost)
    {
        return SPELL_FAILED_NO_POWER;
    }

    return SPELL_CAST_OK;
}

/**
 * @brief Determines whether reagent and item requirements should be ignored.
 *
 * @return True if item requirements are ignored; otherwise, false.
 */
bool Spell::IgnoreItemRequirements() const
{
    /// Check if it's an enchant scroll. These have no required reagents even though their spell does.
    if (m_CastItem && (m_CastItem->GetProto()->Flags & ITEM_FLAG_ENCHANT_SCROLL))
    {
        return true;
    }

    if (m_IsTriggeredSpell)
    {
        /// Not own traded item (in trader trade slot) req. reagents including triggered spell case
        if (Item* targetItem = m_targets.getItemTarget())
        {
            if (targetItem->GetOwnerGuid() != m_caster->GetObjectGuid())
            {
                return false;
            }
        }

        /// Some triggered spells have same reagents that have master spell
        /// expected in test: master spell have reagents in first slot then triggered don't must use own
        if (m_triggeredBySpellInfo && !m_triggeredBySpellInfo->Reagent[0])
        {
            return false;
        }

        return true;
    }

    return false;
}

/**
 * @brief Validates cast item, target item, reagent, focus, and item-based spell requirements.
 *
 * @return The resulting cast status.
 */
SpellCastResult Spell::CheckItems()
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return SPELL_CAST_OK;
    }

    Player* p_caster = (Player*)m_caster;
    bool isScrollItem = false;
    bool isVellumTarget = false;

    // cast item checks
    if (m_CastItem)
    {
        if (m_CastItem->IsInTrade())
        {
            return SPELL_FAILED_ITEM_NOT_FOUND;
        }

        uint32 itemid = m_CastItem->GetEntry();
        if (!p_caster->HasItemCount(itemid, 1))
        {
            return SPELL_FAILED_ITEM_NOT_FOUND;
        }

        ItemPrototype const* proto = m_CastItem->GetProto();
        if (!proto)
        {
            return SPELL_FAILED_ITEM_NOT_FOUND;
        }

        if (proto->Flags & ITEM_FLAG_ENCHANT_SCROLL)
        {
            isScrollItem = true;
        }

        for (int i = 0; i < 5; ++i)
        {
            if (proto->Spells[i].SpellCharges)
            {
                if (m_CastItem->GetSpellCharges(i) == 0)
                {
                    return SPELL_FAILED_NO_CHARGES_REMAIN;
                }
            }
        }

        // consumable cast item checks
        if (proto->Class == ITEM_CLASS_CONSUMABLE && m_targets.getUnitTarget())
        {
            // such items should only fail if there is no suitable effect at all - see Rejuvenation Potions for example
            SpellCastResult failReason = SPELL_CAST_OK;
            for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                // skip check, pet not required like checks, and for TARGET_PET m_targets.getUnitTarget() is not the real target but the caster
                if (m_spellInfo->EffectImplicitTargetA[i] == TARGET_PET)
                {
                    continue;
                }

                if (m_spellInfo->Effect[i] == SPELL_EFFECT_HEAL)
                {
                    if (m_targets.getUnitTarget()->GetHealth() == m_targets.getUnitTarget()->GetMaxHealth())
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_HEALTH;
                        continue;
                    }
                    else
                    {
                        failReason = SPELL_CAST_OK;
                        break;
                    }
                }

                // Mana Potion, Rage Potion, Thistle Tea(Rogue), ...
                if (m_spellInfo->Effect[i] == SPELL_EFFECT_ENERGIZE)
                {
                    if (m_spellInfo->EffectMiscValue[i] < 0 || m_spellInfo->EffectMiscValue[i] >= MAX_POWERS)
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_POWER;
                        continue;
                    }

                    Powers power = Powers(m_spellInfo->EffectMiscValue[i]);
                    if (m_targets.getUnitTarget()->GetPower(power) == m_targets.getUnitTarget()->GetMaxPower(power))
                    {
                        failReason = SPELL_FAILED_ALREADY_AT_FULL_POWER;
                        continue;
                    }
                    else
                    {
                        failReason = SPELL_CAST_OK;
                        break;
                    }
                }
            }
            if (failReason != SPELL_CAST_OK)
            {
                return failReason;
            }
        }
    }

    // check target item (for triggered case not report error)
    if (m_targets.getItemTargetGuid())
    {
        if (m_caster->GetTypeId() != TYPEID_PLAYER)
        {
            return m_IsTriggeredSpell && !(m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                   ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_BAD_TARGETS;
        }

        if (!m_targets.getItemTarget())
        {
            return m_IsTriggeredSpell  && !(m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                   ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_ITEM_GONE;
        }

        isVellumTarget = m_targets.getItemTarget()->GetProto()->IsVellum();
        if (!m_targets.getItemTarget()->IsFitToSpellRequirements(m_spellInfo))
        {
            return m_IsTriggeredSpell  && !(m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                   ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;
        }

        // Do not enchant vellum with scroll
        if (isVellumTarget && isScrollItem)
        {
            return m_IsTriggeredSpell  && !(m_targets.m_targetMask & TARGET_FLAG_TRADE_ITEM)
                   ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_BAD_TARGETS;
        }
    }
    // if not item target then required item must be equipped (for triggered case not report error)
    else
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER && !((Player*)m_caster)->HasItemFitToSpellReqirements(m_spellInfo))
        {
            return m_IsTriggeredSpell ? SPELL_FAILED_DONT_REPORT : SPELL_FAILED_EQUIPPED_ITEM_CLASS;
        }
    }

    // check reagents (ignore triggered spells with reagents processed by original spell) and special reagent ignore case.
    if (!IgnoreItemRequirements())
    {
        if (!p_caster->CanNoReagentCast(m_spellInfo))
        {
            for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
            {
                if (m_spellInfo->Reagent[i] <= 0)
                {
                    continue;
                }

                uint32 itemid    = m_spellInfo->Reagent[i];
                uint32 itemcount = m_spellInfo->ReagentCount[i];

                // if CastItem is also spell reagent
                if (m_CastItem && m_CastItem->GetEntry() == itemid)
                {
                    ItemPrototype const* proto = m_CastItem->GetProto();
                    if (!proto)
                    {
                        return SPELL_FAILED_REAGENTS;
                    }
                    for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
                    {
                        // CastItem will be used up and does not count as reagent
                        int32 charges = m_CastItem->GetSpellCharges(s);
                        if (proto->Spells[s].SpellCharges < 0 && !(proto->ExtraFlags & ITEM_EXTRA_NON_CONSUMABLE) && abs(charges) < 2)
                        {
                            ++itemcount;
                            break;
                        }
                    }
                }

                if (!p_caster->HasItemCount(itemid, itemcount))
                {
                    return SPELL_FAILED_REAGENTS;
                }
            }
        }

        // check totem-item requirements (items presence in inventory)
        uint32 totems = MAX_SPELL_TOTEMS;
        for (int i = 0; i < MAX_SPELL_TOTEMS ; ++i)
        {
            if (m_spellInfo->Totem[i] != 0)
            {
                if (p_caster->HasItemCount(m_spellInfo->Totem[i], 1))
                {
                    totems -= 1;
                    continue;
                }
            }
            else
            {
                totems -= 1;
            }
        }

        if (totems != 0)
        {
            return SPELL_FAILED_TOTEMS;
        }

        // Check items for TotemCategory  (items presence in inventory)
        uint32 TotemCategory = MAX_SPELL_TOTEM_CATEGORIES;
        for (int i = 0; i < MAX_SPELL_TOTEM_CATEGORIES; ++i)
        {
            if (m_spellInfo->TotemCategory[i] != 0)
            {
                if (p_caster->HasItemTotemCategory(m_spellInfo->TotemCategory[i]))
                {
                    TotemCategory -= 1;
                    continue;
                }
            }
            else
            {
                TotemCategory -= 1;
            }
        }

        if (TotemCategory != 0)
        {
            return SPELL_FAILED_TOTEM_CATEGORY;
        }
    }

    // special checks for spell effects
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        switch (m_spellInfo->Effect[i])
        {
            case SPELL_EFFECT_CREATE_ITEM:
            {
                if (!m_IsTriggeredSpell && m_spellInfo->EffectItemType[i])
                {
                    // Conjure Mana Gem (skip same or low level ranks for later recharge)
                    if (i == EFFECT_INDEX_0 && m_spellInfo->Effect[EFFECT_INDEX_1] == SPELL_EFFECT_DUMMY)
                    {
                        if (ItemPrototype const* itemProto = ObjectMgr::GetItemPrototype(m_spellInfo->EffectItemType[i]))
                        {
                            if (Item* item = p_caster->GetItemByLimitedCategory(itemProto->ItemLimitCategory))
                            {
                                if (item->GetProto()->ItemLevel <= itemProto->ItemLevel)
                                {
                                    if (item->HasMaxCharges())
                                    {
                                        return SPELL_FAILED_ITEM_AT_MAX_CHARGES;
                                    }

                                    // will recharge in next effect
                                    continue;
                                }
                            }
                        }
                    }

                    ItemPosCountVec dest;
                    InventoryResult msg = p_caster->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, m_spellInfo->EffectItemType[i], 1);
                    if (msg != EQUIP_ERR_OK)
                    {
                        p_caster->SendEquipError(msg, NULL, NULL, m_spellInfo->EffectItemType[i]);
                        return SPELL_FAILED_DONT_REPORT;
                    }
                }
                break;
            }
            case SPELL_EFFECT_RESTORE_ITEM_CHARGES:
            {
                if (Item* item = p_caster->GetItemByEntry(m_spellInfo->EffectItemType[i]))
                {
                    if (item->HasMaxCharges())
                    {
                        return SPELL_FAILED_ITEM_AT_MAX_CHARGES;
                    }
                }
                break;
            }
            case SPELL_EFFECT_ENCHANT_ITEM:
            case SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC:
            {
                Item* targetItem = m_targets.getItemTarget();
                if (!targetItem)
                {
                    return SPELL_FAILED_ITEM_NOT_FOUND;
                }

                if (targetItem->GetProto()->ItemLevel < m_spellInfo->baseLevel)
                {
                    return SPELL_FAILED_LOWLEVEL;
                }
                // Check if we can store a new scroll, enchanting vellum has implicit SPELL_EFFECT_CREATE_ITEM
                if (isVellumTarget && m_spellInfo->EffectItemType[i])
                {
                    ItemPosCountVec dest;
                    InventoryResult msg = p_caster->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, m_spellInfo->EffectItemType[i], 1);
                    if (msg != EQUIP_ERR_OK)
                    {
                        p_caster->SendEquipError(msg, NULL, NULL);
                        return SPELL_FAILED_DONT_REPORT;
                    }
                }
                // Not allow enchant in trade slot for some enchant type
                if (targetItem->GetOwner() != m_caster)
                {
                    uint32 enchant_id = m_spellInfo->EffectMiscValue[i];
                    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                    if (!pEnchant)
                    {
                        return SPELL_FAILED_ERROR;
                    }
                    if (pEnchant->slot & ENCHANTMENT_CAN_SOULBOUND)
                    {
                        return SPELL_FAILED_NOT_TRADEABLE;
                    }
                    // cannot replace vellum with scroll in trade slot
                    if (isVellumTarget)
                    {
                        return SPELL_FAILED_ITEM_ENCHANT_TRADE_WINDOW;
                    }
                }
                break;
            }
            case SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY:
            {
                Item* item = m_targets.getItemTarget();
                if (!item)
                {
                    return SPELL_FAILED_ITEM_NOT_FOUND;
                }
                // Not allow enchant in trade slot for some enchant type
                if (item->GetOwner() != m_caster)
                {
                    uint32 enchant_id = m_spellInfo->EffectMiscValue[i];
                    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                    if (!pEnchant)
                    {
                        return SPELL_FAILED_ERROR;
                    }
                    if (pEnchant->slot & ENCHANTMENT_CAN_SOULBOUND)
                    {
                        return SPELL_FAILED_NOT_TRADEABLE;
                    }
                }
                break;
            }
            case SPELL_EFFECT_ENCHANT_HELD_ITEM:
                // check item existence in effect code (not output errors at offhand hold item effect to main hand for example
                break;
            case SPELL_EFFECT_DISENCHANT:
            {
                if (!m_targets.getItemTarget())
                {
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;
                }

                // prevent disenchanting in trade slot
                if (m_targets.getItemTarget()->GetOwnerGuid() != m_caster->GetObjectGuid())
                {
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;
                }

                ItemPrototype const* itemProto = m_targets.getItemTarget()->GetProto();
                if (!itemProto)
                {
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;
                }

                // must have disenchant loot (other static req. checked at item prototype loading)
                if (!itemProto->DisenchantID)
                {
                    return SPELL_FAILED_CANT_BE_DISENCHANTED;
                }

                // 2.0.x addon: Check player enchanting level against the item disenchanting requirements
                int32 item_disenchantskilllevel = itemProto->RequiredDisenchantSkill;
                if (item_disenchantskilllevel > int32(p_caster->GetSkillValue(SKILL_ENCHANTING)))
                {
                    return SPELL_FAILED_LOW_CASTLEVEL;
                }
                break;
            }
            case SPELL_EFFECT_PROSPECTING:
            {
                if (!m_targets.getItemTarget())
                {
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                }
                // ensure item is a prospectable ore
                if (!(m_targets.getItemTarget()->GetProto()->Flags & ITEM_FLAG_PROSPECTABLE))
                {
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                }
                // prevent prospecting in trade slot
                if (m_targets.getItemTarget()->GetOwnerGuid() != m_caster->GetObjectGuid())
                {
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                }
                // Check for enough skill in jewelcrafting
                uint32 item_prospectingskilllevel = m_targets.getItemTarget()->GetProto()->RequiredSkillRank;
                if (item_prospectingskilllevel > p_caster->GetSkillValue(SKILL_JEWELCRAFTING))
                {
                    return SPELL_FAILED_LOW_CASTLEVEL;
                }
                // make sure the player has the required ores in inventory
                if (int32(m_targets.getItemTarget()->GetCount()) < CalculateDamage(SpellEffectIndex(i), m_caster))
                {
                    return SPELL_FAILED_NEED_MORE_ITEMS;
                }

                if (!LootTemplates_Prospecting.HaveLootFor(m_targets.getItemTargetEntry()))
                {
                    return SPELL_FAILED_CANT_BE_PROSPECTED;
                }

                break;
            }
            case SPELL_EFFECT_MILLING:
            {
                if (!m_targets.getItemTarget())
                {
                    return SPELL_FAILED_CANT_BE_MILLED;
                }
                // ensure item is a millable herb
                if (!(m_targets.getItemTarget()->GetProto()->Flags & ITEM_FLAG_MILLABLE))
                {
                    return SPELL_FAILED_CANT_BE_MILLED;
                }
                // prevent milling in trade slot
                if (m_targets.getItemTarget()->GetOwnerGuid() != m_caster->GetObjectGuid())
                {
                    return SPELL_FAILED_CANT_BE_MILLED;
                }
                // Check for enough skill in inscription
                uint32 item_millingskilllevel = m_targets.getItemTarget()->GetProto()->RequiredSkillRank;
                if (item_millingskilllevel > p_caster->GetSkillValue(SKILL_INSCRIPTION))
                {
                    return SPELL_FAILED_LOW_CASTLEVEL;
                }
                // make sure the player has the required herbs in inventory
                if (int32(m_targets.getItemTarget()->GetCount()) < CalculateDamage(SpellEffectIndex(i), m_caster))
                {
                    return SPELL_FAILED_NEED_MORE_ITEMS;
                }

                if (!LootTemplates_Milling.HaveLootFor(m_targets.getItemTargetEntry()))
                {
                    return SPELL_FAILED_CANT_BE_MILLED;
                }

                break;
            }
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            {
                if (m_caster->GetTypeId() != TYPEID_PLAYER)
                {
                    return SPELL_FAILED_TARGET_NOT_PLAYER;
                }
                if (m_attackType != RANGED_ATTACK)
                {
                    break;
                }
                Item* pItem = ((Player*)m_caster)->GetWeaponForAttack(m_attackType, true, false);
                if (!pItem)
                {
                    return SPELL_FAILED_EQUIPPED_ITEM;
                }

                switch (pItem->GetProto()->SubClass)
                {
                    case ITEM_SUBCLASS_WEAPON_THROWN:
                    {
                        uint32 ammo = pItem->GetEntry();
                        if (!((Player*)m_caster)->HasItemCount(ammo, 1))
                        {
                            return SPELL_FAILED_NO_AMMO;
                        }
                        break;
                    }
                    case ITEM_SUBCLASS_WEAPON_GUN:
                    case ITEM_SUBCLASS_WEAPON_BOW:
                    case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                    {
                        uint32 ammo = ((Player*)m_caster)->GetUInt32Value(PLAYER_AMMO_ID);
                        if (!ammo)
                        {
                            // Requires No Ammo
                            if (m_caster->GetDummyAura(46699))
                            {
                                break;                       // skip other checks
                            }

                            return SPELL_FAILED_NO_AMMO;
                        }

                        ItemPrototype const* ammoProto = ObjectMgr::GetItemPrototype(ammo);
                        if (!ammoProto)
                        {
                            return SPELL_FAILED_NO_AMMO;
                        }

                        if (ammoProto->Class != ITEM_CLASS_PROJECTILE)
                        {
                            return SPELL_FAILED_NO_AMMO;
                        }

                        // check ammo ws. weapon compatibility
                        switch (pItem->GetProto()->SubClass)
                        {
                            case ITEM_SUBCLASS_WEAPON_BOW:
                            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                                if (ammoProto->SubClass != ITEM_SUBCLASS_ARROW)
                                {
                                    return SPELL_FAILED_NO_AMMO;
                                }
                                break;
                            case ITEM_SUBCLASS_WEAPON_GUN:
                                if (ammoProto->SubClass != ITEM_SUBCLASS_BULLET)
                                {
                                    return SPELL_FAILED_NO_AMMO;
                                }
                                break;
                            default:
                                return SPELL_FAILED_NO_AMMO;
                        }

                        if (!((Player*)m_caster)->HasItemCount(ammo, 1))
                        {
                            return SPELL_FAILED_NO_AMMO;
                        }
                    };  break;
                    case ITEM_SUBCLASS_WEAPON_WAND:
                        break;
                    default:
                        break;
                }
                break;
            }
            default: break;
        }
    }

    return SPELL_CAST_OK;
}
