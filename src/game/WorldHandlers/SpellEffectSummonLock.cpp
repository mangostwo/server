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
 * @file SpellEffectSummonLock.cpp
 * @brief Cohesion split of SpellEffects.cpp -- summon and lock/open effect handlers.
 *        Same `Spell` class; no behaviour change.
 */

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "SkillExtraItems.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "SpellAuras.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "Pet.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Creature.h"
#include "Totem.h"
#include "CreatureAI.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundEY.h"
#include "BattleGround/BattleGroundWS.h"
#include "Language.h"
#include "SocialMgr.h"
#include "VMapFactory.h"
#include "Util.h"
#include "TemporarySummon.h"
#include "ScriptMgr.h"
#include "SkillDiscovery.h"
#include "Formulas.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Vehicle.h"
#include "G3D/Vector3.h"
#include <random>
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Opens or sends loot for the specified object guid.
 *
 * @param guid The loot source guid.
 * @param loottype The loot type to open.
 * @param lockType The lock interaction type.
 */
void Spell::SendLoot(ObjectGuid guid, LootType loottype, LockType lockType)
{
    if (gameObjTarget)
    {
        switch (gameObjTarget->GetGoType())
        {
            case GAMEOBJECT_TYPE_DOOR:
            case GAMEOBJECT_TYPE_BUTTON:
            case GAMEOBJECT_TYPE_QUESTGIVER:
            case GAMEOBJECT_TYPE_SPELL_FOCUS:
            case GAMEOBJECT_TYPE_GOOBER:
                gameObjTarget->Use(m_caster);
                return;

            case GAMEOBJECT_TYPE_CHEST:
                gameObjTarget->Use(m_caster);
                // Don't return, let loots been taken
                break;

            case GAMEOBJECT_TYPE_TRAP:
                if (lockType == LOCKTYPE_DISARM_TRAP)
                {
                    gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
                    return;
                }
                sLog.outError("Spell::SendLoot unhandled locktype %u for GameObject trap (entry %u) for spell %u.", lockType, gameObjTarget->GetEntry(), m_spellInfo->ID);
                return;
            default:
                sLog.outError("Spell::SendLoot unhandled GameObject type %u (entry %u) for spell %u.", gameObjTarget->GetGoType(), gameObjTarget->GetEntry(), m_spellInfo->ID);
                return;
        }
    }

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Send loot
    ((Player*)m_caster)->SendLoot(guid, loottype);
}

/**
 * @brief Opens a locked game object or item and awards related skill progress.
 *
 * @param eff_idx The open-lock effect index.
 */
void Spell::EffectOpenLock(SpellEffectIndex eff_idx)
{
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        DEBUG_LOG("WORLD: Open Lock - No Player Caster!");
        return;
    }

    Player* player = (Player*)m_caster;

    uint32 lockId = 0;
    ObjectGuid guid;

    // Get lockId
    if (gameObjTarget)
    {
        GameObjectInfo const* goInfo = gameObjTarget->GetGOInfo();
        // Arathi Basin banner opening !
        if ((goInfo->type == GAMEOBJECT_TYPE_BUTTON && goInfo->button.noDamageImmune) ||
            (goInfo->type == GAMEOBJECT_TYPE_GOOBER && goInfo->goober.losOK))
        {
            // CanUseBattleGroundObject() already called in CheckCast()
            // in battleground check
            if (BattleGround* bg = player->GetBattleGround())
            {
                // check if it's correct bg
                if (bg->GetTypeID(true) == BATTLEGROUND_AB || bg->GetTypeID(true) == BATTLEGROUND_AV)
                {
                    bg->EventPlayerClickedOnFlag(player, gameObjTarget);
                }
                return;
            }
        }
        else if (goInfo->type == GAMEOBJECT_TYPE_FLAGSTAND)
        {
            // CanUseBattleGroundObject() already called in CheckCast()
            // in battleground check
            if (BattleGround* bg = player->GetBattleGround())
            {
                if (bg->GetTypeID(true) == BATTLEGROUND_EY)
                {
                    bg->EventPlayerClickedOnFlag(player, gameObjTarget);
                }
                return;
            }
        }
        lockId = goInfo->GetLockId();
        guid = gameObjTarget->GetObjectGuid();
    }
    else if (itemTarget)
    {
        lockId = itemTarget->GetProto()->LockID;
        guid = itemTarget->GetObjectGuid();
    }
    else
    {
        DEBUG_LOG("WORLD: Open Lock - No GameObject/Item Target!");
        return;
    }

    SkillType skillId = SKILL_NONE;
    int32 reqSkillValue = 0;
    int32 skillValue;

    SpellCastResult res = CanOpenLock(eff_idx, lockId, skillId, reqSkillValue, skillValue);
    if (res != SPELL_CAST_OK)
    {
        SendCastResult(res);
        return;
    }

    // mark item as unlocked
    if (itemTarget)
    {
        itemTarget->SetFlag(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_UNLOCKED);
    }

    SendLoot(guid, LOOT_SKINNING, LockType(m_spellInfo->EffectMiscValue[eff_idx]));

    // not allow use skill grow at item base open
    if (!m_CastItem && skillId != SKILL_NONE)
    {
        // update skill if really known
        if (uint32 pureSkillValue = player->GetPureSkillValue(skillId))
        {
            if (gameObjTarget)
            {
                // Allow one skill-up until respawned
                if (!gameObjTarget->IsInSkillupList(player) &&
                    player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue))
                    {
                        gameObjTarget->AddToSkillupList(player);
                    }
            }
            else if (itemTarget)
            {
                // Do one skill-up
                player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue);
            }
        }
    }
}

