/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file LFGMgrQueue.cpp
 * @brief Cohesion split of LFGMgr.cpp -- queue join/leave and player/proposal
 *        data accessors: join/leave LFG, join-result and status queries, and
 *        player comment/state/update-type setters. Same `LFGMgr` class; no
 *        behaviour change.
 */

#include <set>
#include <string>
#include <vector>
#include "DBCEnums.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "LFGLogic.h"
#include "LFGMgr.h"
#include "Object.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"

void LFGMgr::JoinLFG(uint32 roles, std::set<uint32> dungeons, std::string comments, Player* plr)
{
    if (!plr)
    {
        return;
    }

    Group* pGroup = plr->GetGroup();
    ObjectGuid guid = (pGroup) ? pGroup->GetObjectGuid() : plr->GetObjectGuid();

    LFGPlayers* currentInfo = GetPlayerOrPartyData(guid);
    if (currentInfo)
    {
        bool groupCurrentlyInDungeon = pGroup && pGroup->isLFGGroup() && currentInfo->currentState != LFG_STATE_FINISHED_DUNGEON;
        if (currentInfo->currentState == LFG_STATE_QUEUED)
        {
            RemoveFromQueue(guid);
        }

        if (groupCurrentlyInDungeon && !currentInfo->dungeonList.empty())
        {
            dungeons = currentInfo->dungeonList;
        }
    }

    LfgJoinResult result = GetJoinResult(plr);
    std::set<uint32> requestedDungeons = dungeons;
    std::set<uint32> candidateDungeons;
    uint32 randomDungeonID = 0;
    LfgDungeonsEntry const* randomCategory = NULL;

    if (result == ERR_LFG_OK && requestedDungeons.empty())
    {
        result = ERR_LFG_INVALID_SLOT;
    }
    else if (result == ERR_LFG_OK && (roles & (PLAYER_ROLE_TANK | PLAYER_ROLE_HEALER | PLAYER_ROLE_DAMAGE)) == 0)
    {
        result = ERR_LFG_ROLE_CHECK_FAILED;
    }

    if (result == ERR_LFG_OK)
    {
        for (std::set<uint32>::const_iterator it = requestedDungeons.begin(); it != requestedDungeons.end(); ++it)
        {
            LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*it);
            if (!dungeon)
            {
                result = ERR_LFG_NO_LFG_OBJECT;
                break;
            }

            bool const isActual = dungeon->TypeID == LFG_TYPE_DUNGEON ||
                dungeon->TypeID == LFG_TYPE_HEROIC_DUNGEON;
            if (!isActual)
            {
                bool const isCategory = dungeon->TypeID == LFG_TYPE_RANDOM_DUNGEON ||
                    IsSeasonal(dungeon->Flags);
                if (!isCategory || requestedDungeons.size() != 1)
                {
                    result = ERR_LFG_INVALID_SLOT;
                    break;
                }

                randomCategory = dungeon;
                randomDungeonID = dungeon->ID;
                continue;
            }

            if (randomCategory)
            {
                result = ERR_LFG_MISMATCHED_SLOTS;
                break;
            }

            MapEntry const* map = dungeon->MapID > 0 ?
                sMapStore.LookupEntry(uint32(dungeon->MapID)) : NULL;
            if (!map || !map->IsNonRaidDungeon())
            {
                result = ERR_LFG_INVALID_SLOT;
                break;
            }

            if (!IsSeasonal(dungeon->Flags) || IsSeasonActive(dungeon->ID))
            {
                candidateDungeons.insert(dungeon->ID);
            }
        }
    }

    if (result == ERR_LFG_OK && randomCategory)
    {
        std::vector<LFGLogic::DungeonCandidate> candidates;
        if (!IsSeasonal(randomCategory->Flags) || IsSeasonActive(randomCategory->ID))
        {
            for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
            {
                LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(id);
                if (!dungeon || (IsSeasonal(dungeon->Flags) && !IsSeasonActive(dungeon->ID)))
                {
                    continue;
                }

                bool const isActual = dungeon->TypeID == LFG_TYPE_DUNGEON ||
                    dungeon->TypeID == LFG_TYPE_HEROIC_DUNGEON;
                MapEntry const* map = dungeon->MapID > 0 ?
                    sMapStore.LookupEntry(uint32(dungeon->MapID)) : NULL;
                candidates.push_back({dungeon->ID, dungeon->Group_ID,
                    dungeon->MapID, dungeon->Difficulty, !isActual,
                    map && map->IsNonRaidDungeon(), map && map->Instanceable()});
            }

            candidateDungeons = LFGLogic::FilterRandomCandidates(
                randomCategory->Group_ID, candidates);
        }
    }

    partyForbidden partyLockedDungeons;
    if (result == ERR_LFG_OK)
    {
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
                        candidateDungeons.erase(dungeonID);
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
                candidateDungeons.erase(dungeonID);
            }
        }

        if (!candidateDungeons.empty())
        {
            partyLockedDungeons.clear();
        }
        else
        {
            result = (pGroup) ? ERR_LFG_NO_SLOTS_PARTY : ERR_LFG_NO_SLOTS_PLAYER;
        }
    }

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
        roleCheck.dungeonList = candidateDungeons;
        roleCheck.randomDungeonID = randomDungeonID;
        roleCheck.leaderGuidRaw = leaderGuid.GetRawValue();
        roleCheck.waitForRoleTime = time_t(time(NULL) + LFG_TIME_ROLECHECK);

        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            if (Player* pGroupPlr = itr->getSource())
            {
                ObjectGuid plrGuid = pGroupPlr->GetObjectGuid();
                LFGPlayerStatus overallStatus(LFG_STATE_ROLECHECK,
                    LFG_UPDATE_JOIN, requestedDungeons, comments);

                pGroupPlr->GetSession()->SendLfgUpdate(true, overallStatus);
                roleCheck.currentRoles[plrGuid] = plrGuid == leaderGuid ? uint8(roles) : PLAYER_ROLE_NONE;
                roleCheck.randomDungeonByPlayer[plrGuid] = randomDungeonID;
                m_playerStatusMap[plrGuid] = overallStatus;
                m_playerQueueOwners[plrGuid] = guid;
            }
        }

        m_roleCheckMap[guid] = roleCheck;
        queueSourceMap sourceUnits;
        LFGPlayers groupInfo(LFG_STATE_ROLECHECK, candidateDungeons,
            roleCheck.currentRoles, comments, true, time(NULL), 0, 0, 0,
            roleCheck.randomDungeonByPlayer, plr->GetTeamId(), sourceUnits);
        m_playerData[guid] = groupInfo;

        if (!TransitionQueueUnit(guid, LFG_STATE_ROLECHECK, LFG_UPDATE_JOIN))
        {
            m_roleCheckMap.erase(guid);
            m_playerData.erase(guid);
            for (roleMap::const_iterator it = roleCheck.currentRoles.begin(); it != roleCheck.currentRoles.end(); ++it)
            {
                m_playerQueueOwners.erase(it->first);
            }
            plr->GetSession()->SendLfgJoinResult(ERR_LFG_MEMBERS_NOT_PRESENT,
                LFG_STATE_NONE, partyLockedDungeons);
            return;
        }

        PerformRoleCheck(plr, pGroup, (uint8)roles);
    }
    else
    {
        roleMap playerRole;
        playerRole[guid] = (uint8)roles;
        playerDungeonMap randomDungeonByPlayer;
        randomDungeonByPlayer[guid] = randomDungeonID;
        time_t const joinedTime = time(NULL);
        queueSourceMap sourceUnits;
        sourceUnits[guid] = LFGQueueSource(guid, candidateDungeons,
            randomDungeonByPlayer, playerRole, comments, plr->GetTeamId(),
            false, joinedTime);
        LFGPlayers playerInfo(LFG_STATE_QUEUED, candidateDungeons,
            playerRole, comments, false, joinedTime, 0, 0, 0,
            randomDungeonByPlayer, plr->GetTeamId(), sourceUnits);
        m_playerData[guid] = playerInfo;
        m_playerQueueOwners[guid] = guid;

        LFGPlayerStatus plrStatus(LFG_STATE_NONE, LFG_UPDATE_JOIN,
            requestedDungeons, comments);

        plr->GetSession()->SendLfgJoinResult(result, LFG_STATE_NONE, partyLockedDungeons);
        plr->GetSession()->SendLfgUpdate(false, plrStatus);
        m_playerStatusMap[guid] = plrStatus;
        AddToQueue(guid);
    }
}

