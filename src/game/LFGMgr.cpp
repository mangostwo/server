/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2014  MaNGOS project <http://getmangos.eu>
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

#include "DBCEnums.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "LFGMgr.h"
#include "Object.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"

INSTANTIATE_SINGLETON_1(LFGMgr);

LFGMgr::LFGMgr() { }

LFGMgr::~LFGMgr()
{
    m_dailyAny.clear();
    m_dailyTBCHeroic.clear();
    m_dailyLKNormal.clear();
    m_dailyLKHeroic.clear();
    m_playerData.clear();
}

void LFGMgr::JoinLFG(uint32 roles, std::set<uint32> dungeons, std::string comments, Player* plr)
{
    /* -Implementation-
     * Rules:
     *   I. If already queued, remove from that one / place in this one
     *  II. Cannot proceed if already in a dungeon/lfg group (1/2 done: lfggroup check added to group class)
     * III. GetJoinResult must return ERR_LFG_OK to actually join
     *  IV. Make sure logic follows LFG rules.
     */
    //Group* pGroup = plr->GetGroup();
}

void LFGMgr::LeaveLFG()
{
    
}

LfgJoinResult LFGMgr::GetJoinResult(Player* plr)
{
    LfgJoinResult result;
    Group* pGroup = plr->GetGroup();
    
    /* Reasons for not entering:
     *   Deserter spell
     *   Dungeon finder cooldown 
     *   In a battleground
     *   In an arena
     *   Queued for battleground
     *   Too many members in group
     *   Group member disconnected
     *   Group member too low/high level
     *   Any group member cannot enter for x reason any other player can't
     */
    
    if (!plr)
        result = ERR_LFG_MEMBERS_NOT_PRESENT;
    else if (plr->HasAura(LFG_DESERTER_SPELL))
        result = ERR_LFG_DESERTER_PLAYER;
    else if (plr->InBattleGround() || plr->InBattleGroundQueue() || plr->InArena())
        result = ERR_LFG_CANT_USE_DUNGEONS;
    else if (plr->HasAura(LFG_COOLDOWN_SPELL))
        result = ERR_LFG_RANDOM_COOLDOWN_PLAYER;
    else if (pGroup)
    {
        uint32 plrLevel = plr->getLevel();
        
        if (pGroup->GetMembersCount() > 5)
            result = ERR_LFG_TOO_MANY_MEMBERS;
        else
        {
            uint8 currentMemberCount = 0;
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                if (Player* pGroupPlr = itr->GetSource())
                {
                    // check if the group members are level 15+ to use finder
                    if (pGroupPlr->getLevel() < 15)
                        result = ERR_LFG_CANT_USE_DUNGEONS;
                    else if (pGroupPlr->getLevel() > (plrLevel + LFG_LEVEL_RANGE) || pGroupPlr->getLevel() < (plrLevel - LFG_LEVEL_RANGE))
                        result = ERR_LFG_CANT_USE_DUNGEONS;
                    else if (pGroupPlr->HasAura(LFG_DESERTER_SPELL))
                        result = ERR_LFG_DESERTER_PARTY;
                    else if (pGroupPlr->InBattleGround() || pGroupPlr->InBattleGroundQueue() || pGroupPlr->InArena())
                        result = ERR_LFG_CANT_USE_DUNGEONS;
                    else if (pGroupPlr->HasAura(LFG_COOLDOWN_SPELL))
                        result = ERR_LFG_RANDOM_COOLDOWN_PARTY;
                    else
                        result = ERR_LFG_OK;
                    
                    ++currentMemberCount;
                }
            }
            
            if (result == ERR_LFG_OK && currentMemberCount != pGroup->GetMembersCount())
                result = ERR_LFG_MEMBERS_NOT_PRESENT;
        }
    }
    else
        result = ERR_LFG_OK;
            
    return result;
}

