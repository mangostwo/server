/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2019  MaNGOS project <https://getmangos.eu>
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

LFGMgr::LFGMgr()
{
    m_proposalId = 0;
}

LFGMgr::~LFGMgr()
{
    m_dailyAny.clear();
    m_dailyTBCHeroic.clear();
    m_dailyLKNormal.clear();
    m_dailyLKHeroic.clear();
    
    m_playerData.clear();
    m_queueSet.clear();
    
    m_playerStatusMap.clear();
    m_groupStatusMap.clear();
    m_groupSet.clear();
    m_proposalMap.clear();
    
    m_roleCheckMap.clear();
    
    m_bootStatusMap.clear();
    
    m_tankWaitTime.clear();
    m_healerWaitTime.clear();
    m_dpsWaitTime.clear();
    m_avgWaitTime.clear();
}

void LFGMgr::Update()
{
    //todo: remove old queues, proposals & boot votes
    
    // remove old role checks
    RemoveOldRoleChecks();
    
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
    // Todo:
    //       - see if any of this code/information can be put into a generalized class for other use
    //       - look into splitting this into 2 fns- one for player case, one for group
    Group* pGroup = plr->GetGroup();
    ObjectGuid guid = (pGroup) ? pGroup->GetObjectGuid() : plr->GetObjectGuid();
    uint32 randomDungeonID; // used later if random dungeon has been chosen
    
    LFGPlayers* currentInfo = GetPlayerOrPartyData(guid);
    
    // check if we actually have info on the player/group right now
    if (currentInfo)
    {
        bool groupCurrentlyInDungeon = pGroup && pGroup->isLFGGroup() && currentInfo->currentState != LFG_STATE_FINISHED_DUNGEON;
        
        // are they already queued?
        if (currentInfo->currentState == LFG_STATE_QUEUED)
        {
            // remove from that queue so they can later join this one
            queueSet::iterator qItr = m_queueSet.find(guid);
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
                    ObjectGuid plrGuid = pGroupPlr->GetObjectGuid();
                     
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
            partyLockedDungeons[guid] = lockedDungeons;
            
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
        ObjectGuid leaderGuid = pGroup->GetLeaderGuid();
        
        LFGRoleCheck roleCheck;
        roleCheck.state = LFG_ROLECHECK_INITIALITING;
        roleCheck.dungeonList = dungeons;
        roleCheck.randomDungeonID = randomDungeonID;
        roleCheck.leaderGuidRaw = leaderGuid.GetRawValue();
        roleCheck.waitForRoleTime = time_t(time(NULL) + LFG_TIME_ROLECHECK);

        m_roleCheckMap[guid] = roleCheck;
        
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
                
                ObjectGuid plrGuid = pGroupPlr->GetObjectGuid();
                roleCheck.currentRoles[plrGuid] = 0;
                
                m_playerStatusMap[plrGuid] = overallStatus;
            }
        }
        // used later if they enter the queue
        LFGPlayers groupInfo(LFG_STATE_NONE, dungeons, roleCheck.currentRoles, comments, false, time(NULL), 0, 0, 0);
        m_playerData[guid] = groupInfo;
        
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
        playerRole[guid] = (uint8)roles;
            
        LFGPlayers playerInfo(LFG_STATE_QUEUED, dungeons, playerRole, comments, false, time(NULL), 0, 0, 0);
        m_playerData[guid] = playerInfo;
        
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
        m_playerStatusMap[guid] = plrStatus;
        AddToQueue(guid);
    }
}

void LFGMgr::LeaveLFG(Player* plr, bool isGroup)
{    
    if (isGroup)
    {
        Group* pGroup = plr->GetGroup();
        ObjectGuid grpGuid = pGroup->GetObjectGuid();
        
        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            if (Player* pGroupPlr = itr->getSource())
            {
                ObjectGuid grpPlrGuid = pGroupPlr->GetObjectGuid();
                
                LFGPlayerStatus grpPlrStatus = GetPlayerStatus(grpPlrGuid);
                switch (grpPlrStatus.state)
                {
                    case LFG_STATE_PROPOSAL:
                    case LFG_STATE_QUEUED:
                        grpPlrStatus.updateType = LFG_UPDATE_LEAVE;
                        grpPlrStatus.state = LFG_STATE_NONE;
                        SendLfgUpdate(grpPlrGuid, grpPlrStatus, true);
                        break;
                    case LFG_STATE_ROLECHECK:
                        PerformRoleCheck(NULL, pGroup, 0);
                        break;
                    //todo: other state cases after they get implemented
                }
                
                m_playerData.erase(grpPlrGuid);
                m_playerStatusMap.erase(grpPlrGuid);
            }
        }
        
        m_queueSet.erase(grpGuid);
        m_playerData.erase(grpGuid);
    }
    else
    {
        ObjectGuid plrGuid = plr->GetObjectGuid();
        
        LFGPlayerStatus plrStatus = GetPlayerStatus(plrGuid);
        switch (plrStatus.state)
        {
            case LFG_STATE_PROPOSAL:
            case LFG_STATE_QUEUED:
                plrStatus.updateType = LFG_UPDATE_LEAVE;
                plrStatus.state = LFG_STATE_NONE;
                SendLfgUpdate(plrGuid, plrStatus, false);
                break;
            // do other states after being implemented, if applicable for a single plr
        }
        
        m_queueSet.erase(plrGuid);
        m_playerData.erase(plrGuid);
        m_playerStatusMap.erase(plrGuid);
    }
    
}