/**
 * @brief Replaces the cast item with another item entry.
 *
 * @param eff_idx The effect index defining the replacement item.
 */
void Spell::EffectSummonChangeItem(SpellEffectIndex eff_idx)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)m_caster;

    // applied only to using item
    if (!m_CastItem)
    {
        return;
    }

    // ... only to item in own inventory/bank/equip_slot
    if (m_CastItem->GetOwnerGuid() != player->GetObjectGuid())
    {
        return;
    }

    uint32 newitemid = m_spellInfo->EffectItemType[eff_idx];
    if (!newitemid)
    {
        return;
    }

    Item* oldItem = m_CastItem;

    // prevent crash at access and unexpected charges counting with item update queue corrupt
    ClearCastItem();

    player->ConvertItem(oldItem, newitemid);
}

/**
 * @brief Grants weapon or armor proficiency to a player target.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectProficiency(SpellEffectIndex /*eff_idx*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    Player* p_target = (Player*)unitTarget;

    uint32 subClassMask = m_spellInfo->EquippedItemSubclass;
    if (m_spellInfo->EquippedItemClass == ITEM_CLASS_WEAPON && !(p_target->GetWeaponProficiency() & subClassMask))
    {
        p_target->AddWeaponProficiency(subClassMask);
        p_target->SendProficiency(ITEM_CLASS_WEAPON, p_target->GetWeaponProficiency());
    }
    if (m_spellInfo->EquippedItemClass == ITEM_CLASS_ARMOR && !(p_target->GetArmorProficiency() & subClassMask))
    {
        p_target->AddArmorProficiency(subClassMask);
        p_target->SendProficiency(ITEM_CLASS_ARMOR, p_target->GetArmorProficiency());
    }
}

/**
 * @brief Creates and attaches an area aura for the current unit target.
 *
 * @param eff_idx The area aura effect index.
 */
void Spell::EffectApplyAreaAura(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    AreaAura* Aur = new AreaAura(m_spellInfo, eff_idx, &m_currentBasePoints[eff_idx], m_spellAuraHolder, unitTarget, m_caster, m_CastItem);
    m_spellAuraHolder->AddAura(Aur, eff_idx);
}

