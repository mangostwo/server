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

#include <set>
#include "DBCEnums.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "LFGMgr.h"
#include "Object.h"
#include "Player.h"
#include "PlayerRegistry.h"
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










ItemRewards LFGMgr::GetDungeonItemRewards(uint32 dungeonId, DungeonTypes type)
{
    ItemRewards rewards;
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonId);
    if (dungeon)
    {
        uint32 minLevel = dungeon->MinLevel;
        uint32 maxLevel = dungeon->MaxLevel;
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
        switch (dungeon->ExpansionLevel)
        {
            case 0:
                return DUNGEON_CLASSIC;
            case 1:
            {
                if (dungeon->Difficulty == DUNGEON_DIFFICULTY_NORMAL)
                {
                    return DUNGEON_TBC;
                }
                else if (dungeon->Difficulty == DUNGEON_DIFFICULTY_HEROIC)
                {
                    return DUNGEON_TBC_HEROIC;
                }
            }
            case 2:
            {
                if (dungeon->Difficulty == DUNGEON_DIFFICULTY_NORMAL)
                {
                    return DUNGEON_WOTLK;
                }
                else if (dungeon->Difficulty == DUNGEON_DIFFICULTY_HEROIC)
                {
                    return DUNGEON_WOTLK_HEROIC;
                }
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
            if ( (dungeon->TypeID == LFG_TYPE_RANDOM_DUNGEON)
                || (IsSeasonal(dungeon->Flags) && IsSeasonActive(dungeon->ID)) )
                if ((uint8)dungeon->ExpansionLevel <= expansion && dungeon->MinLevel <= level
                    && dungeon->MaxLevel >= level)
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

            if ((uint8)dungeon->ExpansionLevel > expansion)
            {
                forbiddenReason = (uint32)LFG_FORBIDDEN_EXPANSION;
            }
            else if (dungeon->TypeID == LFG_TYPE_RAID)
            {
                forbiddenReason = (uint32)LFG_FORBIDDEN_RAID;
            }
            else if (dungeon->MinLevel > level)
            {
                forbiddenReason = (uint32)LFG_FORBIDDEN_LOW_LEVEL;
            }
            else if (dungeon->MaxLevel < level)
            {
                forbiddenReason = (uint32)LFG_FORBIDDEN_HIGH_LEVEL;
            }
            else if (IsSeasonal(dungeon->Flags) && !IsSeasonActive(dungeon->ID)) // check pointers/function args
            {
                forbiddenReason = (uint32)LFG_FORBIDDEN_NOT_IN_SEASON;
            }
            else if (DungeonFinderRequirements const* req = sObjectMgr.GetDungeonFinderRequirements((uint32)dungeon->MapID, dungeon->Difficulty))
            {
                if (req->minItemLevel && (plr->GetEquipGearScore(false,false) < req->minItemLevel))
                {
                    forbiddenReason = (uint32)LFG_FORBIDDEN_LOW_GEAR_SCORE;
                }
                else if (req->achievement && !plr->GetAchievementMgr().HasAchievement(req->achievement))
                {
                    forbiddenReason = (uint32)LFG_FORBIDDEN_MISSING_ACHIEVEMENT;
                }
                else if (plr->GetTeam() == ALLIANCE && req->allianceQuestId && !plr->GetQuestRewardStatus(req->allianceQuestId))
                {
                    forbiddenReason = (uint32)LFG_FORBIDDEN_QUEST_INCOMPLETE;
                }
                else if (plr->GetTeam() == HORDE && req->hordeQuestId && !plr->GetQuestRewardStatus(req->hordeQuestId))
                {
                    forbiddenReason = (uint32)LFG_FORBIDDEN_QUEST_INCOMPLETE;
                }
                else
                    if (req->item)
                    {
                        if (!plr->HasItemCount(req->item, 1) && (!req->item2 || !plr->HasItemCount(req->item2, 1)))
                        {
                            forbiddenReason = LFG_FORBIDDEN_MISSING_ITEM;
                        }
                    }
                    else if (req->item2 && !plr->HasItemCount(req->item2, 1))
                    {
                        forbiddenReason = LFG_FORBIDDEN_MISSING_ITEM;
                    }
            }

            if (forbiddenReason)
            {
                randomDungeons[dungeon->Entry()] = forbiddenReason;
            }
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
        if (dungeon->Difficulty == DUNGEON_DIFFICULTY_NORMAL)
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
    {
        return;
    }

    // This will be necessary for finding matches in the queue
    UpdateNeededRoles(guid, information);

    // put info into wait time maps for starters
    for (roleMap::iterator it = information->currentRoles.begin(); it != information->currentRoles.end(); ++it)
    {
        AddToWaitMap(it->second, information->dungeonList);
    }

    // just in case someone's already been in the queue.
    queueSet::iterator qItr = m_queueSet.find(guid);
    if (qItr == m_queueSet.end())
    {
        m_queueSet.insert(guid);
    }
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
        {
            ++it->second.playerCount;
        }
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
    {
        FindSpecificQueueMatches(*itr);
    }
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
            {
                continue;
            }

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

                for (std::set<uint32>::iterator dItr = matchInfo->dungeonList.begin(); dItr != matchInfo->dungeonList.end(); ++dItr)
                {
                    if (queueInfo->dungeonList.find(*dItr) != queueInfo->dungeonList.end())
                    {
                        compatibleDungeons.insert(*dItr);
                    }
                }

                if (!compatibleDungeons.empty())
                {
                    // check for player / role count and also team compatibility
                    // if function returns true, then merge groups into one
                    if (RoleMapsAreCompatible(queueInfo, matchInfo) && MatchesAreOfSameTeam(queueInfo, matchInfo))
                    {
                        MergeGroups(guid, *itr, compatibleDungeons);
                    }
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
    {
        return false;
    }
    else
    {
        // make sure we don't have too many players of a certain role here
        if (((NORMAL_DAMAGE_COUNT - groupOne->neededDps) + (NORMAL_DAMAGE_COUNT - groupTwo->neededDps)) > NORMAL_DAMAGE_COUNT)
        {
            return false;
        }
        else if (((NORMAL_TANK_OR_HEALER_COUNT - groupOne->neededHealers) + (NORMAL_TANK_OR_HEALER_COUNT - groupTwo->neededHealers)) > NORMAL_TANK_OR_HEALER_COUNT)
        {
            return false;
        }
        else if (((NORMAL_TANK_OR_HEALER_COUNT - groupOne->neededTanks) + (NORMAL_TANK_OR_HEALER_COUNT - groupTwo->neededTanks)) > NORMAL_TANK_OR_HEALER_COUNT)
        {
            return false;
        }
        else
        {
            return true; // the player/role counts line up!
        }
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
    Player* pPlayer1 = sPlayerRegistry.Find(it1->first);
    Player* pPlayer2 = sPlayerRegistry.Find(it2->first);

    // todo: disable this if a config option is set
    if (pPlayer1->GetTeamId() == pPlayer2->GetTeamId())
    {
        return true;
    }

    return false;
}

void LFGMgr::MergeGroups(ObjectGuid guidOne, ObjectGuid guidTwo, std::set<uint32> compatibleDungeons)
{
    // merge into the entry for rawGuidOne, then see if they are
    // able to enter the dungeon at this point or not
    LFGPlayers* mainGroup   = GetPlayerOrPartyData(guidOne);
    LFGPlayers* bufferGroup = GetPlayerOrPartyData(guidTwo);

    if (!mainGroup || !bufferGroup)
    {
        return;
    }

    // update the dungeon selection with the compatible ones
    mainGroup->dungeonList.clear();
    mainGroup->dungeonList = compatibleDungeons;

    // move players / roles into a single roleMap
    for (roleMap::iterator it = bufferGroup->currentRoles.begin(); it != bufferGroup->currentRoles.end(); ++it)
    {
        mainGroup->currentRoles[it->first] = it->second;
    }

    // update the role count / needed role info
    UpdateNeededRoles(guidOne, mainGroup);

    // being safe
    //mainGroup = GetPlayerOrPartyData(rawGuidOne);

    // Then do the following:
    if ((mainGroup->neededTanks == 0) && (mainGroup->neededHealers == 0) && (mainGroup->neededDps == 0))
    {
        SendDungeonProposal(mainGroup);
    }

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
                if (Player* pPlayer = sPlayerRegistry.Find(rItr->first))
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
                        default:
                            playerWaitTime = m_avgWaitTime[dungeonId].time;
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
    {
        return dungeon->Entry();
    }
    else
    {
        return 0;
    }
}




















