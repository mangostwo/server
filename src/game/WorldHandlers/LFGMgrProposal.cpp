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
 * @file LFGMgrProposal.cpp
 * @brief Cohesion split of LFGMgr.cpp -- proposal and role-check flow: dungeon
 *        group creation, role validation/votes, proposal updates, kick votes,
 *        teleport to dungeon and the related LFG update senders. Same `LFGMgr`
 *        class; no behaviour change.
 */

#include <set>
#include <string>
#include <utility>
#include <vector>
#include "Common/TimeConstants.h"
#include "DBCEnums.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "LFGLogic.h"
#include "LFGMgr.h"
#include "Object.h"
#include "Player.h"
#include "PlayerRegistry.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "Util.h"
#include "WorldSession.h"

// called each time a player selects their role
void LFGMgr::PerformRoleCheck(Player* pPlayer, Group* pGroup, uint8 roles)
{
    if (!pGroup)
    {
        return;
    }

    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    ObjectGuid plrGuid = pPlayer? pPlayer->GetObjectGuid() : ObjectGuid();

    roleCheckMap::iterator it = m_roleCheckMap.find(groupGuid);
    if (it == m_roleCheckMap.end())
    {
        return; // no role check map found
    }

    LFGRoleCheck& roleCheck = it->second;
    bool roleChosen = bool(plrGuid);

    if (!plrGuid)
    {
        roleCheck.state = LFG_ROLECHECK_ABORTED;
    }
    else if (roleCheck.currentRoles.find(plrGuid) == roleCheck.currentRoles.end())
    {
        return;
    }
    else if ((roles & (PLAYER_ROLE_TANK | PLAYER_ROLE_HEALER | PLAYER_ROLE_DAMAGE)) == 0)
    {
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
    }
    else
    {
        roleCheck.currentRoles[plrGuid] = roles;

        std::vector<LFGLogic::RoleRequest> requests;
        requests.reserve(roleCheck.currentRoles.size());
        for (roleMap::const_iterator rItr = roleCheck.currentRoles.begin();
            rItr != roleCheck.currentRoles.end(); ++rItr)
        {
            requests.push_back({rItr->first.GetRawValue(), rItr->second});
        }

        if (LFGLogic::AllRolesAnswered(requests))
        {
            roleCheck.state = ValidateGroupRoles(roleCheck.currentRoles) ? LFG_ROLECHECK_FINISHED : LFG_ROLECHECK_MISSING_ROLE;
        }
    }

    bool queued = false;
    if (roleCheck.state == LFG_ROLECHECK_FINISHED)
    {
        LFGPlayers* queueInfo = GetPlayerOrPartyData(groupGuid);
        if (!queueInfo)
        {
            roleCheck.state = LFG_ROLECHECK_ABORTED;
        }
        else
        {
            queueInfo->currentRoles = roleCheck.currentRoles;
            queueInfo->dungeonList = roleCheck.dungeonList;
            queueInfo->randomDungeonByPlayer = roleCheck.randomDungeonByPlayer;
            queueInfo->joinedTime = time(NULL);
            queueInfo->sourceUnits.clear();
            queueInfo->sourceUnits[groupGuid] = LFGQueueSource(groupGuid,
                queueInfo->dungeonList, queueInfo->randomDungeonByPlayer,
                queueInfo->currentRoles, queueInfo->comments, queueInfo->team,
                true, queueInfo->joinedTime);
            m_playerData[groupGuid] = *queueInfo;
            AddToQueue(groupGuid);
            queued = m_queueSet.find(groupGuid) != m_queueSet.end() ||
                m_ownerProposalIds.find(groupGuid) != m_ownerProposalIds.end();
            if (!queued)
            {
                roleCheck.state = LFG_ROLECHECK_ABORTED;
            }
        }
    }

    bool const terminal = roleCheck.state != LFG_ROLECHECK_INITIALITING;
    if (terminal && !queued)
    {
        LfgUpdateType const updateType = roleCheck.state == LFG_ROLECHECK_ABORTED ?
            LFG_UPDATE_ROLECHECK_ABORTED : LFG_UPDATE_ROLECHECK_FAILED;
        TransitionQueueUnit(groupGuid, LFG_STATE_NONE, updateType);
    }

    partyForbidden nullForbidden;
    for (roleMap::const_iterator itr = roleCheck.currentRoles.begin();
        itr != roleCheck.currentRoles.end(); ++itr)
    {
        ObjectGuid guidBuff = itr->first;
        if (roleChosen)
        {
            SendRoleChosen(guidBuff, plrGuid, roles); // send SMSG_LFG_ROLE_CHOSEN to each player
        }

        SendRoleCheckUpdate(guidBuff, roleCheck);
        if (!terminal)
        {
            continue;
        }

        if (!queued && roleCheck.leaderGuidRaw == guidBuff.GetRawValue())
        {
            SendLfgJoinResult(guidBuff, ERR_LFG_ROLE_CHECK_FAILED,
                LFG_STATE_ROLECHECK, nullForbidden);
        }
        SendLfgUpdate(guidBuff, GetPlayerStatus(guidBuff), true);
    }

    if (!terminal)
    {
        return;
    }

    if (!queued)
    {
        for (roleMap::const_iterator roleItr = roleCheck.currentRoles.begin();
            roleItr != roleCheck.currentRoles.end(); ++roleItr)
        {
            m_playerQueueOwners.erase(roleItr->first);
        }
        m_playerData.erase(groupGuid);
    }
    m_roleCheckMap.erase(it);
}