ItemRewards LFGMgr::GetDungeonItemRewards(uint32 dungeonId, DungeonTypes type)
{
    ItemRewards rewards;
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonId);
    if (dungeon)
    {
        uint32 minLevel = dungeon->minLevel;
        uint32 maxLevel = dungeon->maxLevel;
        uint32 avgLevel = (minLevel+maxLevel)/2; // otherwise there are issues
        
        DungeonFinderItemsMap const& itemBuffer = sObjectMgr.GetDungeonFinderItemsMap();
        for (DungeonFinderItemsMap::const_iterator it = itemBuffer.begin(); it != itemBuffer.end(); ++it)
        {
            DungeonFinderItems itemCache = it->second;
            if (itemCache.dungeonType == type)
            {
                // should only be one of this inequality in the map
                if ((avgLevel >= itemCache.minLevel) && (avgLevel <= itemCache.maxLevel))
                {
                    rewards.itemId = itemCache.itemReward;
                    rewards.itemAmount = itemCache.itemAmount;
                    return rewards;
                }
            }
        }
    }
    return rewards;
}

DungeonTypes LFGMgr::GetDungeonType(uint32 dungeonId)
{
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonId);
    if (dungeon)
    {
        switch (dungeon->expansionLevel)
        {
            case 0:
                return DUNGEON_CLASSIC;
            case 1:
            {
                if (dungeon->difficulty == DUNGEON_DIFFICULTY_NORMAL)
                    return DUNGEON_TBC;
                else if (dungeon->difficulty == DUNGEON_DIFFICULTY_HEROIC)
                    return DUNGEON_TBC_HEROIC;
            }
            case 2:
            {
                if (dungeon->difficulty == DUNGEON_DIFFICULTY_NORMAL)
                    return DUNGEON_WOTLK;
                else if (dungeon->difficulty == DUNGEON_DIFFICULTY_HEROIC)
                    return DUNGEON_WOTLK_HEROIC;
            }
            default:
                return DUNGEON_UNKNOWN;
        }
    }
    return DUNGEON_UNKNOWN;
}

void LFGMgr::RegisterPlayerDaily(uint32 guidLow, DungeonTypes dungeon)
{
    switch (dungeon)
    {
        case DUNGEON_CLASSIC:
        case DUNGEON_TBC:
            m_dailyAny.insert(guidLow);
            break;
        case DUNGEON_TBC_HEROIC:
            m_dailyTBCHeroic.insert(guidLow);
            break;
        case DUNGEON_WOTLK:
            m_dailyLKNormal.insert(guidLow);
            break;
        case DUNGEON_WOTLK_HEROIC:
            m_dailyLKHeroic.insert(guidLow);
            break;
        default:
            break;
    }
}

bool LFGMgr::HasPlayerDoneDaily(uint32 guidLow, DungeonTypes dungeon)
{
    switch (dungeon)
    {
        case DUNGEON_CLASSIC:
        case DUNGEON_TBC:
            return (m_dailyAny.find(guidLow) != m_dailyAny.end()) ? true : false;
        case DUNGEON_TBC_HEROIC:
            return (m_dailyTBCHeroic.find(guidLow) != m_dailyTBCHeroic.end()) ? true : false;
        case DUNGEON_WOTLK:
            return (m_dailyLKNormal.find(guidLow) != m_dailyLKNormal.end()) ? true : false;
        case DUNGEON_WOTLK_HEROIC:
            return (m_dailyLKHeroic.find(guidLow) != m_dailyLKHeroic.end()) ? true : false;
        default:
            return false;
    }
    return false;
}

void LFGMgr::ResetDailyRecords()
{
    m_dailyAny.clear();
    m_dailyTBCHeroic.clear();
    m_dailyLKNormal.clear();
    m_dailyLKHeroic.clear();
}