LFGPlayers* LFGMgr::GetPlayerOrPartyData(ObjectGuid guid)
{
    playerData::iterator it = m_playerData.find(guid);
    if (it != m_playerData.end())
        return &(it->second);
    else
        return NULL;
}

LFGProposal* LFGMgr::GetProposalData(uint32 proposalID)
{
    proposalMap::iterator it = m_proposalMap.find(proposalID);
    if (it != m_proposalMap.end())
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

LFGPlayerStatus LFGMgr::GetPlayerStatus(ObjectGuid guid)
{
    LFGPlayerStatus status;
    
    playerStatusMap::iterator it = m_playerStatusMap.find(guid);
    if (it != m_playerStatusMap.end())
        status = it->second;
    
    return status;
}

void LFGMgr::SetPlayerComment(ObjectGuid guid, std::string comment)
{
    LFGPlayerStatus status = GetPlayerStatus(guid);
    status.comment = comment;
    
    m_playerStatusMap[guid] = status;
}

void LFGMgr::SetPlayerState(ObjectGuid guid, LFGState state)
{
    LFGPlayerStatus status = GetPlayerStatus(guid);
    status.state = state;
    
    m_playerStatusMap[guid] = status;
}

void LFGMgr::SetPlayerUpdateType(ObjectGuid guid, LfgUpdateType updateType)
{
    LFGPlayerStatus status = GetPlayerStatus(guid);
    status.updateType = updateType;
    
    m_playerStatusMap[guid] = status;
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

void LFGMgr::UpdateNeededRoles(ObjectGuid guid, LFGPlayers* information)
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
    
    m_playerData[guid] = *information;
}

void LFGMgr::AddToQueue(ObjectGuid guid)
{
    LFGPlayers* information = GetPlayerOrPartyData(guid);
    if (!information)
        return;
    
    // This will be necessary for finding matches in the queue
    UpdateNeededRoles(guid, information);
    
    // put info into wait time maps for starters
    for (roleMap::iterator it = information->currentRoles.begin(); it != information->currentRoles.end(); ++it)
        AddToWaitMap(it->second, information->dungeonList);
        
    // just in case someone's already been in the queue.
    queueSet::iterator qItr = m_queueSet.find(guid);
    if (qItr == m_queueSet.end())
        m_queueSet.insert(guid);
}

void LFGMgr::RemoveFromQueue(ObjectGuid guid)
{
    m_queueSet.erase(guid);
    
    //todo - might need to implement a removefromwaitmap function
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

void LFGMgr::FindSpecificQueueMatches(ObjectGuid guid)
{
    uint64 rawGuid = guid.GetRawValue();
    LFGPlayers* queueInfo = GetPlayerOrPartyData(guid);
    if (queueInfo)
    {
        // compare to everyone else in queue for compatibility
        // after a match is found call UpdateNeededRoles
        // Use the roleMap to store player guid/role information; merge into queueInfo struct & delete other struct/map entry
        for (queueSet::iterator itr = m_queueSet.begin(); itr != m_queueSet.end(); ++itr)
        {
            if (*itr == guid)
                continue;
            
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
                    // check for player / role count and also team compatibility
                    // if function returns true, then merge groups into one
                    if (RoleMapsAreCompatible(queueInfo, matchInfo) && MatchesAreOfSameTeam(queueInfo, matchInfo))
                        MergeGroups(guid, *itr, compatibleDungeons);
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

bool LFGMgr::MatchesAreOfSameTeam(LFGPlayers* groupOne, LFGPlayers* groupTwo)
{
    // we should safely be able to compare any two players from each struct to
    // determine compatibility
    roleMap::iterator it1 = groupOne->currentRoles.begin();
    roleMap::iterator it2 = groupTwo->currentRoles.begin();
    
    // now we find the players from the maps
    Player* pPlayer1 = sObjectAccessor.FindPlayer(it1->first);
    Player* pPlayer2 = sObjectAccessor.FindPlayer(it2->first);
    
    // todo: disable this if a config option is set
    if (pPlayer1->GetTeamId() == pPlayer2->GetTeamId())
        return true;
        
    return false;
}

void LFGMgr::MergeGroups(ObjectGuid guidOne, ObjectGuid guidTwo, std::set<uint32> compatibleDungeons)
{    
    // merge into the entry for rawGuidOne, then see if they are
    // able to enter the dungeon at this point or not
    LFGPlayers* mainGroup   = GetPlayerOrPartyData(guidOne);
    LFGPlayers* bufferGroup = GetPlayerOrPartyData(guidTwo);
    
    if (!mainGroup || !bufferGroup)
        return;
        
    // update the dungeon selection with the compatible ones
    mainGroup->dungeonList.clear();
    mainGroup->dungeonList = compatibleDungeons;
        
    // move players / roles into a single roleMap
    for (roleMap::iterator it = bufferGroup->currentRoles.begin(); it != bufferGroup->currentRoles.end(); ++it)
        mainGroup->currentRoles[it->first] = it->second;
        
    // update the role count / needed role info
    UpdateNeededRoles(guidOne, mainGroup);
    
    // being safe
    //mainGroup = GetPlayerOrPartyData(rawGuidOne);
    
    // Then do the following:
    if ((mainGroup->neededTanks == 0) && (mainGroup->neededHealers == 0) && (mainGroup->neededDps == 0))
        SendDungeonProposal(mainGroup);
        
    m_playerData.erase(guidTwo);
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
                if (Player* pPlayer = sObjectAccessor.FindPlayer(rItr->first))
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
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    ObjectGuid plrGuid = pPlayer? pPlayer->GetObjectGuid() : ObjectGuid();
    
    roleCheckMap::iterator it = m_roleCheckMap.find(groupGuid);
    if (it == m_roleCheckMap.end())
        return; // no role check map found
    
    LFGRoleCheck roleCheck = it->second;
    bool roleChosen = roleCheck.state != LFG_ROLECHECK_DEFAULT && plrGuid;
    
    if (!plrGuid)
        roleCheck.state = LFG_ROLECHECK_ABORTED;  // aborted if anyone cancels during role check
    else if (roles < PLAYER_ROLE_TANK)            // kind of a sanity check- the client shouldn't allow this to happen
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
    else
    {
        roleCheck.currentRoles[plrGuid] = roles;
        
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
        dungeonBuff = roleCheck.dungeonList;
        
    partyForbidden nullForbidden;
    
    for (roleMap::iterator itr = roleCheck.currentRoles.begin(); itr != roleCheck.currentRoles.end(); ++itr)
    {
        ObjectGuid guidBuff = itr->first;
        if (roleChosen)
            SendRoleChosen(guidBuff, plrGuid, roles); // send SMSG_LFG_ROLE_CHOSEN to each player
            
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
                if (roleCheck.leaderGuidRaw == guidBuff.GetRawValue())
                    SendLfgJoinResult(guidBuff, ERR_LFG_ROLE_CHECK_FAILED, LFG_STATE_ROLECHECK, nullForbidden);
                SetPlayerUpdateType(guidBuff, LFG_UPDATE_ROLECHECK_FAILED);
                SendLfgUpdate(guidBuff, GetPlayerStatus(guidBuff), true);
                break;
        }
    }
    
    if (roleCheck.state == LFG_ROLECHECK_FINISHED)
    {
        LFGPlayers* queueInfo   = GetPlayerOrPartyData(groupGuid);
        queueInfo->currentState = LFG_STATE_QUEUED;
        queueInfo->currentRoles = roleCheck.currentRoles;
        queueInfo->joinedTime   = time(NULL);
        
        m_playerData[groupGuid] = *queueInfo;
        
        AddToQueue(groupGuid);
    }
    else if (roleCheck.state != LFG_ROLECHECK_INITIALITING)
    {
        // todo: add players back to individual queues if applicable
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
            
        for (roleMap::iterator roleMapItr = roleCheck.currentRoles.begin(); roleMapItr != roleCheck.currentRoles.end(); ++roleMapItr)
        {
            ObjectGuid plrGuid = roleMapItr->first;
                
            SetPlayerState(plrGuid, LFG_STATE_NONE);
                
            SendRoleCheckUpdate(plrGuid, roleCheck);                 // role check failed 
            SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), true);  // not in lfg system anymore
        }
        m_roleCheckMap.erase(groupGuid);
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

//todo: remove from queue, update queue average settings
void LFGMgr::SendDungeonProposal(LFGPlayers* lfgGroup)
{
    ++m_proposalId; // increment number to make a new proposal id
    
    std::set<uint32>::iterator dItr = lfgGroup->dungeonList.begin();
        
    // note: group create function's parameters are leader guid & leader name
    LFGProposal newProposal;
    newProposal.id = m_proposalId;
    newProposal.state = LFG_PROPOSAL_INITIATING;
    newProposal.encounters = 0; // todo: check if group has already started a dungeon and are looking for another plr
    newProposal.currentRoles = lfgGroup->currentRoles;
    newProposal.dungeonID = *dItr;
    newProposal.isNew = true;
    newProposal.joinedQueue = lfgGroup->joinedTime;
    
    bool premadeGroup = IsProposalSameGroup(newProposal);
    
    // iterate through role map just so get everyone's guid
    for (roleMap::iterator it = lfgGroup->currentRoles.begin(); it != lfgGroup->currentRoles.end(); ++it)
    {
        ObjectGuid plrGuid = it->first;
        SetPlayerState(plrGuid, LFG_STATE_PROPOSAL);

        Player* pPlayer = sObjectAccessor.FindPlayer(plrGuid);
        
        if (Group* pGroup = pPlayer->GetGroup())
        {
            ObjectGuid grpGuid = pGroup->GetObjectGuid();
            
            SetPlayerUpdateType(plrGuid, LFG_UPDATE_PROPOSAL_BEGIN);
            
            if (premadeGroup && pGroup->IsLeader(plrGuid))
                newProposal.groupLeaderGuid = plrGuid.GetRawValue();
                
            if (premadeGroup && !newProposal.groupRawGuid)
                newProposal.groupRawGuid = grpGuid.GetRawValue();

            newProposal.groups[plrGuid] = grpGuid;
            
            SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), true);
        }
        else
        {
            newProposal.groups[plrGuid] = ObjectGuid();

            //SetPlayerUpdateType(plrGuid, LFG_UPDATE_GROUP_FOUND);
            //SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), false);
            
            SetPlayerUpdateType(plrGuid, LFG_UPDATE_PROPOSAL_BEGIN);
            SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), false);
        }
            
        newProposal.answers[plrGuid] = LFG_ANSWER_PENDING;
        
        // then send SMSG_LFG_PROPOSAL_UPDATE
        pPlayer->GetSession()->SendLfgProposalUpdate(newProposal);
    }
    
    // then if group guid is set, call Group::SetAsLfgGroup()
    if (premadeGroup)
    {
        Player* pGroupLeader = sObjectAccessor.FindPlayer(ObjectGuid(newProposal.groupLeaderGuid));
        pGroupLeader->GetGroup()->SetAsLfgGroup();
    }
    
    // also save the proposal
    m_proposalMap[newProposal.id] = newProposal;
}

bool LFGMgr::IsProposalSameGroup(LFGProposal const& proposal)
{
    bool firstLoop = true;
    bool isSameGroup = true;
    
    ObjectGuid priorGroupGuid;
    
    // when this is called we don't have the groups part filled, so iterate via role map
    for (roleMap::const_iterator it = proposal.currentRoles.begin(); it != proposal.currentRoles.end(); ++it)
    {
        ObjectGuid plrGuid = it->first;
        
        Player* pPlayer = sObjectAccessor.FindPlayer(plrGuid);
        if (Group* pGroup = pPlayer->GetGroup())
        {
            ObjectGuid grpGuid = pGroup->GetObjectGuid();
            
            if (firstLoop)
                priorGroupGuid = grpGuid;
            else
            {
                if (isSameGroup)
                {
                    if (grpGuid != priorGroupGuid)
                        isSameGroup = false;
                }
            }
        }
    }
    return isSameGroup;
}

// From a CMSG_LFG_PROPOSAL_RESPONSE call
void LFGMgr::ProposalUpdate(uint32 proposalID, ObjectGuid plrGuid, bool accepted)
{
    //note: create a group here if it doesn't exist and everyone accepted proposal
    LFGProposal* proposal = GetProposalData(proposalID);
    
    if (!proposal)
        return;
        
    bool allOkay = true; // true if everyone answered LFG_ANSWER_AGREE
        
    // Update answer map to given value
    LFGProposalAnswer plrAnswer = (LFGProposalAnswer)accepted;
    proposal->answers[plrGuid] = plrAnswer;
    
    // If the player declined, the proposal is over
    if (plrAnswer == LFG_ANSWER_DENY)
        ProposalDeclined(plrGuid, proposal);
        
    for (proposalAnswerMap::iterator it = proposal->answers.begin(); it != proposal->answers.end(); ++it)
    {
        if (it->second != LFG_ANSWER_AGREE)
            allOkay = false;
    }
    
    // if !allOkay, send proposal updates to all
    if (!allOkay)
    {
        for (proposalAnswerMap::iterator itr = proposal->answers.begin(); itr != proposal->answers.end(); ++itr)
        {
            ObjectGuid proposalPlrGuid  = itr->first;
            Player* pProposalPlayer = sObjectAccessor.FindPlayer(proposalPlrGuid);
            pProposalPlayer->GetSession()->SendLfgProposalUpdate(*proposal);
        }
        
        return;
    }
    
    // at this point everyone's good to join the dungeon!
    
    time_t joinedTime = time(NULL);
    bool sendProposalUpdate = proposal->state != LFG_PROPOSAL_SUCCESS;
    
    // now update the proposal's state to successful and inform the players
    proposal->state = LFG_PROPOSAL_SUCCESS;
    for (roleMap::iterator rItr = proposal->currentRoles.begin(); rItr != proposal->currentRoles.end(); ++rItr)
    {
        // get the player's role
        uint8 proposalPlrRole   = rItr->second;
        proposalPlrRole &= ~PLAYER_ROLE_LEADER;
        
        ObjectGuid proposalPlrGuid  = rItr->first;
        Player* pProposalPlayer = sObjectAccessor.FindPlayer(proposalPlrGuid);
        
        if (sendProposalUpdate)
            pProposalPlayer->GetSession()->SendLfgProposalUpdate(*proposal);
            
        // amount of time spent in queue
        int32 timeWaited = (joinedTime - proposal->joinedQueue) / IN_MILLISECONDS;
        
        // tell the lfg system to update the average wait times on the next tick
        UpdateWaitMap(LFGRoles(proposalPlrRole), proposal->dungeonID, timeWaited);
        
        // send some updates to the player, depending on group status
        LFGPlayerStatus proposalPlrStatus = GetPlayerStatus(proposalPlrGuid);
        proposalPlrStatus.updateType = LFG_UPDATE_GROUP_FOUND;
        
        if (pProposalPlayer->GetGroup())
        {
            SendLfgUpdate(proposalPlrGuid, proposalPlrStatus, true);
            RemoveFromQueue(pProposalPlayer->GetGroup()->GetObjectGuid()); // not the best way to handle this
        }
        else
        {
            SendLfgUpdate(proposalPlrGuid, proposalPlrStatus, false);
            RemoveFromQueue(proposalPlrGuid);
        }

        proposalPlrStatus.updateType = LFG_UPDATE_LEAVE;
        SendLfgUpdate(proposalPlrGuid, proposalPlrStatus, false);
        SendLfgUpdate(proposalPlrGuid, proposalPlrStatus, true);
        
        proposalPlrStatus.state = LFG_STATE_IN_DUNGEON;
    }
    
    CreateDungeonGroup(proposal);
    m_proposalMap.erase(proposal->id);
}

bool LFGMgr::HasLeaderFlag(roleMap const& roles)
{
    for (roleMap::const_iterator it = roles.begin(); it != roles.end(); ++it)
    {
        if (it->second & PLAYER_ROLE_LEADER)
            return true;
    }
    return false;
}

void LFGMgr::CreateDungeonGroup(LFGProposal* proposal)
{
    if (!proposal)
        return;
    
    Group* pGroup;
    
    if (!proposal->groupRawGuid)
    {
        bool leaderIsSet = false;
        bool leaderRoleIsSet = HasLeaderFlag(proposal->currentRoles);
        ObjectGuid leaderGuid;
        
        pGroup = new Group();
        
        for (playerGroupMap::iterator it = proposal->groups.begin(); it != proposal->groups.end(); ++it)
        {
            // remove plr from group w/ guid it->second
            // set leader on first loop, then set leaderisset to true
            ObjectGuid pGroupPlrGuid = it->first;
            Player* pGroupPlr = sObjectAccessor.FindPlayer(pGroupPlrGuid);
            
            if (it->second)
                pGroupPlr->GetGroup()->RemoveMember(pGroupPlrGuid, 0);
                
            if (!leaderIsSet)
            {
                bool currentPlrIsLeader = false;
                if (leaderRoleIsSet)
                {
                    for (roleMap::iterator itr = proposal->currentRoles.begin(); itr != proposal->currentRoles.end(); ++itr)
                    {
                        if (itr->second & PLAYER_ROLE_LEADER)
                        {
                            leaderGuid = itr->first;
                            Player* leaderRef = sObjectAccessor.FindPlayer(leaderGuid);
                            
                            pGroup->Create(leaderRef->GetObjectGuid(), leaderRef->GetName());
                            
                            if (pGroupPlrGuid == leaderGuid)
                                currentPlrIsLeader = true;
                        }
                    }
                }
                else
                    pGroup->Create(pGroupPlrGuid, pGroupPlr->GetName());
                
                if (!currentPlrIsLeader)
                    pGroup->AddMember(pGroupPlrGuid, pGroupPlr->GetName());
                    
                leaderIsSet = true;
            }
            else if (leaderIsSet && pGroupPlrGuid != leaderGuid)
                pGroup->AddMember(pGroupPlrGuid, pGroupPlr->GetName());
        }
            pGroup->SetAsLfgGroup();
    }
    else
    {
        Player* pGroupLeader = sObjectAccessor.FindPlayer(ObjectGuid(proposal->groupLeaderGuid));
        pGroup = pGroupLeader->GetGroup();
    }
    
    // set dungeon difficulty for group
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(proposal->dungeonID);
    if (!dungeon)
        return;
        
    pGroup->SetDungeonDifficulty(Difficulty(dungeon->difficulty)); //todo: check for raids and if so call setraiddifficulty
    
    // Add group to our group set and group map, then teleport to the dungeon
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus groupStatus(LFG_STATE_IN_DUNGEON, dungeon->ID, proposal->currentRoles, pGroup->GetLeaderGuid());
    
    m_groupSet.insert(groupGuid);
    m_groupStatusMap[groupGuid] = groupStatus;
    TeleportToDungeon(dungeon->ID, pGroup);
    
    pGroup->SendUpdate();
}

void LFGMgr::TeleportToDungeon(uint32 dungeonID, Group* pGroup)
{
    // if the group's leader is already in the dungeon, teleport anyone not in dungeon to them
    // if nobody is in the dungeon, teleport all to beginning of dungeon (sObjectMgr.GetMapEntranceTrigger(mapid [not dungeonid]))    
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonID);
    if (!dungeon || !pGroup)
        return;
        
    uint32 mapID = (uint32)dungeon->mapID;
    float x, y, z, o;
    LFGTeleportError err = LFG_TELEPORTERROR_OK;
    
    Player* pGroupLeader = sObjectAccessor.FindPlayer(pGroup->GetLeaderGuid());
    
    if (pGroupLeader && pGroupLeader->GetMapId() == mapID) // Already in the dungeon
    {
        // set teleport location to that of the group leader
        x = pGroupLeader->GetPositionX();
        y = pGroupLeader->GetPositionY();
        z = pGroupLeader->GetPositionZ();
        o = pGroupLeader->GetOrientation();
    }
    else
    {
        if (AreaTrigger const* at = sObjectMgr.GetMapEntranceTrigger(mapID))
        {
            x = at->target_X;
            y = at->target_Y;
            z = at->target_Z;
            o = at->target_Orientation;
        }
        else
            err = LFG_TELEPORTERROR_INVALID_LOCATION;
    }
    
    dungeonForbidden lockedDungeons;
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            // further checks: player is dead, in vehicle, in battleground, on taxi, etc
            LFGTeleportError plrErr = LFG_TELEPORTERROR_OK;
            
            if (pGroupPlr->IsDead())
                plrErr = LFG_TELEPORTERROR_PLAYER_DEAD;
            if (pGroupPlr->IsFalling())
                plrErr = LFG_TELEPORTERROR_FALLING;
            if (pGroupPlr->GetVehicleInfo())
                plrErr = LFG_TELEPORTERROR_IN_VEHICLE;
            
            lockedDungeons = FindRandomDungeonsNotForPlayer(pGroupPlr);
            if (lockedDungeons.find(dungeon->Entry()) != lockedDungeons.end())
                plrErr = LFG_TELEPORTERROR_INVALID_LOCATION;
                
            if (err == LFG_TELEPORTERROR_OK && plrErr == LFG_TELEPORTERROR_OK && pGroupPlr->GetMapId() != mapID)
            {
                if (pGroupPlr->GetMap() && !pGroupPlr->GetMap()->IsDungeon() && !pGroupPlr->GetMap()->IsRaid() && !pGroupPlr->InBattleGround())
                    pGroupPlr->SetBattleGroundEntryPoint(); // store current position and such
                    
                if (!pGroupPlr->TeleportTo(mapID, x, y, z, o))
                    plrErr = LFG_TELEPORTERROR_INVALID_LOCATION;
            }
            
            if (err != LFG_TELEPORTERROR_OK)
                pGroupPlr->GetSession()->SendLfgTeleportError(err);
            else if (plrErr != LFG_TELEPORTERROR_OK)
                pGroupPlr->GetSession()->SendLfgTeleportError(plrErr);
            else
                SetPlayerState(pGroupPlr->GetObjectGuid(), LFG_STATE_IN_DUNGEON);
        }
    }
}

