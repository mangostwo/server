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
    
    m_tankWaitTime.clear();
    m_healerWaitTime.clear();
    m_dpsWaitTime.clear();
}

void LFGMgr::Update()
{
    // go through a waitTimeMap::iterator for each wait map and update times based on player count
    for (waitTimeMap::iterator tankItr = m_tankWaitTime.begin(); tankItr != m_tankWaitTime.end(); ++tankItr)
    {            
        LFGWait waitInfo = tankItr->second;
        if (waitInfo.doAverage)
        {
            int32 lastTime = waitInfo.previousTime;
            int32 thisTime = waitInfo.time;
            
            // average of the two join times
            waitInfo.time = (thisTime + lastTime) / 2;
            
            // now set what was just the current wait time to the previous time for a later calculation
            waitInfo.previousTime = thisTime;
            waitInfo.doAverage = false;
            
            tankItr->second = waitInfo;
        }
    }
    for (waitTimeMap::iterator healItr = m_healerWaitTime.begin(); healItr != m_healerWaitTime.end(); ++healItr)
    {
        LFGWait waitInfo = healItr->second;
        if (waitInfo.doAverage)
        {
            int32 lastTime = waitInfo.previousTime;
            int32 thisTime = waitInfo.time;
            
            // average of the two join times
            waitInfo.time = (thisTime + lastTime) / 2;
            
            // now set what was just the current wait time to the previous time for a later calculation
            waitInfo.previousTime = thisTime;
            waitInfo.doAverage = false;
            
            healItr->second = waitInfo;
        }
    }
    for (waitTimeMap::iterator dpsItr = m_dpsWaitTime.begin(); dpsItr != m_dpsWaitTime.end(); ++dpsItr)
    {
        LFGWait waitInfo = dpsItr->second;
        if (waitInfo.doAverage)
        {
            int32 lastTime = waitInfo.previousTime;
            int32 thisTime = waitInfo.time;
            
            // average of the two join times
            waitInfo.time = (thisTime + lastTime) / 2;
            
            // now set what was just the current wait time to the previous time for a later calculation
            waitInfo.previousTime = thisTime;
            waitInfo.doAverage = false;
            
            dpsItr->second = waitInfo;
        }
    }
    
    // todo: overall average time (similar to above)
}