bool LFGMgr::IsSeasonActive(uint32 dungeonId)
{
    switch (dungeonId)
    {
        case 285:
            return IsHolidayActive(HOLIDAY_HALLOWS_END);
        case 286:
            return IsHolidayActive(HOLIDAY_FIRE_FESTIVAL);
        case 287:
            return IsHolidayActive(HOLIDAY_BREWFEST);
        case 288:
            return IsHolidayActive(HOLIDAY_LOVE_IS_IN_THE_AIR);
        default:
            return false;
    }
    return false;
}

dungeonEntries LFGMgr::FindRandomDungeonsForPlayer(uint32 level, uint8 expansion)
{
    dungeonEntries randomDungeons;
    
    // go through the dungeon dbc and select the applicable dungeons
    for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(id);
        if (dungeon)
        {
            if ( (dungeon->typeID == LFG_TYPE_RANDOM_DUNGEON)
                || (IsSeasonal(dungeon->flags) && IsSeasonActive(dungeon->ID)) )
                if ((uint8)dungeon->expansionLevel <= expansion && dungeon->minLevel <= level
                    && dungeon->maxLevel >= level)
                    randomDungeons[dungeon->ID] = dungeon->Entry();
        }
    }
    return randomDungeons;
}

dungeonForbidden LFGMgr::FindRandomDungeonsNotForPlayer(Player* plr)
{
    uint32 level = plr->getLevel();
    uint8 expansion = plr->GetSession()->Expansion();
    
    dungeonForbidden randomDungeons;

    for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(id);
        if (dungeon)
        {
            uint32 forbiddenReason = 0;
            
            if ((uint8)dungeon->expansionLevel > expansion)
                forbiddenReason = (uint32)LFG_FORBIDDEN_EXPANSION;
            else if (dungeon->typeID == LFG_TYPE_RAID)
                forbiddenReason = (uint32)LFG_FORBIDDEN_RAID;
            else if (dungeon->minLevel > level)
                forbiddenReason = (uint32)LFG_FORBIDDEN_LOW_LEVEL;
            else if (dungeon->maxLevel < level)
                forbiddenReason = (uint32)LFG_FORBIDDEN_HIGH_LEVEL;
            else if (IsSeasonal(dungeon->flags) && !IsSeasonActive(dungeon->ID)) // check pointers/function args
                forbiddenReason = (uint32)LFG_FORBIDDEN_NOT_IN_SEASON;
            else if (DungeonFinderRequirements const* req = sObjectMgr.GetDungeonFinderRequirements((uint32)dungeon->mapID, dungeon->difficulty))
            {
                if (req->minItemLevel && (plr->GetEquipGearScore(false,false) < req->minItemLevel))
                    forbiddenReason = (uint32)LFG_FORBIDDEN_LOW_GEAR_SCORE;
                else if (req->achievement && !plr->GetAchievementMgr().HasAchievement(req->achievement))
                    forbiddenReason = (uint32)LFG_FORBIDDEN_MISSING_ACHIEVEMENT;
                else if (plr->GetTeam() == ALLIANCE && req->allianceQuestId && !plr->GetQuestRewardStatus(req->allianceQuestId))
                    forbiddenReason = (uint32)LFG_FORBIDDEN_QUEST_INCOMPLETE;
                else if (plr->GetTeam() == HORDE && req->hordeQuestId && !plr->GetQuestRewardStatus(req->hordeQuestId))
                    forbiddenReason = (uint32)LFG_FORBIDDEN_QUEST_INCOMPLETE;
                else
                    if (req->item)
                    {
                        if (!plr->HasItemCount(req->item, 1) && (!req->item2 || !plr->HasItemCount(req->item2, 1)))
                            forbiddenReason = LFG_FORBIDDEN_MISSING_ITEM;
                    }
                    else if (req->item2 && !plr->HasItemCount(req->item2, 1))
                        forbiddenReason = LFG_FORBIDDEN_MISSING_ITEM;
            }
            
            if (forbiddenReason)
                randomDungeons[dungeon->Entry()] = forbiddenReason;
        }
    }
    return randomDungeons;
}