void Spell::EffectSummonType(SpellEffectIndex eff_idx)
{
    uint32 prop_id = m_spellInfo->EffectMiscValueB[eff_idx];
    SummonPropertiesEntry const* summon_prop = sSummonPropertiesStore.LookupEntry(prop_id);
    if (!summon_prop)
    {
        sLog.outError("EffectSummonType: Unhandled summon type %u", prop_id);
        return;
    }

    // Pet's are atm handled differently
    if (summon_prop->Control == SUMMON_PROP_GROUP_PETS && prop_id != 1562)
    {
        DoSummonPet(eff_idx);
        return;
    }

    // Get casting object
    WorldObject* realCaster = GetCastingObject();
    if (!realCaster)
    {
        sLog.outError("EffectSummonType: No Casting Object found for spell %u, (caster = %s)", m_spellInfo->ID, m_caster->GetGuidStr().c_str());
        return;
    }

    Unit* responsibleCaster = m_originalCaster;
    if (realCaster->GetTypeId() == TYPEID_GAMEOBJECT)
    {
        responsibleCaster = ((GameObject*)realCaster)->GetOwner();
    }

    // Expected Amount: TODO - there are quite some exceptions (like totems, engineering dragonlings..)
    uint32 amount = damage > 0 ? damage : 1;

    // basepoints of SUMMON_PROP_GROUP_VEHICLE is often a spellId, set amount to 1
    if ((summon_prop->Control == SUMMON_PROP_GROUP_VEHICLE) || (prop_id == 1961))
    {
        amount = 1;
    }

    // Expected Level                                       (Totem, Pet and Critter may not use this)
    uint32 level = responsibleCaster ? responsibleCaster->getLevel() : m_caster->getLevel();
    // level of creature summoned using engineering item based at engineering skill level
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_CastItem)
    {
        ItemPrototype const* proto = m_CastItem->GetProto();
        if (proto && proto->RequiredSkill == SKILL_ENGINEERING)
            if (uint16 engineeringSkill = ((Player*)m_caster)->GetSkillValue(SKILL_ENGINEERING))
            {
                level = engineeringSkill / 5;
                amount = 1;                                 // TODO HACK (needs a neat way of doing)
            }
    }

    CreatureSummonPositions summonPositions;
    summonPositions.resize(amount, CreaturePosition());

    // Set middle position
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z);
    }
    else
    {
        realCaster->GetPosition(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z);

        // TODO - Is this really an error?
        sLog.outDebug("Spell Effect EFFECT_SUMMON (%u) - summon without destination (spell id %u, effIndex %u)", m_spellInfo->Effect[eff_idx], m_spellInfo->ID, eff_idx);
    }

    // Set summon positions
    float radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[eff_idx]));
    CreatureSummonPositions::iterator itr = summonPositions.begin();
    for (++itr; itr != summonPositions.end(); ++itr)        // In case of multiple summons around position for not-fist positions
    {
        if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION || radius > 1.0f)
        {
            realCaster->GetRandomPoint(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z, radius, itr->x, itr->y, itr->z);
            if (realCaster->GetMap()->GetHitPosition(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z, itr->x, itr->y, itr->z, m_caster->GetPhaseMask(), -0.5f))
            {
                realCaster->UpdateAllowedPositionZ(itr->x, itr->y, itr->z);
            }
        }
        else                                                // Get a point near the caster
        {
            realCaster->GetClosePoint(itr->x, itr->y, itr->z, 0.0f, radius, frand(0.0f, 2 * M_PI_F));
            if (realCaster->GetMap()->GetHitPosition(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z, itr->x, itr->y, itr->z, m_caster->GetPhaseMask(), -0.5f))
            {
                realCaster->UpdateAllowedPositionZ(itr->x, itr->y, itr->z);
            }
        }
    }

    bool summonResult = false;
    switch (summon_prop->Control)
    {
            // faction handled later on, or loaded from template
        case SUMMON_PROP_GROUP_WILD:
        case SUMMON_PROP_GROUP_FRIENDLY:
        {
            switch (summon_prop->Title)                     // better from known way sorting summons by AI types
            {
                case UNITNAME_SUMMON_TITLE_NONE:
                {
                    // those are classical totems - effectbasepoints is their hp and not summon ammount!
                    // 121: 23035, battlestands
                    // 647: 52893, Anti-Magic Zone (npc used)
                    if (prop_id == 121 || prop_id == 647)
                    {
                        summonResult = DoSummonTotem(eff_idx);
                    }
                    else
                    {
                        summonResult = DoSummonWild(summonPositions, summon_prop, eff_idx, level);
                    }
                    break;
                }
                case UNITNAME_SUMMON_TITLE_PET:
                case UNITNAME_SUMMON_TITLE_MINION:
                case UNITNAME_SUMMON_TITLE_RUNEBLADE:
                    summonResult = DoSummonGuardian(summonPositions, summon_prop, eff_idx, level);
                    break;
                case UNITNAME_SUMMON_TITLE_GUARDIAN:
                {
                    if (prop_id == 61)                      // mixed guardians, totems, statues
                    {
                        // * Stone Statue, etc  -- fits much better totem AI
                        if (m_spellInfo->SpellIconID == 2056)
                        {
                            summonResult = DoSummonTotem(eff_idx);
                        }
                        else
                        {
                            // possible sort totems/guardians only by summon creature type
                            CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(m_spellInfo->EffectMiscValue[eff_idx]);

                            if (!cInfo)
                            {
                                return;
                            }

                            // FIXME: not all totems and similar cases seelcted by this check...
                            if (cInfo->CreatureType == CREATURE_TYPE_TOTEM)
                            {
                                summonResult = DoSummonTotem(eff_idx);
                            }
                            else
                            {
                                summonResult = DoSummonGuardian(summonPositions, summon_prop, eff_idx, level);
                            }
                        }
                    }
                    else
                    {
                        summonResult = DoSummonGuardian(summonPositions, summon_prop, eff_idx, level);
                    }
                    break;
                }
                case UNITNAME_SUMMON_TITLE_CONSTRUCT:
                {
                    if (prop_id == 2913)                    // Scrapbot
                    {
                        summonResult = DoSummonWild(summonPositions, summon_prop, eff_idx, level);
                    }
                    else
                    {
                        summonResult = DoSummonGuardian(summonPositions, summon_prop, eff_idx, level);
                    }
                    break;
                }
                case UNITNAME_SUMMON_TITLE_TOTEM:
                    summonResult = DoSummonTotem(eff_idx, summon_prop->Slot);
                    break;
                case UNITNAME_SUMMON_TITLE_COMPANION:
                    // slot 6 set for critters that can help to player in fighting
                    if (summon_prop->Slot == 6)
                    {
                        summonResult = DoSummonGuardian(summonPositions, summon_prop, eff_idx, level);
                    }
                    else
                    {
                        summonResult = DoSummonCritter(summonPositions, summon_prop, eff_idx, level);
                    }
                    break;
                case UNITNAME_SUMMON_TITLE_OPPONENT:
                case UNITNAME_SUMMON_TITLE_VEHICLE:
                case UNITNAME_SUMMON_TITLE_MOUNT:
                case UNITNAME_SUMMON_TITLE_LIGHTWELL:
                case UNITNAME_SUMMON_TITLE_BUTLER:
                    summonResult = DoSummonWild(summonPositions, summon_prop, eff_idx, level);
                    break;
                default:
                    sLog.outError("EffectSummonType: Unhandled summon title %u", summon_prop->Title);
                    break;
            }
            break;
        }
        case SUMMON_PROP_GROUP_PETS:
        {
            // FIXME : multiple summons -  not yet supported as pet
            // 1562 - force of nature  - sid 33831
            // 1161 - feral spirit - sid 51533
            if (prop_id == 1562)                            // 3 uncontrolable instead of one controllable :/
            {
                summonResult = DoSummonGuardian(summonPositions, summon_prop, eff_idx, level);
            }
            break;
        }
        case SUMMON_PROP_GROUP_CONTROLLABLE:
        {
            // TODO: Fix spell 46619
            if (m_spellInfo->ID != 46619)
            {
                summonResult = DoSummonPossessed(summonPositions, summon_prop, eff_idx, level);
            }
            break;
        }
        case SUMMON_PROP_GROUP_VEHICLE:
        {
            summonResult = DoSummonVehicle(summonPositions, summon_prop, eff_idx, level);
            break;
        }
        default:
            sLog.outError("EffectSummonType: Unhandled summon group type %u", summon_prop->Control);
            break;
    }

    if (!summonResult)
    {
        return;                                             // No further handling required
    }

    for (CreatureSummonPositions::iterator itr = summonPositions.begin(); itr != summonPositions.end(); ++itr)
    {
        MANGOS_ASSERT(itr->creature || itr != summonPositions.begin());
        if (!itr->creature)
        {
            sLog.outError("EffectSummonType: Expected to have %u NPCs summoned, but some failed (Spell id %u)", amount, m_spellInfo->ID);
            continue;
        }

        if (summon_prop->Faction)
        {
            itr->creature->setFaction(summon_prop->Faction);
        }
        // Else set faction to summoner's faction for pet-like summoned
        else if ((summon_prop->Flags & SUMMON_PROP_FLAG_INHERIT_FACTION) || !itr->creature->IsTemporarySummon())
        {
            itr->creature->setFaction(responsibleCaster ? responsibleCaster->getFaction() : m_caster->getFaction());
        }

        if (!itr->creature->IsTemporarySummon())
        {
            itr->creature->AIM_Initialize();

            m_caster->GetMap()->Add(itr->creature);

            // Notify Summoner
            if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
            {
                ((Creature*)m_caster)->AI()->JustSummoned(itr->creature);
            }
            if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
            {
                ((Creature*)m_originalCaster)->AI()->JustSummoned(itr->creature);
            }

            // used by eluna
#ifdef ENABLE_ELUNA
            if (Unit* summoner = m_caster->ToUnit())
            {
                if (Eluna* e = summoner->GetEluna())
                {
                    e->OnSummoned(itr->creature, summoner);
                }
            }
            else if (m_originalCaster)
            {
                if (Unit* summoner = m_originalCaster->ToUnit())
                {
                    if (Eluna* e = summoner->GetEluna())
                    {
                        e->OnSummoned(itr->creature, summoner);
                    }
                }
            }
#endif
        }
    }
}

