/*
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
 * @file AchievementCriteriaRequirement.cpp
 * @brief Cohesion split of AchievementMgr.cpp -- AchievementCriteriaRequirement[Set] validation and criterion gating.
 */

#include "Common.h"
#include "AchievementMgr.h"
#include "DBCStores.h"
#include "Player.h"
#include "DBCEnums.h"
#include "GameEventMgr.h"
#include "ObjectMgr.h"
#include "World.h"
#include "SpellMgr.h"
#include "BattleGround/BattleGround.h"
#include "Map.h"
#include "InstanceData.h"

bool AchievementCriteriaRequirement::IsValid(AchievementCriteriaEntry const* criteria)
{
    switch (criteria->requiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:      // only hardcoded list
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            break;
        default:
            sLog.outErrorDb("Table `achievement_criteria_requirement` have not supported data for criteria %u (Not supported as of its criteria type: %u), ignore.", criteria->ID, criteria->requiredType);
            return false;
    }

    switch (requirementType)
    {
        case ACHIEVEMENT_CRITERIA_REQUIRE_NONE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_VALUE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED:
        case ACHIEVEMENT_CRITERIA_REQUIRE_BG_LOSS_TEAM_SCORE:
        case ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT:
        case ACHIEVEMENT_CRITERIA_REQUIRE_NTH_BIRTHDAY:
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_CREATURE:
            if (!creature.id || !ObjectMgr::GetCreatureTemplate(creature.id))
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent creature id in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, creature.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_CLASS_RACE:
            if (!classRace.class_id && !classRace.race_id)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_PLAYER_CLASS_RACE (%u) must have not 0 in one from value fields, ignore.",
                                criteria->ID, criteria->requiredType, requirementType);
                return false;
            }
            if (classRace.class_id && ((1 << (classRace.class_id - 1)) & CLASSMASK_ALL_PLAYABLE) == 0)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent class in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, classRace.class_id);
                return false;
            }
            if (classRace.race_id && ((1 << (classRace.race_id - 1)) & RACEMASK_ALL_PLAYABLE) == 0)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_CREATURE (%u) have nonexistent race in value2 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, classRace.race_id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_LESS_HEALTH:
            if (health.percent < 1 || health.percent > 100)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_PLAYER_LESS_HEALTH (%u) have wrong percent value in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, health.percent);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD:
            if (player_dead.own_team_flag > 1)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD (%u) have wrong boolean value1 (%u).",
                                criteria->ID, criteria->requiredType, requirementType, player_dead.own_team_flag);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA:
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA:
        {
            SpellEntry const* spellEntry = sSpellStore.LookupEntry(aura.spell_id);
            if (!spellEntry)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have wrong spell id in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, (requirementType == ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA ? "ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA" : "ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"), requirementType, aura.spell_id);
                return false;
            }
            if (aura.effect_idx >= MAX_EFFECT_INDEX)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have wrong spell effect index in value2 (%u), ignore.",
                                criteria->ID, criteria->requiredType, (requirementType == ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA ? "ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA" : "ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"), requirementType, aura.effect_idx);
                return false;
            }
            if (!spellEntry->EffectApplyAuraName[aura.effect_idx])
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement %s (%u) have non-aura spell effect (ID: %u Effect: %u), ignore.",
                                criteria->ID, criteria->requiredType, (requirementType == ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA ? "ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA" : "ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA"), requirementType, aura.spell_id, aura.effect_idx);
                return false;
            }
            return true;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA:
            if (!GetAreaEntryByAreaID(area.id))
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA (%u) have wrong area id in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, area.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL:
            if (level.minlevel > STRONG_MAX_LEVEL)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL (%u) have wrong minlevel in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, level.minlevel);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER:
            if (gender.gender > GENDER_NONE)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER (%u) have wrong gender in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, gender.gender);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY:
            if (difficulty.difficulty >= MAX_DIFFICULTY)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY (%u) have wrong difficulty in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, difficulty.difficulty);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT:
            if (map_players.maxcount <= 0)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT (%u) have wrong max players count in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, map_players.maxcount);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM:
            if (team.team != ALLIANCE && team.team != HORDE)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM (%u) have unknown team in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, team.team);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK:
            if (drunk.state >= MAX_DRUNKEN)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK (%u) have unknown drunken state in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, drunk.state);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY:
            if (!sHolidaysStore.LookupEntry(holiday.id))
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY (%u) have unknown holiday in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, holiday.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL:
            if (equipped_item.item_quality >= MAX_ITEM_QUALITY)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL (%u) have unknown quality state in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, equipped_item.item_quality);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE:
        {
            CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(known_title.title_id);
            if (!titleInfo)
            {
                sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) for requirement ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE (%u) have unknown title_id in value1 (%u), ignore.",
                                criteria->ID, criteria->requiredType, requirementType, known_title.title_id);
                return false;
            }
            return true;
        }
        default:
            sLog.outErrorDb("Table `achievement_criteria_requirement` (Entry: %u Type: %u) have data for not supported data type (%u), ignore.", criteria->ID, criteria->requiredType, requirementType);
            return false;
    }
}