void LFGMgr::LeaveLFG(Player* plr, bool isGroup)
{
    if (!plr)
    {
        return;
    }

    if (isGroup)
    {
        Group* pGroup = plr->GetGroup();
        if (!pGroup)
        {
            return;
        }

        ObjectGuid grpGuid = pGroup->GetObjectGuid();
        roleCheckMap::iterator roleCheckItr = m_roleCheckMap.find(grpGuid);
        bool const abortedRoleCheck = roleCheckItr != m_roleCheckMap.end();
        if (abortedRoleCheck)
        {
            PerformRoleCheck(NULL, pGroup, 0);
        }
        else
        {
            TransitionQueueUnit(grpGuid, LFG_STATE_NONE, LFG_UPDATE_LEAVE);
        }

        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            if (Player* pGroupPlr = itr->getSource())
            {
                ObjectGuid grpPlrGuid = pGroupPlr->GetObjectGuid();
                LFGPlayerStatus grpPlrStatus = GetPlayerStatus(grpPlrGuid);
                if (!abortedRoleCheck && (grpPlrStatus.state == LFG_STATE_NONE ||
                    grpPlrStatus.state == LFG_STATE_QUEUED ||
                    grpPlrStatus.state == LFG_STATE_PROPOSAL))
                {
                    SendLfgUpdate(grpPlrGuid, grpPlrStatus, true);
                }
                m_playerQueueOwners.erase(grpPlrGuid);
                m_playerStatusMap.erase(grpPlrGuid);
            }
        }

        RemoveFromQueue(grpGuid);
        m_roleCheckMap.erase(grpGuid);
        m_playerData.erase(grpGuid);
    }
    else
    {
        ObjectGuid plrGuid = plr->GetObjectGuid();

        if (TransitionQueueUnit(plrGuid, LFG_STATE_NONE, LFG_UPDATE_LEAVE))
        {
            SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), false);
        }

        RemoveFromQueue(plrGuid);
        m_playerData.erase(plrGuid);
        m_playerQueueOwners.erase(plrGuid);
        m_playerStatusMap.erase(plrGuid);
    }
}