void LFGMgr::JoinLFG(uint32 roles, std::set<uint32> dungeons, std::string comments, Player* plr)
{
    // Todo: - add queue / role check elements when systems are complete
    //       - see if any of this code/information can be put into a generalized class for other use
    //       - look into splitting this into 2 fns- one for player case, one for group
    Group* pGroup = plr->GetGroup();
    uint64 rawGuid = (pGroup) ? pGroup->GetObjectGuid().GetRawValue() : plr->GetObjectGuid().GetRawValue();
    uint32 randomDungeonID; // used later if random dungeon has been chosen
    
    LFGPlayers* currentInfo = GetPlayerOrPartyData(rawGuid);
    
    // check if we actually have info on the player/group right now
    if (currentInfo)
    {
        // are they already queued?
        if (currentInfo->currentState == LFG_STATE_QUEUED)
        {
            // remove from that queue, place in this one
            // todo: implement queue system
        }
        
        // are they already in a dungeon?
        if (pGroup && pGroup->isLFGGroup() && currentInfo->currentState != LFG_STATE_FINISHED_DUNGEON)
        {
            std::set<uint32> currentDungeon = currentInfo->dungeonList;
            
            dungeons.clear();
            dungeons.insert(*currentDungeon.begin()); // they should only have 1 dungeon in the map
        }
    }
    
    // used for upcoming checks
    bool isRandom  = false;
    bool isRaid    = false;
    bool isDungeon = false;
    
    LfgJoinResult result = GetJoinResult(plr);
    if (result == ERR_LFG_OK)
    {
        // additional checks on dungeon selection
        for (std::set<uint32>::iterator it = dungeons.begin(); it != dungeons.end(); ++it)
        {
            LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*it);
            switch (dungeon->typeID)
            {
                case LFG_TYPE_RANDOM_DUNGEON:
                    if (dungeons.size() > 1)
                        result = ERR_LFG_INVALID_SLOT;
                    else
                        isRandom = true;
                case LFG_TYPE_DUNGEON:
                case LFG_TYPE_HEROIC_DUNGEON:
                    if (isRaid)
                        result = ERR_LFG_MISMATCHED_SLOTS;
                    isDungeon = true;
                    break;
                case LFG_TYPE_RAID:
                    if (isDungeon)
                        result = ERR_LFG_MISMATCHED_SLOTS;
                    isRaid = true;
                    break;
                default: // one of the other types 
                    result = ERR_LFG_INVALID_SLOT;
                    break;
            }
        }
    }
    
    // since our join result may have just changed, check it again
    if (result == ERR_LFG_OK)
    {
        if (isRandom)
        {
            // store the current dungeon id (replaced into the dungeon set later)
            randomDungeonID = *dungeons.begin();
            // fetch all dungeons with our groupID and add to set
            LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*dungeons.begin());
            
            if (dungeon)
            {
                uint32 group = dungeon->groupID;
                
                for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
                {
                    LfgDungeonsEntry const* dungeonList = sLfgDungeonsStore.LookupEntry(id);
                    if (dungeonList)
                    {
                        if (dungeonList->groupID == group)
                            dungeons.insert(dungeonList->ID); // adding to set
                    }
                }
            }
            else
                result = ERR_LFG_NO_LFG_OBJECT;
        }
    }
    
    partyForbidden partyLockedDungeons;
    if (result == ERR_LFG_OK)
    {
        // do FindRandomDungeonsNotForPlayer for the plr or whole group
        if (pGroup)
        {
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                if (Player* pGroupPlr = itr->getSource())
                {
                    uint64 plrGuid = pGroupPlr->GetObjectGuid().GetRawValue();
                     
                    dungeonForbidden lockedDungeons = FindRandomDungeonsNotForPlayer(pGroupPlr);
                    partyLockedDungeons[plrGuid] = lockedDungeons;
                    
                    for (dungeonForbidden::iterator it = lockedDungeons.begin(); it != lockedDungeons.end(); ++it)
                    {
                        uint32 dungeonID = (it->first & 0x00FFFFFF);
                        
                        std::set<uint32>::iterator setItr = dungeons.find(dungeonID);
                        if (setItr != dungeons.end())
                            dungeons.erase(*setItr);
                    }
                }
            }
        }
        else
        {             
            dungeonForbidden lockedDungeons = FindRandomDungeonsNotForPlayer(plr);
            partyLockedDungeons[rawGuid] = lockedDungeons;
            
            for (dungeonForbidden::iterator it = lockedDungeons.begin(); it != lockedDungeons.end(); ++it)
            {
                uint32 dungeonID = (it->first & 0x00FFFFFF);
                        
                std::set<uint32>::iterator setItr = dungeons.find(dungeonID);
                if (setItr != dungeons.end())
                    dungeons.erase(*setItr);
            }
        }
        
        if (!dungeons.empty())
            partyLockedDungeons.clear();
        else
            result = (pGroup) ? ERR_LFG_NO_SLOTS_PARTY : ERR_LFG_NO_SLOTS_PLAYER;
    }
        
    // If our result is not ERR_LFG_OK, send join result now with err message
    if (result != ERR_LFG_OK)
    {
        plr->GetSession()->SendLfgJoinResult(result, LFG_STATE_NONE, partyLockedDungeons);
        return;
    }
    
    // place original dungeon ID back in the set
    if (isRandom)
    {
        dungeons.clear();
        dungeons.insert(randomDungeonID);
    }
    
    if (pGroup)
    {
        // role check, then queue
        //   * Get each player's roles and add to the roleMap
        return; // temporary
    }
    else
    {
        if (currentInfo)
        {
            currentInfo->currentState = LFG_STATE_QUEUED;
            currentInfo->dungeonList  = dungeons;
            currentInfo->comments     = comments;
            currentInfo->isGroup      = false;
            
            currentInfo->currentRoles.clear();
            currentInfo->currentRoles[rawGuid] = (uint8)roles;
            
            currentInfo->joinedTime = time(nullptr);
            m_playerData[rawGuid] = *currentInfo;
        }
        else
        {
            // set up a role map and then an lfgplayer struct
            roleMap playerRole;
            playerRole[rawGuid] = (uint8)roles;
            
            LFGPlayers playerInfo(LFG_STATE_QUEUED, dungeons, playerRole, comments, false, time(nullptr), 0, 0, 0);
            m_playerData[rawGuid] = playerInfo;
        }
        
        // Send information back to the client
        //plr->GetSession()->SendLfgJoinResult(result, LFG_STATE_QUEUED, partyLockedDungeons);
        // fix up SendLfgUpdate in lfghander and send after join result.
        // then call AddToQueue(rawGuid);
    }
}