bool Spell::DoSummonWild(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectIndex effIdx, uint32 level)
{
    MANGOS_ASSERT(!list.empty() && prop);

    uint32 creature_entry = m_spellInfo->EffectMiscValue[effIdx];
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(creature_entry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonWild: creature entry %u not found for spell %u.", creature_entry, m_spellInfo->ID);
        return false;
    }

    TempSpawnType summonType = (m_duration == 0) ? TEMPSPAWN_DEAD_DESPAWN : TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN;

    for (CreatureSummonPositions::iterator itr = list.begin(); itr != list.end(); ++itr)
    {
        if (Creature* summon = m_caster->SummonCreature(creature_entry, itr->x, itr->y, itr->z, m_caster->GetOrientation(), summonType, m_duration))
        {
            itr->creature = summon;

            summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->ID);

            // UNIT_FIELD_CREATEDBY are not set for these kind of spells.
            // Does exceptions exist? If so, what are they?
            // summon->SetCreatorGuid(m_caster->GetObjectGuid());

            // Notify original caster if not done already
            if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
            {
                ((Creature*)m_originalCaster)->AI()->JustSummoned(summon);
#ifdef ENABLE_ELUNA
                if (Unit* summoner = m_originalCaster->ToUnit())
                {
                    if (Eluna* e = summoner->GetEluna())
                    {
                        e->OnSummoned(summon, summoner);
                    }
                }
#endif
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool Spell::DoSummonCritter(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectIndex effIdx, uint32 /*level*/)
{
    MANGOS_ASSERT(!list.empty() && prop);

    // ATM only first position is supported for summoning
    uint32 pet_entry = m_spellInfo->EffectMiscValue[effIdx];
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(pet_entry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonCritter: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->ID);
        return false;
    }

    Pet* old_critter = m_caster->GetMiniPet();

    // for same pet just despawn (player unsummon command)
    if (m_caster->GetTypeId() == TYPEID_PLAYER && old_critter && old_critter->GetEntry() == pet_entry)
    {
        m_caster->RemoveMiniPet();
        return false;
    }

    // despawn old pet before summon new
    if (old_critter)
    {
        m_caster->RemoveMiniPet();
    }

    // for (CreatureSummonPositions::iterator itr = list.begin(); itr != list.end(); ++itr)
    CreatureCreatePos pos(m_caster->GetMap(), list[0].x, list[0].y, list[0].z, m_caster->GetOrientation(), m_caster->GetPhaseMask());

    // summon new pet
    Pet* critter = new Pet(MINI_PET);

    uint32 pet_number = sObjectMgr.GeneratePetNumber();
    if (!critter->Create(m_caster->GetMap()->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
    {
        sLog.outError("Spell::EffectSummonCritter, spellid %u: no such creature entry %u", m_spellInfo->ID, pet_entry);
        delete critter;
        return false;
    }

    // itr!
    list[0].creature = critter;

    critter->SetRespawnCoord(pos);

    // critter->SetName("");                                // generated by client
    critter->SetOwnerGuid(m_caster->GetObjectGuid());
    critter->SetCreatorGuid(m_caster->GetObjectGuid());

    critter->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->ID);

    critter->InitPetCreateSpells();                         // e.g. disgusting oozeling has a create spell as critter...
    // critter->InitLevelupSpellsForLevel();                // none?
    critter->SelectLevel(critter->GetCreatureInfo());       // some summoned creaters have different from 1 DB data for level/hp
    critter->SetUInt32Value(UNIT_NPC_FLAGS, critter->GetCreatureInfo()->NpcFlags);
    // some mini-pets have quests
    // set timer for unsummon
    if (m_duration > 0)
    {
        critter->SetDuration(m_duration);
    }

    m_caster->SetMiniPet(critter);

#ifdef ENABLE_ELUNA
    if (Unit* summoner = m_caster->ToUnit())
    {
        if (Eluna* e = summoner->GetEluna())
        {
            e->OnSummoned(critter, summoner);
        }
    }
    if (m_originalCaster)
        if (Unit* summoner = m_originalCaster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(critter, summoner);
            }
        }
#endif

    return true;
}

bool Spell::DoSummonGuardian(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectIndex effIdx, uint32 level)
{
    MANGOS_ASSERT(!list.empty() && prop);

    uint32 pet_entry = m_spellInfo->EffectMiscValue[effIdx];
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(pet_entry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonGuardian: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->ID);
        return false;
    }

    PetType petType = prop->Title == UNITNAME_SUMMON_TITLE_COMPANION ? PROTECTOR_PET : GUARDIAN_PET;

    // second direct cast unsummon guardian(s) (guardians without like functionality have cooldown > spawn time)
    if (!m_IsTriggeredSpell && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        bool found = false;
        // including protector
        while (Pet* old_summon = m_caster->FindGuardianWithEntry(pet_entry))
        {
            old_summon->Unsummon(PET_SAVE_AS_DELETED, m_caster);
            found = true;
        }

        if (found)
        {
            return false;
        }
    }

    // protectors allowed only in single amount
    if (petType == PROTECTOR_PET)
        if (Pet* old_protector = m_caster->GetProtectorPet())
        {
            old_protector->Unsummon(PET_SAVE_AS_DELETED, m_caster);
        }

    // in another case summon new
    for (CreatureSummonPositions::iterator itr = list.begin(); itr != list.end(); ++itr)
    {
        Pet* spawnCreature = new Pet(petType);

        CreatureCreatePos pos(m_caster->GetMap(), itr->x, itr->y, itr->z, -m_caster->GetOrientation(), m_caster->GetPhaseMask());

        uint32 pet_number = sObjectMgr.GeneratePetNumber();
        if (!spawnCreature->Create(m_caster->GetMap()->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
        {
            sLog.outError("Spell::DoSummonGuardian: can't create creature entry %u for spell %u.", pet_entry, m_spellInfo->ID);
            delete spawnCreature;
            return false;
        }

        itr->creature = spawnCreature;

        spawnCreature->SetRespawnCoord(pos);

        if (m_duration > 0)
        {
            spawnCreature->SetDuration(m_duration);
        }

        // spawnCreature->SetName("");                      // generated by client
        spawnCreature->SetOwnerGuid(m_caster->GetObjectGuid());
        spawnCreature->SetPowerType(POWER_MANA);
        spawnCreature->SetUInt32Value(UNIT_NPC_FLAGS, spawnCreature->GetCreatureInfo()->NpcFlags);

        spawnCreature->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
        spawnCreature->SetCreatorGuid(m_caster->GetObjectGuid());
        spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->ID);

        spawnCreature->InitStatsForLevel(level, m_caster);
        spawnCreature->GetCharmInfo()->SetPetNumber(pet_number, false);

        m_caster->AddGuardian(spawnCreature);

#ifdef ENABLE_ELUNA
        if (Unit* summoner = m_caster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(spawnCreature, summoner);
            }
        }
        if (m_originalCaster)
        {
            if (Unit* summoner = m_originalCaster->ToUnit())
            {
                if (Eluna* e = summoner->GetEluna())
                {
                    e->OnSummoned(spawnCreature, summoner);
                }
            }
        }
#endif
    }

    return true;
}

bool Spell::DoSummonTotem(SpellEffectIndex eff_idx, uint8 slot_dbc)
{
    // DBC store slots starting from 1, with no slot 0 value)
    int slot = slot_dbc ? slot_dbc - 1 : TOTEM_SLOT_NONE;

    // unsummon old totem
    if (slot < MAX_TOTEM_SLOT)
        if (Totem* OldTotem = m_caster->GetTotem(TotemSlot(slot)))
        {
            OldTotem->UnSummon();
        }

    // FIXME: Setup near to finish point because GetObjectBoundingRadius set in Create but some Create calls can be dependent from proper position
    // if totem have creature_template_addon.auras with persistent point for example or script call
    float angle = slot < MAX_TOTEM_SLOT ? M_PI_F / MAX_TOTEM_SLOT - (slot * 2 * M_PI_F / MAX_TOTEM_SLOT) : 0;

    CreatureCreatePos pos(m_caster, m_caster->GetOrientation(), 2.0f, angle);

    CreatureInfo const* cinfo = ObjectMgr::GetCreatureTemplate(m_spellInfo->EffectMiscValue[eff_idx]);
    if (!cinfo)
    {
        sLog.outErrorDb("Creature entry %u does not exist but used in spell %u totem summon.", m_spellInfo->ID, m_spellInfo->EffectMiscValue[eff_idx]);
        return false;
    }

    Totem* pTotem = new Totem;

    if (!pTotem->Create(m_caster->GetMap()->GenerateLocalLowGuid(HIGHGUID_UNIT), pos, cinfo, m_caster))
    {
        delete pTotem;
        return false;
    }

    pTotem->SetRespawnCoord(pos);

    if (slot < MAX_TOTEM_SLOT)
    {
        m_caster->_AddTotem(TotemSlot(slot), pTotem);
    }

    // pTotem->SetName("");                                 // generated by client
    pTotem->SetOwner(m_caster);
    pTotem->SetTypeBySummonSpell(m_spellInfo);              // must be after Create call where m_spells initialized

    pTotem->SetDuration(m_duration);

    if (damage)                                             // if not spell info, DB values used
    {
        pTotem->SetMaxHealth(damage);
        pTotem->SetHealth(damage);
    }

    pTotem->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->ID);

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        pTotem->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    }

    if (m_caster->IsPvP())
    {
        pTotem->SetPvP(true);
    }

    if (m_caster->IsFFAPvP())
    {
        pTotem->SetFFAPvP(true);
    }

    // sending SMSG_TOTEM_CREATED before add to map (done in Summon)
    if (slot < MAX_TOTEM_SLOT && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_TOTEM_CREATED, 1 + 8 + 4 + 4);
        data << uint8(slot);
        data << pTotem->GetObjectGuid();
        data << uint32(m_duration);
        data << uint32(m_spellInfo->ID);
        ((Player*)m_caster)->SendDirectMessage(&data);
    }

    pTotem->Summon(m_caster);

    return false;
}

