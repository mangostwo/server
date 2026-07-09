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
 * @file SpellEffectTail.cpp
 * @brief Cohesion split of SpellEffects.cpp -- remaining trailing effect handlers.
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
 * @brief Removes auras from the target that match the specified mechanic.
 *
 * @param eff_idx The effect index containing the mechanic id.
 */
void Spell::EffectDispelMechanic(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }

    uint32 mechanic = m_spellInfo->EffectMiscValue[eff_idx];

    Unit::SpellAuraHolderMap& Auras = unitTarget->GetSpellAuraHolderMap();
    for (Unit::SpellAuraHolderMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
    {
        next = iter;
        ++next;
        SpellEntry const* spell = iter->second->GetSpellProto();
        if (iter->second->HasMechanic(mechanic))
        {
            unitTarget->RemoveAurasDueToSpell(spell->ID);
            if (Auras.empty())
            {
                break;
            }
            else
            {
                next = Auras.begin();
            }
        }
    }
}

/**
 * @brief Restores the caster's dead pet and revives it with percentage-based health.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectSummonDeadPet(SpellEffectIndex /*eff_idx*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    Player* _player = (Player*)m_caster;
    Pet* pet = _player->GetPet();
    if (!pet)
    {
        return;
    }
    if (pet->IsAlive())
    {
        return;
    }
    if (damage < 0)
    {
        return;
    }

    pet->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
    pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
    pet->SetDeathState(ALIVE);
    pet->clearUnitState(UNIT_STAT_ALL_STATE);
    pet->SetHealth(uint32(pet->GetMaxHealth() * (float(damage) / 100)));

    pet->AIM_Initialize();

    // _player->PetSpellInitialize(); -- action bar not removed at death and not required send at revive
    pet->SavePetToDB(PET_SAVE_AS_CURRENT);
}

void Spell::EffectSummonAllTotems(SpellEffectIndex eff_idx)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    int32 start_button = ACTION_BUTTON_SHAMAN_TOTEMS_BAR + m_spellInfo->EffectMiscValue[eff_idx];
    int32 amount_buttons = m_spellInfo->EffectMiscValueB[eff_idx];

    for (int32 slot = 0; slot < amount_buttons; ++slot)
    {
        if (ActionButton const* actionButton = ((Player*)m_caster)->GetActionButton(start_button + slot))
            if (actionButton->GetType() == ACTION_BUTTON_SPELL)
                if (uint32 spell_id = actionButton->GetAction())
                {
                    m_caster->CastSpell(unitTarget, spell_id, true);
                }
    }
}

/**
 * @brief Unsummons all totems currently owned by the caster.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectDestroyAllTotems(SpellEffectIndex /*eff_idx*/)
{
    int32 mana = 0;
    for (int slot = 0;  slot < MAX_TOTEM_SLOT; ++slot)
    {
        if (Totem* totem = m_caster->GetTotem(TotemSlot(slot)))
        {
            if (damage)
            {
                uint32 spell_id = totem->GetUInt32Value(UNIT_CREATED_BY_SPELL);
                if (SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id))
                {
                    uint32 manacost = spellInfo->ManaCost + m_caster->GetCreateMana() * spellInfo->ManaCostPct / 100;
                    mana += manacost * damage / 100;
                }
            }
            {
                totem->UnSummon();
            }
        }
    }

    if (mana)
    {
        m_caster->CastCustomSpell(m_caster, 39104, &mana, NULL, NULL, true);
    }
}

void Spell::EffectBreakPlayerTargeting(SpellEffectIndex /* eff_idx */)
{
    if (!unitTarget)
    {
        return;
    }

    WorldPacket data(SMSG_CLEAR_TARGET, 8);
    data << unitTarget->GetObjectGuid();
    unitTarget->SendMessageToSet(&data, false);
}

/**
 * @brief Removes a fixed number of durability points from one or more player items.
 *
 * @param eff_idx The effect index containing the inventory slot selector.
 */