void LFGMgr::TeleportPlayer(Player* pPlayer, bool out)
{
    // Fetch necessary data first
    Group* pGroup = pPlayer->GetGroup();
    LFGGroupStatus* status = GetGroupStatus(pGroup->GetObjectGuid());
    
    if (!pGroup || !status)
    {
        pPlayer->GetSession()->SendLfgTeleportError((uint8)LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }
    
    // Get dungeon info and then teleport the player out if applicable
    if (out)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(status->dungeonID);
        if (dungeon && pPlayer->GetMapId() == dungeon->mapID)
            pPlayer->TeleportToBGEntryPoint();
    }
}

LFGGroupStatus* LFGMgr::GetGroupStatus(ObjectGuid guid)
{
    groupStatusMap::iterator it = m_groupStatusMap.find(guid);
    if (it != m_groupStatusMap.end())
        return &(it->second);
    else
        return NULL;
}

void LFGMgr::ProposalDeclined(ObjectGuid guid, LFGProposal* proposal)
{
    Player* pPlayer = sObjectAccessor.FindPlayer(guid);
    
    if (!pPlayer)
        return;
        
    bool leaveGroupLFG = false;
    
    for (roleMap::iterator it = proposal->currentRoles.begin(); it != proposal->currentRoles.end(); ++it)
    {
        ObjectGuid groupPlrGuid = it->first;
        
        // update each player with a LFG_UPDATE_PROPOSAL_DECLINED        
        SetPlayerUpdateType(groupPlrGuid, LFG_UPDATE_PROPOSAL_DECLINED);

        Player* pGroupPlayer = sObjectAccessor.FindPlayer(groupPlrGuid);
        Group* pGroup = pGroupPlayer->GetGroup();
        
        // if player was in a premade group and declined, remove the group.
        if (groupPlrGuid == guid)
        {
            //LeaveLFG(pGroupPlayer, true);
            if (pGroup && (pGroup->GetObjectGuid().GetRawValue() == proposal->groupRawGuid))
                leaveGroupLFG = true;
                
            SendLfgUpdate(groupPlrGuid, GetPlayerStatus(groupPlrGuid), false);
        }
        else
        {
            if (proposal->groupRawGuid)
                SendLfgUpdate(groupPlrGuid, GetPlayerStatus(groupPlrGuid), true);
            else
                SendLfgUpdate(groupPlrGuid, GetPlayerStatus(groupPlrGuid), false);
        }
    }
    
    if (!leaveGroupLFG)
    {
        proposal->currentRoles.erase(guid);
        proposal->answers.erase(guid);
        proposal->groups.erase(guid);
    }
    else
    {
        m_proposalMap.erase(proposal->id);
    }
    
    LeaveLFG(pPlayer, leaveGroupLFG);
}

