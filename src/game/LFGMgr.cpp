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
#include "ObjectAccessor.h"
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
    m_queueSet.clear();
    
    m_playerStatusMap.clear();
    
    m_roleCheckMap.clear();
    
    m_tankWaitTime.clear();
    m_healerWaitTime.clear();
    m_dpsWaitTime.clear();
    m_avgWaitTime.clear();
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
    for (waitTimeMap::iterator avgItr = m_avgWaitTime.begin(); avgItr != m_avgWaitTime.end(); ++avgItr)
    {
        LFGWait waitInfo = avgItr->second;
        if (waitInfo.doAverage)
        {
            int32 lastTime = waitInfo.previousTime;
            int32 thisTime = waitInfo.time;
            
            // average of the two join times
            waitInfo.time = (thisTime + lastTime) / 2;
            
            // now set what was just the current wait time to the previous time for a later calculation
            waitInfo.previousTime = thisTime;
            waitInfo.doAverage = false;
            
            avgItr->second = waitInfo;
        }
    }
    
    // Queue System    
    FindQueueMatches();
    SendQueueStatus();
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

    bool groupCurrentlyInDungeon = pGroup && pGroup->isLFGGroup() && currentInfo->currentState != LFG_STATE_FINISHED_DUNGEON
    
    // check if we actually have info on the player/group right now
    if (currentInfo)
    {
        // are they already queued?
        if (currentInfo->currentState == LFG_STATE_QUEUED)
        {
            // remove from that queue so they can later join this one
            queueSet::iterator qItr = m_queueSet.find(rawGuid);
            if (qItr != m_queueSet.end())
                m_queueSet.erase(qItr);
            // note: do we need to send a packet telling them the current queue is over?
        }
        
        // are they already in a dungeon?
        if (groupCurrentlyInDungeon)
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
    
    if (pGroup)
    {
        // todo: set up a LFGPlayers struct for the group
        uint64 leaderRawGuid = plr->GetObjectGuid().GetRawValue();
        
        LFGRoleCheck roleCheck;
        roleCheck.state = LFG_ROLECHECK_INITIALITING;
        roleCheck.dungeonList = dungeons;
        roleCheck.randomDungeonID = randomDungeonID;
        roleCheck.leaderGuidRaw = leaderRawGuid;
        roleCheck.waitForRoleTime = time_t(time(nullptr) + LFG_TIME_ROLECHECK);

        m_roleCheckMap[rawGuid] = roleCheck;
        
        // place original dungeon ID back in the set
        if (isRandom)
        {
            dungeons.clear();
            dungeons.insert(randomDungeonID);
        }
        
        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            if (Player* pGroupPlr = itr->getSource())
            {
                LFGPlayerStatus overallStatus(LFG_STATE_NONE, LFG_UPDATE_JOIN, dungeons, comments);
                
                pGroupPlr->GetSession()->SendLfgUpdate(true, overallStatus);
                overallStatus.state = LFG_STATE_ROLECHECK;
                
                uint64 plrGuid = pGroupPlr->GetObjectGuid().GetRawValue();
                roleCheck.currentRoles[plrGuid] = 0;
                
                m_playerStatusMap[plrGuid] = overallStatus;
            }
        }
        // used later if they enter the queue
        LFGPlayers groupInfo(LFG_STATE_NONE, dungeons, roleCheck.currentRoles, comments, false, time(nullptr), 0, 0, 0);
        m_playerData[rawGuid] = groupInfo;
        
        PerformRoleCheck(plr, pGroup, (uint8)roles);
    }
    else
    {
        // place original dungeon ID back in the set
        if (isRandom)
        {
            dungeons.clear();
            dungeons.insert(randomDungeonID);
        }
        
        // set up a role map and then an lfgplayer struct
        roleMap playerRole;
        playerRole[rawGuid] = (uint8)roles;
            
        LFGPlayers playerInfo(LFG_STATE_QUEUED, dungeons, playerRole, comments, false, time(nullptr), 0, 0, 0);
        m_playerData[rawGuid] = playerInfo;
        
        // set up a status struct for client requests/updates
        LFGPlayerStatus plrStatus;
        plrStatus.updateType  = LFG_UPDATE_JOIN;
        plrStatus.state = LFG_STATE_NONE;
        plrStatus.dungeonList = dungeons;
        plrStatus.comment = comments;
        
        // Send information back to the client
        plr->GetSession()->SendLfgJoinResult(result, LFG_STATE_NONE, partyLockedDungeons);
        plr->GetSession()->SendLfgUpdate(false, plrStatus);
        
        plrStatus.state = LFG_STATE_QUEUED;
        m_playerStatusMap[rawGuid] = plrStatus;
        AddToQueue(rawGuid);
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

LFGPlayerStatus LFGMgr::GetPlayerStatus(uint64 rawGuid)
{
    LFGPlayerStatus status;
    
    LFGPlayerStatus::iterator it = m_playerStatusMap.find(rawGuid);
    if (it != m_playerStatusMap.end())
        status = it->second;
    
    return status;
}

void LFGMgr::SetPlayerComment(uint64 rawGuid, std::string comment)
{
    LFGPlayerStatus status = GetPlayerStatus(rawGuid);
    status.comment = comment;
    
    m_playerStatusMap[rawGuid] = status;
}

void LFGMgr::SetPlayerState(uint64 rawGuid, LFGState state)
{
    LFGPlayerStatus status = GetPlayerStatus(rawGuid);
    status.state = state;
    
    m_playerStatusMap[rawGuid] = status;
}

void LFGMgr::SetPlayerUpdateType(uint64 rawGuid, LfgUpdateType updateType)
{
    LFGPlayerStatus status = GetPlayerStatus(rawGuid);
    status.updateType = updateType;
    
    m_playerStatusMap[rawGuid] = status;
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

void LFGMgr::UpdateNeededRoles(uint64 rawGuid, LFGPlayers* information)
{
    uint8 tankCount = 0, dpsCount = 0, healCount = 0;
    for (roleMap::iterator it = information->currentRoles.begin(); it != information->currentRoles.end(); ++it)
    {
        uint8 withoutLeader = it->second;
        withoutLeader &= ~PLAYER_ROLE_LEADER;
        
        switch (withoutLeader)
        {
            case PLAYER_ROLE_TANK:
                ++tankCount;
                break;
            case PLAYER_ROLE_HEALER:
                ++healCount;
                break;
            case PLAYER_ROLE_DAMAGE:
                ++dpsCount;
                break;
        }
    }

    std::set<uint32>::iterator itr = information->dungeonList.begin();
    
    // check dungeon type for max of each role [normal heroic etc.]
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*itr);
    if (dungeon)
    {
        // atm we're just handling DUNGEON_DIFFICULTY_NORMAL
        if (dungeon->difficulty == DUNGEON_DIFFICULTY_NORMAL)
        {
            information->neededTanks = NORMAL_TANK_OR_HEALER_COUNT - tankCount;
            information->neededHealers = NORMAL_TANK_OR_HEALER_COUNT - healCount;
            information->neededDps = NORMAL_DAMAGE_COUNT - dpsCount;
        }
    }
    
    m_playerData[rawGuid] = *information;
}

void LFGMgr::AddToQueue(uint64 rawGuid)
{
    LFGPlayers* information = GetPlayerOrPartyData(rawGuid);
    if (!information)
        return;
    
    // This will be necessary for finding matches in the queue
    UpdateNeededRoles(rawGuid, information);
    
    // put info into wait time maps for starters
    for (roleMap::iterator it = information->currentRoles.begin(); it != information->currentRoles.end(); ++it)
        AddToWaitMap(it->second, information->dungeonList);
        
    // just in case someone's already been in the queue.
    queueSet::iterator qItr = m_queueSet.find(rawGuid);
    if (qItr == m_queueSet.end())
        m_queueSet.insert(rawGuid);
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
    
    // insert the average time regardless of role
    for (std::set<uint32>::iterator itr = dungeons.begin(); itr != dungeons.end(); ++itr)
    {
        waitTimeMap::iterator it = m_avgWaitTime.find(*itr);
        if (it != m_avgWaitTime.end())
            ++it->second.playerCount;
        else
        {
            LFGWait waitInfo(QUEUE_DEFAULT_TIME, -1, 1, false);
            m_avgWaitTime[*itr] = waitInfo;
        }
    }
}

void LFGMgr::FindQueueMatches()
{
    // Fetch information on all the queued players/groups
    for (queueSet::iterator itr = m_queueSet.begin(); itr != m_queueSet.end(); ++itr)
        FindSpecificQueueMatches(*itr);
}

void LFGMgr::FindSpecificQueueMatches(uint64 rawGuid)
{
    LFGPlayers* queueInfo = GetPlayerOrPartyData(rawGuid);
    if (queueInfo)
    {
        // compare to everyone else in queue for compatibility
        // after a match is found call UpdateNeededRoles
        // Use the roleMap to store player guid/role information; merge into queueInfo struct & delete other struct/map entry
        for (queueSet::iterator itr = m_queueSet.begin(); itr != m_queueSet.end(); ++itr)
        {
            LFGPlayers* matchInfo = GetPlayerOrPartyData(*itr);
            if (matchInfo)
            {
                // 1. iterate through queueInfo's dungeon set and search the matchInfo for a matching entry.
                // 2. if an(y) entry is found, great and proceed!
                // 2a. if an entry is found and the amounts of players-to-roles are compatible, make
                //     a new map of only the inter-compatible dungeons and use that if the other checks pass
                // 3. Regardless of outcome, after the end of calculations send a LFGQueueStatus packet
                bool fullyCompatible = false;
                std::set<uint32> compatibleDungeons;
                
                for (std::set<uint32>::iterator dItr = matchInfo->dungeonList.begin(); dItr != matchInfo->dungeonList.end(); ++itr)
                {
                    if (queueInfo->dungeonList.find(*dItr) != queueInfo->dungeonList.end())
                        compatibleDungeons.insert(*dItr);
                }
                
                if (!compatibleDungeons.empty())
                {
                    // check for player / role count compatibility
                    // if function returns true, then merge groups into one
                    if (RoleMapsAreCompatible(queueInfo, matchInfo))
                        MergeGroups(rawGuid, *itr, compatibleDungeons);
                }
            }
        }
    }
}

bool LFGMgr::RoleMapsAreCompatible(LFGPlayers* groupOne, LFGPlayers* groupTwo)
{
    // When this is called we already know that the dungeons match, so just focus on roles
    // compare: neededX(role) from each struct and the amount of people per role in the roleMap
    if ((groupOne->currentRoles.size() + groupTwo->currentRoles.size()) > NORMAL_TOTAL_ROLE_COUNT)
        return false;
    else
    {
        // make sure we don't have too many players of a certain role here
        if (((NORMAL_DAMAGE_COUNT - groupOne->neededDps) + (NORMAL_DAMAGE_COUNT - groupTwo->neededDps)) > NORMAL_DAMAGE_COUNT)
            return false;
        else if (((NORMAL_TANK_OR_HEALER_COUNT - groupOne->neededHealers) + (NORMAL_TANK_OR_HEALER_COUNT - groupTwo->neededHealers)) > NORMAL_TANK_OR_HEALER_COUNT)
            return false;
        else if (((NORMAL_TANK_OR_HEALER_COUNT - groupOne->neededTanks) + (NORMAL_TANK_OR_HEALER_COUNT - groupTwo->neededTanks)) > NORMAL_TANK_OR_HEALER_COUNT)
            return false;
        else
            return true; // the player/role counts line up!
    }
    return false;
}

void LFGMgr::MergeGroups(uint64 rawGuidOne, uint64 rawGuidTwo, std::set<uint32> compatibleDungeons)
{    
    // merge into the entry for rawGuidOne, then see if they are
    // able to enter the dungeon at this point or not
    LFGPlayers* mainGroup   = GetPlayerOrPartyData(rawGuidOne);
    LFGPlayers* bufferGroup = GetPlayerOrPartyData(rawGuidTwo);
    
    if (!mainGroup || !bufferGroup)
        return;
        
    // update the dungeon selection with the compatible ones
    mainGroup->dungeonList.clear();
    mainGroup->dungeonList = compatibleDungeons;
        
    // move players / roles into a single roleMap
    for (roleMap::iterator it = bufferGroup->currentRoles.begin(); it != bufferGroup->currentRoles.end(); ++it)
        mainGroup->currentRoles[it->first] = it->second;
        
    // update the role count / needed role info
    UpdateNeededRoles(rawGuidOne, mainGroup);
    
    // being safe
    mainGroup = GetPlayerOrPartyData(rawGuidOne);
    
    // Then do the following:
    // if ((mainGroup->neededTanks == 0) && (mainGroup->neededHealers == 0) && (mainGroup->neededDps == 0))
    //     SendDungeonProposal(...);
    //
    m_playerData.erase(rawGuidTwo);
}

void LFGMgr::SendQueueStatus()
{
    // First we should get the current time
    time_t timeNow = time(0);
    
    // Check who is listed as being in the queue
    for (queueSet::iterator itr = m_queueSet.begin(); itr != m_queueSet.end(); ++itr)
    {
        // make sure it's not a false entry
        LFGPlayers* queueInfo = GetPlayerOrPartyData(*itr);
        if (queueInfo && queueInfo->currentState == LFG_STATE_QUEUED)
        {
            for (roleMap::iterator rItr = queueInfo->currentRoles.begin(); rItr != queueInfo->currentRoles.end(); ++rItr)
            {
                if (Player* pPlayer = ObjectAccessor::FindPlayer(ObjectGuid(rItr->first)))
                {
                    uint32 dungeonId = *queueInfo->dungeonList.begin();
                    
                    LFGQueueStatus status;
                    status.dungeonID        = dungeonId;
                    status.neededTanks      = queueInfo->neededTanks;
                    status.neededHeals      = queueInfo->neededHealers;
                    status.neededDps        = queueInfo->neededDps;
                    status.timeSpentInQueue = uint32(timeNow - queueInfo->joinedTime);
                    
                    int32 playerWaitTime;
                    
                    // strip leader flag from role
                    uint8 withoutLeader = rItr->second;
                    withoutLeader &= ~PLAYER_ROLE_LEADER;
                    
                    switch (withoutLeader)
                    {
                        case PLAYER_ROLE_TANK:
                            playerWaitTime = m_tankWaitTime[dungeonId].time;
                            break;
                        case PLAYER_ROLE_HEALER:
                            playerWaitTime = m_healerWaitTime[dungeonId].time;
                            break;
                        case PLAYER_ROLE_DAMAGE:
                            playerWaitTime = m_dpsWaitTime[dungeonId].time;
                            break;
                    }
                    
                    status.playerAvgWaitTime = playerWaitTime;
                    status.dpsAvgWaitTime    = m_dpsWaitTime[dungeonId].time;
                    status.healerAvgWaitTime = m_healerWaitTime[dungeonId].time;
                    status.tankAvgWaitTime   = m_tankWaitTime[dungeonId].time;
                    status.avgWaitTime       = m_avgWaitTime[dungeonId].time;
                    
                    // Send packet to client
                    pPlayer->GetSession()->SendLfgQueueStatus(status);
                }
            }
        }
    }
}

uint32 LFGMgr::GetDungeonEntry(uint32 ID)
{
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(ID);
    if (dungeon)
        return dungeon->Entry();
    else
        return 0;
}

// called each time a player selects their role
void LFGMgr::PerformRoleCheck(Player* pPlayer, Group* pGroup, uint8 roles)
{
    uint64 groupRawGuid = pGroup->GetObjectGuid().GetRawValue();
    uint64 plrRawGuid = pPlayer->GetObjectGuid().GetRawValue();
    
    roleCheckMap::iterator it = m_roleCheckMap.find(groupRawGuid);
    if (it == m_roleCheckMap.end())
        return; // no role check map found
    
    LFGRoleCheck roleCheck = it->second;
    bool roleChosen = roleCheck.state != LFG_ROLECHECK_DEFAULT && plrRawGuid;
    
    if (!plrRawGuid)
        roleCheck.state = LFG_ROLECHECK_ABORTED;  // aborted if anyone cancels during role check
    else if (roles < PLAYER_ROLE_TANK)            // kind of a sanity check- the client shouldn't allow this to happen
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
    else
    {
        roleCheck.currentRoles[plrRawGuid] = roles;
        
        roleMap::iterator rItr = roleCheck.currentRoles.begin();
        do
        {
            if (rItr->second != PLAYER_ROLE_NONE)
                ++rItr;
        } while (rItr != roleCheck.currentRoles.end());
        
        if (rItr == roleCheck.currentRoles.end()) // meaning that everyone confirmed their roles
            roleCheck.state = ValidateGroupRoles(roleCheck.currentRoles) ? LFG_ROLECHECK_FINISHED : LFG_ROLECHECK_MISSING_ROLE;
    }
    
    std::set<uint32> dungeonBuff;
    if (roleCheck.randomDungeonID)
        dungeonBuff.insert(roleCheck.randomDungeonID);
    else
        dungeonBuff.insert(roleCheck.dungeonList);
        
    partyForbidden nullForbidden;
    
    for (roleMap::iterator itr = roleCheck.currentRoles.begin(); itr != roleCheck.currentRoles.end(); ++itr)
    {
        uint64 guidBuff = itr->first;
        if (roleChosen)
            SendRoleChosen(guidBuff, plrRawGuid, roles); // send SMSG_LFG_ROLE_CHOSEN to each player
            
        // send SMSG_LFG_ROLE_CHECK_UPDATE
        SendRoleCheckUpdate(guidBuff, roleCheck);
        
        switch (roleCheck.state)
        {
            case LFG_ROLECHECK_INITIALITING:
                continue;
            case LFG_ROLECHECK_FINISHED:
                // set current plr's state to queued. then set their role in that struct
                // then send lfgupdate packet with UPDATETYPE_ADDED_TO_QUEUE
                SetPlayerState(guidBuff, LFG_STATE_QUEUED);
                SetPlayerUpdateType(guidBuff, LFG_UPDATE_ADDED_TO_QUEUE);
                SendLfgUpdate(guidBuff, GetPlayerStatus(guidBuff), true);
                break;
            default:
                if (roleCheck.leaderGuidRaw == guidBuff)
                    SendLfgJoinResult(guidBuff, ERR_LFG_ROLE_CHECK_FAILED, roleCheck.state, nullForbidden);
                SetPlayerUpdateType(guidBuff, LFG_UPDATE_ROLECHECK_FAILED);
                SendLfgUpdate(guidBuff, GetPlayerStatus(guidBuff), true);
                break;
        }
    }
    
    if (roleCheck.state == LFG_ROLECHECK_FINISHED)
    {
        LFGPlayers* queueInfo   = GetPlayerOrPartyData(groupRawGuid);
        queueInfo->currentState = LFG_STATE_QUEUED;
        queueInfo->currentRoles = roleCheck.currentRoles;
        queueInfo->joinedTime   = time(nullptr);
        
        m_playerData[groupRawGuid] = queueInfo;
        
        AddToQueue(groupRawGuid);
    }
    else if (roleCheck.state != LFG_ROLECHECK_INITIALITING)
    {
        // todo: add players back to individual queues if applicable
        m_roleCheckMap.erase(groupRawGuid);
    }
}

bool LFGMgr::ValidateGroupRoles(roleMap groupMap)
{
    if (groupMap.empty()) // sanity check
        return false;
        
    uint8 tankCount = 0, dpsCount = 0, healCount = 0;
    
    for (roleMap::iterator it = groupMap.begin(); it != groupMap.end(); ++it)
    {
        uint8 withoutLeader = it->second;
        withoutLeader &= ~PLAYER_ROLE_LEADER;
        
        switch (withoutLeader)
        {
            case PLAYER_ROLE_TANK:
                ++tankCount;
                break;
            case PLAYER_ROLE_HEALER:
                ++healCount;
                break;
            case PLAYER_ROLE_DAMAGE:
                ++dpsCount;
                break;
        }
    }
    
    return (tankCount + dpsCount + healCount == groupMap.size()) ? true : false;
}

void LFGMgr::SendRoleChosen(uint64 plrGuid, uint64 confirmedGuid, uint8 roles)
{
    Player* pPlayer = ObjectAccessor::FindPlayer(ObjectGuid(plrGuid));
    
    if (pPlayer)
        pPlayer->GetSession()->SendLfgRoleChosen(confirmedGuid, roles);
}

void LFGMgr::SendRoleCheckUpdate(uint64 plrGuid, LFGRoleCheck const& roleCheck)
{
    Player* pPlayer = ObjectAccessor::FindPlayer(ObjectGuid(plrGuid));
    
    if (pPlayer)
        pPlayer->GetSession()->SendLfgRoleCheckUpdate(roleCheck);
}

void LFGMgr::SendLfgUpdate(uint64 plrGuid, LFGPlayerStatus status, bool isGroup)
{
    Player* pPlayer = ObjectAccessor::FindPlayer(ObjectGuid(plrGuid));
    
    if (pPlayer)
        pPlayer->GetSession()->SendLfgUpdate(isGroup, status);
}

void LFGMgr::SendLfgJoinResult(uint64 plrGuid, LfgJoinResult result, LFGState state, partyForbidden const& lockedDungeons)
{
    Player* pPlayer = ObjectAccessor::FindPlayer(ObjectGuid(plrGuid));
    
    if (pPlayer)
        pPlayer->GetSession()->SendLfgJoinResult(result, state, lockedDungeons);
}