void Spell::EffectDurabilityDamage(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    int32 slot = m_spellInfo->EffectMiscValue[eff_idx];

    // FIXME: some spells effects have value -1/-2
    // Possibly its mean -1 all player equipped items and -2 all items
    if (slot < 0)
    {
        ((Player*)unitTarget)->DurabilityPointsLossAll(damage, (slot < -1));
        return;
    }

    // invalid slot value
    if (slot >= INVENTORY_SLOT_BAG_END)
    {
        return;
    }

    if (Item* item = ((Player*)unitTarget)->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
    {
        ((Player*)unitTarget)->DurabilityPointsLoss(item, damage);
    }
}

/**
 * @brief Removes a percentage of durability from one or more player items.
 *
 * @param eff_idx The effect index containing the inventory slot selector.
 */
void Spell::EffectDurabilityDamagePCT(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    int32 slot = m_spellInfo->EffectMiscValue[eff_idx];

    // FIXME: some spells effects have value -1/-2
    // Possibly its mean -1 all player equipped items and -2 all items
    if (slot < 0)
    {
        ((Player*)unitTarget)->DurabilityLossAll(double(damage) / 100.0f, (slot < -1));
        return;
    }

    // invalid slot value
    if (slot >= INVENTORY_SLOT_BAG_END)
    {
        return;
    }

    if (damage <= 0)
    {
        return;
    }

    if (Item* item = ((Player*)unitTarget)->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
    {
        ((Player*)unitTarget)->DurabilityLoss(item, double(damage) / 100.0f);
    }
}

/**
 * @brief Modifies the caster's threat on the target by a percentage.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectModifyThreatPercent(SpellEffectIndex /*eff_idx*/)
{
    if (!unitTarget)
    {
        return;
    }

    unitTarget->GetThreatManager().modifyThreatPercent(m_caster, damage);
}

/**
 * @brief Summons a transmitted game object such as fishing nodes, rituals, or spell casters.
 *
 * @param eff_idx The effect index containing the game object entry.
 */
void Spell::EffectTransmitted(SpellEffectIndex eff_idx)
{
    uint32 name_id = m_spellInfo->EffectMiscValue[eff_idx];

    GameObjectInfo const* goinfo = ObjectMgr::GetGameObjectInfo(name_id);

    if (!goinfo)
    {
        sLog.outErrorDb("Gameobject (Entry: %u) not exist and not created at spell (ID: %u) cast", name_id, m_spellInfo->ID);
        return;
    }

    float fx, fy, fz;

    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(fx, fy, fz);
    }
    // FIXME: this can be better check for most objects but still hack
    else if (m_spellInfo->EffectRadiusIndex[eff_idx] && m_spellInfo->Speed == 0)
    {
        float dis = GetSpellRadius(sSpellRadiusStore.LookupEntry(m_spellInfo->EffectRadiusIndex[eff_idx]));
        m_caster->GetClosePoint(fx, fy, fz, DEFAULT_WORLD_OBJECT_SIZE, dis);
    }
    else
    {
        float min_dis = GetSpellMinRange(sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex));
        float max_dis = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->RangeIndex));
        float dis = rand_norm_f() * (max_dis - min_dis) + min_dis;

        // special code for fishing bobber (TARGET_SELF_FISHING), should not try to avoid objects
        // nor try to find ground level, but randomly vary in angle
        if (goinfo->type == GAMEOBJECT_TYPE_FISHINGNODE)
        {
            // calculate angle variation for roughly equal dimensions of target area
            float max_angle = (max_dis - min_dis) / (max_dis + m_caster->GetObjectBoundingRadius());
            float angle_offset = max_angle * (rand_norm_f() - 0.5f);
            m_caster->GetNearPoint2D(fx, fy, dis + m_caster->GetObjectBoundingRadius(), m_caster->GetOrientation() + angle_offset);

            SpellCastResult err = SPELL_FAILED_SUCCESS;
            float waistHeight = GetModelMidpoint(m_caster->GetDisplayId()) * m_caster->GetObjectScale();

            if (!m_caster->GetTerrain()->IsAboveWater(fx, fy, m_caster->GetPositionZ() + waistHeight + 0.5f, &fz))
            {
                err = SPELL_FAILED_NOT_FISHABLE;
            }
            else if (m_caster->GetPositionZ() < (fz - waistHeight))
            {
                err = SPELL_FAILED_ONLY_ABOVEWATER;
            }
            else if (!m_caster->IsWithinLOS(fx, fy, fz))
            {
                err = SPELL_FAILED_LINE_OF_SIGHT;
            }

            if (err != SPELL_FAILED_SUCCESS)
            {
                SendCastResult(err);
                SendChannelUpdate(0);
                return;
            }
        }
        else
        {
            m_caster->GetClosePoint(fx, fy, fz, DEFAULT_WORLD_OBJECT_SIZE, dis);
        }
    }

    Map* cMap = m_caster->GetMap();


    // if gameobject is summoning object, it should be spawned right on caster's position
    if (goinfo->type == GAMEOBJECT_TYPE_SUMMONING_RITUAL)
    {
        m_caster->GetPosition(fx, fy, fz);
    }

    GameObject* pGameObj = new GameObject;

    if (!pGameObj->Create(cMap->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), name_id, cMap,
                          m_caster->GetPhaseMask(), fx, fy, fz, m_caster->GetOrientation()))
    {
        delete pGameObj;
        return;
    }

    int32 duration = m_duration;

    switch (goinfo->type)
    {
        case GAMEOBJECT_TYPE_FISHINGNODE:
        {
            m_caster->SetChannelObjectGuid(pGameObj->GetObjectGuid());
            m_caster->AddGameObject(pGameObj);              // will removed at spell cancel

            // end time of range when possible catch fish (FISHING_BOBBER_READY_TIME..GetDuration(m_spellInfo))
            // start time == fish-FISHING_BOBBER_READY_TIME (0..GetDuration(m_spellInfo)-FISHING_BOBBER_READY_TIME)
            int32 lastSec = 0;
            switch (urand(0, 3))
            {
                case 0: lastSec =  3; break;
                case 1: lastSec =  7; break;
                case 2: lastSec = 13; break;
                case 3: lastSec = 17; break;
            }

            duration = duration - lastSec * IN_MILLISECONDS + FISHING_BOBBER_READY_TIME * IN_MILLISECONDS;
            break;
        }
        case GAMEOBJECT_TYPE_SUMMONING_RITUAL:
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                pGameObj->AddUniqueUse((Player*)m_caster);
                m_caster->AddGameObject(pGameObj);          // will removed at spell cancel
            }
            break;
        }
        case GAMEOBJECT_TYPE_FISHINGHOLE:
        case GAMEOBJECT_TYPE_CHEST:
        default:
            break;
    }

    pGameObj->SetRespawnTime(duration > 0 ? duration / IN_MILLISECONDS : 0);

    pGameObj->SetOwnerGuid(m_caster->GetObjectGuid());

    pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
    pGameObj->SetSpellId(m_spellInfo->ID);

    DEBUG_LOG("AddObject at SpellEfects.cpp EffectTransmitted");
    // m_caster->AddGameObject(pGameObj);
    // m_ObjToDel.push_back(pGameObj);

    cMap->Add(pGameObj);
    pGameObj->AIM_Initialize();

    pGameObj->SummonLinkedTrapIfAny();

    if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
    {
        ((Creature*)m_caster)->AI()->JustSummoned(pGameObj);
    }
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned(pGameObj);
    }
}