bool LFGMgr::ValidateGroupRoles(roleMap groupMap)
{
    std::vector<LFGLogic::RoleRequest> requests;
    requests.reserve(groupMap.size());
    for (roleMap::const_iterator it = groupMap.begin(); it != groupMap.end(); ++it)
    {
        requests.push_back({it->first.GetRawValue(), it->second});
    }

    std::vector<LFGLogic::RoleAssignment> assignments;
    LFGLogic::RoleNeeds needs;
    return LFGLogic::ResolveRoles(requests, assignments, needs);
}

bool LFGMgr::BeginProposal(ObjectGuid ownerGuid)
{
    playerData::iterator dataItr = m_playerData.find(ownerGuid);
    if (dataItr == m_playerData.end() ||
        dataItr->second.currentState != LFG_STATE_QUEUED ||
        dataItr->second.dungeonList.empty() ||
        dataItr->second.sourceUnits.empty())
    {
        return false;
    }

    LFGPlayers const aggregate = dataItr->second;
    for (queueSourceMap::const_iterator sourceItr = aggregate.sourceUnits.begin();
        sourceItr != aggregate.sourceUnits.end(); ++sourceItr)
    {
        LFGQueueSource const& source = sourceItr->second;
        for (roleMap::const_iterator roleItr = source.selectedRoles.begin();
            roleItr != source.selectedRoles.end(); ++roleItr)
        {
            Player* player = sPlayerRegistry.Find(roleItr->first);
            if (!player || (source.isGroup && (!player->GetGroup() ||
                player->GetGroup()->GetObjectGuid() != source.ownerGuid)) ||
                (!source.isGroup && player->GetGroup()))
            {
                return false;
            }
        }
    }

    std::vector<LFGLogic::RoleRequest> requests;
    requests.reserve(aggregate.currentRoles.size());
    for (roleMap::const_iterator itr = aggregate.currentRoles.begin();
        itr != aggregate.currentRoles.end(); ++itr)
    {
        requests.push_back({itr->first.GetRawValue(), itr->second});
        if (m_playerStatusMap.find(itr->first) == m_playerStatusMap.end() ||
            !sPlayerRegistry.Find(itr->first))
        {
            return false;
        }
    }

    std::vector<LFGLogic::RoleAssignment> assignments;
    LFGLogic::RoleNeeds needs;
    bool const testingSolo = m_testing && !aggregate.isGroup &&
        requests.size() == 1 && !aggregate.dungeonList.empty() &&
        LFGLogic::AllRolesAnswered(requests);
    // TEMPORARY LFD SMOKE TEST: production still requires the full 1/1/3 party.
    if (!LFGLogic::ResolveRoles(requests, assignments, needs) ||
        !LFGLogic::IsProposalReady(requests.size(), testingSolo, needs))
    {
        return false;
    }

    std::set<uint32> proposalCandidates;
    for (std::set<uint32>::const_iterator itr = aggregate.dungeonList.begin();
        itr != aggregate.dungeonList.end(); ++itr)
    {
        LfgDungeonsEntry const* candidate = sLfgDungeonsStore.LookupEntry(*itr);
        MapEntry const* candidateMap = candidate && candidate->MapID > 0 ?
            sMapStore.LookupEntry(uint32(candidate->MapID)) : NULL;
        if (candidate && (candidate->TypeID == LFG_TYPE_DUNGEON ||
            candidate->TypeID == LFG_TYPE_HEROIC_DUNGEON) && candidateMap &&
            candidateMap->IsNonRaidDungeon() &&
            (!IsSeasonal(candidate->Flags) || IsSeasonActive(candidate->ID)))
        {
            proposalCandidates.insert(candidate->ID);
        }
    }
    if (proposalCandidates.empty())
    {
        return false;
    }

    uint32 dungeonID = 0;
    std::size_t const selectedIndex = urand(0,
        uint32(proposalCandidates.size() - 1));
    if (!LFGLogic::SelectCandidate(proposalCandidates, selectedIndex,
        dungeonID))
    {
        return false;
    }

    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonID);
    MapEntry const* map = dungeon && dungeon->MapID > 0 ?
        sMapStore.LookupEntry(uint32(dungeon->MapID)) : NULL;
    if (!dungeon || (dungeon->TypeID != LFG_TYPE_DUNGEON &&
        dungeon->TypeID != LFG_TYPE_HEROIC_DUNGEON) ||
        !map || !map->IsNonRaidDungeon())
    {
        return false;
    }

    LFGProposal newProposal;
    newProposal.id = ++m_proposalId;
    newProposal.dungeonID = dungeonID;
    newProposal.isNew = true;
    newProposal.randomDungeonByPlayer = aggregate.randomDungeonByPlayer;
    newProposal.sourceUnits = aggregate.sourceUnits;
    newProposal.joinedQueue = aggregate.joinedTime;
    newProposal.expiresAt = time(NULL) + LFG_TIME_PROPOSAL;

    for (std::vector<LFGLogic::RoleAssignment>::const_iterator itr = assignments.begin();
        itr != assignments.end(); ++itr)
    {
        newProposal.currentRoles[ObjectGuid(itr->playerGuid)] = itr->assignedRole;
    }

    ObjectGuid reusableGroupGuid;
    ObjectGuid reusableGroupLeader;
    bool conflictingGroups = false;
    for (roleMap::const_iterator itr = newProposal.currentRoles.begin();
        itr != newProposal.currentRoles.end(); ++itr)
    {
        ObjectGuid const plrGuid = itr->first;
        Player* pPlayer = sPlayerRegistry.Find(plrGuid);
        if (Group* pGroup = pPlayer->GetGroup())
        {
            ObjectGuid const groupGuid = pGroup->GetObjectGuid();
            newProposal.groups[plrGuid] = groupGuid;
            if (!reusableGroupGuid)
            {
                reusableGroupGuid = groupGuid;
                reusableGroupLeader = pGroup->GetLeaderGuid();
            }
            else if (reusableGroupGuid != groupGuid)
            {
                conflictingGroups = true;
            }
        }
        else
        {
            newProposal.groups[plrGuid] = ObjectGuid();
        }
        newProposal.answers[plrGuid] = LFG_ANSWER_PENDING;
    }

    ObjectGuid leaderGuid;
    if (reusableGroupGuid && !conflictingGroups &&
        newProposal.currentRoles.find(reusableGroupLeader) !=
            newProposal.currentRoles.end())
    {
        newProposal.groupRawGuid = reusableGroupGuid.GetRawValue();
        leaderGuid = reusableGroupLeader;
    }
    else
    {
        for (roleMap::const_iterator itr = aggregate.currentRoles.begin();
            itr != aggregate.currentRoles.end(); ++itr)
        {
            if ((itr->second & PLAYER_ROLE_LEADER) != 0 &&
                (!leaderGuid || itr->first.GetRawValue() < leaderGuid.GetRawValue()))
            {
                leaderGuid = itr->first;
            }
        }
        if (!leaderGuid)
        {
            for (roleMap::const_iterator itr = newProposal.currentRoles.begin();
                itr != newProposal.currentRoles.end(); ++itr)
            {
                if (!leaderGuid || itr->first.GetRawValue() < leaderGuid.GetRawValue())
                {
                    leaderGuid = itr->first;
                }
            }
        }
    }
    if (!leaderGuid)
    {
        return false;
    }
    newProposal.groupLeaderGuid = leaderGuid.GetRawValue();
    newProposal.currentRoles[leaderGuid] |= PLAYER_ROLE_LEADER;

    m_queueSet.erase(ownerGuid);
    m_playerData.erase(dataItr);
    for (queueSourceMap::const_iterator sourceItr = newProposal.sourceUnits.begin();
        sourceItr != newProposal.sourceUnits.end(); ++sourceItr)
    {
        m_ownerProposalIds[sourceItr->first] = newProposal.id;
        for (roleMap::const_iterator roleItr = sourceItr->second.selectedRoles.begin();
            roleItr != sourceItr->second.selectedRoles.end(); ++roleItr)
        {
            m_playerQueueOwners[roleItr->first] = sourceItr->first;
        }
    }
    m_proposalMap[newProposal.id] = newProposal;

    LFGProposal const& storedProposal = m_proposalMap[newProposal.id];
    for (roleMap::const_iterator itr = storedProposal.currentRoles.begin();
        itr != storedProposal.currentRoles.end(); ++itr)
    {
        ObjectGuid const plrGuid = itr->first;
        TransitionPlayer(plrGuid, LFG_STATE_PROPOSAL,
            LFG_UPDATE_PROPOSAL_BEGIN);
        SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid),
            bool(storedProposal.groups.find(plrGuid)->second));
        if (Player* pPlayer = sPlayerRegistry.Find(plrGuid))
        {
            pPlayer->GetSession()->SendLfgProposalUpdate(storedProposal);
        }
    }
    return true;
}