void LFGMgr::UpdateWaitMap(LFGRoles role, uint32 dungeonID, time_t waitTime)
{
    if (!role || !dungeonID || !waitTime)
        return;
        
    switch (role)
    {
        case PLAYER_ROLE_TANK:
        {
            waitTimeMap::iterator it = m_tankWaitTime.find(dungeonID);
            if (it != m_tankWaitTime.end())
            {
                LFGWait wait = it->second;
                
                wait.previousTime = wait.time;
                wait.time = waitTime;
                wait.doAverage = true;
                
                m_tankWaitTime[dungeonID] = wait;
            }
        }
            break;
        case PLAYER_ROLE_HEALER:
        {
            waitTimeMap::iterator hIt = m_healerWaitTime.find(dungeonID);
            if (hIt != m_healerWaitTime.end())
            {
                LFGWait wait = hIt->second;
                
                wait.previousTime = wait.time;
                wait.time = waitTime;
                wait.doAverage = true;
                
                m_healerWaitTime[dungeonID] = wait;
            }
        }
            break;
        case PLAYER_ROLE_DAMAGE:
        {
            waitTimeMap::iterator dIt = m_dpsWaitTime.find(dungeonID);
            if (dIt != m_dpsWaitTime.end())
            {
                LFGWait wait = dIt->second;
                
                wait.previousTime = wait.time;
                wait.time = waitTime;
                wait.doAverage = true;
                
                m_dpsWaitTime[dungeonID] = wait;
            }
        }
            break;
        default:
        {
            waitTimeMap::iterator aIt = m_avgWaitTime.find(dungeonID);
            if (aIt != m_avgWaitTime.end())
            {
                LFGWait wait = aIt->second;
                
                wait.previousTime = wait.time;
                wait.time = waitTime;
                wait.doAverage = true;
                
                m_avgWaitTime[dungeonID] = wait;
            }
        }
            break;
    }
    
}