bool Spell::DoSummonPossessed(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectIndex effIdx, uint32 level)
{
    MANGOS_ASSERT(!list.empty() && prop);

    uint32 creatureEntry = m_spellInfo->EffectMiscValue[effIdx];
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(creatureEntry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonPossessed: creature entry %u not found for spell %u.", creatureEntry, m_spellInfo->ID);
        return false;
    }

    Creature* spawnCreature = m_caster->SummonCreature(creatureEntry, list[0].x, list[0].y, list[0].z, m_caster->GetOrientation(), TEMPSPAWN_CORPSE_DESPAWN, 0);
    if (!spawnCreature)
    {
        sLog.outError("Spell::DoSummonPossessed: creature entry %u for spell %u could not be summoned.", creatureEntry, m_spellInfo->ID);
        return false;
    }

    list[0].creature = spawnCreature;

    // Changes to be sent
    spawnCreature->SetCharmerGuid(m_caster->GetObjectGuid());
    spawnCreature->SetCreatorGuid(m_caster->GetObjectGuid());
    spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->ID);
    spawnCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

    spawnCreature->SetLevel(level);

    spawnCreature->SetWalk(m_caster->IsWalking(), true);
    // TODO: Set Fly (ie glyph dependend)

    // Internal changes
    spawnCreature->addUnitState(UNIT_STAT_CONTROLLED);

    // Changes to owner
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = (Player*)m_caster;

        player->GetCamera().SetView(spawnCreature);

        player->SetCharm(spawnCreature);
        player->SetClientControl(spawnCreature, 1);
        player->SetMover(spawnCreature);

        if (CharmInfo* charmInfo = spawnCreature->InitCharmInfo(spawnCreature))
        {
            charmInfo->InitPossessCreateSpells();
        }
        player->PossessSpellInitialize();
    }

    // Notify Summoner
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned(spawnCreature);