bool LFGMgr::RestoreQueueSource(LFGQueueSource const& source)
{
    if (!source.ownerGuid || source.dungeonList.empty() ||
        source.selectedRoles.empty())
    {
        return false;
    }

    std::set<uint32> candidates = source.dungeonList;
    for (roleMap::const_iterator itr = source.selectedRoles.begin();
        itr != source.selectedRoles.end(); ++itr)
    {
        Player* player = sPlayerRegistry.Find(itr->first);
        if (!player || m_playerStatusMap.find(itr->first) ==
            m_playerStatusMap.end() || GetJoinResult(player) != ERR_LFG_OK)
        {
            return false;
        }
        if (source.isGroup && (!player->GetGroup() ||
            player->GetGroup()->GetObjectGuid() != source.ownerGuid))
        {
            return false;
        }
        if (!source.isGroup && player->GetGroup())
        {
            return false;
        }

        dungeonForbidden const locked = FindRandomDungeonsNotForPlayer(player);
        for (dungeonForbidden::const_iterator lockedItr = locked.begin();
            lockedItr != locked.end(); ++lockedItr)
        {
            candidates.erase(lockedItr->first & 0x00FFFFFF);
        }
    }
    if (candidates.empty())
    {
        return false;
    }

    LFGQueueSource restoredSource = source;
    restoredSource.dungeonList = candidates;
    queueSourceMap sources;
    sources[source.ownerGuid] = restoredSource;
    LFGPlayers restored(LFG_STATE_QUEUED, candidates, source.selectedRoles,
        source.comment, source.isGroup, source.joinedTime, 0, 0, 0,
        source.randomDungeonByPlayer, source.team, sources);
    m_playerData[source.ownerGuid] = restored;
    for (roleMap::const_iterator itr = source.selectedRoles.begin();
        itr != source.selectedRoles.end(); ++itr)
    {
        m_playerQueueOwners[itr->first] = source.ownerGuid;
    }

    AddToQueue(source.ownerGuid);
    if (m_queueSet.find(source.ownerGuid) == m_queueSet.end())
    {
        m_playerData.erase(source.ownerGuid);
        for (roleMap::const_iterator itr = source.selectedRoles.begin();
            itr != source.selectedRoles.end(); ++itr)
        {
            m_playerQueueOwners.erase(itr->first);
        }
        return false;
    }

    for (roleMap::const_iterator itr = source.selectedRoles.begin();
        itr != source.selectedRoles.end(); ++itr)
    {
        SendLfgUpdate(itr->first, GetPlayerStatus(itr->first), source.isGroup);
    }
    return true;
}