void LFGMgr::HandleBossKilled(Player* pPlayer)
{
    Group* pGroup = pPlayer->GetGroup();
    
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    
    if (!pGroup || !status)
        return;
        
    // set each player's lfgstate to LFG_STATE_FINISHED_DUNGEON
    // fetch reward info, and if it's the first dungeon of the day (per player),
    //    give them 2x the xp (or 1x if it's not the first), and the reward item
    //    (special case for 2nd wotlk heroic and +). If no room in inventory, send
    //    via ingame mail.
    status->state = LFG_STATE_FINISHED_DUNGEON;
    
    DungeonTypes type = GetDungeonType(status->dungeonID);
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next()) //todo: check if we will need to use mail or not
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            SetPlayerState(pGroupPlr->GetObjectGuid(), LFG_STATE_FINISHED_DUNGEON);
            
            // check if player did a random dungeon
            uint32 randomDungeonId = 0;
            LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(status->dungeonID);
            if (dungeon->typeID == LFG_TYPE_RANDOM_DUNGEON || IsSeasonal(dungeon->flags))
                randomDungeonId = dungeon->ID;
            
            // get rewards
            uint32 groupPlrLevel = pGroupPlr->getLevel();
            const DungeonFinderRewards* rewards = sObjectMgr.GetDungeonFinderRewards(groupPlrLevel); // Fetch base xp/money reward
            ItemRewards itemRewards = GetDungeonItemRewards(status->dungeonID, type);                // fetch item reward
            
            int32 multiplier;                                                                        // base reward modifier
            bool hasDoneDaily = HasPlayerDoneDaily(pGroupPlr->GetGUIDLow(), type);                                 // first dungeon of the day?
            (hasDoneDaily) ? multiplier = 1 : multiplier = 2;
            
            uint32 xpReward = multiplier*rewards->baseXPReward;                                      // player's xp reward
            uint32 moneyReward = uint32(multiplier*rewards->baseMonetaryReward);                              // player's money reward
            
            uint32 itemReward = 0;                                                                   // reward item
            uint32 itemAmount = 0;                                                                   // amount of item
            if (hasDoneDaily && (type == DUNGEON_WOTLK_HEROIC))
            {
                itemReward = WOTLK_SPECIAL_HEROIC_ITEM;
                itemAmount = WOTLK_SPECIAL_HEROIC_AMNT;
            }
            else if (!hasDoneDaily)
            {
                itemReward = itemRewards.itemId;
                itemAmount = itemRewards.itemAmount;
            }
            
            // and then fill a structure corresponding to SMSG_LFG_PLAYER_REWARD and
            // send one of these to each player
            LFGRewards reward(randomDungeonId, status->dungeonID, hasDoneDaily, moneyReward, xpReward, itemReward, itemAmount);
            pGroupPlr->GetSession()->SendLfgRewards(reward);
        }
    }
    
    // now we can remove the group from our maps
    m_groupStatusMap.erase(groupGuid);
    m_groupSet.erase(groupGuid);
}