LFGPlayers* LFGMgr::GetPlayerOrPartyData(ObjectGuid guid)
{
    playerData::iterator it = m_playerData.find(guid);
    if (it != m_playerData.end())
    {
        return &(it->second);
    }
    else
    {
        return NULL;
    }
}

LFGProposal* LFGMgr::GetProposalData(uint32 proposalID)
{
    proposalMap::iterator it = m_proposalMap.find(proposalID);
    if (it != m_proposalMap.end())
    {
        return &(it->second);
    }
    else
    {
        return NULL;
    }
}

LfgJoinResult LFGMgr::GetJoinResult(Player* plr)
{
    if (!plr)
    {
        return ERR_LFG_NO_LFG_OBJECT;
    }

    Group* pGroup = plr->GetGroup();
    if (pGroup && pGroup->GetMembersCount() > 5)
    {
        return ERR_LFG_TOO_MANY_MEMBERS;
    }

    auto playerResult = [](Player* player, bool party) -> LfgJoinResult
    {
        if (player->getLevel() < 15)
        {
            return ERR_LFG_CANT_USE_DUNGEONS;
        }
        if (player->HasAura(LFG_DESERTER_SPELL))
        {
            return party ? ERR_LFG_DESERTER_PARTY : ERR_LFG_DESERTER_PLAYER;
        }
        if (player->InBattleGround() || player->InBattleGroundQueue() || player->InArena())
        {
            return ERR_LFG_CANT_USE_DUNGEONS;
        }
        if (player->HasAura(LFG_COOLDOWN_SPELL))
        {
            return party ? ERR_LFG_RANDOM_COOLDOWN_PARTY : ERR_LFG_RANDOM_COOLDOWN_PLAYER;
        }
        return ERR_LFG_OK;
    };

    if (!pGroup)
    {
        return playerResult(plr, false);
    }

    std::vector<uint32> results;
    uint32 currentMemberCount = 0;
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            results.push_back(uint32(playerResult(pGroupPlr, true)));
            ++currentMemberCount;
        }
    }
    if (currentMemberCount != pGroup->GetMembersCount())
    {
        results.push_back(uint32(ERR_LFG_MEMBERS_NOT_PRESENT));
    }

    return LfgJoinResult(LFGLogic::FirstFailure(results, uint32(ERR_LFG_OK)));
}

LFGPlayerStatus LFGMgr::GetPlayerStatus(ObjectGuid guid)
{
    LFGPlayerStatus status;

    playerStatusMap::iterator it = m_playerStatusMap.find(guid);
    if (it != m_playerStatusMap.end())
    {
        status = it->second;
    }

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

bool LFGMgr::TransitionQueueUnit(ObjectGuid ownerGuid, LFGState state,
    LfgUpdateType updateType)
{
    playerData::iterator dataItr = m_playerData.find(ownerGuid);
    if (dataItr == m_playerData.end() || dataItr->second.currentRoles.empty())
    {
        return false;
    }

    for (roleMap::const_iterator itr = dataItr->second.currentRoles.begin();
        itr != dataItr->second.currentRoles.end(); ++itr)
    {
        if (m_playerStatusMap.find(itr->first) == m_playerStatusMap.end())
        {
            return false;
        }
    }

    dataItr->second.currentState = state;
    for (roleMap::const_iterator itr = dataItr->second.currentRoles.begin();
        itr != dataItr->second.currentRoles.end(); ++itr)
    {
        LFGPlayerStatus& status = m_playerStatusMap[itr->first];
        status.state = state;
        status.updateType = updateType;
    }
    return true;
}

bool LFGMgr::TransitionPlayer(ObjectGuid playerGuid, LFGState state,
    LfgUpdateType updateType)
{
    playerStatusMap::iterator itr = m_playerStatusMap.find(playerGuid);
    if (itr == m_playerStatusMap.end())
    {
        return false;
    }

    itr->second.state = state;
    itr->second.updateType = updateType;
    return true;
}