void Spell::EffectProspecting(SpellEffectIndex /*eff_idx*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER || !itemTarget)
    {
        return;
    }

    Player* p_caster = (Player*)m_caster;

    if (sWorld.getConfig(CONFIG_BOOL_SKILL_PROSPECTING))
    {
        uint32 SkillValue = p_caster->GetPureSkillValue(SKILL_JEWELCRAFTING);
        uint32 reqSkillValue = itemTarget->GetProto()->RequiredSkillRank;
        p_caster->UpdateGatherSkill(SKILL_JEWELCRAFTING, SkillValue, reqSkillValue);
    }

    ((Player*)m_caster)->SendLoot(itemTarget->GetObjectGuid(), LOOT_PROSPECTING);
}

void Spell::EffectMilling(SpellEffectIndex /*eff_idx*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER || !itemTarget)
    {
        return;
    }

    Player* p_caster = (Player*)m_caster;

    if (sWorld.getConfig(CONFIG_BOOL_SKILL_MILLING))
    {
        uint32 SkillValue = p_caster->GetPureSkillValue(SKILL_INSCRIPTION);
        uint32 reqSkillValue = itemTarget->GetProto()->RequiredSkillRank;
        p_caster->UpdateGatherSkill(SKILL_INSCRIPTION, SkillValue, reqSkillValue);
    }

    ((Player*)m_caster)->SendLoot(itemTarget->GetObjectGuid(), LOOT_MILLING);
}

void Spell::EffectSkill(SpellEffectIndex /*eff_idx*/)
{
    DEBUG_LOG("WORLD: SkillEFFECT");
}