void LFGMgr::AttemptToKickPlayer(Group* pGroup, ObjectGuid guid, ObjectGuid kicker, std::string reason)
{
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    
    bootStatusMap::iterator bIt = m_bootStatusMap.find(groupGuid);
    if (!status)
        return;
    
    status->state = LFG_STATE_BOOT;
    m_groupStatusMap[groupGuid] = *status;
    
    // This function is only called when a group is set/in a dungeon so we can go straight to the boot packets
    time_t now = time(NULL);
    proposalAnswerMap votes;
    
    // safe to say the person attempting to kick them will vote yes, the kick-ee will vote no
    votes[guid] = LFG_ANSWER_DENY;
    votes[kicker] = LFG_ANSWER_AGREE;
    
    // set group state to boot vote, same for player states until it's over
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next()) //todo: check if we will need to use mail or not
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            ObjectGuid pGroupPlrGuid = pGroupPlr->GetObjectGuid();
            
            SetPlayerState(pGroupPlrGuid, LFG_STATE_BOOT);
            
            if ( (pGroupPlrGuid != guid) && (pGroupPlrGuid != kicker) )
                votes[pGroupPlrGuid] = LFG_ANSWER_PENDING;
        }
    }
    
    LFGBoot boot(true, guid, reason, votes, now);
    m_bootStatusMap[groupGuid] = boot;
    
    for (GroupReference* it = pGroup->GetFirstMember(); it != NULL; it = it->next())
    {
        if (Player* groupPlr = it->getSource())
            groupPlr->GetSession()->SendLfgBootUpdate(boot);
    }
}

