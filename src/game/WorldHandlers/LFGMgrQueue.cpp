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
 * @file LFGMgrQueue.cpp
 * @brief Cohesion split of LFGMgr.cpp -- queue join/leave and player/proposal
 *        data accessors: join/leave LFG, join-result and status queries, and
 *        player comment/state/update-type setters. Same `LFGMgr` class; no
 *        behaviour change.
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

void LFGMgr::JoinLFG(uint32 roles, std::set<uint32> dungeons, std::string comments, Player* plr)
{
    // Todo:
    //       - see if any of this code/information can be put into a generalized class for other use
    //       - look into splitting this into 2 fns- one for player case, one for group
    Group* pGroup = plr->GetGroup();
    ObjectGuid guid = (pGroup) ? pGroup->GetObjectGuid() : plr->GetObjectGuid();
    // store the current dungeon id (replaced into the dungeon set later)
    uint32 randomDungeonID = *dungeons.begin();    // used later if random dungeon has been chosen

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
            {
                m_queueSet.erase(qItr);
            }
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

    LfgJoinResult result = GetJoinResult(plr);
    if (result == ERR_LFG_OK)
    {
        bool isRaid    = false;
        bool isDungeon = false;

        // additional checks on dungeon selection
        for (std::set<uint32>::iterator it = dungeons.begin(); it != dungeons.end(); ++it)
        {
            LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*it);
            switch (dungeon->TypeID)
            {
                case LFG_TYPE_RANDOM_DUNGEON:
                    if (dungeons.size() > 1)
                    {
                        result = ERR_LFG_INVALID_SLOT;
                    }
                    else
                    {
                        isRandom = true;
                    }
                case LFG_TYPE_DUNGEON:
                case LFG_TYPE_HEROIC_DUNGEON:
                    if (isRaid)
                    {
                        result = ERR_LFG_MISMATCHED_SLOTS;
                    }
                    isDungeon = true;
                    break;
                case LFG_TYPE_RAID:
                    if (isDungeon)
                    {
                        result = ERR_LFG_MISMATCHED_SLOTS;
                    }
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
            // fetch all dungeons with our groupID and add to set
            LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*dungeons.begin());

            if (dungeon)
            {
                uint32 group = dungeon->Group_ID;

                for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
                {
                    LfgDungeonsEntry const* dungeonList = sLfgDungeonsStore.LookupEntry(id);
                    if (dungeonList)
                    {
                        if (dungeonList->Group_ID == group)
                        {
                            dungeons.insert(dungeonList->ID); // adding to set
                        }
                    }
                }
            }
            else
            {
                result = ERR_LFG_NO_LFG_OBJECT;
            }
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
                        {
                            dungeons.erase(*setItr);
                        }
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
                {
                    dungeons.erase(*setItr);
                }
            }
        }

        if (!dungeons.empty())
        {
            partyLockedDungeons.clear();
        }
        else
        {
            result = (pGroup) ? ERR_LFG_NO_SLOTS_PARTY : ERR_LFG_NO_SLOTS_PLAYER;
        }
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
    {
        result = ERR_LFG_DESERTER_PLAYER;
    }
    else if (plr->InBattleGround() || plr->InBattleGroundQueue() || plr->InArena())
    {
        result = ERR_LFG_CANT_USE_DUNGEONS;
    }
    else if (plr->HasAura(LFG_COOLDOWN_SPELL))
    {
        result = ERR_LFG_RANDOM_COOLDOWN_PLAYER;
    }

    if (pGroup)
    {
        if (pGroup->GetMembersCount() > 5)
        {
            result = ERR_LFG_TOO_MANY_MEMBERS;
        }
        else
        {
            uint8 currentMemberCount = 0;
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                if (Player* pGroupPlr = itr->getSource())
                {
                    // check if the group members are level 15+ to use finder
                    if (pGroupPlr->getLevel() < 15)
                    {
                        result = ERR_LFG_CANT_USE_DUNGEONS;
                    }
                    else if (pGroupPlr->HasAura(LFG_DESERTER_SPELL))
                    {
                        result = ERR_LFG_DESERTER_PARTY;
                    }
                    else if (pGroupPlr->InBattleGround() || pGroupPlr->InBattleGroundQueue() || pGroupPlr->InArena())
                    {
                        result = ERR_LFG_CANT_USE_DUNGEONS;
                    }
                    else if (pGroupPlr->HasAura(LFG_COOLDOWN_SPELL))
                    {
                        result = ERR_LFG_RANDOM_COOLDOWN_PARTY;
                    }
                    else
                    {
                        result = ERR_LFG_OK;
                    }

                    ++currentMemberCount;
                }
            }

            if (result == ERR_LFG_OK && currentMemberCount != pGroup->GetMembersCount())
            {
                result = ERR_LFG_MEMBERS_NOT_PRESENT;
            }
        }
    }
    else
    {
        result = ERR_LFG_OK;
    }

    return result;
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