/**
 * @brief Fully resurrects a dead player target as part of a spirit heal effect.
 *
 * @param eff_idx Unused effect index.
 */
void Spell::EffectSpiritHeal(SpellEffectIndex /*eff_idx*/)
{
    // TODO player can't see the heal-animation - he should respawn some ticks later
    if (!unitTarget || unitTarget->IsAlive())
    {
        return;
    }
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    if (!unitTarget->IsInWorld())
    {
        return;
    }
    if (m_spellInfo->ID == 22012 && !unitTarget->HasAura(2584))
    {
        return;
    }

    ((Player*)unitTarget)->ResurrectPlayer(1.0f);
    ((Player*)unitTarget)->SpawnCorpseBones();
}

// remove insignia spell effect
void Spell::EffectSkinPlayerCorpse(SpellEffectIndex /*eff_idx*/)
{
    DEBUG_LOG("Effect: SkinPlayerCorpse");
    if ((m_caster->GetTypeId() != TYPEID_PLAYER) || (unitTarget->GetTypeId() != TYPEID_PLAYER) || (unitTarget->IsAlive()))
    {
        return;
    }

    ((Player*)unitTarget)->RemovedInsignia((Player*)m_caster);
}

void Spell::EffectStealBeneficialBuff(SpellEffectIndex eff_idx)
{
    DEBUG_LOG("Effect: StealBeneficialBuff");

    if (!unitTarget || unitTarget == m_caster)              // can't steal from self
    {
        return;
    }

    typedef std::vector<SpellAuraHolder*> StealList;
    StealList steal_list;
    // Create dispel mask by dispel type
    uint32 dispelMask  = GetDispellMask(DispelType(m_spellInfo->EffectMiscValue[eff_idx]));
    Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
    for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        SpellAuraHolder* holder = itr->second;
        if (holder && (1 << holder->GetSpellProto()->DispelType) & dispelMask)
        {
            // Need check for passive? this
            if (holder->IsPositive() && !holder->IsPassive() && !holder->GetSpellProto()->HasAttribute(SPELL_ATTR_EX4_NOT_STEALABLE))
            {
                steal_list.push_back(holder);
            }
        }
    }
    // Ok if exist some buffs for dispel try dispel it
    if (!steal_list.empty())
    {
        typedef std::list < std::pair<uint32, ObjectGuid> > SuccessList;
        SuccessList success_list;
        int32 list_size = steal_list.size();
        // Dispell N = damage buffs (or while exist buffs for dispel)
        for (int32 count = 0; count < damage && list_size > 0; ++count)
        {
            // Random select buff for dispel
            SpellAuraHolder* holder = steal_list[urand(0, list_size-1)];
            // Not use chance for steal
            // TODO possible need do it
            success_list.push_back(SuccessList::value_type(holder->GetId(), holder->GetCasterGuid()));

            // Remove buff from list for prevent doubles
            for (StealList::iterator j = steal_list.begin(); j != steal_list.end();)
            {
                SpellAuraHolder* stealed = *j;
                if (stealed->GetId() == holder->GetId() && stealed->GetCasterGuid() == holder->GetCasterGuid())
                {
                    j = steal_list.erase(j);
                    --list_size;
                }
                else
                {
                    ++j;
                }
            }
        }
        // Really try steal and send log
        if (!success_list.empty())
        {
            int32 count = success_list.size();
            WorldPacket data(SMSG_SPELLSTEALLOG, 8 + 8 + 4 + 1 + 4 + count * 5);
            data << unitTarget->GetPackGUID();       // Victim GUID
            data << m_caster->GetPackGUID();         // Caster GUID
            data << uint32(m_spellInfo->ID);         // Dispell spell id
            data << uint8(0);                        // not used
            data << uint32(count);                   // count
            for (SuccessList::iterator j = success_list.begin(); j != success_list.end(); ++j)
            {
                SpellEntry const* spellInfo = sSpellStore.LookupEntry(j->first);
                data << uint32(spellInfo->ID);       // Spell Id
                data << uint8(0);                    // 0 - steals !=0 transfers
                unitTarget->RemoveAurasDueToSpellBySteal(spellInfo->ID, j->second, m_caster);
            }
            m_caster->SendMessageToSet(&data, true);
        }
    }
}