void LFGMgr::CastVote(Player* pPlayer, bool vote)
{
    if (!pPlayer)
        return;
        
    Group* pGroup = pPlayer->GetGroup();
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    
    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    
    if (!status || status->state != LFG_STATE_BOOT)
        return;
        
    bootStatusMap::iterator it = m_bootStatusMap.find(groupGuid);
    if (it == m_bootStatusMap.end())
        return;
        
    LFGBoot boot = it->second;
    boot.answers[pPlayer->GetObjectGuid()] = LFGProposalAnswer(vote);
    
    int32 yay = 0, nay = 0; // keep a count of votes
    for (proposalAnswerMap::iterator pIt = boot.answers.begin(); pIt != boot.answers.end(); ++pIt)
    {
        LFGProposalAnswer answer = pIt->second;
        if (answer == LFG_ANSWER_AGREE)
            ++yay;
        else if (answer == LFG_ANSWER_DENY)
            ++nay;
    }
    
    if (yay < REQUIRED_VOTES_FOR_BOOT && nay < REQUIRED_VOTES_FOR_BOOT)
    {
        m_bootStatusMap[groupGuid] = boot;
        return;
    }
    
    // if we dont have enough votes to kick or keep plr, don't send packet update
    // if else, set boot.inProgress to false, set plr + group states back to lfg-state-dungeon,
    // send packet update to group, kick plr if we had the votes, and then erase entry from boot map
    
    boot.inProgress = false;
    status->state = LFG_STATE_IN_DUNGEON;
    
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            ObjectGuid plrGuid = pGroupPlr->GetObjectGuid();
            
            if (plrGuid != boot.playerVotedOn)
            {
                SetPlayerState(plrGuid, LFG_STATE_IN_DUNGEON);
                pGroupPlr->GetSession()->SendLfgBootUpdate(boot);
            }
        }
    }
    
    if (yay == REQUIRED_VOTES_FOR_BOOT)
    {
        // kick player from group
        if (pGroup->RemoveMember(boot.playerVotedOn, 1) <= 1)
        {
            // group->Disband(); already disbanded in RemoveMember
            sObjectMgr.RemoveGroup(pGroup);
            delete pGroup;
            // removemember sets the player's group pointer to NULL
        }
    }
}