void LFGMgr::UnwindProposal(uint32 proposalId,
    std::set<ObjectGuid> const& failedPlayers)
{
    proposalMap::iterator proposalItr = m_proposalMap.find(proposalId);
    if (proposalItr == m_proposalMap.end())
    {
        return;
    }

    LFGProposal proposal = std::move(proposalItr->second);
    m_proposalMap.erase(proposalItr);
    proposal.state = LFG_PROPOSAL_FAILED;

    for (queueSourceMap::const_iterator sourceItr = proposal.sourceUnits.begin();
        sourceItr != proposal.sourceUnits.end(); ++sourceItr)
    {
        m_ownerProposalIds.erase(sourceItr->first);
        for (roleMap::const_iterator roleItr = sourceItr->second.selectedRoles.begin();
            roleItr != sourceItr->second.selectedRoles.end(); ++roleItr)
        {
            m_playerQueueOwners.erase(roleItr->first);
        }
    }

    for (proposalAnswerMap::const_iterator answerItr = proposal.answers.begin();
        answerItr != proposal.answers.end(); ++answerItr)
    {
        if (Player* player = sPlayerRegistry.Find(answerItr->first))
        {
            player->GetSession()->SendLfgProposalUpdate(proposal);
        }
    }

    for (queueSourceMap::const_iterator sourceItr = proposal.sourceUnits.begin();
        sourceItr != proposal.sourceUnits.end(); ++sourceItr)
    {
        LFGQueueSource const& source = sourceItr->second;
        bool sourceFailed = false;
        bool sourceDeclined = false;
        for (roleMap::const_iterator roleItr = source.selectedRoles.begin();
            roleItr != source.selectedRoles.end(); ++roleItr)
        {
            if (failedPlayers.find(roleItr->first) != failedPlayers.end())
            {
                sourceFailed = true;
            }
            proposalAnswerMap::const_iterator answerItr =
                proposal.answers.find(roleItr->first);
            if (answerItr != proposal.answers.end() &&
                answerItr->second == LFG_ANSWER_DENY)
            {
                sourceDeclined = true;
            }
        }

        if (!sourceFailed && RestoreQueueSource(source))
        {
            continue;
        }

        LfgUpdateType const updateType = sourceDeclined ?
            LFG_UPDATE_PROPOSAL_DECLINED : LFG_UPDATE_PROPOSAL_FAILED;
        for (roleMap::const_iterator roleItr = source.selectedRoles.begin();
            roleItr != source.selectedRoles.end(); ++roleItr)
        {
            if (TransitionPlayer(roleItr->first, LFG_STATE_NONE, updateType))
            {
                SendLfgUpdate(roleItr->first, GetPlayerStatus(roleItr->first),
                    source.isGroup);
            }
        }
    }
}

void LFGMgr::RemoveOldProposals()
{
    time_t const now = time(NULL);
    std::vector<uint32> expiredIds;
    for (proposalMap::const_iterator itr = m_proposalMap.begin();
        itr != m_proposalMap.end(); ++itr)
    {
        if (itr->second.state == LFG_PROPOSAL_INITIATING &&
            itr->second.expiresAt <= now)
        {
            expiredIds.push_back(itr->first);
        }
    }

    for (std::vector<uint32>::const_iterator idItr = expiredIds.begin();
        idItr != expiredIds.end(); ++idItr)
    {
        proposalMap::const_iterator proposalItr = m_proposalMap.find(*idItr);
        if (proposalItr == m_proposalMap.end())
        {
            continue;
        }

        std::set<ObjectGuid> failedPlayers;
        for (proposalAnswerMap::const_iterator answerItr =
            proposalItr->second.answers.begin();
            answerItr != proposalItr->second.answers.end(); ++answerItr)
        {
            if (answerItr->second == LFG_ANSWER_PENDING)
            {
                failedPlayers.insert(answerItr->first);
            }
        }
        UnwindProposal(*idItr, failedPlayers);
    }
}

