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
 * @file SpellMgrAreas.cpp
 * @brief Cohesion split of SpellMgr.cpp -- spell-area requirement and skill-line / skill-race-class map loaders.
 */

#include "Common/TimeConstants.h"
#include "SpellMgr.h"
#include "SpellAuraDefines.h"
#include "ObjectMgr.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "ProgressBar.h"
#include "DBCStores.h"
#include "SQLStorages.h"

/**
 * @brief Loads spell area requirement records from the database.
 */
void SpellMgr::LoadSpellAreas()
{
    mSpellAreaMap.clear();                                  // need for reload case
    mSpellAreaForAuraMap.clear();

    uint32 count = 0;

    //                                                0      1     2            3                   4          5             6           7         8       9
    QueryResult* result = WorldDatabase.Query("SELECT `spell`, `area`, `quest_start`, `quest_start_active`, `quest_end`, `condition_id`, `aura_spell`, `racemask`, `gender`, `autocast` FROM `spell_area`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outString(">> Loaded %u spell area requirements", count);
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        bar.step();

        uint32 spell = fields[0].GetUInt32();
        SpellArea spellArea;
        spellArea.spellId             = spell;
        spellArea.areaId              = fields[1].GetUInt32();
        spellArea.questStart          = fields[2].GetUInt32();
        spellArea.questStartCanActive = fields[3].GetBool();
        spellArea.questEnd            = fields[4].GetUInt32();
        spellArea.conditionId         = fields[5].GetUInt16();
        spellArea.auraSpell           = fields[6].GetInt32();
        spellArea.raceMask            = fields[7].GetUInt32();
        spellArea.gender              = Gender(fields[8].GetUInt8());
        spellArea.autocast            = fields[9].GetBool();

        if (!sSpellStore.LookupEntry(spell))
        {
            sLog.outErrorDb("Spell %u listed in `spell_area` does not exist", spell);
            continue;
        }

        {
            bool ok = true;
            SpellAreaMapBounds sa_bounds = GetSpellAreaMapBounds(spellArea.spellId);
            for (SpellAreaMap::const_iterator itr = sa_bounds.first; itr != sa_bounds.second; ++itr)
            {
                if (spellArea.spellId != itr->second.spellId)
                {
                    continue;
                }
                if (spellArea.areaId != itr->second.areaId)
                {
                    continue;
                }
                if (spellArea.questStart != itr->second.questStart)
                {
                    continue;
                }
                if (spellArea.auraSpell != itr->second.auraSpell)
                {
                    continue;
                }
                if ((spellArea.raceMask & itr->second.raceMask) == 0)
                {
                    continue;
                }
                if (spellArea.gender != itr->second.gender)
                {
                    continue;
                }

                // duplicate by requirements
                ok = false;
                break;
            }

            if (!ok)
            {
                sLog.outErrorDb("Spell %u listed in `spell_area` already listed with similar requirements.", spell);
                continue;
            }
        }

        if (spellArea.areaId && !GetAreaEntryByAreaID(spellArea.areaId))
        {
            sLog.outErrorDb("Spell %u listed in `spell_area` have wrong area (%u) requirement", spell, spellArea.areaId);
            continue;
        }

        if (spellArea.conditionId && !sConditionStorage.LookupEntry<PlayerCondition>(spellArea.conditionId))
        {
            sLog.outErrorDb("Spell %u listed in `spell_area` have wrong conditionId (%u) requirement", spell, spellArea.conditionId);
            continue;
        }
        else if (!spellArea.conditionId)
        {
            if (spellArea.questStart && !sObjectMgr.GetQuestTemplate(spellArea.questStart))
            {
                sLog.outErrorDb("Spell %u listed in `spell_area` have wrong start quest (%u) requirement", spell, spellArea.questStart);
                continue;
            }

            if (spellArea.questEnd)
            {
                if (!sObjectMgr.GetQuestTemplate(spellArea.questEnd))
                {
                    sLog.outErrorDb("Spell %u listed in `spell_area` have wrong end quest (%u) requirement", spell, spellArea.questEnd);
                    continue;
                }

                if (spellArea.questEnd == spellArea.questStart && !spellArea.questStartCanActive)
                {
                    sLog.outErrorDb("Spell %u listed in `spell_area` have quest (%u) requirement for start and end in same time", spell, spellArea.questEnd);
                    continue;
                }
            }

            if (spellArea.raceMask && (spellArea.raceMask & RACEMASK_ALL_PLAYABLE) == 0)
            {
                sLog.outErrorDb("Spell %u listed in `spell_area` have wrong race mask (%u) requirement", spell, spellArea.raceMask);
                continue;
            }

            if (spellArea.gender != GENDER_NONE && spellArea.gender != GENDER_FEMALE && spellArea.gender != GENDER_MALE)
            {
                sLog.outErrorDb("Spell %u listed in `spell_area` have wrong gender (%u) requirement", spell, spellArea.gender);
                continue;
            }
        }

        if (spellArea.auraSpell)
        {
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(abs(spellArea.auraSpell));
            if (!spellInfo)
            {
                sLog.outErrorDb("Spell %u listed in `spell_area` have wrong aura spell (%u) requirement", spell, abs(spellArea.auraSpell));
                continue;
            }

            switch (spellInfo->EffectAura[EFFECT_INDEX_0])
            {
                case SPELL_AURA_DUMMY:
                case SPELL_AURA_PHASE:
                case SPELL_AURA_GHOST:
                    break;
                default:
                    sLog.outErrorDb("Spell %u listed in `spell_area` have aura spell requirement (%u) without dummy/phase/ghost aura in effect 0", spell, abs(spellArea.auraSpell));
                    continue;
            }

            if (uint32(abs(spellArea.auraSpell)) == spellArea.spellId)
            {
                sLog.outErrorDb("Spell %u listed in `spell_area` have aura spell (%u) requirement for itself", spell, abs(spellArea.auraSpell));
                continue;
            }

            // not allow autocast chains by auraSpell field (but allow use as alternative if not present)
            if (spellArea.autocast && spellArea.auraSpell > 0)
            {
                bool chain = false;
                SpellAreaForAuraMapBounds saBound = GetSpellAreaForAuraMapBounds(spellArea.spellId);
                for (SpellAreaForAuraMap::const_iterator itr = saBound.first; itr != saBound.second; ++itr)
                {
                    if (itr->second->autocast && itr->second->auraSpell > 0)
                    {
                        chain = true;
                        break;
                    }
                }

                if (chain)
                {
                    sLog.outErrorDb("Spell %u listed in `spell_area` have aura spell (%u) requirement that itself autocast from aura", spell, spellArea.auraSpell);
                    continue;
                }

                SpellAreaMapBounds saBound2 = GetSpellAreaMapBounds(spellArea.auraSpell);
                for (SpellAreaMap::const_iterator itr2 = saBound2.first; itr2 != saBound2.second; ++itr2)
                {
                    if (itr2->second.autocast && itr2->second.auraSpell > 0)
                    {
                        chain = true;
                        break;
                    }
                }

                if (chain)
                {
                    sLog.outErrorDb("Spell %u listed in `spell_area` have aura spell (%u) requirement that itself autocast from aura", spell, spellArea.auraSpell);
                    continue;
                }
            }
        }

        SpellArea const* sa = &mSpellAreaMap.insert(SpellAreaMap::value_type(spell, spellArea))->second;

        // for search by current zone/subzone at zone/subzone change
        if (spellArea.areaId)
        {
            mSpellAreaForAreaMap.insert(SpellAreaForAreaMap::value_type(spellArea.areaId, sa));
        }

        // for search at aura apply
        if (spellArea.auraSpell)
        {
            mSpellAreaForAuraMap.insert(SpellAreaForAuraMap::value_type(abs(spellArea.auraSpell), sa));
        }

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u spell area requirements", count);
    sLog.outString();
}

/**
 * @brief Checks whether a spell is allowed in a given location.
 *
 * @param spellInfo The spell entry to validate.
 * @param map_id The current map identifier.
 * @param zone_id The current zone identifier.
 * @param area_id The current area identifier.
 * @param player The player attempting the cast, if any.
 * @return The spell cast failure code, or SPELL_CAST_OK when allowed.
 */
SpellCastResult SpellMgr::GetSpellAllowedInLocationError(SpellEntry const* spellInfo, uint32 map_id, uint32 zone_id, uint32 area_id, Player const* player)
{
    // normal case
    int32 areaGroupId = spellInfo->RequiredAreasID;
    if (areaGroupId > 0)
    {
        bool found = false;
        AreaGroupEntry const* groupEntry = sAreaGroupStore.LookupEntry(areaGroupId);
        while (groupEntry)
        {
            for (uint32 i = 0; i < 6; ++i)
            {
                if (groupEntry->AreaId[i] == zone_id || groupEntry->AreaId[i] == area_id)
                {
                    found = true;
                }
            }
            if (found || !groupEntry->NextAreaID)
            {
                break;
            }
            // Try search in next group
            groupEntry = sAreaGroupStore.LookupEntry(groupEntry->NextAreaID);
        }

        if (!found)
        {
            return SPELL_FAILED_INCORRECT_AREA;
        }
    }

    // continent limitation (virtual continent), ignore for GM
    if (spellInfo->HasAttribute(SPELL_ATTR_EX4_CAST_ONLY_IN_OUTLAND) && !(player && player->isGameMaster()))
    {
        uint32 v_map = GetVirtualMapForMapAndZone(map_id, zone_id);
        MapEntry const* mapEntry = sMapStore.LookupEntry(v_map);
        if (!mapEntry || mapEntry->ExpansionID < 1 || !mapEntry->IsContinent())
        {
            return SPELL_FAILED_INCORRECT_AREA;
        }
    }

    // raid instance limitation
    if (spellInfo->HasAttribute(SPELL_ATTR_EX6_NOT_IN_RAID_INSTANCE))
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry || mapEntry->IsRaid())
        {
            return SPELL_FAILED_NOT_IN_RAID_INSTANCE;
        }
    }

    // DB base check (if non empty then must fit at least single for allow)
    SpellAreaMapBounds saBounds = GetSpellAreaMapBounds(spellInfo->ID);
    if (saBounds.first != saBounds.second)
    {
        for (SpellAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
        {
            if (itr->second.IsFitToRequirements(player, zone_id, area_id))
            {
                return SPELL_CAST_OK;
            }
        }
        return SPELL_FAILED_INCORRECT_AREA;
    }

    // bg spell checks

    // do not allow spells to be cast in arenas
    // - with SPELL_ATTR_EX4_NOT_USABLE_IN_ARENA flag
    // - with greater than 10 min CD
    if (spellInfo->HasAttribute(SPELL_ATTR_EX4_NOT_USABLE_IN_ARENA) ||
            (GetSpellRecoveryTime(spellInfo) > 10 * MINUTE * IN_MILLISECONDS && !spellInfo->HasAttribute(SPELL_ATTR_EX4_USABLE_IN_ARENA)))
        if (player && player->InArena())
        {
            return SPELL_FAILED_NOT_IN_ARENA;
        }

    // Spell casted only on battleground
    if (spellInfo->HasAttribute(SPELL_ATTR_EX3_BATTLEGROUND))
        if (!player || !player->InBattleGround())
        {
            return SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }

    switch (spellInfo->ID)
    {
            // a trinket in alterac valley allows to teleport to the boss
        case 22564:                                         // recall
        case 22563:                                         // recall
        {
            if (!player)
            {
                return SPELL_FAILED_REQUIRES_AREA;
            }
            BattleGround* bg = player->GetBattleGround();
            return map_id == 30 && bg
                   && bg->GetStatus() != STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
        case 23333:                                         // Warsong Flag
        case 23335:                                         // Silverwing Flag
            return map_id == 489 && player && player->InBattleGround() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        case 34976:                                         // Netherstorm Flag
            return map_id == 566 && player && player->InBattleGround() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        case 2584:                                          // Waiting to Resurrect
        case 42792:                                         // Recently Dropped Flag
        case 43681:                                         // Inactive
        {
            return player && player->InBattleGround() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }
        case 22011:                                         // Spirit Heal Channel
        case 22012:                                         // Spirit Heal
        case 24171:                                         // Resurrection Impact Visual
        case 44535:                                         // Spirit Heal (mana)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
            {
                return SPELL_FAILED_INCORRECT_AREA;
            }
            return mapEntry->IsBattleGround() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }
        case 44521:                                         // Preparation
        {
            if (!player)
            {
                return SPELL_FAILED_REQUIRES_AREA;
            }

            BattleGround* bg = player->GetBattleGround();
            return bg && bg->GetStatus() == STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }
        case 32724:                                         // Gold Team (Alliance)
        case 32725:                                         // Green Team (Alliance)
        case 35774:                                         // Gold Team (Horde)
        case 35775:                                         // Green Team (Horde)
        {
            return player && player->InArena() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_IN_ARENA;
        }
        case 32727:                                         // Arena Preparation
        {
            if (!player)
            {
                return SPELL_FAILED_REQUIRES_AREA;
            }
            if (!player->InArena())
            {
                return SPELL_FAILED_REQUIRES_AREA;
            }

            BattleGround* bg = player->GetBattleGround();
            return bg && bg->GetStatus() == STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_ONLY_IN_ARENA;
        }
        case 74410:                                         // Arena - Dampening
            return player && player->InArena() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_IN_ARENA;
        case 74411:                                         // Battleground - Dampening
        {
            if (!player)
            {
                return SPELL_FAILED_ONLY_BATTLEGROUNDS;
            }

            BattleGround* bg = player->GetBattleGround();
            return bg && !bg->isArena() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }
    }

    return SPELL_CAST_OK;
}