void Spell::EffectWMODamage(SpellEffectIndex effIdx)
{
    DEBUG_LOG("Effect: WMODamage");

    if (!gameObjTarget)
    {
        return;
    }

    if (gameObjTarget->GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
    {
        sLog.outError("Spell::EffectWMODamage called without valid targets. Spell Id %u", m_spellInfo->ID);
        return;
    }

    if (!gameObjTarget->GetHealth())
    {
        return;
    }

    Unit* caster = GetAffectiveCaster();
    if (!caster)
    {
        return;
    }

    DEBUG_LOG("Spell::EffectWMODamage, spell Id %u, go entry %u, damage %u", m_spellInfo->ID, gameObjTarget->GetEntry(), uint32(damage));
    gameObjTarget->DealGameObjectDamage(uint32(damage), m_spellInfo->ID, caster);
}

void Spell::EffectWMORepair(SpellEffectIndex effIdx)
{
    DEBUG_LOG("Effect: WMORepair");

    if (!gameObjTarget)
    {
        return;
    }

    if (gameObjTarget->GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
    {
        sLog.outError("Spell::EffectWMORepair called without valid targets. Spell Id %u", m_spellInfo->ID);
        return;
    }

    Unit* caster = GetAffectiveCaster();
    if (!caster)
    {
        return;
    }

    DEBUG_LOG("Spell::EffectWMORepair, spell Id %u, go entry %u", m_spellInfo->ID, gameObjTarget->GetEntry());
    gameObjTarget->RebuildGameObject(caster);
}

void Spell::EffectWMOChange(SpellEffectIndex effIdx)
{
    DEBUG_LOG("Effect: WMOChange");

    if (!gameObjTarget)
    {
        return;
    }

    if (gameObjTarget->GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
    {
        sLog.outError("Spell::EffectWMOChange called without valid targets. Spell Id %u", m_spellInfo->ID);
        return;
    }

    DEBUG_LOG("Spell::EffectWMOChange, spell Id %u, object %u, misc-value %u", m_spellInfo->ID, gameObjTarget->GetEntry(), m_spellInfo->EffectMiscValue[effIdx]);

    Unit* caster = GetAffectiveCaster();
    if (!caster)
    {
        return;
    }

    switch (m_spellInfo->EffectMiscValue[effIdx])
    {
        case 0:                                             // Set to full health
            gameObjTarget->ForceGameObjectHealth(gameObjTarget->GetMaxHealth(), caster);
            break;
        case 1:                                             // Set to damaged
            gameObjTarget->ForceGameObjectHealth(gameObjTarget->GetGOInfo()->destructibleBuilding.damagedNumHits, caster);
            break;
        case 2:                                             // Set to destroyed
            gameObjTarget->ForceGameObjectHealth(-int32(gameObjTarget->GetHealth()), caster);
            break;
        case 3:                                             // Set to rebuilding
            gameObjTarget->ForceGameObjectHealth(0, caster);
            break;
        default:
            sLog.outError("Spell::EffectWMOChange, spell Id %u with undefined change value %u", m_spellInfo->ID, m_spellInfo->EffectMiscValue[effIdx]);
            break;
    }
}

void Spell::EffectKillCreditPersonal(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->KilledMonsterCredit(m_spellInfo->EffectMiscValue[eff_idx]);
}

void Spell::EffectKillCreditGroup(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->RewardPlayerAndGroupAtEvent(m_spellInfo->EffectMiscValue[eff_idx], unitTarget);
}

void Spell::EffectQuestFail(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->FailQuest(m_spellInfo->EffectMiscValue[eff_idx]);
}

void Spell::EffectActivateRune(SpellEffectIndex eff_idx)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* plr = (Player*)m_caster;

    if (plr->getClass() != CLASS_DEATH_KNIGHT)
    {
        return;
    }

    int32 count = damage;                                   // max amount of reset runes
    if (plr->ActivateRunes(RuneType(m_spellInfo->EffectMiscValue[eff_idx]), count))
    {
        plr->ResyncRunes();
    }
}

void Spell::EffectTitanGrip(SpellEffectIndex eff_idx)
{
    // Make sure "Titan's Grip" (49152) penalty spell does not silently change
    if (m_spellInfo->EffectMiscValue[eff_idx] != 49152)
    {
        sLog.outError("Spell::EffectTitanGrip: Spell %u has unexpected EffectMiscValue '%u'", m_spellInfo->ID, m_spellInfo->EffectMiscValue[eff_idx]);
    }
    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        Player* plr = (Player*)m_caster;
        plr->SetCanTitanGrip(true);
        if (plr->HasTwoHandWeaponInOneHand() && !plr->HasAura(49152))
        {
            plr->CastSpell(plr, 49152, true);
        }
    }
}

void Spell::EffectRenamePet(SpellEffectIndex /*eff_idx*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT ||
            !((Creature*)unitTarget)->IsPet() || ((Pet*)unitTarget)->getPetType() != HUNTER_PET)
        return;

    unitTarget->RemoveByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);
}

void Spell::EffectPlaySound(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    uint32 soundId = m_spellInfo->EffectMiscValue[eff_idx];
    if (!sSoundEntriesStore.LookupEntry(soundId))
    {
        sLog.outError("EffectPlaySound: Sound (Id: %u) in spell %u does not exist.", soundId, m_spellInfo->ID);
        return;
    }

    unitTarget->PlayDirectSound(soundId, (Player*)unitTarget);
}

void Spell::EffectPlayMusic(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    uint32 soundId = m_spellInfo->EffectMiscValue[eff_idx];
    if (!sSoundEntriesStore.LookupEntry(soundId))
    {
        sLog.outError("EffectPlayMusic: Sound (Id: %u) in spell %u does not exist.", soundId, m_spellInfo->ID);
        return;
    }

    WorldPacket data(SMSG_PLAY_MUSIC, 4);
    data << uint32(soundId);
    ((Player*)unitTarget)->GetSession()->SendPacket(&data);
}

void Spell::EffectSpecCount(SpellEffectIndex /*eff_idx*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->UpdateSpecCount(damage);
}

void Spell::EffectActivateSpec(SpellEffectIndex /*eff_idx*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    uint32 spec = damage - 1;

    ((Player*)unitTarget)->ActivateSpec(spec);
}

/**
 * @brief Sets the player's homebind location to the current position.
 *
 * @param eff_idx The bind effect index.
 */
void Spell::EffectBind(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)unitTarget;

    uint32 area_id = m_spellInfo->EffectMiscValue[eff_idx];
    WorldLocation loc;
    if (m_spellInfo->ImplicitTargetA[eff_idx] == TARGET_TABLE_X_Y_Z_COORDINATES ||
            m_spellInfo->ImplicitTargetB[eff_idx] == TARGET_TABLE_X_Y_Z_COORDINATES)
    {
        SpellTargetPosition const* st = sSpellMgr.GetSpellTargetPosition(m_spellInfo->ID);
        if (!st)
        {
            sLog.outError("Spell::EffectBind - unknown Teleport coordinates for spell ID %u", m_spellInfo->ID);
            return;
        }

        loc.mapid       = st->target_mapId;
        loc.coord_x     = st->target_X;
        loc.coord_y     = st->target_Y;
        loc.coord_z     = st->target_Z;
        loc.orientation = st->target_Orientation;
        if (!area_id)
        {
            area_id = sTerrainMgr.GetAreaId(loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z);
        }
    }
    else
    {
        player->GetPosition(loc);
        if (!area_id)
        {
            area_id = player->GetAreaId();
        }
    }

    player->SetHomebindToLocation(loc, area_id);

    // binding
    WorldPacket data(SMSG_BINDPOINTUPDATE, (4 + 4 + 4 + 4 + 4));
    data << float(loc.coord_x);
    data << float(loc.coord_y);
    data << float(loc.coord_z);
    data << uint32(loc.mapid);
    data << uint32(area_id);
    player->SendDirectMessage(&data);

    DEBUG_LOG("New Home Position for %s: XYZ: %f %f %f on Map %u", player->GetGuidStr().c_str(), loc.coord_x, loc.coord_y, loc.coord_z, loc.mapid);

    // zone update
    data.Initialize(SMSG_PLAYERBOUND, 8 + 4);
    data << m_caster->GetObjectGuid();
    data << uint32(area_id);
    player->SendDirectMessage(&data);
}

void Spell::EffectRestoreItemCharges(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ItemPrototype const* itemProto = ObjectMgr::GetItemPrototype(m_spellInfo->EffectItemType[eff_idx]);
    if (!itemProto)
    {
        return;
    }

    // In case item from limited category recharge any from category, is this valid checked early in spell checks
    Item* item;
    if (itemProto->ItemLimitCategory)
    {
        item = ((Player*)unitTarget)->GetItemByLimitedCategory(itemProto->ItemLimitCategory);
    }
    else
    {
        item = ((Player*)unitTarget)->GetItemByEntry(m_spellInfo->EffectItemType[eff_idx]);
    }

    if (!item)
    {
        return;
    }

    item->RestoreCharges();
}

/**
 * @brief Sends a battleground player target to its graveyard.
 *
 * @param eff_idx The teleport effect index.
 */
void Spell::EffectRedirectThreat(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }

    if (m_spellInfo->ID == 59665)                           // Vigilance
        if (Aura* glyph = unitTarget->GetDummyAura(63326))  // Glyph of Vigilance
        {
            damage += glyph->GetModifier()->m_amount;
        }

    m_caster->GetHostileRefManager().SetThreatRedirection(unitTarget->GetObjectGuid(), uint32(damage));
}