// From a CMSG_LFG_PROPOSAL_RESPONSE call
void LFGMgr::ProposalUpdate(uint32 proposalID, ObjectGuid plrGuid, bool accepted)
{
    LFGProposal* proposal = GetProposalData(proposalID);
    if (!proposal || proposal->state != LFG_PROPOSAL_INITIATING)
    {
        return;
    }

    proposalAnswerMap::iterator answerItr = proposal->answers.find(plrGuid);
    if (answerItr == proposal->answers.end())
    {
        return;
    }
    LFGProposalAnswer plrAnswer = (LFGProposalAnswer)accepted;
    answerItr->second = plrAnswer;

    if (plrAnswer == LFG_ANSWER_DENY)
    {
        std::set<ObjectGuid> failedPlayers;
        failedPlayers.insert(plrGuid);
        UnwindProposal(proposalID, failedPlayers);
        return;
    }

    bool allOkay = true;
    for (proposalAnswerMap::const_iterator it = proposal->answers.begin();
        it != proposal->answers.end(); ++it)
    {
        if (it->second != LFG_ANSWER_AGREE)
        {
            allOkay = false;
        }
    }

    if (!allOkay)
    {
        for (proposalAnswerMap::const_iterator itr = proposal->answers.begin();
            itr != proposal->answers.end(); ++itr)
        {
            if (Player* pProposalPlayer = sPlayerRegistry.Find(itr->first))
            {
                pProposalPlayer->GetSession()->SendLfgProposalUpdate(*proposal);
            }
        }
        return;
    }

    proposal->state = LFG_PROPOSAL_SUCCESS;
    for (proposalAnswerMap::const_iterator itr = proposal->answers.begin();
        itr != proposal->answers.end(); ++itr)
    {
        if (Player* pProposalPlayer = sPlayerRegistry.Find(itr->first))
        {
            pProposalPlayer->GetSession()->SendLfgProposalUpdate(*proposal);
        }
    }

    if (!CreateDungeonGroup(proposal))
    {
        std::set<ObjectGuid> failedPlayers;
        UnwindProposal(proposalID, failedPlayers);
        return;
    }

    time_t const now = time(NULL);
    for (roleMap::const_iterator rItr = proposal->currentRoles.begin();
        rItr != proposal->currentRoles.end(); ++rItr)
    {
        ObjectGuid const proposalPlrGuid = rItr->first;
        uint8 const proposalPlrRole = rItr->second & ~PLAYER_ROLE_LEADER;
        time_t sourceJoined = proposal->joinedQueue;
        playerGroupMap::const_iterator ownerItr =
            m_playerQueueOwners.find(proposalPlrGuid);
        if (ownerItr != m_playerQueueOwners.end())
        {
            queueSourceMap::const_iterator sourceItr =
                proposal->sourceUnits.find(ownerItr->second);
            if (sourceItr != proposal->sourceUnits.end())
            {
                sourceJoined = sourceItr->second.joinedTime;
            }
        }

        UpdateWaitMap(LFGRoles(proposalPlrRole), proposal->dungeonID,
            time_t(LFGLogic::ElapsedSeconds(sourceJoined, now)));
        TransitionPlayer(proposalPlrGuid, LFG_STATE_IN_DUNGEON,
            LFG_UPDATE_GROUP_FOUND);
        if (Player* pProposalPlayer = sPlayerRegistry.Find(proposalPlrGuid))
        {
            SendLfgUpdate(proposalPlrGuid, GetPlayerStatus(proposalPlrGuid),
                bool(pProposalPlayer->GetGroup()));
        }
    }

    for (queueSourceMap::const_iterator sourceItr = proposal->sourceUnits.begin();
        sourceItr != proposal->sourceUnits.end(); ++sourceItr)
    {
        m_ownerProposalIds.erase(sourceItr->first);
        for (roleMap::const_iterator roleItr = sourceItr->second.selectedRoles.begin();
            roleItr != sourceItr->second.selectedRoles.end(); ++roleItr)
        {
            m_playerQueueOwners.erase(roleItr->first);
        }
    }
    m_proposalMap.erase(proposalID);
}

bool LFGMgr::CreateDungeonGroup(LFGProposal* proposal)
{
    if (!proposal || proposal->groups.empty() ||
        proposal->groups.size() > NORMAL_TOTAL_ROLE_COUNT)
    {
        return false;
    }

    LfgDungeonsEntry const* dungeon =
        sLfgDungeonsStore.LookupEntry(proposal->dungeonID);
    MapEntry const* map = dungeon && dungeon->MapID > 0 ?
        sMapStore.LookupEntry(uint32(dungeon->MapID)) : NULL;
    if (!dungeon || (dungeon->TypeID != LFG_TYPE_DUNGEON &&
        dungeon->TypeID != LFG_TYPE_HEROIC_DUNGEON) ||
        !map || !map->IsNonRaidDungeon())
    {
        return false;
    }

    for (playerGroupMap::const_iterator itr = proposal->groups.begin();
        itr != proposal->groups.end(); ++itr)
    {
        if (!sPlayerRegistry.Find(itr->first))
        {
            return false;
        }
    }

    ObjectGuid const leaderGuid(proposal->groupLeaderGuid);
    Player* leader = sPlayerRegistry.Find(leaderGuid);
    if (!leader)
    {
        return false;
    }

    Group* pGroup = NULL;
    bool createdGroup = false;
    if (proposal->groupRawGuid)
    {
        pGroup = leader->GetGroup();
        if (!pGroup || pGroup->GetObjectGuid().GetRawValue() !=
            proposal->groupRawGuid)
        {
            return false;
        }
    }
    else
    {
        if (Group* oldGroup = leader->GetGroup())
        {
            if (oldGroup->RemoveMember(leaderGuid, 0) <= 1)
            {
                sObjectMgr.RemoveGroup(oldGroup);
                delete oldGroup;
            }
        }
        pGroup = new Group();
        if (!pGroup->Create(leaderGuid, leader->GetName()))
        {
            delete pGroup;
            return false;
        }
        sObjectMgr.AddGroup(pGroup);
        createdGroup = true;
    }

    for (playerGroupMap::const_iterator itr = proposal->groups.begin();
        itr != proposal->groups.end(); ++itr)
    {
        if (itr->first == leaderGuid || pGroup->IsMember(itr->first))
        {
            continue;
        }

        Player* player = sPlayerRegistry.Find(itr->first);
        if (Group* oldGroup = player->GetGroup())
        {
            if (oldGroup->RemoveMember(itr->first, 0) <= 1)
            {
                sObjectMgr.RemoveGroup(oldGroup);
                delete oldGroup;
            }
        }
        if (!pGroup->AddMember(itr->first, player->GetName()))
        {
            if (createdGroup)
            {
                pGroup->Disband();
                sObjectMgr.RemoveGroup(pGroup);
                delete pGroup;
            }
            return false;
        }
    }

    pGroup->SetAsLfgGroup();
    pGroup->SetDungeonDifficulty(Difficulty(dungeon->Difficulty));
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus groupStatus(LFG_STATE_IN_DUNGEON, dungeon->ID,
        proposal->currentRoles, proposal->randomDungeonByPlayer,
        pGroup->GetLeaderGuid());

    m_groupSet.insert(groupGuid);
    m_groupStatusMap[groupGuid] = groupStatus;
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL;
        itr = itr->next())
    {
        if (Player* player = itr->getSource())
        {
            TeleportPlayer(player, false, true);
        }
    }

    pGroup->SendUpdate();
    return true;
}