#ifdef ENABLE_ELUNA
        if (Unit* summoner = m_originalCaster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(spawnCreature, summoner);
            }
        }
#endif
    }
        return true;

}

bool Spell::DoSummonPet(SpellEffectIndex eff_idx)
{
    if (m_caster->GetPetGuid())
    {
        return false;
    }

    if (!unitTarget)
    {
        return false;
    }

    uint32 pet_entry = m_spellInfo->EffectMiscValue[eff_idx];
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(pet_entry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonPet: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->ID);
        return false;
    }

    uint32 level = m_caster->getLevel();                    // TODO Engineering Pets have also caster-level? (if they exist)
    Pet* spawnCreature = new Pet(SUMMON_PET);

    if (m_caster->GetTypeId() == TYPEID_PLAYER && spawnCreature->LoadPetFromDB((Player*)m_caster, pet_entry))
    {
        // Summon in dest location
        if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
        {
            spawnCreature->Relocate(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, -m_caster->GetOrientation());
        }

        // set timer for unsummon
        if (m_duration > 0)
        {
            spawnCreature->SetDuration(m_duration);
        }

        return false;
    }

    // Summon in dest location
    CreatureCreatePos pos(m_caster->GetMap(), m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, -m_caster->GetOrientation(), m_caster->GetPhaseMask());

    if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
    {
        pos = CreatureCreatePos(m_caster, -m_caster->GetOrientation());
    }

    Map* map = m_caster->GetMap();
    uint32 pet_number = sObjectMgr.GeneratePetNumber();
    if (!spawnCreature->Create(map->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
    {
        sLog.outErrorDb("Spell::EffectSummon: can't create creature with entry %u for spell %u", cInfo->Entry, m_spellInfo->ID);
        delete spawnCreature;
        return false;
    }

    spawnCreature->SetRespawnCoord(pos);

    // set timer for unsummon
    if (m_duration > 0)
    {
        spawnCreature->SetDuration(m_duration);
    }

    spawnCreature->SetOwnerGuid(m_caster->GetObjectGuid());
    spawnCreature->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
    spawnCreature->SetPowerType(POWER_MANA);
    spawnCreature->setFaction(m_caster->getFaction());
    spawnCreature->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
    spawnCreature->SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
    spawnCreature->SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
    spawnCreature->SetCreatorGuid(m_caster->GetObjectGuid());
    spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->ID);

    spawnCreature->InitStatsForLevel(level, m_caster);

    spawnCreature->GetCharmInfo()->SetPetNumber(pet_number, false);

    spawnCreature->AIM_Initialize();
    spawnCreature->InitPetCreateSpells();
    spawnCreature->InitLevelupSpellsForLevel();
    spawnCreature->SetHealth(spawnCreature->GetMaxHealth());
    spawnCreature->SetPower(POWER_MANA, spawnCreature->GetMaxPower(POWER_MANA));

    // spawnCreature->SetName("");                          // generated by client

    map->Add((Creature*)spawnCreature);

    m_caster->SetPet(spawnCreature);

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        spawnCreature->GetCharmInfo()->SetReactState(REACT_DEFENSIVE);
        spawnCreature->SavePetToDB(PET_SAVE_AS_CURRENT);
        ((Player*)m_caster)->PetSpellInitialize();
    }

    if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
    {
        ((Creature*)m_caster)->AI()->JustSummoned((Creature*)spawnCreature);
    }
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned((Creature*)spawnCreature);
    }