void LFGMgr::SendRoleChosen(ObjectGuid plrGuid, ObjectGuid confirmedGuid, uint8 roles)
{
    Player* pPlayer = sObjectAccessor.FindPlayer(plrGuid);
    
    if (pPlayer)
        pPlayer->GetSession()->SendLfgRoleChosen(confirmedGuid.GetRawValue(), roles);
}

void LFGMgr::SendRoleCheckUpdate(ObjectGuid plrGuid, LFGRoleCheck const& roleCheck)
{
    Player* pPlayer = sObjectAccessor.FindPlayer(plrGuid);
    
    if (pPlayer)
        pPlayer->GetSession()->SendLfgRoleCheckUpdate(roleCheck);
}

void LFGMgr::SendLfgUpdate(ObjectGuid plrGuid, LFGPlayerStatus status, bool isGroup)
{
    Player* pPlayer = sObjectAccessor.FindPlayer(plrGuid);
    
    if (pPlayer)
        pPlayer->GetSession()->SendLfgUpdate(isGroup, status);
}

void LFGMgr::SendLfgJoinResult(ObjectGuid plrGuid, LfgJoinResult result, LFGState state, partyForbidden const& lockedDungeons)
{
    Player* pPlayer = sObjectAccessor.FindPlayer(plrGuid);
    
    if (pPlayer)
        pPlayer->GetSession()->SendLfgJoinResult(result, state, lockedDungeons);
}

void LFGMgr::RemoveOldRoleChecks()
{
    for (roleCheckMap::iterator roleItr = m_roleCheckMap.begin(); roleItr != m_roleCheckMap.end(); ++roleItr)
    {
        ObjectGuid groupGuid = roleItr->first;
        
        LFGRoleCheck roleCheck = roleItr->second;
        if ((roleCheck.waitForRoleTime - time(NULL)) <= 0) // no time left
        {
            roleCheck.state = LFG_ROLECHECK_NO_ROLE;
            
            for (roleMap::iterator roleMapItr = roleCheck.currentRoles.begin(); roleMapItr != roleCheck.currentRoles.end(); ++roleMapItr)
            {
                ObjectGuid plrGuid = roleMapItr->first;
                
                SetPlayerState(plrGuid, LFG_STATE_NONE);
                
                SendRoleCheckUpdate(plrGuid, roleCheck);                 // role check failed 
                SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), true);  // not in lfg system anymore
            }
            
            m_roleCheckMap.erase(groupGuid);
        }
    }
}