bool AchievementCriteriaRequirement::Meets(uint32 criteria_id, Player const* source, Unit const* target, uint32 miscvalue1 /*= 0*/) const
{
    switch (requirementType)
    {
        case ACHIEVEMENT_CRITERIA_REQUIRE_NONE:
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_CREATURE:
            if (!target || target->GetTypeId() != TYPEID_UNIT)
            {
                return false;
            }
            return target->GetEntry() == creature.id;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_CLASS_RACE:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
            {
                return false;
            }
            if (classRace.class_id && classRace.class_id != ((Player*)target)->getClass())
            {
                return false;
            }
            if (classRace.race_id && classRace.race_id != ((Player*)target)->getRace())
            {
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_LESS_HEALTH:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
            {
                return false;
            }
            return target->GetHealth() * 100 <= health.percent * target->GetMaxHealth();
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_PLAYER_DEAD:
            if (!target || target->GetTypeId() != TYPEID_PLAYER || target->IsAlive() || ((Player*)target)->GetDeathTimer() == 0)
            {
                return false;
            }
            // flag set == must be same team, not set == different team
            return (((Player*)target)->GetTeam() == source->GetTeam()) == (player_dead.own_team_flag != 0);
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AURA:
            return source->HasAura(aura.spell_id, SpellEffectIndex(aura.effect_idx));
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_AREA:
        {
            uint32 zone_id, area_id;
            source->GetZoneAndAreaId(zone_id, area_id);
            return area.id == zone_id || area.id == area_id;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_AURA:
            return target && target->HasAura(aura.spell_id, SpellEffectIndex(aura.effect_idx));
        case ACHIEVEMENT_CRITERIA_REQUIRE_VALUE:
            return miscvalue1 >= value.minvalue;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_LEVEL:
            if (!target)
            {
                return false;
            }
            return target->getLevel() >= level.minlevel;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_GENDER:
            if (!target)
            {
                return false;
            }
            return target->getGender() == gender.gender;
        case ACHIEVEMENT_CRITERIA_REQUIRE_DISABLED:
            return false;                                   // always fail
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_DIFFICULTY:
            return source->GetMap()->GetSpawnMode() == difficulty.difficulty;
        case ACHIEVEMENT_CRITERIA_REQUIRE_MAP_PLAYER_COUNT:
            return source->GetMap()->GetPlayersCountExceptGMs() <= map_players.maxcount;
        case ACHIEVEMENT_CRITERIA_REQUIRE_T_TEAM:
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
            {
                return false;
            }
            return ((Player*)target)->GetTeam() == Team(team.team);
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_DRUNK:
            return (uint32)Player::GetDrunkenstateByValue(source->GetDrunkValue()) >= drunk.state;
        case ACHIEVEMENT_CRITERIA_REQUIRE_HOLIDAY:
            return sGameEventMgr.IsActiveHoliday(HolidayIds(holiday.id));
        case ACHIEVEMENT_CRITERIA_REQUIRE_BG_LOSS_TEAM_SCORE:
        {
            BattleGround* bg = source->GetBattleGround();
            if (!bg)
            {
                return false;
            }
            return bg->IsTeamScoreInRange(source->GetTeam() == ALLIANCE ? HORDE : ALLIANCE, bg_loss_team_score.min_score, bg_loss_team_score.max_score);
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT:
        {
            if (!source->IsInWorld())
            {
                return false;
            }
            InstanceData* data = source->GetInstanceData();
            if (!data)
            {
                sLog.outErrorDb("Achievement system call ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT (%u) for achievement criteria %u for map %u but map not have instance script",
                                ACHIEVEMENT_CRITERIA_REQUIRE_INSTANCE_SCRIPT, criteria_id, source->GetMapId());
                return false;
            }
            return data->CheckAchievementCriteriaMeet(criteria_id, source, target, miscvalue1);
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM_LVL:
        {
            Item* item = source->GetItemByPos(INVENTORY_SLOT_BAG_0, miscvalue1);
            if (!item)
            {
                return false;
            }
            ItemPrototype const* proto = item->GetProto();
            return proto->ItemLevel >= equipped_item.item_level && proto->Quality >= equipped_item.item_quality;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_NTH_BIRTHDAY:
        {
            time_t birthday_start = time_t(sWorld.getConfig(CONFIG_UINT32_BIRTHDAY_TIME));

            tm birthday_tm = *localtime(&birthday_start);

            // exactly N birthday
            birthday_tm.tm_year += birthday_login.nth_birthday;

            time_t birthday = mktime(&birthday_tm);

            time_t now = sWorld.GetGameTime();
            return now <= birthday + DAY && now >= birthday;
        }
        case ACHIEVEMENT_CRITERIA_REQUIRE_KNOWN_TITLE:
        {
            if (CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(known_title.title_id))
            {
                return source && source->HasTitle(titleInfo->bit_index);
            }

            return false;
        }
    }
    return false;
}

bool AchievementCriteriaRequirementSet::Meets(Player const* source, Unit const* target, uint32 miscvalue /*= 0*/) const
{
    for (Storage::const_iterator itr = storage.begin(); itr != storage.end(); ++itr)
        if (!itr->Meets(criteria_id, source, target, miscvalue))
        {
            return false;
        }

    return true;
}
