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
 * @file LFGMgrProposal.cpp
 * @brief Cohesion split of LFGMgr.cpp -- proposal and role-check flow: dungeon
 *        group creation, role validation/votes, proposal updates, kick votes,
 *        teleport to dungeon and the related LFG update senders. Same `LFGMgr`
 *        class; no behaviour change.
 */

#include <set>
#include <string>
#include "Common/TimeConstants.h"
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

// called each time a player selects their role
void LFGMgr::PerformRoleCheck(Player* pPlayer, Group* pGroup, uint8 roles)
{
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    ObjectGuid plrGuid = pPlayer? pPlayer->GetObjectGuid() : ObjectGuid();

    roleCheckMap::iterator it = m_roleCheckMap.find(groupGuid);
    if (it == m_roleCheckMap.end())
    {
        return; // no role check map found
    }

    LFGRoleCheck roleCheck = it->second;
    bool roleChosen = roleCheck.state != LFG_ROLECHECK_DEFAULT && plrGuid;

    if (!plrGuid)
    {
        roleCheck.state = LFG_ROLECHECK_ABORTED;  // aborted if anyone cancels during role check
    }
    else if (roles < PLAYER_ROLE_TANK)            // kind of a sanity check- the client shouldn't allow this to happen
    {
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
    }
    else
    {
        roleCheck.currentRoles[plrGuid] = roles;

        roleMap::iterator rItr = roleCheck.currentRoles.begin();
        do
        {
            if (rItr->second != PLAYER_ROLE_NONE)
            {
                ++rItr;
            }
        } while (rItr != roleCheck.currentRoles.end());

        if (rItr == roleCheck.currentRoles.end()) // meaning that everyone confirmed their roles
        {
            roleCheck.state = ValidateGroupRoles(roleCheck.currentRoles) ? LFG_ROLECHECK_FINISHED : LFG_ROLECHECK_MISSING_ROLE;
        }
    }

    std::set<uint32> dungeonBuff;
    if (roleCheck.randomDungeonID)
    {
        dungeonBuff.insert(roleCheck.randomDungeonID);
    }
    else
    {
        dungeonBuff = roleCheck.dungeonList;
    }

    partyForbidden nullForbidden;

    for (roleMap::iterator itr = roleCheck.currentRoles.begin(); itr != roleCheck.currentRoles.end(); ++itr)
    {
        ObjectGuid guidBuff = itr->first;
        if (roleChosen)
        {
            SendRoleChosen(guidBuff, plrGuid, roles); // send SMSG_LFG_ROLE_CHOSEN to each player
        }

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
                {
                    SendLfgJoinResult(guidBuff, ERR_LFG_ROLE_CHECK_FAILED, LFG_STATE_ROLECHECK, nullForbidden);
                }
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
    {
        return false;
    }

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

        Player* pPlayer = sPlayerRegistry.Find(plrGuid);

        if (Group* pGroup = pPlayer->GetGroup())
        {
            ObjectGuid grpGuid = pGroup->GetObjectGuid();

            SetPlayerUpdateType(plrGuid, LFG_UPDATE_PROPOSAL_BEGIN);

            if (premadeGroup && pGroup->IsLeader(plrGuid))
            {
                newProposal.groupLeaderGuid = plrGuid.GetRawValue();
            }

            if (premadeGroup && !newProposal.groupRawGuid)
            {
                newProposal.groupRawGuid = grpGuid.GetRawValue();
            }

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
        Player* pGroupLeader = sPlayerRegistry.Find(ObjectGuid(newProposal.groupLeaderGuid));

        if (pGroupLeader)
        {
            Group* pGroup = pGroupLeader->GetGroup();
            if (pGroup)
            {
                pGroup->SetAsLfgGroup();
            }
            else
            {
                // Log an error: group not found for group leader
                // In the future, we should determine the right actions for this scenario.
            }
        }
        else
        {
            // Log an error: group leader not found
            // In the future, we should determine the right actions for this scenario.
        }
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

        Player* pPlayer = sPlayerRegistry.Find(plrGuid);
        if (Group* pGroup = pPlayer->GetGroup())
        {
            ObjectGuid grpGuid = pGroup->GetObjectGuid();

            if (firstLoop)
            {
                priorGroupGuid = grpGuid;
                firstLoop = false;
            }
            else
            {
                if (isSameGroup)
                {
                    if (grpGuid != priorGroupGuid)
                    {
                        isSameGroup = false;
                    }
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
    {
        return;
    }

    bool allOkay = true; // true if everyone answered LFG_ANSWER_AGREE

    // Update answer map to given value
    LFGProposalAnswer plrAnswer = (LFGProposalAnswer)accepted;
    proposal->answers[plrGuid] = plrAnswer;

    // If the player declined, the proposal is over
    if (plrAnswer == LFG_ANSWER_DENY)
    {
        ProposalDeclined(plrGuid, proposal);
    }

    for (proposalAnswerMap::iterator it = proposal->answers.begin(); it != proposal->answers.end(); ++it)
    {
        if (it->second != LFG_ANSWER_AGREE)
        {
            allOkay = false;
        }
    }

    // if !allOkay, send proposal updates to all
    if (!allOkay)
    {
        for (proposalAnswerMap::iterator itr = proposal->answers.begin(); itr != proposal->answers.end(); ++itr)
        {
            ObjectGuid proposalPlrGuid  = itr->first;
            Player* pProposalPlayer = sPlayerRegistry.Find(proposalPlrGuid);
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
        Player* pProposalPlayer = sPlayerRegistry.Find(proposalPlrGuid);

        if (sendProposalUpdate)
        {
            pProposalPlayer->GetSession()->SendLfgProposalUpdate(*proposal);
        }

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
    }

    CreateDungeonGroup(proposal);
    m_proposalMap.erase(proposal->id);
}

bool LFGMgr::HasLeaderFlag(roleMap const& roles)
{
    for (roleMap::const_iterator it = roles.begin(); it != roles.end(); ++it)
    {
        if (it->second & PLAYER_ROLE_LEADER)
        {
            return true;
        }
    }
    return false;
}

void LFGMgr::CreateDungeonGroup(LFGProposal* proposal)
{
    if (!proposal)
    {
        return;
    }

    Group* pGroup = nullptr;

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
            Player* pGroupPlr = sPlayerRegistry.Find(pGroupPlrGuid);

            if (pGroupPlr && it->second)
            {
                Group* existingGroup = pGroupPlr->GetGroup();
                if (existingGroup)
                {
                    existingGroup->RemoveMember(pGroupPlrGuid, 0);
                }
            }

            if (pGroupPlr && !leaderIsSet)
            {
                bool currentPlrIsLeader = false;
                if (leaderRoleIsSet)
                {
                    for (roleMap::iterator itr = proposal->currentRoles.begin(); itr != proposal->currentRoles.end(); ++itr)
                    {
                        if (itr->second & PLAYER_ROLE_LEADER)
                        {
                            leaderGuid = itr->first;
                            Player* leaderRef = sPlayerRegistry.Find(leaderGuid);

                            if (leaderRef)
                            {
                                pGroup->Create(leaderRef->GetObjectGuid(), leaderRef->GetName());
                                currentPlrIsLeader = (pGroupPlrGuid == leaderGuid);
                            }
                        }
                    }
                }
                else
                {
                    pGroup->Create(pGroupPlrGuid, pGroupPlr->GetName());
                }

                if (!currentPlrIsLeader)
                {
                    pGroup->AddMember(pGroupPlrGuid, pGroupPlr->GetName());
                }

                leaderIsSet = true;
            }
            else if (leaderIsSet && pGroupPlr && pGroupPlrGuid != leaderGuid)
            {
                pGroup->AddMember(pGroupPlrGuid, pGroupPlr->GetName());
            }
        }
        pGroup->SetAsLfgGroup();
    }
    else
    {
        Player* pGroupLeader = sPlayerRegistry.Find(ObjectGuid(proposal->groupLeaderGuid));

        // Check if the group leader was found before accessing their group
        if (pGroupLeader)
        {
            pGroup = pGroupLeader->GetGroup();
        }
        else
        {
            // Log that the group leader is missing and fall back to creating a new group
            // In the future, we should determine the right actions for this scenario.
            // LOG_ERROR("LFGMgr::CreateDungeonGroup", "Group leader with GUID %u not found. Creating new group.", proposal->groupLeaderGuid);

            // Attempt to create a new group using the first available player in the proposal group
            if (!proposal->groups.empty())
            {
                ObjectGuid fallbackLeaderGuid = proposal->groups.begin()->first;
                Player* fallbackLeader = sPlayerRegistry.Find(fallbackLeaderGuid);

                if (fallbackLeader)
                {
                    pGroup = new Group();
                    pGroup->Create(fallbackLeader->GetObjectGuid(), fallbackLeader->GetName());
                    pGroup->SetAsLfgGroup();

                    // Add remaining members to the new group
                    for (playerGroupMap::iterator it = proposal->groups.begin(); it != proposal->groups.end(); ++it)
                    {
                        ObjectGuid pGroupPlrGuid = it->first;
                        if (pGroupPlrGuid != fallbackLeaderGuid)
                        {
                            Player* pGroupPlr = sPlayerRegistry.Find(pGroupPlrGuid);
                            if (pGroupPlr)
                            {
                                pGroup->AddMember(pGroupPlrGuid, pGroupPlr->GetName());
                            }
                        }
                    }
                }
                else
                {
                    // If no valid players are found, we return without proceeding
                    // In the future, we should determine the right actions for this scenario.
                    // LOG_ERROR("LFGMgr::CreateDungeonGroup", "No valid players found to create a fallback group.");
                    return;
                }
            }
            else
            {
                // Log if there are no players in the proposal groups map
                // In the future, we should determine the right actions for this scenario.
                // LOG_ERROR("LFGMgr::CreateDungeonGroup", "Proposal groups map is empty, cannot create fallback group.");
                return;
            }
        }
    }

    // Set dungeon difficulty for group
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(proposal->dungeonID);
    if (!dungeon || !pGroup)
    {
        return;
    }

    pGroup->SetDungeonDifficulty(Difficulty(dungeon->Difficulty));

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
    {
        return;
    }

    uint32 mapID = (uint32)dungeon->MapID;
    float x, y, z, o;
    LFGTeleportError err = LFG_TELEPORTERROR_OK;

    Player* pGroupLeader = sPlayerRegistry.Find(pGroup->GetLeaderGuid());

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
        {
            err = LFG_TELEPORTERROR_INVALID_LOCATION;
        }
    }

    dungeonForbidden lockedDungeons;
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            // further checks: player is dead, in vehicle, in battleground, on taxi, etc
            LFGTeleportError plrErr = LFG_TELEPORTERROR_OK;

            if (pGroupPlr->IsDead())
            {
                plrErr = LFG_TELEPORTERROR_PLAYER_DEAD;
            }
            if (pGroupPlr->IsFalling())
            {
                plrErr = LFG_TELEPORTERROR_FALLING;
            }
            if (pGroupPlr->GetVehicleInfo())
            {
                plrErr = LFG_TELEPORTERROR_IN_VEHICLE;
            }

            lockedDungeons = FindRandomDungeonsNotForPlayer(pGroupPlr);
            if (lockedDungeons.find(dungeon->Entry()) != lockedDungeons.end())
            {
                plrErr = LFG_TELEPORTERROR_INVALID_LOCATION;
            }

            if (err == LFG_TELEPORTERROR_OK && plrErr == LFG_TELEPORTERROR_OK && pGroupPlr->GetMapId() != mapID)
            {
                if (pGroupPlr->GetMap() && !pGroupPlr->GetMap()->IsDungeon() && !pGroupPlr->GetMap()->IsRaid() && !pGroupPlr->InBattleGround())
                {
                    pGroupPlr->SetBattleGroundEntryPoint(); // store current position and such
                }

                if (!pGroupPlr->TeleportTo(mapID, x, y, z, o))
                {
                    plrErr = LFG_TELEPORTERROR_INVALID_LOCATION;
                }
            }

            if (err != LFG_TELEPORTERROR_OK)
            {
                pGroupPlr->GetSession()->SendLfgTeleportError(err);
            }
            else if (plrErr != LFG_TELEPORTERROR_OK)
            {
                pGroupPlr->GetSession()->SendLfgTeleportError(plrErr);
            }
            else
            {
                SetPlayerState(pGroupPlr->GetObjectGuid(), LFG_STATE_IN_DUNGEON);
            }
        }
    }
}

void LFGMgr::TeleportPlayer(Player* pPlayer, bool out)
{
    // Fetch necessary data first
    Group* pGroup = pPlayer->GetGroup();
    if (!pGroup)
    {
        pPlayer->GetSession()->SendLfgTeleportError((uint8)LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    LFGGroupStatus* status = GetGroupStatus(pGroup->GetObjectGuid());
    if (!status)
    {
        pPlayer->GetSession()->SendLfgTeleportError((uint8)LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    // Get dungeon info and then teleport the player out if applicable
    if (out)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(status->dungeonID);
        if (dungeon && pPlayer->GetMapId() == dungeon->MapID)
        {
            pPlayer->TeleportToBGEntryPoint();
        }
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

void LFGMgr::ProposalDeclined(ObjectGuid guid, LFGProposal* proposal)
{
    Player* pPlayer = sPlayerRegistry.Find(guid);

    if (!pPlayer)
    {
        return;
    }

    bool leaveGroupLFG = false;

    for (roleMap::iterator it = proposal->currentRoles.begin(); it != proposal->currentRoles.end(); ++it)
    {
        ObjectGuid groupPlrGuid = it->first;

        // update each player with a LFG_UPDATE_PROPOSAL_DECLINED
        SetPlayerUpdateType(groupPlrGuid, LFG_UPDATE_PROPOSAL_DECLINED);

        Player* pGroupPlayer = sPlayerRegistry.Find(groupPlrGuid);
        Group* pGroup = pGroupPlayer->GetGroup();

        // if player was in a premade group and declined, remove the group.
        if (groupPlrGuid == guid)
        {
            //LeaveLFG(pGroupPlayer, true);
            if (pGroup && (pGroup->GetObjectGuid().GetRawValue() == proposal->groupRawGuid))
            {
                leaveGroupLFG = true;
            }

            SendLfgUpdate(groupPlrGuid, GetPlayerStatus(groupPlrGuid), false);
        }
        else
        {
            if (proposal->groupRawGuid)
            {
                SendLfgUpdate(groupPlrGuid, GetPlayerStatus(groupPlrGuid), true);
            }
            else
            {
                SendLfgUpdate(groupPlrGuid, GetPlayerStatus(groupPlrGuid), false);
            }
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
    {
        return;
    }

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
            if (dungeon->TypeID == LFG_TYPE_RANDOM_DUNGEON || IsSeasonal(dungeon->Flags))
            {
                randomDungeonId = dungeon->ID;
            }

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
    {
        return;
    }

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
            {
                votes[pGroupPlrGuid] = LFG_ANSWER_PENDING;
            }
        }
    }

    LFGBoot boot(true, guid, reason, votes, now);
    m_bootStatusMap[groupGuid] = boot;

    for (GroupReference* it = pGroup->GetFirstMember(); it != NULL; it = it->next())
    {
        if (Player* groupPlr = it->getSource())
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

    LFGBoot boot = it->second;
    boot.answers[pPlayer->GetObjectGuid()] = LFGProposalAnswer(vote);

    int32 yay = 0, nay = 0; // keep a count of votes
    for (proposalAnswerMap::iterator pIt = boot.answers.begin(); pIt != boot.answers.end(); ++pIt)
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