/**
 * @brief Builds the skill-line ability multimap from DBC data.
 */
void SpellMgr::LoadSkillLineAbilityMap()
{
    mSkillLineAbilityMap.clear();

    BarGoLink bar(sSkillLineAbilityStore.GetNumRows());
    uint32 count = 0;

    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
    {
        bar.step();
        SkillLineAbilityEntry const* SkillInfo = sSkillLineAbilityStore.LookupEntry(i);
        if (!SkillInfo)
        {
            continue;
        }

        mSkillLineAbilityMap.insert(SkillLineAbilityMap::value_type(SkillInfo->Spell, SkillInfo));
        ++count;
    }

    sLog.outString(">> Loaded %u SkillLineAbility MultiMap Data", count);
    sLog.outString();
}

/**
 * @brief Builds the skill race/class requirement multimap from DBC data.
 */
void SpellMgr::LoadSkillRaceClassInfoMap()
{
    mSkillRaceClassInfoMap.clear();

    BarGoLink bar(sSkillRaceClassInfoStore.GetNumRows());
    uint32 count = 0;

    for (uint32 i = 0; i < sSkillRaceClassInfoStore.GetNumRows(); ++i)
    {
        bar.step();
        SkillRaceClassInfoEntry const* skillRCInfo = sSkillRaceClassInfoStore.LookupEntry(i);
        if (!skillRCInfo)
        {
            continue;
        }

        // not all skills really listed in ability skills list
        if (!sSkillLineStore.LookupEntry(skillRCInfo->skillId))
        {
            continue;
        }

        mSkillRaceClassInfoMap.insert(SkillRaceClassInfoMap::value_type(skillRCInfo->skillId, skillRCInfo));

        ++count;
    }

    sLog.outString(">> Loaded %u SkillRaceClassInfo MultiMap Data", count);
    sLog.outString();
}