void LFGMgr::LeaveLFG()
{
    
}

LFGPlayers* LFGMgr::GetPlayerOrPartyData(uint64 rawGuid)
{
    playerData::iterator it = m_playerData.find(rawGuid);
    if (it != m_playerData.end())
        return &(it->second);
    else
        return NULL;
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

    if (plr->HasAura(LFG_DESERTER_SPELL))
        result = ERR_LFG_DESERTER_PLAYER;
    else if (plr->InBattleGround() || plr->InBattleGroundQueue() || plr->InArena())
        result = ERR_LFG_CANT_USE_DUNGEONS;
    else if (plr->HasAura(LFG_COOLDOWN_SPELL))
        result = ERR_LFG_RANDOM_COOLDOWN_PLAYER;
    
    if (pGroup)
    {        
        if (pGroup->GetMembersCount() > 5)
            result = ERR_LFG_TOO_MANY_MEMBERS;
        else
        {
            uint8 currentMemberCount = 0;
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                if (Player* pGroupPlr = itr->getSource())
                {
                    // check if the group members are level 15+ to use finder
                    if (pGroupPlr->getLevel() < 15)
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

void LFGMgr::AddToQueue(uint64 rawGuid)
{
    LFGPlayers* information = GetPlayerOrPartyData(rawGuid);
    if (!information)
        return;
    
    // put info into wait time maps for starters
    for (roleMap::iterator it = information->currentRoles.begin(); it != information->currentRoles.end(); ++it)
        AddToWaitMap(it->second, information->dungeonList);
        
    // note: this function still needs to be completed
}

void LFGMgr::AddToWaitMap(uint8 role, std::set<uint32> dungeons)
{
    // use withoutLeader for switch operator
    uint8 withoutLeader = role;
    withoutLeader &= ~PLAYER_ROLE_LEADER;
    
    switch (withoutLeader)
    {
        case PLAYER_ROLE_TANK:
        {
            for (std::set<uint32>::iterator itr = dungeons.begin(); itr != dungeons.end(); ++itr)
            {
                waitTimeMap::iterator it = m_tankWaitTime.find(*itr);
                if (it != m_tankWaitTime.end())
                {
                    // Increment current player count by one
                    ++it->second.playerCount;
                }
                else
                {
                    LFGWait waitInfo(QUEUE_DEFAULT_TIME, -1, 1, false);
                    m_tankWaitTime[*itr] = waitInfo;
                }
            }
        } break;
        case PLAYER_ROLE_HEALER:
        {
            for (std::set<uint32>::iterator itr = dungeons.begin(); itr != dungeons.end(); ++itr)
            {
                waitTimeMap::iterator it = m_healerWaitTime.find(*itr);
                if (it != m_healerWaitTime.end())
                {
                    // Increment current player count by one
                    ++it->second.playerCount;
                }
                else
                {
                    LFGWait waitInfo(QUEUE_DEFAULT_TIME, -1, 1, false);
                    m_healerWaitTime[*itr] = waitInfo;
                }
            }
        } break;
        case PLAYER_ROLE_DAMAGE:
        {
            for (std::set<uint32>::iterator itr = dungeons.begin(); itr != dungeons.end(); ++itr)
            {
                waitTimeMap::iterator it = m_dpsWaitTime.find(*itr);
                if (it != m_dpsWaitTime.end())
                {
                    // Increment current player count by one
                    ++it->second.playerCount;
                }
                else
                {
                    LFGWait waitInfo(QUEUE_DEFAULT_TIME, -1, 1, false);
                    m_dpsWaitTime[*itr] = waitInfo;
                }
            }
        } break;
        default:
            break;
    }
}