void Spell::EffectTeachTaxiNode(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    uint32 taxiNodeId = m_spellInfo->EffectMiscValue[eff_idx];
    if (!sTaxiNodesStore.LookupEntry(taxiNodeId))
    {
        return;
    }

    Player* player = (Player*)unitTarget;

    if (player->m_taxi.SetTaximaskNode(taxiNodeId))
    {
        WorldPacket data(SMSG_NEW_TAXI_PATH, 0);
        player->SendDirectMessage(&data);

        data.Initialize(SMSG_TAXINODE_STATUS, 9);
        data << m_caster->GetObjectGuid();
        data << uint8(1);
        player->SendDirectMessage(&data);
    }
}

void Spell::EffectQuestOffer(SpellEffectIndex eff_idx)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (Quest const* quest = sObjectMgr.GetQuestTemplate(m_spellInfo->EffectMiscValue[eff_idx]))
    {
        Player* player = (Player*)unitTarget;

        if (player->CanTakeQuest(quest, false))
        {
            player->PlayerTalkClass->SendQuestGiverQuestDetails(quest, player->GetObjectGuid(), true);
        }
    }
}

void Spell::EffectCancelAura(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }

    uint32 spellId = m_spellInfo->EffectTriggerSpell[eff_idx];

    if (!sSpellStore.LookupEntry(spellId))
    {
        sLog.outError("Spell::EffectCancelAura: spell %u doesn't exist", spellId);
        return;
    }

    unitTarget->RemoveAurasDueToSpell(spellId);
}

void Spell::EffectKnockBackFromPosition(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }

    float x, y, z;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);
    }
    else
    {
        m_caster->GetPosition(x, y, z);
    }

    float angle = unitTarget->GetAngle(x, y) + M_PI_F;
    float horizontalSpeed = m_spellInfo->EffectMiscValue[eff_idx] * 0.1f;
    float verticalSpeed = damage * 0.1f;
    unitTarget->KnockBackWithAngle(angle, horizontalSpeed, verticalSpeed);
}

void Spell::EffectGravityPull(SpellEffectIndex eff_idx)
{
    if (!unitTarget)
    {
        return;
    }

    float x, y, z;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);
    }
    else
    {
        m_caster->GetPosition(x, y, z);
    }

    float speed = float(m_spellInfo->EffectMiscValue[eff_idx]) * 0.15f;
    float height = float(unitTarget->GetDistance(x, y, z) * 0.2f);

    unitTarget->GetMotionMaster()->MoveJump(x, y, z, speed, height);
}