#ifdef ENABLE_ELUNA
    if (Unit* summoner = m_caster->ToUnit())
    {
        if (Eluna* e = summoner->GetEluna())
        {
            e->OnSummoned(spawnCreature, summoner);
        }
    }
    if (m_originalCaster)
        if (Unit* summoner = m_originalCaster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(spawnCreature, summoner);
            }
        }
#endif

    return false;
}

bool Spell::DoSummonVehicle(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectIndex effIdx, uint32 /*level*/)
{
    MANGOS_ASSERT(!list.empty() && prop);

    uint32 creatureEntry = m_spellInfo->EffectMiscValue[effIdx];
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(creatureEntry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonVehicle: creature entry %u not found for spell %u.", creatureEntry, m_spellInfo->ID);
        return false;
    }

    Creature* spawnCreature = m_caster->SummonCreature(creatureEntry, list[0].x, list[0].y, list[0].z, m_caster->GetOrientation(),
                              (m_duration == 0) ? TEMPSPAWN_CORPSE_DESPAWN : TEMPSPAWN_TIMED_OOC_OR_CORPSE_DESPAWN, m_duration);

    if (!spawnCreature)
    {
        sLog.outError("Spell::DoSummonVehicle: creature entry %u for spell %u could not be summoned.", creatureEntry, m_spellInfo->ID);
        return false;
    }

    list[0].creature = spawnCreature;

    // Changes to be sent
    spawnCreature->SetCreatorGuid(m_caster->GetObjectGuid());
    spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->ID);
    //spawnCreature->SetLevel(level); // Do we need to set level for vehicles?

    // Board the caster right after summoning
    SpellEntry const* controlSpellEntry = sSpellStore.LookupEntry(m_spellInfo->CalculateSimpleValue(effIdx));
    if (controlSpellEntry && IsSpellHaveAura(controlSpellEntry, SPELL_AURA_CONTROL_VEHICLE))
    {
        m_caster->CastSpell(spawnCreature, controlSpellEntry, true);
    }
    else
    {
        m_caster->CastSpell(spawnCreature, SPELL_RIDE_VEHICLE_HARDCODED, true);
    }

    // If the boarding failed...
    if (!spawnCreature->HasAuraType(SPELL_AURA_CONTROL_VEHICLE))
    {
        spawnCreature->ForcedDespawn();
        return false;
    }

    // Notify Summoner
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned(spawnCreature);
    }

#ifdef ENABLE_ELUNA
    if (m_originalCaster)
        if (Unit* summoner = m_originalCaster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(spawnCreature, summoner);
            }
        }
#endif

    return true;
}
