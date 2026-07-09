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
 * @file SpellAuraDummy.cpp
 * @brief Cohesion split of SpellAuras.cpp -- HandleAuraDummy handler.
 *        Same Aura/SpellAuraHolder classes; no behaviour change.
 */

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "ObjectAccessor.h"
#include "Policies/Singleton.h"
#include "Totem.h"
#include "Creature.h"
#include "Formulas.h"
#include "BattleGround/BattleGround.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "CreatureAI.h"
#include "ScriptMgr.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Vehicle.h"
#include "CellImpl.h"
#include "Language.h"

void Aura::HandleAuraDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
    {
        return;
    }

    Unit* target = GetTarget();

    // AT APPLY
    if (apply)
    {
        switch (GetSpellProto()->SpellClassSet)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (GetId())
                {
                    case 1515:                              // Tame beast
                        // FIX_ME: this is 2.0.12 threat effect replaced in 2.1.x by dummy aura, must be checked for correctness
                        if (target->CanHaveThreatList())
                            if (Unit* caster = GetCaster())
                            {
                                target->AddThreat(caster, 10.0f, false, GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
                            }
                        return;
                    case 7057:                              // Haunting Spirits
                        // expected to tick with 30 sec period (tick part see in Aura::PeriodicTick)
                        m_isPeriodic = true;
                        m_modifier.periodictime = 30 * IN_MILLISECONDS;
                        m_periodicTimer = m_modifier.periodictime;
                        return;
                    case 10255:                             // Stoned
                    {
                        if (Unit* caster = GetCaster())
                        {
                            if (caster->GetTypeId() != TYPEID_UNIT)
                            {
                                return;
                            }

                            caster->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            caster->addUnitState(UNIT_STAT_ROOT);
                        }
                        return;
                    }
                    case 13139:                             // net-o-matic
                        // root to self part of (root_target->charge->root_self sequence
                        if (Unit* caster = GetCaster())
                        {
                            caster->CastSpell(caster, 13138, true, NULL, this);
                        }
                        return;
                    case 28832:                             // Mark of Korth'azz
                    case 28833:                             // Mark of Blaumeux
                    case 28834:                             // Mark of Rivendare
                    case 28835:                             // Mark of Zeliek
                    {
                        int32 damage = 0;

                        switch (GetStackAmount())
                        {
                            case 1:
                                return;
                            case 2: damage =   500; break;
                            case 3: damage =  1500; break;
                            case 4: damage =  4000; break;
                            case 5: damage = 12500; break;
                            default:
                                damage = 14000 + 1000 * GetStackAmount();
                                break;
                        }

                        if (Unit* caster = GetCaster())
                        {
                            caster->CastCustomSpell(target, 28836, &damage, NULL, NULL, true, NULL, this);
                        }
                        return;
                    }
                    case 31606:                             // Stormcrow Amulet
                    {
                        CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(17970);

                        // we must assume db or script set display id to native at ending flight (if not, target is stuck with this model)
                        if (cInfo)
                        {
                            target->SetDisplayId(Creature::ChooseDisplayId(cInfo));
                        }

                        return;
                    }
                    case 32045:                             // Soul Charge
                    case 32051:
                    case 32052:
                    {
                        // max duration is 2 minutes, but expected to be random duration
                        // real time randomness is unclear, using max 30 seconds here
                        // see further down for expire of this aura
                        GetHolder()->SetAuraDuration(urand(1, 30)*IN_MILLISECONDS);
                        return;
                    }
                    case 33326:                             // Stolen Soul Dispel
                    {
                        target->RemoveAurasDueToSpell(32346);
                        return;
                    }
                    case 36587:                             // Vision Guide
                    {
                        target->CastSpell(target, 36573, true, NULL, this);
                        return;
                    }
                    // Gender spells
                    case 38224:                             // Illidari Agent Illusion
                    case 37096:                             // Blood Elf Illusion
                    case 46354:                             // Blood Elf Illusion
                    {
                        uint8 gender = target->getGender();
                        uint32 spellId;
                        switch (GetId())
                        {
                            case 38224: spellId = (gender == GENDER_MALE ? 38225 : 38227); break;
                            case 37096: spellId = (gender == GENDER_MALE ? 37093 : 37095); break;
                            case 46354: spellId = (gender == GENDER_MALE ? 46355 : 46356); break;
                            default: return;
                        }
                        target->CastSpell(target, spellId, true, NULL, this);
                        return;
                    }
                    case 39850:                             // Rocket Blast
                        if (roll_chance_i(20))              // backfire stun
                        {
                            target->CastSpell(target, 51581, true, NULL, this);
                        }
                        return;
                    case 43873:                             // Headless Horseman Laugh
                        target->PlayDistanceSound(11965);
                        return;
                    case 45963:                             // Call Alliance Deserter
                    {
                        // Escorting Alliance Deserter
                        if (target->GetMiniPet())
                        {
                            target->CastSpell(target, 45957, true);
                        }

                        return;
                    }
                    case 46637:                             // Break Ice
                        target->CastSpell(target, 46638, true, NULL, this);
                        return;
                    case 46699:                             // Requires No Ammo
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            // not use ammo and not allow use
                            ((Player*)target)->RemoveAmmo();
                        return;
                    case 47190:                             // Toalu'u's Spiritual Incense
                        target->CastSpell(target, 47189, true, NULL, this);
                        // allow script to process further (text)
                        break;
                    case 47563:                             // Freezing Cloud
                        target->CastSpell(target, 47574, true, NULL, this);
                        return;
                    case 47593:                             // Freezing Cloud
                        target->CastSpell(target, 47594, true, NULL, this);
                        return;
                    case 48025:                             // Headless Horseman's Mount
                        Spell::SelectMountByAreaAndSkill(target, GetSpellProto(), 51621, 48024, 51617, 48023, 0);
                        return;
                    case 48143:                             // Forgotten Aura
                        // See Death's Door
                        target->CastSpell(target, 48814, true, NULL, this);
                        return;
                    case 51405:                             // Digging for Treasure
                        target->HandleEmote(EMOTE_STATE_WORK);
                        // Pet will be following owner, this makes him stop
                        target->addUnitState(UNIT_STAT_STUNNED);
                        return;
                    case 54729:                             // Winged Steed of the Ebon Blade
                        Spell::SelectMountByAreaAndSkill(target, GetSpellProto(), 0, 0, 54726, 54727, 0);
                        return;
                    case 58600:                             // Restricted Flight Area
                    {
                        if (!target || target->GetTypeId() != TYPEID_PLAYER)
                        {
                            return;
                        }
                        const char* text = sObjectMgr.GetMangosString(LANG_NO_FLY_ZONE, ((Player*)target)->GetSession()->GetSessionDbLocaleIndex());
                        target->MonsterWhisper(text, target, true);
                        return;
                    }
                    case 61187:                             // Twilight Shift (single target)
                    case 61190:                             // Twilight Shift (many targets)
                        target->RemoveAurasDueToSpell(57620);
                        target->CastSpell(target, 61885, true, NULL, this);
                        return;
                    case 62061:                             // Festive Holiday Mount
                        if (target->HasAuraType(SPELL_AURA_MOUNTED))
                            // Reindeer Transformation
                            target->CastSpell(target, 25860, true, NULL, this);
                        return;
                    case 62109:                             // Tails Up: Aura
                        target->setFaction(1990);           // Ambient (hostile)
                        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                        return;
                    case 63122:                             // Clear Insane
                        target->RemoveAurasDueToSpell(GetSpellProto()->CalculateSimpleValue(m_effIndex));
                        return;
                    case 63624:                             // Learn a Second Talent Specialization
                        // Teach Learn Talent Specialization Switches, required for client triggered casts, allow after 30 sec delay
                        if (target->GetTypeId() == TYPEID_PLAYER)
                        {
                            ((Player*)target)->learnSpell(63680, false);
                        }
                        return;
                    case 63651:                             // Revert to One Talent Specialization
                        // Teach Learn Talent Specialization Switches, remove
                        if (target->GetTypeId() == TYPEID_PLAYER)
                        {
                            ((Player*)target)->removeSpell(63680);
                        }
                        return;
                    case 64132:                             // Constrictor Tentacle
                        if (target->GetTypeId() == TYPEID_PLAYER)
                        {
                            target->CastSpell(target, 64133, true, NULL, this);
                        }
                        return;
                    case 65684:                             // Dark Essence
                        target->RemoveAurasDueToSpell(65686);
                        return;
                    case 65686:                             // Light Essence
                        target->RemoveAurasDueToSpell(65684);
                        return;
                    case 68912:                             // Wailing Souls
                        if (Unit* caster = GetCaster())
                        {
                            caster->SetTargetGuid(target->GetObjectGuid());

                            // TODO - this is confusing, it seems the boss should channel this aura, and start casting the next spell
                            caster->CastSpell(caster, 68899, false);
                        }
                        return;
                    case 70623:                             // Jaina's Call
                        if (target->GetTypeId() == TYPEID_PLAYER)
                        {
                            target->CastSpell(target, 70525, true, NULL, this);
                        }
                        return;
                    case 70638:                             // Call of Sylvanas
                        if (target->GetTypeId() == TYPEID_PLAYER)
                        {
                            target->CastSpell(target, 70639, true, NULL, this);
                        }
                        return;
                    case 71342:                             // Big Love Rocket
                        Spell::SelectMountByAreaAndSkill(target, GetSpellProto(), 71344, 71345, 71346, 71347, 0);
                        return;
                    case 71563:                             // Deadly Precision
                        target->CastSpell(target, 71564, true, NULL, this);
                        return;
                    case 72286:                             // Invincible
                        Spell::SelectMountByAreaAndSkill(target, GetSpellProto(), 72281, 72282, 72283, 72284, 0);
                        return;
                    case 74856:                             // Blazing Hippogryph
                        Spell::SelectMountByAreaAndSkill(target, GetSpellProto(), 0, 0, 74854, 74855, 0);
                        return;
                    case 75614:                             // Celestial Steed
                        Spell::SelectMountByAreaAndSkill(target, GetSpellProto(), 75619, 75620, 75617, 75618, 76153);
                        return;
                    case 75973:                             // X-53 Touring Rocket
                        Spell::SelectMountByAreaAndSkill(target, GetSpellProto(), 0, 0, 75957, 75972, 76154);
                        return;
                }
                break;
            }
            case SPELLFAMILY_WARRIOR:
            {
                switch (GetId())
                {
                    case 41099:                             // Battle Stance
                    {
                        if (target->GetTypeId() != TYPEID_UNIT)
                        {
                            return;
                        }

                        // Stance Cooldown
                        target->CastSpell(target, 41102, true, NULL, this);

                        // Battle Aura
                        target->CastSpell(target, 41106, true, NULL, this);

                        // equipment
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 32614);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 0);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
                        return;
                    }
                    case 41100:                             // Berserker Stance
                    {
                        if (target->GetTypeId() != TYPEID_UNIT)
                        {
                            return;
                        }

                        // Stance Cooldown
                        target->CastSpell(target, 41102, true, NULL, this);

                        // Berserker Aura
                        target->CastSpell(target, 41107, true, NULL, this);

                        // equipment
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 32614);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 0);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
                        return;
                    }
                    case 41101:                             // Defensive Stance
                    {
                        if (target->GetTypeId() != TYPEID_UNIT)
                        {
                            return;
                        }

                        // Stance Cooldown
                        target->CastSpell(target, 41102, true, NULL, this);

                        // Defensive Aura
                        target->CastSpell(target, 41105, true, NULL, this);

                        // equipment
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 32604);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 31467);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
                        return;
                    }
                    case 53790:                             // Defensive Stance
                    {
                        if (target->GetTypeId() != TYPEID_UNIT)
                        {
                            return;
                        }

                        // Stance Cooldown
                        target->CastSpell(target, 59526, true, NULL, this);

                        // Defensive Aura
                        target->CastSpell(target, 41105, true, NULL, this);

                        // equipment
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 43625);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 39384);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
                        return;
                    }
                    case 53791:                             // Berserker Stance
                    {
                        if (target->GetTypeId() != TYPEID_UNIT)
                        {
                            return;
                        }

                        // Stance Cooldown
                        target->CastSpell(target, 59526, true, NULL, this);

                        // Berserker Aura
                        target->CastSpell(target, 41107, true, NULL, this);

                        // equipment
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 43625);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 43625);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
                        return;
                    }
                    case 53792:                             // Battle Stance
                    {
                        if (target->GetTypeId() != TYPEID_UNIT)
                        {
                            return;
                        }

                        // Stance Cooldown
                        target->CastSpell(target, 59526, true, NULL, this);

                        // Battle Aura
                        target->CastSpell(target, 41106, true, NULL, this);

                        // equipment
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_0, 43623);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_1, 0);
                        ((Creature*)target)->SetVirtualItem(VIRTUAL_ITEM_SLOT_2, 0);
                        return;
                    }
                }

                // Overpower
                if (GetSpellProto()->SpellClassMask & UI64LIT(0x0000000000000004))
                {
                    // Must be casting target
                    if (!target->IsNonMeleeSpellCasted(false))
                    {
                        return;
                    }

                    Unit* caster = GetCaster();
                    if (!caster)
                    {
                        return;
                    }

                    Unit::AuraList const& modifierAuras = caster->GetAurasByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                    for (Unit::AuraList::const_iterator itr = modifierAuras.begin(); itr != modifierAuras.end(); ++itr)
                    {
                        // Unrelenting Assault
                        if ((*itr)->GetSpellProto()->SpellClassSet == SPELLFAMILY_WARRIOR && (*itr)->GetSpellProto()->SpellIconID == 2775)
                        {
                            switch ((*itr)->GetSpellProto()->ID)
                            {
                                case 46859:                 // Unrelenting Assault, rank 1
                                    target->CastSpell(target, 64849, true, NULL, (*itr));
                                    break;
                                case 46860:                 // Unrelenting Assault, rank 2
                                    target->CastSpell(target, 64850, true, NULL, (*itr));
                                    break;
                                default:
                                    break;
                            }
                            break;
                        }
                    }
                    return;
                }
                break;
            }
            case SPELLFAMILY_MAGE:
                break;
            case SPELLFAMILY_HUNTER:
            {
                switch (GetId())
                {
                    case 34026:                             // Kill Command
                        target->CastSpell(target, 34027, true, NULL, this);
                        return;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                switch (GetId())
                {
                    case 55198:                             // Tidal Force
                        target->CastSpell(target, 55166, true, NULL, this);
                        return;
                }

                // Earth Shield
                if ((GetSpellProto()->SpellClassMask & UI64LIT(0x40000000000)))
                {
                    // prevent double apply bonuses
                    if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
                    {
                        if (Unit* caster = GetCaster())
                        {
                            m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                            m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                        }
                    }
                    return;
                }
                break;
            }
        }
    }
    // AT REMOVE
    else
    {
        if (IsQuestTameSpell(GetId()) && target->IsAlive())
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsAlive())
            {
                return;
            }

            uint32 finalSpellId = 0;
            switch (GetId())
            {
                case 19548: finalSpellId = 19597; break;
                case 19674: finalSpellId = 19677; break;
                case 19687: finalSpellId = 19676; break;
                case 19688: finalSpellId = 19678; break;
                case 19689: finalSpellId = 19679; break;
                case 19692: finalSpellId = 19680; break;
                case 19693: finalSpellId = 19684; break;
                case 19694: finalSpellId = 19681; break;
                case 19696: finalSpellId = 19682; break;
                case 19697: finalSpellId = 19683; break;
                case 19699: finalSpellId = 19685; break;
                case 19700: finalSpellId = 19686; break;
                case 30646: finalSpellId = 30647; break;
                case 30653: finalSpellId = 30648; break;
                case 30654: finalSpellId = 30652; break;
                case 30099: finalSpellId = 30100; break;
                case 30102: finalSpellId = 30103; break;
                case 30105: finalSpellId = 30104; break;
            }

            if (finalSpellId)
            {
                caster->CastSpell(target, finalSpellId, true, NULL, this);
            }

            return;
        }

        switch (GetId())
        {
            case 10255:                                     // Stoned
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetTypeId() != TYPEID_UNIT)
                    {
                        return;
                    }

                    // see dummy effect of spell 10254 for removal of flags etc
                    caster->CastSpell(caster, 10254, true);
                }
                return;
            }
            case 12479:                                     // Hex of Jammal'an
                target->CastSpell(target, 12480, true, NULL, this);
                return;
            case 12774:                                     // (DND) Belnistrasz Idol Shutdown Visual
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                {
                    return;
                }

                // Idom Rool Camera Shake <- wtf, don't drink while making spellnames?
                if (Unit* caster = GetCaster())
                {
                    caster->CastSpell(caster, 12816, true);
                }

                return;
            }
            case 28169:                                     // Mutating Injection
            {
                // Mutagen Explosion
                target->CastSpell(target, 28206, true, NULL, this);
                // Poison Cloud
                target->CastSpell(target, 28240, true, NULL, this);
                return;
            }
            case 32045:                                     // Soul Charge
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 32054, true, NULL, this);
                }

                return;
            }
            case 32051:                                     // Soul Charge
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 32057, true, NULL, this);
                }

                return;
            }
            case 32052:                                     // Soul Charge
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 32053, true, NULL, this);
                }

                return;
            }
            case 32286:                                     // Focus Target Visual
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 32301, true, NULL, this);
                }

                return;
            }
            case 35079:                                     // Misdirection, triggered buff
            case 59628:                                     // Tricks of the Trade, triggered buff
            {
                if (Unit* pCaster = GetCaster())
                {
                    pCaster->GetHostileRefManager().ResetThreatRedirection();
                }
                return;
            }
            case 36730:                                     // Flame Strike
            {
                target->CastSpell(target, 36731, true, NULL, this);
                return;
            }
            case 41099:                                     // Battle Stance
            {
                // Battle Aura
                target->RemoveAurasDueToSpell(41106);
                return;
            }
            case 41100:                                     // Berserker Stance
            {
                // Berserker Aura
                target->RemoveAurasDueToSpell(41107);
                return;
            }
            case 41101:                                     // Defensive Stance
            {
                // Defensive Aura
                target->RemoveAurasDueToSpell(41105);
                return;
            }
            case 42385:                                     // Alcaz Survey Aura
            {
                target->CastSpell(target, 42316, true, NULL, this);
                return;
            }
            case 42454:                                     // Captured Totem
            {
                if (m_removeMode == AURA_REMOVE_BY_DEFAULT)
                {
                    if (target->GetDeathState() != CORPSE)
                    {
                        return;
                    }

                    Unit* pCaster = GetCaster();

                    if (!pCaster)
                    {
                        return;
                    }

                    // Captured Totem Test Credit
                    if (Player* pPlayer = pCaster->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        pPlayer->CastSpell(pPlayer, 42455, true);
                    }
                }

                return;
            }
            case 42517:                                     // Beam to Zelfrax
            {
                // expecting target to be a dummy creature
                Creature* pSummon = target->SummonCreature(23864, 0.0f, 0.0f, 0.0f, target->GetOrientation(), TEMPSPAWN_DEAD_DESPAWN, 0);

                Unit* pCaster = GetCaster();

                if (pSummon && pCaster)
                {
                    pSummon->GetMotionMaster()->MovePoint(0, pCaster->GetPositionX(), pCaster->GetPositionY(), pCaster->GetPositionZ());
                }

                return;
            }
            case 43681:                                     // Inactive
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE && target->GetTypeId() == TYPEID_PLAYER)
                {
                    ((Player*)target)->ToggleAFK();
                }
                return;
            }
            case 43969:                                     // Feathered Charm
            {
                // Steelfeather Quest Credit, Are there any requirements for this, like area?
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 43984, true);
                }

                return;
            }
            case 44191:                                     // Flame Strike
            {
                if (target->GetMap()->IsDungeon())
                {
                    uint32 spellId = target->GetMap()->IsRegularDifficulty() ? 44190 : 46163;

                    target->CastSpell(target, spellId, true, NULL, this);
                }
                return;
            }
            case 45934:                                     // Dark Fiend
            {
                // Kill target if dispelled
                if (m_removeMode == AURA_REMOVE_BY_DISPEL)
                {
                    target->DealDamage(target, target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                }
                return;
            }
            case 45963:                                     // Call Alliance Deserter
            {
                // Escorting Alliance Deserter
                target->RemoveAurasDueToSpell(45957);
                return;
            }
            case 46308:                                     // Burning Winds
            {
                // casted only at creatures at spawn
                target->CastSpell(target, 47287, true, NULL, this);
                return;
            }
            case 46637:                                     // Break Ice
            {
                target->CastSpell(target, 47030, true, NULL, this);
                return;
            }
            case 48385:                                     // Create Spirit Fount Beam
            {
                target->CastSpell(target, target->GetMap()->IsRegularDifficulty() ? 48380 : 59320, true);
                return;
            }
            case 50141:                                     // Blood Oath
            {
                // Blood Oath
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 50001, true, NULL, this);
                }

                return;
            }
            case 51405:                                     // Digging for Treasure
            {
                const uint32 spell_list[7] =
                {
                    51441,                                  // hare
                    51397,                                  // crystal
                    51398,                                  // armor
                    51400,                                  // gem
                    51401,                                  // platter
                    51402,                                  // treasure
                    51443                                   // bug
                };

                target->CastSpell(target, spell_list[urand(0, 6)], true);

                target->HandleEmote(EMOTE_STATE_NONE);
                target->clearUnitState(UNIT_STAT_STUNNED);
                return;
            }
            case 51870:                                     // Collect Hair Sample
            {
                if (Unit* pCaster = GetCaster())
                {
                    if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                    {
                        pCaster->CastSpell(target, 51872, true, NULL, this);
                    }
                }

                return;
            }
            case 52098:                                     // Charge Up
            {
                if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                {
                    target->CastSpell(target, 52092, true, NULL, this);
                }

                return;
            }
            case 53039:                                     // Deploy Parachute
            {
                // Crusader Parachute
                target->RemoveAurasDueToSpell(53031);
                return;
            }
            case 53790:                                     // Defensive Stance
            {
                // Defensive Aura
                target->RemoveAurasDueToSpell(41105);
                return;
            }
            case 53791:                                     // Berserker Stance
            {
                // Berserker Aura
                target->RemoveAurasDueToSpell(41107);
                return;
            }
            case 53792:                                     // Battle Stance
            {
                // Battle Aura
                target->RemoveAurasDueToSpell(41106);
                return;
            }
            case 56511:                                     // Towers of Certain Doom: Tower Bunny Smoke Flare Effect
            {
                // Towers of Certain Doom: Skorn Cannonfire
                if (m_removeMode == AURA_REMOVE_BY_DEFAULT)
                {
                    target->CastSpell(target, 43069, true);
                }

                return;
            }
            case 58600:                                     // Restricted Flight Area
            {
                AreaTableEntry const* area = GetAreaEntryByAreaID(target->GetAreaId());

                // Dalaran restricted flight zone (recheck before apply unmount)
                if (area && target->GetTypeId() == TYPEID_PLAYER && (area->flags & AREA_FLAG_CANNOT_FLY) &&
                        ((Player*)target)->IsFreeFlying() && !((Player*)target)->isGameMaster())
                {
                    target->CastSpell(target, 58601, true); // Remove Flight Auras (also triggered Parachute (45472))
                }
                return;
            }
            case 61900:                                     // Electrical Charge
            {
                if (m_removeMode == AURA_REMOVE_BY_DEATH)
                {
                    target->CastSpell(target, GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_0), true);
                }

                return;
            }
            case 62483:                                     // Stonebark's Essence Channel
            case 62484:                                     // Ironbranch's Essence Channel
            case 62485:                                     // Brightleaf's Essence Channel
            case 65587:                                     // Brightleaf's Essence Channel (h)
            case 65588:                                     // Ironbranch's Essence Channel (h)
            case 65589:                                     // Stonebark's Essence Channel (h)
            {
                if (Unit* caster = GetCaster())
                {
                    if (m_removeMode == AURA_REMOVE_BY_EXPIRE)
                    {
                        caster->CastSpell(caster, 62467, true);
                    }
                }
                return;
            }
            case 64398:                                     // Summon Scrap Bot (Ulduar, Mimiron) - for Scrap Bots
            case 64426:                                     // Summon Scrap Bot (Ulduar, Mimiron) - for Assault Bots
            case 64621:                                     // Summon Fire Bot (Ulduar, Mimiron)
            {
                uint32 triggerSpell = 0;
                switch (GetId())
                {
                    case 64398: triggerSpell = 63819; break;
                    case 64426: triggerSpell = 64427; break;
                    case 64621: triggerSpell = 64622; break;
                }
                target->CastSpell(target, triggerSpell, false);
                return;
            }
            case 68839:                                     // Corrupt Soul
            {
                // Knockdown Stun
                target->CastSpell(target, 68848, true, NULL, this);
                // Draw Corrupted Soul
                target->CastSpell(target, 68846, true, NULL, this);
                return;
            }
        }

        // Living Bomb
        if (GetSpellProto()->SpellClassSet == SPELLFAMILY_MAGE && (GetSpellProto()->SpellClassMask & UI64LIT(0x2000000000000)))
        {
            if (m_removeMode == AURA_REMOVE_BY_EXPIRE || m_removeMode == AURA_REMOVE_BY_DISPEL)
            {
                target->CastSpell(target, m_modifier.m_amount, true, NULL, this);
            }

            return;
        }
    }

    // AT APPLY & REMOVE
    switch (GetSpellProto()->SpellClassSet)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (GetId())
            {
                case 6606:                                  // Self Visual - Sleep Until Cancelled (DND)
                {
                    if (apply)
                    {
                        target->SetStandState(UNIT_STAND_STATE_SLEEP);
                        target->addUnitState(UNIT_STAT_ROOT);
                    }
                    else
                    {
                        target->clearUnitState(UNIT_STAT_ROOT);
                        target->SetStandState(UNIT_STAND_STATE_STAND);
                    }

                    return;
                }
                case 11196:                                 // Recently Bandaged
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, GetMiscValue(), apply);
                    return;
                case 24658:                                 // Unstable Power
                {
                    if (apply)
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                        {
                            return;
                        }

                        caster->CastSpell(target, 24659, true, NULL, NULL, GetCasterGuid());
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(24659);
                    }
                    return;
                }
                case 24661:                                 // Restless Strength
                {
                    if (apply)
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                        {
                            return;
                        }

                        caster->CastSpell(target, 24662, true, NULL, NULL, GetCasterGuid());
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(24662);
                    }
                    return;
                }
                case 29266:                                 // Permanent Feign Death
                case 31261:                                 // Permanent Feign Death (Root)
                case 37493:                                 // Feign Death
                case 52593:                                 // Bloated Abomination Feign Death
                case 55795:                                 // Falling Dragon Feign Death
                case 57626:                                 // Feign Death
                case 57685:                                 // Permanent Feign Death
                case 58768:                                 // Permanent Feign Death (Freeze Jumpend)
                case 58806:                                 // Permanent Feign Death (Drowned Anim)
                case 58951:                                 // Permanent Feign Death
                case 64461:                                 // Permanent Feign Death (No Anim) (Root)
                case 65985:                                 // Permanent Feign Death (Root Silence Pacify)
                case 70592:                                 // Permanent Feign Death
                case 70628:                                 // Permanent Feign Death
                case 70630:                                 // Frozen Aftermath - Feign Death
                case 71598:                                 // Feign Death
                {
                    // Unclear what the difference really is between them.
                    // Some has effect1 that makes the difference, however not all.
                    // Some appear to be used depending on creature location, in water, at solid ground, in air/suspended, etc
                    // For now, just handle all the same way
                    if (target->GetTypeId() == TYPEID_UNIT)
                    {
                        target->SetFeignDeath(apply);
                    }

                    return;
                }
                case 35356:                                 // Spawn Feign Death
                case 35357:                                 // Spawn Feign Death
                case 42557:                                 // Feign Death
                case 51329:                                 // Feign Death
                {
                    if (target->GetTypeId() == TYPEID_UNIT)
                    {
                        // Flags not set like it's done in SetFeignDeath()
                        // UNIT_DYNFLAG_DEAD does not appear with these spells.
                        // All of the spells appear to be present at spawn and not used to feign in combat or similar.
                        if (apply)
                        {
                            target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                            target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);

                            target->addUnitState(UNIT_STAT_DIED);
                        }
                        else
                        {
                            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                            target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);

                            target->clearUnitState(UNIT_STAT_DIED);
                        }
                    }
                    return;
                }
                case 40133:                                 // Summon Fire Elemental
                {
                    Unit* caster = GetCaster();
                    if (!caster)
                    {
                        return;
                    }

                    Unit* owner = caster->GetOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (apply)
                        {
                            owner->CastSpell(owner, 8985, true);
                        }
                        else
                        {
                            ((Player*)owner)->RemovePet(PET_SAVE_REAGENTS);
                        }
                    }
                    return;
                }
                case 40132:                                 // Summon Earth Elemental
                {
                    Unit* caster = GetCaster();
                    if (!caster)
                    {
                        return;
                    }

                    Unit* owner = caster->GetOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        if (apply)
                        {
                            owner->CastSpell(owner, 19704, true);
                        }
                        else
                        {
                            ((Player*)owner)->RemovePet(PET_SAVE_REAGENTS);
                        }
                    }
                    return;
                }
                case 40214:                                 // Dragonmaw Illusion
                {
                    if (apply)
                    {
                        target->CastSpell(target, 40216, true);
                        target->CastSpell(target, 42016, true);
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(40216);
                        target->RemoveAurasDueToSpell(42016);
                    }
                    return;
                }
                case 42515:                                 // Jarl Beam
                {
                    // aura animate dead (fainted) state for the duration, but we need to animate the death itself (correct way below?)
                    if (Unit* pCaster = GetCaster())
                    {
                        pCaster->ApplyModFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH, apply);
                    }

                    // Beam to Zelfrax at remove
                    if (!apply)
                    {
                        target->CastSpell(target, 42517, true);
                    }
                    return;
                }
                case 42583:                                 // Claw Rage
                case 68987:                                 // Pursuit
                {
                    Unit* caster = GetCaster();
                    if (!caster || target->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    if (apply)
                    {
                        caster->FixateTarget(target);
                    }
                    else if (target->GetObjectGuid() == caster->GetFixateTargetGuid())
                    {
                        caster->FixateTarget(NULL);
                    }

                    return;
                }
                case 43874:                                 // Scourge Mur'gul Camp: Force Shield Arcane Purple x3
                    target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE, apply);
                    if (apply)
                    {
                        target->addUnitState(UNIT_STAT_ROOT);
                    }
                    return;
                case 47178:                                 // Plague Effect Self
                    target->SetFeared(apply, GetCasterGuid(), GetId());
                    return;
                case 56422:                                 // Nerubian Submerge
                    // not known if there are other things todo, only flag are confirmed valid
                    target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE, apply);
                    return;
                case 58204:                                 // LK Intro VO (1)
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 1
                        if (apply)
                        {
                            target->PlayDirectSound(14970, (Player*)target);
                        }
                        // continue in 58205
                        else
                        {
                            target->CastSpell(target, 58205, true);
                        }
                    }
                    return;
                case 58205:                                 // LK Intro VO (2)
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 2
                        if (apply)
                        {
                            target->PlayDirectSound(14971, (Player*)target);
                        }
                        // Play part 3
                        else
                        {
                            target->PlayDirectSound(14972, (Player*)target);
                        }
                    }
                    return;
                case 27978:
                case 40131:
                    if (apply)
                    {
                        target->m_AuraFlags |= UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    }
                    else
                    {
                        target->m_AuraFlags &= ~UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    }
                    return;
            }
            break;
        }
        case SPELLFAMILY_MAGE:
            break;
        case SPELLFAMILY_WARLOCK:
        {
            // Haunt
            if (GetSpellProto()->SpellIconID == 3172 && (GetSpellProto()->SpellClassMask & UI64LIT(0x0004000000000000)))
            {
                // NOTE: for avoid use additional field damage stored in dummy value (replace unused 100%
                if (apply)
                {
                    m_modifier.m_amount = 0;                // use value as damage counter instead redundant 100% percent
                }
                else
                {
                    int32 bp0 = m_modifier.m_amount;

                    if (Unit* caster = GetCaster())
                    {
                        target->CastCustomSpell(caster, 48210, &bp0, NULL, NULL, true, NULL, this);
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch (GetId())
            {
                case 52610:                                 // Savage Roar
                {
                    if (apply)
                    {
                        if (target->GetShapeshiftForm() != FORM_CAT)
                        {
                            return;
                        }

                        target->CastSpell(target, 62071, true);
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(62071);
                    }
                    return;
                }
                case 61336:                                 // Survival Instincts
                {
                    if (apply)
                    {
                        if (!target->IsInFeralForm())
                        {
                            return;
                        }

                        int32 bp0 = int32(target->GetMaxHealth() * m_modifier.m_amount / 100);
                        target->CastCustomSpell(target, 50322, &bp0, NULL, NULL, true);
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(50322);
                    }
                    return;
                }
            }

            // Lifebloom
            if (GetSpellProto()->SpellClassMask & UI64LIT(0x1000000000))
            {
                if (apply)
                {
                    if (Unit* caster = GetCaster())
                    {
                        // prevent double apply bonuses
                        if (target->GetTypeId() != TYPEID_PLAYER || !((Player*)target)->GetSession()->PlayerLoading())
                        {
                            m_modifier.m_amount = caster->SpellHealingBonusDone(target, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                            m_modifier.m_amount = target->SpellHealingBonusTaken(caster, GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE);
                        }
                    }
                }
                else
                {
                    // Final heal on duration end
                    if (m_removeMode != AURA_REMOVE_BY_EXPIRE)
                    {
                        return;
                    }

                    // final heal
                    if (target->IsInWorld() && GetStackAmount() > 0)
                    {
                        int32 amount = m_modifier.m_amount;
                        target->CastCustomSpell(target, 33778, &amount, NULL, NULL, true, NULL, this, GetCasterGuid());

                        if (Unit* caster = GetCaster())
                        {
                            int32 returnmana = (GetSpellProto()->ManaCostPct * caster->GetCreateMana() / 100) * GetStackAmount() / 2;
                            caster->CastCustomSpell(caster, 64372, &returnmana, NULL, NULL, true, NULL, this, GetCasterGuid());
                        }
                    }
                }
                return;
            }

            // Predatory Strikes
            if (target->GetTypeId() == TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
            {
                ((Player*)target)->UpdateAttackPowerAndDamage();
                return;
            }

            // Improved Moonkin Form
            if (GetSpellProto()->SpellIconID == 2855)
            {
                uint32 spell_id;
                switch (GetId())
                {
                    case 48384: spell_id = 50170; break;    // Rank 1
                    case 48395: spell_id = 50171; break;    // Rank 2
                    case 48396: spell_id = 50172; break;    // Rank 3
                    default:
                        sLog.outError("HandleAuraDummy: Not handled rank of IMF (Spell: %u)", GetId());
                        return;
                }

                if (apply)
                {
                    if (target->GetShapeshiftForm() != FORM_MOONKIN)
                    {
                        return;
                    }

                    target->CastSpell(target, spell_id, true);
                }
                else
                {
                    target->RemoveAurasDueToSpell(spell_id);
                }
                return;
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch (GetId())
            {
                case 57934:                                 // Tricks of the Trade, main spell
                {
                    if (apply)
                    {
                        GetHolder()->SetAuraCharges(1);     // not have proper charges set in spell data
                    }
                    else
                    {
                        // used for direct in code aura removes and spell proc event charges expire
                        if (m_removeMode != AURA_REMOVE_BY_DEFAULT)
                        {
                            target->GetHostileRefManager().ResetThreatRedirection();
                        }
                    }
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            switch (GetId())
            {
                case 34477:                                 // Misdirection, main spell
                {
                    if (apply)
                    {
                        GetHolder()->SetAuraCharges(1);     // not have proper charges set in spell data
                    }
                    else
                    {
                        // used for direct in code aura removes and spell proc event charges expire
                        if (m_removeMode != AURA_REMOVE_BY_DEFAULT)
                        {
                            target->GetHostileRefManager().ResetThreatRedirection();
                        }
                    }
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            switch (GetId())
            {
                case 20911:                                 // Blessing of Sanctuary
                case 25899:                                 // Greater Blessing of Sanctuary
                {
                    if (apply)
                    {
                        target->CastSpell(target, 67480, true, NULL, this);
                    }
                    else
                    {
                        target->RemoveAurasDueToSpell(67480);
                    }
                    return;
                }
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            switch (GetId())
            {
                case 6495:                                  // Sentry Totem
                {
                    if (target->GetTypeId() != TYPEID_PLAYER)
                    {
                        return;
                    }

                    Totem* totem = target->GetTotem(TOTEM_SLOT_AIR);

                    if (totem && apply)
                    {
                        ((Player*)target)->GetCamera().SetView(totem);
                    }
                    else
                    {
                        ((Player*)target)->GetCamera().ResetView();
                    }

                    return;
                }
            }
            break;
        }
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr.GetPetAura(GetId(), m_effIndex))
    {
        if (apply)
        {
            target->AddPetAura(petSpell);
        }
        else
        {
            target->RemovePetAura(petSpell);
        }
        return;
    }

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
        if (saBounds.first != saBounds.second)
        {
            uint32 zone, area;
            target->GetZoneAndAreaId(zone, area);

            for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            {
                itr->second->ApplyOrRemoveSpellIfCan((Player*)target, zone, area, false);
            }
        }
    }

    // script has to "handle with care", only use where data are not ok to use in the above code.
    if (target->GetTypeId() == TYPEID_UNIT)
    {
        sScriptMgr.OnAuraDummy(this, apply);
    }
}