void LFGMgr::TeleportPlayer(Player* pPlayer, bool out, bool automatic)
{
    if (!pPlayer)
    {
        return;
    }

    auto sendError = [pPlayer](LFGTeleportError error)
    {
        pPlayer->GetSession()->SendLfgTeleportError(uint8(error));
    };

    Group* pGroup = pPlayer->GetGroup();
    if (!pGroup)
    {
        sendError(LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    LFGGroupStatus* status = GetGroupStatus(pGroup->GetObjectGuid());
    if (!status)
    {
        sendError(LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    LfgDungeonsEntry const* dungeon =
        sLfgDungeonsStore.LookupEntry(status->dungeonID);
    MapEntry const* map = dungeon && dungeon->MapID > 0 ?
        sMapStore.LookupEntry(uint32(dungeon->MapID)) : NULL;
    bool const isActual = dungeon &&
        (dungeon->TypeID == LFG_TYPE_DUNGEON ||
            dungeon->TypeID == LFG_TYPE_HEROIC_DUNGEON);
    LFGLogic::DungeonCandidate const candidate = {
        dungeon ? dungeon->ID : 0,
        dungeon ? dungeon->Group_ID : 0,
        dungeon ? dungeon->MapID : 0,
        dungeon ? dungeon->Difficulty : 0,
        !isActual,
        map && map->IsNonRaidDungeon(),
        map && map->Instanceable()
    };
    if (!dungeon || !LFGLogic::IsTeleportTarget(candidate,
        uint32(pGroup->GetDungeonDifficulty())))
    {
        sendError(LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    uint32 const mapID = uint32(dungeon->MapID);
    if (out)
    {
        if (pPlayer->GetMapId() != mapID || !pPlayer->TeleportToBGEntryPoint())
        {
            sendError(LFG_TELEPORTERROR_INVALID_LOCATION);
        }
        return;
    }

    if (pPlayer->GetMapId() == mapID)
    {
        return;
    }
    if (pPlayer->IsDead())
    {
        sendError(LFG_TELEPORTERROR_PLAYER_DEAD);
        return;
    }
    if (pPlayer->IsFalling())
    {
        sendError(LFG_TELEPORTERROR_FALLING);
        return;
    }
    if (pPlayer->GetVehicleInfo())
    {
        sendError(LFG_TELEPORTERROR_IN_VEHICLE);
        return;
    }
    dungeonForbidden const lockedDungeons = FindRandomDungeonsNotForPlayer(pPlayer);
    if (lockedDungeons.find(dungeon->Entry()) != lockedDungeons.end())
    {
        sendError(LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    Map* currentMap = pPlayer->GetMap();
    if (!currentMap || currentMap->IsDungeon() || currentMap->IsRaid() ||
        currentMap->IsBattleGroundOrArena())
    {
        sendError(LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float o = 0.0f;
    bool destinationFound = false;
    if (automatic)
    {
        Player* destinationPlayer = sPlayerRegistry.Find(pGroup->GetLeaderGuid());
        if (!destinationPlayer || destinationPlayer->GetMapId() != mapID)
        {
            destinationPlayer = NULL;
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL;
                itr = itr->next())
            {
                Player* member = itr->getSource();
                if (member && member->GetMapId() == mapID)
                {
                    destinationPlayer = member;
                    break;
                }
            }
        }

        if (destinationPlayer)
        {
            x = destinationPlayer->Where().X();
            y = destinationPlayer->Where().Y();
            z = destinationPlayer->Where().Z();
            o = destinationPlayer->Where().Facing();
            destinationFound = true;
        }
    }

    if (!destinationFound)
    {
        AreaTrigger const* entrance = sObjectMgr.GetMapEntranceTrigger(mapID);
        if (!entrance)
        {
            sendError(LFG_TELEPORTERROR_INVALID_LOCATION);
            return;
        }
        x = entrance->target_X;
        y = entrance->target_Y;
        z = entrance->target_Z;
        o = entrance->target_Orientation;
    }

    pPlayer->SetBattleGroundEntryPoint();
    if (!pPlayer->TeleportTo(mapID, x, y, z, o))
    {
        sendError(LFG_TELEPORTERROR_INVALID_LOCATION);
    }
}

LFGGroupStatus* LFGMgr::GetGroupStatus(ObjectGuid guid)
{
    groupStatusMap::iterator it = m_groupStatusMap.find(guid);
    if (it != m_groupStatusMap.end())
    {
        return &(it->second);
    }
    else
    {
        return NULL;
    }
}

void LFGMgr::UpdateWaitMap(LFGRoles role, uint32 dungeonID, time_t waitTime)
{
    if (!dungeonID)
    {
        return;
    }

    auto updateSample = [dungeonID, waitTime](waitTimeMap& waitTimes)
    {
        waitTimeMap::iterator itr = waitTimes.find(dungeonID);
        if (itr == waitTimes.end())
        {
            return;
        }
        itr->second.previousTime = itr->second.time;
        itr->second.time = int32(waitTime);
        itr->second.doAverage = true;
    };

    switch (role)
    {
        case PLAYER_ROLE_TANK:
            updateSample(m_tankWaitTime);
            break;
        case PLAYER_ROLE_HEALER:
            updateSample(m_healerWaitTime);
            break;
        case PLAYER_ROLE_DAMAGE:
            updateSample(m_dpsWaitTime);
            break;
        default:
            break;
    }
    updateSample(m_avgWaitTime);
}

void LFGMgr::HandleBossKilled(Player* pPlayer)
{
    if (!pPlayer)
    {
        return;
    }

    Group* pGroup = pPlayer->GetGroup();
    if (!pGroup)
    {
        return;
    }

    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    if (!status)
    {
        return;
    }
    if (status->state == LFG_STATE_FINISHED_DUNGEON)
    {
        return;
    }
    if (m_bootStatusMap.find(groupGuid) != m_bootStatusMap.end())
    {
        FinishBootVote(groupGuid, false);
        status = GetGroupStatus(groupGuid);
        if (!status)
        {
            return;
        }
    }

    status->state = LFG_STATE_FINISHED_DUNGEON;
    for (roleMap::const_iterator roleItr = status->playerRoles.begin();
        roleItr != status->playerRoles.end(); ++roleItr)
    {
        TransitionPlayer(roleItr->first, LFG_STATE_FINISHED_DUNGEON,
            LFG_UPDATE_STATUS);
    }
    pGroup->SendUpdate();

    DungeonTypes type = GetDungeonType(status->dungeonID);
    for (roleMap::const_iterator roleItr = status->playerRoles.begin();
        roleItr != status->playerRoles.end(); ++roleItr)
    {
        Player* pGroupPlr = sPlayerRegistry.Find(roleItr->first);
        if (!pGroupPlr)
        {
            continue;
        }

        uint32 randomDungeonId = 0;
        playerDungeonMap::const_iterator randomItr =
            status->randomDungeonByPlayer.find(roleItr->first);
        if (randomItr != status->randomDungeonByPlayer.end())
        {
            randomDungeonId = randomItr->second;
        }

        bool const hasDoneDaily = HasPlayerDoneDaily(pGroupPlr->GetGUIDLow(), type);
        DungeonFinderRewards const* rewards =
            sObjectMgr.GetDungeonFinderRewards(pGroupPlr->getLevel());
        if (!rewards)
        {
            RegisterPlayerDaily(pGroupPlr->GetGUIDLow(), type);
            continue;
        }
        int32 const multiplier = hasDoneDaily ? 1 : 2;
        uint32 const xpReward = multiplier * rewards->baseXPReward;
        uint32 const moneyReward = uint32(multiplier * rewards->baseMonetaryReward);
        ItemRewards const itemRewards = GetDungeonItemRewards(status->dungeonID, type);
        uint32 itemReward = 0;
        uint32 itemAmount = 0;
        if (hasDoneDaily && type == DUNGEON_WOTLK_HEROIC)
        {
            itemReward = WOTLK_SPECIAL_HEROIC_ITEM;
            itemAmount = WOTLK_SPECIAL_HEROIC_AMNT;
        }
        else if (!hasDoneDaily)
        {
            itemReward = itemRewards.itemId;
            itemAmount = itemRewards.itemAmount;
        }

        LFGRewards reward(randomDungeonId, status->dungeonID, hasDoneDaily,
            moneyReward, xpReward, itemReward, itemAmount);
        pGroupPlr->GetSession()->SendLfgRewards(reward);
        RegisterPlayerDaily(pGroupPlr->GetGUIDLow(), type);
    }
}

void LFGMgr::AttemptToKickPlayer(Group* pGroup, ObjectGuid guid, ObjectGuid kicker, std::string reason)
{
    if (!pGroup || !LFGLogic::ShouldVoteKick(guid.GetRawValue(),
        kicker.GetRawValue()) || !pGroup->IsMember(guid) ||
        !pGroup->IsMember(kicker))
    {
        return;
    }

    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    if (!status || status->state != LFG_STATE_IN_DUNGEON ||
        m_bootStatusMap.find(groupGuid) != m_bootStatusMap.end())
    {
        return;
    }

    LFGState const previousState = status->state;
    status->state = LFG_STATE_BOOT;
    proposalAnswerMap votes;
    for (roleMap::const_iterator itr = status->playerRoles.begin();
        itr != status->playerRoles.end(); ++itr)
    {
        ObjectGuid const playerGuid = itr->first;
        votes[playerGuid] = playerGuid == guid ? LFG_ANSWER_DENY :
            (playerGuid == kicker ? LFG_ANSWER_AGREE : LFG_ANSWER_PENDING);
        TransitionPlayer(playerGuid, LFG_STATE_BOOT, LFG_UPDATE_STATUS);
    }

    LFGBoot boot(true, previousState, guid, reason, votes, time(NULL));
    m_bootStatusMap[groupGuid] = boot;
    for (proposalAnswerMap::const_iterator itr = votes.begin();
        itr != votes.end(); ++itr)
    {
        if (Player* groupPlr = sPlayerRegistry.Find(itr->first))
        {
            groupPlr->GetSession()->SendLfgBootUpdate(boot);
        }
    }
}

void LFGMgr::CastVote(Player* pPlayer, bool vote)
{
    if (!pPlayer)
    {
        return;
    }

    Group* pGroup = pPlayer->GetGroup();
    if (!pGroup)
    {
        return;
    }
    ObjectGuid groupGuid = pGroup->GetObjectGuid();

    LFGGroupStatus* status = GetGroupStatus(groupGuid);

    if (!status || status->state != LFG_STATE_BOOT)
    {
        return;
    }

    bootStatusMap::iterator it = m_bootStatusMap.find(groupGuid);
    if (it == m_bootStatusMap.end())
    {
        return;
    }

    LFGBoot& boot = it->second;
    proposalAnswerMap::iterator answerItr =
        boot.answers.find(pPlayer->GetObjectGuid());
    if (answerItr == boot.answers.end() ||
        answerItr->second != LFG_ANSWER_PENDING)
    {
        return;
    }
    answerItr->second = LFGProposalAnswer(vote);

    int32 yay = 0;
    int32 nay = 0;
    for (proposalAnswerMap::const_iterator pIt = boot.answers.begin();
        pIt != boot.answers.end(); ++pIt)
    {
        LFGProposalAnswer answer = pIt->second;
        if (answer == LFG_ANSWER_AGREE)
        {
            ++yay;
        }
        else if (answer == LFG_ANSWER_DENY)
        {
            ++nay;
        }
    }

    if (yay >= REQUIRED_VOTES_FOR_BOOT)
    {
        FinishBootVote(groupGuid, true);
        return;
    }
    if (nay >= REQUIRED_VOTES_FOR_BOOT)
    {
        FinishBootVote(groupGuid, false);
        return;
    }

    for (proposalAnswerMap::const_iterator pIt = boot.answers.begin();
        pIt != boot.answers.end(); ++pIt)
    {
        if (Player* groupPlayer = sPlayerRegistry.Find(pIt->first))
        {
            groupPlayer->GetSession()->SendLfgBootUpdate(boot);
        }
    }
}

void LFGMgr::FinishBootVote(ObjectGuid groupGuid, bool succeeded)
{
    bootStatusMap::iterator bootItr = m_bootStatusMap.find(groupGuid);
    if (bootItr == m_bootStatusMap.end())
    {
        return;
    }

    LFGBoot boot = bootItr->second;
    m_bootStatusMap.erase(bootItr);
    boot.inProgress = false;

    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    if (status)
    {
        status->state = boot.previousState;
    }
    for (proposalAnswerMap::const_iterator answerItr = boot.answers.begin();
        answerItr != boot.answers.end(); ++answerItr)
    {
        TransitionPlayer(answerItr->first, boot.previousState,
            LFG_UPDATE_STATUS);
        if (Player* player = sPlayerRegistry.Find(answerItr->first))
        {
            player->GetSession()->SendLfgBootUpdate(boot);
        }
    }

    Group* group = sObjectMgr.GetGroupById(groupGuid.GetCounter());
    if (!group)
    {
        return;
    }
    if (succeeded && group->IsMember(boot.playerVotedOn))
    {
        if (group->RemoveMember(boot.playerVotedOn, 1) <= 1)
        {
            sObjectMgr.RemoveGroup(group);
            delete group;
        }
    }
    else
    {
        group->SendUpdate();
    }
}

void LFGMgr::RemoveOldBoots()
{
    time_t const now = time(NULL);
    std::vector<ObjectGuid> expiredGroups;
    for (bootStatusMap::const_iterator itr = m_bootStatusMap.begin();
        itr != m_bootStatusMap.end(); ++itr)
    {
        if (LFGLogic::RemainingSeconds(itr->second.startTime,
            LFG_TIME_BOOT, now) == 0)
        {
            expiredGroups.push_back(itr->first);
        }
    }
    for (std::vector<ObjectGuid>::const_iterator itr = expiredGroups.begin();
        itr != expiredGroups.end(); ++itr)
    {
        FinishBootVote(*itr, false);
    }
}

void LFGMgr::SendRoleChosen(ObjectGuid plrGuid, ObjectGuid confirmedGuid, uint8 roles)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);

    if (pPlayer)
    {
        pPlayer->GetSession()->SendLfgRoleChosen(confirmedGuid.GetRawValue(), roles);
    }
}

void LFGMgr::SendRoleCheckUpdate(ObjectGuid plrGuid, LFGRoleCheck const& roleCheck)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);

    if (pPlayer)
    {
        pPlayer->GetSession()->SendLfgRoleCheckUpdate(roleCheck);
    }
}

void LFGMgr::SendLfgUpdate(ObjectGuid plrGuid, LFGPlayerStatus status, bool isGroup)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);

    if (pPlayer)
    {
        pPlayer->GetSession()->SendLfgUpdate(isGroup, status);
    }
}

void LFGMgr::SendLfgJoinResult(ObjectGuid plrGuid, LfgJoinResult result, LFGState state, partyForbidden const& lockedDungeons)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);

    if (pPlayer)
    {
        pPlayer->GetSession()->SendLfgJoinResult(result, state, lockedDungeons);
    }
}

void LFGMgr::RemoveOldRoleChecks()
{
    time_t const now = time(NULL);
    std::vector<ObjectGuid> expiredGroups;
    for (roleCheckMap::const_iterator roleItr = m_roleCheckMap.begin();
        roleItr != m_roleCheckMap.end(); ++roleItr)
    {
        if (roleItr->second.waitForRoleTime <= now)
        {
            expiredGroups.push_back(roleItr->first);
        }
    }

    for (std::vector<ObjectGuid>::const_iterator groupItr = expiredGroups.begin();
        groupItr != expiredGroups.end(); ++groupItr)
    {
        roleCheckMap::iterator roleItr = m_roleCheckMap.find(*groupItr);
        if (roleItr == m_roleCheckMap.end())
        {
            continue;
        }

        ObjectGuid const groupGuid = roleItr->first;
        LFGRoleCheck& roleCheck = roleItr->second;
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
        TransitionQueueUnit(groupGuid, LFG_STATE_NONE, LFG_UPDATE_ROLECHECK_FAILED);
        partyForbidden nullForbidden;

        for (roleMap::const_iterator roleMapItr = roleCheck.currentRoles.begin();
            roleMapItr != roleCheck.currentRoles.end(); ++roleMapItr)
        {
            ObjectGuid const plrGuid = roleMapItr->first;
            if (roleCheck.leaderGuidRaw == plrGuid.GetRawValue())
            {
                SendLfgJoinResult(plrGuid, ERR_LFG_ROLE_CHECK_FAILED,
                    LFG_STATE_ROLECHECK, nullForbidden);
            }

            SendRoleCheckUpdate(plrGuid, roleCheck);
            SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), true);
            m_playerQueueOwners.erase(plrGuid);
        }

        m_playerData.erase(groupGuid);
        m_roleCheckMap.erase(roleItr);
    }
}
