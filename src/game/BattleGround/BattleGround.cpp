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
 * @file BattleGround.cpp
 * @brief Core implementation of the battleground system.
 *
 * This file contains the implementation of the BattleGround base class, which provides:
 * - Battleground state management (waiting, in-progress, finished)
 * - Player management (joining, leaving, tracking)
 * - Event handling and broadcasting
 * - Reward distribution and scoring
 * - World state synchronization
 * - Team management and raid groups
 * - Creature and game object spawning
 */

#include <cmath>
#include <string>
#include "Common/TimeConstants.h"
#include "PlayerRegistry.h"
#include "Object.h"
#include "Player.h"
#include "BattleGround.h"
#include "BattleGroundMgr.h"
#include "Creature.h"
#include "MapManager.h"
#include "Language.h"
#include "SpellAuras.h"
#include "ArenaTeam.h"
#include "World.h"
#include "Group.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Mail.h"
#include "WorldPacket.h"
#include "Util.h"
#include "Formulas.h"
#include "GridNotifiersImpl.h"
#include "Chat.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

namespace MaNGOS
{
    class BattleGroundChatBuilder
    {
        public:
            /// <summary>
            /// Initializes a new instance of the <see cref="BattleGroundChatBuilder"/> class.
            /// </summary>
            /// <param name="msgtype">The msgtype.</param>
            /// <param name="textId">The text id.</param>
            /// <param name="source">The source.</param>
            /// <param name="args">The args.</param>
            BattleGroundChatBuilder(ChatMsg msgtype, int32 textId, Player const* source, va_list* args = nullptr)
                : i_msgtype(msgtype), i_textId(textId), i_source(source), i_args(args) {}
            void operator()(WorldPacket& data, int32 loc_idx)
            {
                char const* text = sObjectMgr.GetMangosString(i_textId, loc_idx);

                ObjectGuid sourceGuid = i_source ? i_source ->GetObjectGuid() : ObjectGuid();
                std::string sourceName = i_source ? i_source ->GetName() : "";

                if (i_args)
                {
                    // we need copy va_list before use or original va_list will corrupted
                    va_list ap;
                    va_copy(ap, *i_args);

                    char str [2048];
                    vsnprintf(str, 2048, text, ap);
                    va_end(ap);

                    ChatHandler::BuildChatPacket(data, i_msgtype, &str[0], LANG_UNIVERSAL, CHAT_TAG_NONE, sourceGuid, sourceName.c_str());
                }
                else
                {
                    ChatHandler::BuildChatPacket(data, i_msgtype, text, LANG_UNIVERSAL, CHAT_TAG_NONE, sourceGuid, sourceName.c_str(), sourceGuid, sourceName.c_str());
                }
            }
        private:
            ChatMsg i_msgtype;
            int32 i_textId;
            Player const* i_source;
            va_list* i_args;
    };

    class BattleGroundYellBuilder
    {
        public:
            /// <summary>
            /// Initializes a new instance of the <see cref="BattleGroundYellBuilder"/> class.
            /// </summary>
            /// <param name="language">The language.</param>
            /// <param name="textId">The text id.</param>
            /// <param name="source">The source.</param>
            /// <param name="args">The args.</param>
            BattleGroundYellBuilder(Language language, int32 textId, Creature const* source, va_list* args = NULL)
                : i_language(language), i_textId(textId), i_source(source), i_args(args) {}
            void operator()(WorldPacket& data, int32 loc_idx)
            {
                char const* text = sObjectMgr.GetMangosString(i_textId, loc_idx);

                if (i_args)
                {
                    // we need copy va_list before use or original va_list will corrupted
                    va_list ap;
                    va_copy(ap, *i_args);

                    char str [2048];
                    vsnprintf(str, 2048, text, ap);
                    va_end(ap);

                    ChatHandler::BuildChatPacket(data, CHAT_MSG_MONSTER_YELL, &str[0], i_language, CHAT_TAG_NONE, i_source->GetObjectGuid(), i_source->GetName());
                }
                else
                {
                    ChatHandler::BuildChatPacket(data, CHAT_MSG_MONSTER_YELL, text, i_language, CHAT_TAG_NONE, i_source->GetObjectGuid(), i_source->GetName());
                }
            }
        private:
            Language i_language;
            int32 i_textId;
            Creature const* i_source;
            va_list* i_args;
    };


    class BattleGround2ChatBuilder
    {
        public:
            /// <summary>
            /// Initializes a new instance of the <see cref="BattleGround2ChatBuilder"/> class.
            /// </summary>
            /// <param name="msgtype">The msgtype.</param>
            /// <param name="textId">The text id.</param>
            /// <param name="source">The source.</param>
            /// <param name="arg1">The arg1.</param>
            /// <param name="arg2">The arg2.</param>
            BattleGround2ChatBuilder(ChatMsg msgtype, int32 textId, Player const* source, int32 arg1, int32 arg2)
                : i_msgtype(msgtype), i_textId(textId), i_source(source), i_arg1(arg1), i_arg2(arg2) {}
            void operator()(WorldPacket& data, int32 loc_idx)
            {
                char const* text = sObjectMgr.GetMangosString(i_textId, loc_idx);
                char const* arg1str = i_arg1 ? sObjectMgr.GetMangosString(i_arg1, loc_idx) : "";
                char const* arg2str = i_arg2 ? sObjectMgr.GetMangosString(i_arg2, loc_idx) : "";

                char str [2048];
                snprintf(str, 2048, text, arg1str, arg2str);

                ObjectGuid guid;
                char const* pName = NULL;
                if (i_source)
                {
                    guid = i_source->GetObjectGuid();
                    pName = i_source->GetName();
                }
                ChatHandler::BuildChatPacket(data, i_msgtype, str, LANG_UNIVERSAL, CHAT_TAG_NONE, ObjectGuid(), NULL, guid, pName);
            }
        private:
            ChatMsg i_msgtype;
            int32 i_textId;
            Player const* i_source;
            int32 i_arg1;
            int32 i_arg2;
    };

    class BattleGround2YellBuilder
    {
        public:
            /// <summary>
            /// Initializes a new instance of the <see cref="BattleGround2YellBuilder"/> class.
            /// </summary>
            /// <param name="language">The language.</param>
            /// <param name="textId">The text id.</param>
            /// <param name="source">The source.</param>
            /// <param name="arg1">The arg1.</param>
            /// <param name="arg2">The arg2.</param>
            BattleGround2YellBuilder(uint32 language, int32 textId, Creature const* source, int32 arg1, int32 arg2)
                : i_language(language), i_textId(textId), i_source(source), i_arg1(arg1), i_arg2(arg2) {}
            void operator()(WorldPacket& data, int32 loc_idx)
            {
                char const* text = sObjectMgr.GetMangosString(i_textId, loc_idx);
                char const* arg1str = i_arg1 ? sObjectMgr.GetMangosString(i_arg1, loc_idx) : "";
                char const* arg2str = i_arg2 ? sObjectMgr.GetMangosString(i_arg2, loc_idx) : "";

                char str [2048];
                snprintf(str, 2048, text, arg1str, arg2str);

                ChatHandler::BuildChatPacket(data, CHAT_MSG_MONSTER_YELL, str, LANG_UNIVERSAL, CHAT_TAG_NONE, i_source ? i_source ->GetObjectGuid() : ObjectGuid(), i_source ? i_source ->GetName() : "");
            }
        private:

            uint32 i_language;
            int32 i_textId;
            Creature const* i_source;
            int32 i_arg1;
            int32 i_arg2;
    };
}                                                           // namespace MaNGOS

template<class Do>

/**
 * @brief Broadcasts a worker function to all players in the battleground.
 *
 * @param _do The worker function.
 */
void BattleGround::BroadcastWorker(Do& _do)
{
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (Player* plr = sPlayerRegistry.Find(itr->first))
        {
            _do(plr);
        }
    }
}

BattleGround::BattleGround()
{
    m_TypeID            = BattleGroundTypeId(0);
    m_RandomTypeID      = BattleGroundTypeId(0);
    m_Status            = STATUS_NONE;
    m_ClientInstanceID  = 0;
    m_EndTime           = 0;
    m_BracketId         = BG_BRACKET_ID_TEMPLATE;
    m_InvitedAlliance   = 0;
    m_InvitedHorde      = 0;
    m_ArenaType         = ARENA_TYPE_NONE;
    m_IsArena           = false;
    m_Winner            = TEAM_NONE;
    m_StartTime         = 0;
    m_Events            = 0;
    m_IsRated           = false;
    m_IsRandom          = false;
    m_Name              = "";
    m_LevelMin          = 0;
    m_LevelMax          = 0;
    m_InBGFreeSlotQueue = false;

    m_MaxPlayersPerTeam = 0;
    m_MaxPlayers        = 0;
    m_MinPlayersPerTeam = 0;
    m_MinPlayers        = 0;

    m_MapId             = 0;
    m_Map               = NULL;
    m_startMaxDist = 0;

    m_TeamStartLocX[TEAM_INDEX_ALLIANCE]   = 0;
    m_TeamStartLocX[TEAM_INDEX_HORDE]      = 0;

    m_TeamStartLocY[TEAM_INDEX_ALLIANCE]   = 0;
    m_TeamStartLocY[TEAM_INDEX_HORDE]      = 0;

    m_TeamStartLocZ[TEAM_INDEX_ALLIANCE]   = 0;
    m_TeamStartLocZ[TEAM_INDEX_HORDE]      = 0;

    m_TeamStartLocO[TEAM_INDEX_ALLIANCE]   = 0;
    m_TeamStartLocO[TEAM_INDEX_HORDE]      = 0;

    m_ArenaTeamIds[TEAM_INDEX_ALLIANCE]   = 0;
    m_ArenaTeamIds[TEAM_INDEX_HORDE]      = 0;

    m_ArenaTeamRatingChanges[TEAM_INDEX_ALLIANCE]   = 0;
    m_ArenaTeamRatingChanges[TEAM_INDEX_HORDE]      = 0;

    m_BgRaids[TEAM_INDEX_ALLIANCE]         = NULL;
    m_BgRaids[TEAM_INDEX_HORDE]            = NULL;

    m_PlayersCount[TEAM_INDEX_ALLIANCE]    = 0;
    m_PlayersCount[TEAM_INDEX_HORDE]       = 0;

    m_TeamScores[TEAM_INDEX_ALLIANCE]      = 0;
    m_TeamScores[TEAM_INDEX_HORDE]         = 0;

    m_PrematureCountDown = false;
    m_PrematureCountDownTimer = 0;

    m_StartDelayTimes[BG_STARTING_EVENT_FIRST]  = BG_START_DELAY_2M;
    m_StartDelayTimes[BG_STARTING_EVENT_SECOND] = BG_START_DELAY_1M;
    m_StartDelayTimes[BG_STARTING_EVENT_THIRD]  = BG_START_DELAY_30S;
    m_StartDelayTimes[BG_STARTING_EVENT_FOURTH] = BG_START_DELAY_NONE;
    // we must set to some default existing values
    m_StartMessageIds[BG_STARTING_EVENT_FIRST]  = 0;
    m_StartMessageIds[BG_STARTING_EVENT_SECOND] = LANG_BG_WS_START_ONE_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_THIRD]  = LANG_BG_WS_START_HALF_MINUTE;
    m_StartMessageIds[BG_STARTING_EVENT_FOURTH] = LANG_BG_WS_HAS_BEGUN;
}

/// <summary>
/// Finalizes an instance of the <see cref="BattleGround"/> class.
/// </summary>
BattleGround::~BattleGround()
{
#ifdef ENABLE_ELUNA
    // sEluna->OnBGDestroy(this, GetTypeID(), GetInstanceID());
#endif /* ENABLE_ELUNA */

    // remove objects and creatures
    // (this is done automatically in mapmanager update, when the instance is reset after the reset time)
    sBattleGroundMgr.RemoveBattleGround(GetInstanceID(), GetTypeID());

    // skip template bgs as they were never added to visible bg list
    BattleGroundBracketId bracketId = GetBracketId();
    if (bracketId != BG_BRACKET_ID_TEMPLATE)
    {
        sBattleGroundMgr.DeleteClientVisibleInstanceId(GetTypeID(), bracketId, GetClientInstanceID());
    }

    // unload map
    // map can be null at bg destruction
    if (m_Map)
    {
        m_Map->SetUnload();
    }

    // remove from bg free slot queue
    this->RemoveFromBGFreeSlotQueue();

    for (BattleGroundScoreMap::const_iterator itr = m_PlayerScores.begin(); itr != m_PlayerScores.end(); ++itr)
    {
        delete itr->second;
    }
}

/// <summary>
/// Updates the specified diff.
/// </summary>
/// <param name="diff">The diff.</param>
void BattleGround::Update(uint32 diff)
{
    if (!GetPlayersSize())
    {
        // BG is empty
        // if there are no players invited, delete BG
        // this will delete arena or bg object, where any player entered
        // [[   but if you use battleground object again (more battles possible to be played on 1 instance)
        //      then this condition should be removed and code:
        //      if (!GetInvitedCount(HORDE) && !GetInvitedCount(ALLIANCE))
        //          this->AddToFreeBGObjectsQueue(); // not yet implemented
        //      should be used instead of current
        // ]]
        // BattleGround Template instance can not be updated, because it would be deleted
        if (!GetInvitedCount(HORDE) && !GetInvitedCount(ALLIANCE))
        {
            delete this;
        }
        return;
    }

    // remove offline players from bg after 5 minutes
    if (!m_OfflineQueue.empty())
    {
        BattleGroundPlayerMap::iterator itr = m_Players.find(*(m_OfflineQueue.begin()));
        if (itr != m_Players.end())
        {
            if (itr->second.OfflineRemoveTime <= sWorld.GetGameTime())
            {
                RemovePlayerAtLeave(itr->first, true, true);// remove player from BG
                m_OfflineQueue.pop_front();                 // remove from offline queue
                // do not use itr for anything, because it is erased in RemovePlayerAtLeave()
            }
        }
    }

    /*********************************************************/
    /***           BATTLEGROUND BALLANCE SYSTEM            ***/
    /*********************************************************/

    // if less then minimum players are in on one side, then start premature finish timer
    if (GetStatus() == STATUS_IN_PROGRESS && !isArena() && sBattleGroundMgr.GetPrematureFinishTime() && (GetPlayersCountByTeam(ALLIANCE) < GetMinPlayersPerTeam() || GetPlayersCountByTeam(HORDE) < GetMinPlayersPerTeam()))
    {
        if (!m_PrematureCountDown)
        {
            m_PrematureCountDown = true;
            m_PrematureCountDownTimer = sBattleGroundMgr.GetPrematureFinishTime();
        }
        else if (m_PrematureCountDownTimer < diff)
        {
            EndBattleGround(GetPrematureWinner());
            m_PrematureCountDown = false;
        }
        else if (!sBattleGroundMgr.isTesting())
        {
            uint32 newtime = m_PrematureCountDownTimer - diff;
            // announce every minute
            if (newtime > (MINUTE * IN_MILLISECONDS))
            {
                if (newtime / (MINUTE * IN_MILLISECONDS) != m_PrematureCountDownTimer / (MINUTE * IN_MILLISECONDS))
                {
                    PSendMessageToAll(LANG_BATTLEGROUND_PREMATURE_FINISH_WARNING, CHAT_MSG_SYSTEM, NULL, (uint32)(m_PrematureCountDownTimer / (MINUTE * IN_MILLISECONDS)));
                }
            }
            else
            {
                // announce every 15 seconds
                if (newtime / (15 * IN_MILLISECONDS) != m_PrematureCountDownTimer / (15 * IN_MILLISECONDS))
                {
                    PSendMessageToAll(LANG_BATTLEGROUND_PREMATURE_FINISH_WARNING_SECS, CHAT_MSG_SYSTEM, NULL, (uint32)(m_PrematureCountDownTimer / IN_MILLISECONDS));
                }
            }
            m_PrematureCountDownTimer = newtime;
        }
    }
    else if (m_PrematureCountDown)
    {
        m_PrematureCountDown = false;
    }

    /*********************************************************/
    /***           ARENA BUFF OBJECT SPAWNING              ***/
    /*********************************************************/
    if (isArena() && !m_ArenaBuffSpawned)
    {
        // 60 seconds after start the buffobjects in arena should get spawned
        if (m_StartTime > uint32(m_StartDelayTimes[BG_STARTING_EVENT_FIRST] + ARENA_SPAWN_BUFF_OBJECTS))
        {
            SpawnEvent(ARENA_BUFF_EVENT, 0, true);
            m_ArenaBuffSpawned = true;
        }
    }

    /*********************************************************/
    /***           BATTLEGROUND STARTING SYSTEM            ***/
    /*********************************************************/

    if (GetStatus() == STATUS_WAIT_JOIN && GetPlayersSize())
    {
        float maxDist = GetStartMaxDist();
        if (maxDist > 0.0f)
        {
            if (m_validStartPositionTimer < diff)
            {
                for (BattleGroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
                {
                    if (Player* player = sObjectMgr.GetPlayer(itr->first))
                    {
                        float x, y, z, o;
                        GetTeamStartLoc(player->GetTeam(), x, y, z, o);
                        if (!player->IsWithinDist3d(x, y, z, maxDist))
                        {
                            player->TeleportTo(GetMapId(), x, y, z, o);
                        }
                    }
                }
                m_validStartPositionTimer = CHECK_PLAYER_POSITION_INVERVAL;
            }
            else
            {
                m_validStartPositionTimer -= diff;
            }
        }

        ModifyStartDelayTime(diff);

        if (!(m_Events & BG_STARTING_EVENT_1))
        {
            m_Events |= BG_STARTING_EVENT_1;

            StartingEventCloseDoors();
            SetStartDelayTime(m_StartDelayTimes[BG_STARTING_EVENT_FIRST]);
            // first start warning - 2 or 1 minute, only if defined
            if (m_StartMessageIds[BG_STARTING_EVENT_FIRST])
            {
                SendMessageToAll(m_StartMessageIds[BG_STARTING_EVENT_FIRST], CHAT_MSG_BG_SYSTEM_NEUTRAL);
            }
        }
        // After 1 minute or 30 seconds, warning is signalled
        else if (GetStartDelayTime() <= m_StartDelayTimes[BG_STARTING_EVENT_SECOND] && !(m_Events & BG_STARTING_EVENT_2))
        {
            m_Events |= BG_STARTING_EVENT_2;
            SendMessageToAll(m_StartMessageIds[BG_STARTING_EVENT_SECOND], CHAT_MSG_BG_SYSTEM_NEUTRAL);
        }
        // After 30 or 15 seconds, warning is signalled
        else if (GetStartDelayTime() <= m_StartDelayTimes[BG_STARTING_EVENT_THIRD] && !(m_Events & BG_STARTING_EVENT_3))
        {
            m_Events |= BG_STARTING_EVENT_3;
            SendMessageToAll(m_StartMessageIds[BG_STARTING_EVENT_THIRD], CHAT_MSG_BG_SYSTEM_NEUTRAL);
        }
        // delay expired (atfer 2 or 1 minute)
        else if (GetStartDelayTime() <= 0 && !(m_Events & BG_STARTING_EVENT_4))
        {
            m_Events |= BG_STARTING_EVENT_4;

#ifdef ENABLE_ELUNA
            if (Eluna* e = this->GetBgMap()->GetEluna())
            {
                e->OnBGCreate(this, GetTypeID(), GetInstanceID());
            }
#endif /* ENABLE_ELUNA */

            StartingEventOpenDoors();

            SendMessageToAll(m_StartMessageIds[BG_STARTING_EVENT_FOURTH], CHAT_MSG_BG_SYSTEM_NEUTRAL);
            SetStatus(STATUS_IN_PROGRESS);
            SetStartDelayTime(m_StartDelayTimes[BG_STARTING_EVENT_FOURTH]);

            // remove preparation
            if (isArena())
            {
                for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
                {
                    if (Player* player = sObjectMgr.GetPlayer(itr->first))
                    {
                        player->RemoveAurasDueToSpell(SPELL_ARENA_PREPARATION);
                    }
                }

                CheckArenaWinConditions();
            }
            else
            {
                PlaySoundToAll(SOUND_BG_START);

                for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
                {
                    if (Player* plr = sObjectMgr.GetPlayer(itr->first))
                    {
                        plr->RemoveAurasDueToSpell(SPELL_PREPARATION);
                    }
                }

                // Announce BG starting
                if (sWorld.getConfig(CONFIG_BOOL_BATTLEGROUND_QUEUE_ANNOUNCER_START))
                {
                    sWorld.SendWorldText(LANG_BG_STARTED_ANNOUNCE_WORLD, GetName(), GetMinLevel(), GetMaxLevel());
                }
            }
        }
    }

    /*********************************************************/
    /***           BATTLEGROUND ENDING SYSTEM              ***/
    /*********************************************************/

    if (GetStatus() == STATUS_WAIT_LEAVE)
    {
        // remove all players from battleground after 2 minutes
        m_EndTime -= diff;
        if (m_EndTime <= 0)
        {
            m_EndTime = 0;
            BattleGroundPlayerMap::iterator itr, next;
            for (itr = m_Players.begin(); itr != m_Players.end(); itr = next)
            {
                next = itr;
                ++next;
                // itr is erased here!
                RemovePlayerAtLeave(itr->first, true, true);// remove player from BG
                // do not change any battleground's private variables
            }
        }
    }

    // update start time
    m_StartTime += diff;
}

/// <summary>
/// Sets the team start loc.
/// </summary>
/// <param name="team">The team.</param>
/// <param name="X">The X.</param>
/// <param name="Y">The Y.</param>
/// <param name="Z">The Z.</param>
/// <param name="O">The O.</param>
void BattleGround::SetTeamStartLoc(Team team, float X, float Y, float Z, float O)
{
    PvpTeamIndex teamIdx = GetTeamIndexByTeamId(team);
    m_TeamStartLocX[teamIdx] = X;
    m_TeamStartLocY[teamIdx] = Y;
    m_TeamStartLocZ[teamIdx] = Z;
    m_TeamStartLocO[teamIdx] = O;
}

/// <summary>
/// Sends the packet to all.
/// </summary>
/// <param name="packet">The packet.</param>
void BattleGround::SendPacketToAll(WorldPacket* packet)
{
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.OfflineRemoveTime)
        {
            continue;
        }

        if (Player* plr = sObjectMgr.GetPlayer(itr->first))
        {
            plr->GetSession()->SendPacket(packet);
        }
        else
        {
            sLog.outError("BattleGround:SendPacketToAll: %s not found!", itr->first.GetString().c_str());
        }
    }
}

/// <summary>
/// Sends the packet to team.
/// </summary>
/// <param name="teamId">The team id.</param>
/// <param name="packet">The packet.</param>
/// <param name="sender">The sender.</param>
/// <param name="self">The self.</param>
void BattleGround::SendPacketToTeam(Team teamId, WorldPacket* packet, Player* sender, bool self)
{
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.OfflineRemoveTime)
        {
            continue;
        }

        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
        {
            sLog.outError("BattleGround:SendPacketToTeam: %s not found!", itr->first.GetString().c_str());
            continue;
        }

        if (!self && sender == plr)
        {
            continue;
        }

        Team team = itr->second.PlayerTeam;
        if (!team)
        {
            team = plr->GetTeam();
        }

        if (team == teamId)
        {
            plr->GetSession()->SendPacket(packet);
        }
    }
}

/// <summary>
/// Plays the sound to all.
/// </summary>
/// <param name="SoundID">The sound ID.</param>
void BattleGround::PlaySoundToAll(uint32 SoundID)
{
    WorldPacket data;
    sBattleGroundMgr.BuildPlaySoundPacket(&data, SoundID);
    SendPacketToAll(&data);
}

/// <summary>
/// Plays the sound to team.
/// </summary>
/// <param name="SoundID">The sound ID.</param>
/// <param name="teamId">The team id.</param>
void BattleGround::PlaySoundToTeam(uint32 SoundID, Team teamId)
{
    WorldPacket data;

    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.OfflineRemoveTime)
        {
            continue;
        }

        Player* plr = sObjectMgr.GetPlayer(itr->first);
        if (!plr)
        {
            sLog.outError("BattleGround:PlaySoundToTeam: %s not found!", itr->first.GetString().c_str());
            continue;
        }

        Team team = itr->second.PlayerTeam;
        if (!team)
        {
            team = plr->GetTeam();
        }

        if (team == teamId)
        {
            sBattleGroundMgr.BuildPlaySoundPacket(&data, SoundID);
            plr->GetSession()->SendPacket(&data);
        }
    }
}

/// <summary>
/// Casts the spell on team.
/// </summary>
/// <param name="SpellID">The spell ID.</param>
/// <param name="teamId">The team id.</param>
void BattleGround::CastSpellOnTeam(uint32 SpellID, Team teamId)
{
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.OfflineRemoveTime)
        {
            continue;
        }

        Player* plr = sObjectMgr.GetPlayer(itr->first);

        if (!plr)
        {
            sLog.outError("BattleGround:CastSpellOnTeam: %s not found!", itr->first.GetString().c_str());
            continue;
        }

        Team team = itr->second.PlayerTeam;
        if (!team)
        {
            team = plr->GetTeam();
        }

        if (team == teamId)
        {
            plr->CastSpell(plr, SpellID, true);
        }
    }
}



/// <summary>
/// Rewards the XP to team.
/// </summary>
/// <param name="event">The battleground event.</param>
/// <param name="teamId">The team id.</param>
void BattleGround::RewardXPToTeam(uint8 event, Team teamId)
{
    enum BGEvent
    {
        WS_FLAG_CAPTURE                   = 1,
        WS_WIN                            = 2,
    };

    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.OfflineRemoveTime)
        {
            continue;
        }

        Player* plr = sObjectMgr.GetPlayer(itr->first);

        if (!plr)
        {
            sLog.outError("BattleGround:RewardXPToTeam: %s not found!", itr->first.GetString().c_str());
            continue;
        }

        Team team = itr->second.PlayerTeam;
        if (!team)
        {
            team = plr->GetTeam();
        }

        if (team == teamId)
        {
            uint32 xp;
            switch (event)
            {
                case WS_FLAG_CAPTURE:
                    xp = 74 + (4.01 * plr->getLevel()) + (1.19 * pow(plr->getLevel(),2));         // This XP curve is a WIP, was established with a limited dataset pulled from various videos. -fyre
                    break;
                case WS_WIN:
                    xp = (74 + (4.01 * plr->getLevel()) + (1.19 * pow(plr->getLevel(),2)))/2;         // Half of a flag capture. Complete guess. -fyre
                    break;
            }
            plr->GiveXP(xp, nullptr);
        }
    }
}











/// <summary>
/// Blocks the movement.
/// </summary>
/// <param name="plr">The PLR.</param>
void BattleGround::BlockMovement(Player* plr)
{
    plr->SetClientControl(plr, 0);                          // movement disabled NOTE: the effect will be automatically removed by client when the player is teleported from the battleground, so no need to send with uint8(1) in RemovePlayerAtLeave()
}

/// <summary>
/// Removes the player at leave.
/// </summary>
/// <param name="guid">The GUID.</param>
/// <param name="Transport">The transport.</param>
/// <param name="SendPacket">The send packet.</param>
void BattleGround::RemovePlayerAtLeave(ObjectGuid guid, bool Transport, bool SendPacket)
{
    Team team = GetPlayerTeam(guid);
    bool participant = false;
    // Remove from lists/maps
    BattleGroundPlayerMap::iterator itr = m_Players.find(guid);
    if (itr != m_Players.end())
    {
        UpdatePlayersCountByTeam(team, true);               // -1 player
        m_Players.erase(itr);
        // check if the player was a participant of the match, or only entered through gm command (goname)
        participant = true;
    }

    BattleGroundScoreMap::iterator itr2 = m_PlayerScores.find(guid);
    if (itr2 != m_PlayerScores.end())
    {
        delete itr2->second;                                // delete player's score
        m_PlayerScores.erase(itr2);
    }

    Player* plr = sObjectMgr.GetPlayer(guid);

    if (plr)
    {
        // should remove spirit of redemption
        if (plr->HasAuraType(SPELL_AURA_SPIRIT_OF_REDEMPTION))
        {
            plr->RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);
        }

        plr->RemoveAurasDueToSpell(isArena() ? SPELL_ARENA_DAMPENING : SPELL_BATTLEGROUND_DAMPENING);
        plr->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

        if (!plr->IsAlive())                                // resurrect on exit
        {
            plr->ResurrectPlayer(1.0f);
            plr->SpawnCorpseBones();
        }
    }

    RemovePlayer(plr, guid);                                // BG subclass specific code

    if (participant) // if the player was a match participant, remove auras, calc rating, update queue
    {
        BattleGroundTypeId bgTypeId = GetTypeID();
        BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(GetTypeID(), GetArenaType());
        if (plr)
        {
            plr->ClearAfkReports();

            if (!team)
            {
                team = plr->GetTeam();
            }

            // if arena, remove the specific arena auras
            if (isArena())
            {
                plr->RemoveArenaAuras(true);                // removes debuffs / dots etc., we don't want the player to die after porting out
                bgTypeId = BATTLEGROUND_AA;                 // set the bg type to all arenas (it will be used for queue refreshing)

                // unsummon current and summon old pet if there was one and there isn't a current pet
                plr->RemovePet(PET_SAVE_NOT_IN_SLOT);
                plr->ResummonPetTemporaryUnSummonedIfAny();

                if (isRated() && GetStatus() == STATUS_IN_PROGRESS)
                {
                    // left a rated match while the encounter was in progress, consider as loser
                    ArenaTeam* winner_arena_team = sObjectMgr.GetArenaTeamById(GetArenaTeamIdForTeam(GetOtherTeam(team)));
                    ArenaTeam* loser_arena_team = sObjectMgr.GetArenaTeamById(GetArenaTeamIdForTeam(team));
                    if (winner_arena_team && loser_arena_team)
                    {
                        loser_arena_team->MemberLost(plr, winner_arena_team->GetRating());
                    }
                }
            }
            if (SendPacket)
            {
                WorldPacket data;
                sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, this, plr->GetBattleGroundQueueIndex(bgQueueTypeId), STATUS_NONE, 0, 0, ARENA_TYPE_NONE, TEAM_NONE);
                plr->GetSession()->SendPacket(&data);
            }

            // this call is important, because player, when joins to battleground, this method is not called, so it must be called when leaving bg
            plr->RemoveBattleGroundQueueId(bgQueueTypeId);
        }
        else
            // removing offline participant
        {
            if (isRated() && GetStatus() == STATUS_IN_PROGRESS)
            {
                // left a rated match while the encounter was in progress, consider as loser
                ArenaTeam* others_arena_team = sObjectMgr.GetArenaTeamById(GetArenaTeamIdForTeam(GetOtherTeam(team)));
                ArenaTeam* players_arena_team = sObjectMgr.GetArenaTeamById(GetArenaTeamIdForTeam(team));
                if (others_arena_team && players_arena_team)
                {
                    players_arena_team->OfflineMemberLost(guid, others_arena_team->GetRating());
                }
            }
        }

        // remove from raid group if player is member
        if (Group* group = GetBgRaid(team))
        {
            if (!group->RemoveMember(guid, 0))              // group was disbanded
            {
                SetBgRaid(team, NULL);
                delete group;
            }
        }
        DecreaseInvitedCount(team);
        // we should update battleground queue, but only if bg isn't ending
        if (isBattleGround() && GetStatus() < STATUS_WAIT_LEAVE)
        {
            // a player has left the battleground, so there are free slots -> add to queue
            AddToBGFreeSlotQueue();
            sBattleGroundMgr.ScheduleQueueUpdate(0, ARENA_TYPE_NONE, bgQueueTypeId, bgTypeId, GetBracketId());
        }

        // Let others know
        WorldPacket data;
        sBattleGroundMgr.BuildPlayerLeftBattleGroundPacket(&data, guid);
        SendPacketToTeam(team, &data, plr, false);
    }

    if (plr)
    {
        // Do next only if found in battleground
        plr->SetBattleGroundId(0, BATTLEGROUND_TYPE_NONE);  // We're not in BG.
        // reset destination bg team
        plr->SetBGTeam(TEAM_NONE);

        if (Transport)
        {
            plr->TeleportToBGEntryPoint();
        }

        DETAIL_LOG("BATTLEGROUND: Removed player %s from BattleGround.", plr->GetName());
    }

    // battleground object will be deleted next BattleGround::Update() call
}

/// <summary>
/// this method is called when no players remains in battleground
/// </summary>
void BattleGround::Reset()
{
    SetWinner(TEAM_NONE);
    SetStatus(STATUS_WAIT_QUEUE);
    SetStartTime(0);
    SetEndTime(0);
    SetArenaType(ARENA_TYPE_NONE);
    SetRated(false);

    m_Events = 0;

    // door-event2 is always 0
    m_ActiveEvents[BG_EVENT_DOOR] = 0;
    if (isArena())
    {
        m_ActiveEvents[ARENA_BUFF_EVENT] = BG_EVENT_NONE;
        m_ArenaBuffSpawned = false;
    }

    if (m_InvitedAlliance > 0 || m_InvitedHorde > 0)
    {
        sLog.outError("BattleGround system: bad counter, m_InvitedAlliance: %d, m_InvitedHorde: %d", m_InvitedAlliance, m_InvitedHorde);
    }

    m_InvitedAlliance = 0;
    m_InvitedHorde = 0;
    m_InBGFreeSlotQueue = false;

    m_Players.clear();

    for (BattleGroundScoreMap::const_iterator itr = m_PlayerScores.begin(); itr != m_PlayerScores.end(); ++itr)
    {
        delete itr->second;
    }
    m_PlayerScores.clear();
}

/// <summary>
/// Starts the battle ground.
/// </summary>
void BattleGround::StartBattleGround()
{
    SetStartTime(0);

    // add BG to free slot queue
    AddToBGFreeSlotQueue();

    // add bg to update list
    // This must be done here, because we need to have already invited some players when first BG::Update() method is executed
    // and it doesn't matter if we call StartBattleGround() more times, because m_BattleGrounds is a map and instance id never changes
    sBattleGroundMgr.AddBattleGround(GetInstanceID(), GetTypeID(), this);

#ifdef ENABLE_ELUNA
    if (Eluna* e = GetBgMap()->GetEluna())
    {
        e->OnBGCreate(this, GetTypeID(), GetInstanceID());
    }
#endif /* ENABLE_ELUNA */
}

void BattleGround::StartTimedAchievement(AchievementCriteriaTypes type, uint32 entry)
{
    for (BattleGroundPlayerMap::const_iterator itr = GetPlayers().begin(); itr != GetPlayers().end(); ++itr)
    {
        if (Player* pPlayer = GetBgMap()->GetPlayer(itr->first))
        {
            pPlayer->GetAchievementMgr().StartTimedAchievementCriteria(type, entry);
        }
    }
}

/// <summary>
/// Adds the player.
/// </summary>
/// <param name="plr">The PLR.</param>
void BattleGround::AddPlayer(Player* plr)
{
    // remove afk from player
    if (plr->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK))
    {
        plr->ToggleAFK();
    }

    // score struct must be created in inherited class

    ObjectGuid guid = plr->GetObjectGuid();
    Team team = plr->GetBGTeam();

    BattleGroundPlayer bp;
    bp.OfflineRemoveTime = 0;
    bp.PlayerTeam = team;

    // Add to list/maps
    m_Players[guid] = bp;

    bool const isInBattleground = IsPlayerInBattleGround(guid);
    if (!isInBattleground)
    {
        UpdatePlayersCountByTeam(team, false); // +1 player
    }

    WorldPacket data;
    sBattleGroundMgr.BuildPlayerJoinedBattleGroundPacket(&data, plr);
    SendPacketToTeam(team, &data, plr, false);

    // add arena specific auras
    if (isArena())
    {
        plr->RemoveArenaSpellCooldowns();
        plr->RemoveArenaAuras();
        plr->RemoveAllEnchantments(TEMP_ENCHANTMENT_SLOT);
        if (team == ALLIANCE)                               // gold
        {
            if (plr->GetTeam() == HORDE)
            {
                plr->CastSpell(plr, SPELL_HORDE_GOLD_FLAG, true);
            }
            else
            {
                plr->CastSpell(plr, SPELL_ALLIANCE_GOLD_FLAG, true);
            }
        }
        else                                                // green
        {
            if (plr->GetTeam() == HORDE)
            {
                plr->CastSpell(plr, SPELL_HORDE_GREEN_FLAG, true);
            }
            else
            {
                plr->CastSpell(plr, SPELL_ALLIANCE_GREEN_FLAG, true);
            }
        }

        plr->DestroyConjuredItems(true);
        plr->UnsummonPetTemporaryIfAny();

        if (GetStatus() == STATUS_WAIT_JOIN)                // not started yet
        {
            plr->CastSpell(plr, SPELL_ARENA_PREPARATION, true);
        }

        plr->CastSpell(plr, SPELL_ARENA_DAMPENING, true);
    }
    else
    {
        if (GetStatus() == STATUS_WAIT_JOIN)                // not started yet
        {
            plr->CastSpell(plr, SPELL_PREPARATION, true);   // reduces all mana cost of spells.
        }

        plr->CastSpell(plr, SPELL_BATTLEGROUND_DAMPENING, true);
    }

    plr->GetAchievementMgr().ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE, ACHIEVEMENT_CRITERIA_CONDITION_MAP, GetMapId());
    plr->GetAchievementMgr().ResetAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE, ACHIEVEMENT_CRITERIA_CONDITION_MAP, GetMapId());

    // setup BG group membership
    PlayerAddedToBGCheckIfBGIsRunning(plr);
    AddOrSetPlayerToCorrectBgGroup(plr, guid, team);

    // Log
    DETAIL_LOG("BATTLEGROUND: Player %s joined the battle.", plr->GetName());
}

/// <summary>
/// this method adds player to his team's bg group, or sets his correct group if player is already in bg group.
/// </summary>
/// <param name="plr">The PLR.</param>
/// <param name="plr_guid">The plr_guid.</param>
/// <param name="team">The team.</param>
void BattleGround::AddOrSetPlayerToCorrectBgGroup(Player* plr, ObjectGuid plr_guid, Team team)
{
    if (Group* group = GetBgRaid(team))                     // raid already exist
    {
        if (group->IsMember(plr_guid))
        {
            uint8 subgroup = group->GetMemberGroup(plr_guid);
            plr->SetBattleGroundRaid(group, subgroup);
        }
        else
        {
            group->AddMember(plr_guid, plr->GetName());
            if (Group* originalGroup = plr->GetOriginalGroup())
            {
                if (originalGroup->IsLeader(plr_guid))
                {
                    group->ChangeLeader(plr_guid);
                }
            }
        }
    }
    else                                                    // first player joined
    {
        group = new Group;
        SetBgRaid(team, group);
        group->Create(plr_guid, plr->GetName());
    }
}

/// <summary>
/// This method should be called when player logs into running battleground
/// </summary>
/// <param name="player">The player.</param>
void BattleGround::EventPlayerLoggedIn(Player* player)
{
    ObjectGuid playerGuid = player->GetObjectGuid();

    // player is correct pointer
    for (OfflineQueue::iterator itr = m_OfflineQueue.begin(); itr != m_OfflineQueue.end(); ++itr)
    {
        if (*itr == playerGuid)
        {
            m_OfflineQueue.erase(itr);
            break;
        }
    }
    m_Players[playerGuid].OfflineRemoveTime = 0;
    PlayerAddedToBGCheckIfBGIsRunning(player);
    // if battleground is starting, then add preparation aura
    // we don't have to do that, because preparation aura isn't removed when player logs out
}

/// <summary>
/// This method should be called when player logs out from running battleground
/// </summary>
/// <param name="player">The player.</param>
void BattleGround::EventPlayerLoggedOut(Player* player)
{
    // player is correct pointer, it is checked in WorldSession::LogoutPlayer()
    m_OfflineQueue.push_back(player->GetObjectGuid());
    m_Players[player->GetObjectGuid()].OfflineRemoveTime = sWorld.GetGameTime() + MAX_OFFLINE_TIME;
    if (GetStatus() == STATUS_IN_PROGRESS)
    {
        // drop flag and handle other cleanups
        RemovePlayer(player, player->GetObjectGuid());

        // 1 player is logging out, if it is the last, then end arena!
        if (isArena())
        {
            if (GetAlivePlayersCountByTeam(player->GetTeam()) <= 1 && GetPlayersCountByTeam(GetOtherTeam(player->GetTeam())))
            {
                EndBattleGround(GetOtherTeam(player->GetTeam()));
            }
        }
    }
}

/// <summary>
/// This method should be called only once ... it adds pointer to queue.
/// </summary>
void BattleGround::AddToBGFreeSlotQueue()
{
    // make sure to add only once
    if (!m_InBGFreeSlotQueue && isBattleGround())
    {
        sBattleGroundMgr.BGFreeSlotQueue[m_TypeID].push_front(this);
        m_InBGFreeSlotQueue = true;
    }
}

/// <summary>
/// This method removes this battleground from free queue - it must be called when deleting battleground - not used now.
/// </summary>
void BattleGround::RemoveFromBGFreeSlotQueue()
{
    // set to be able to re-add if needed
    m_InBGFreeSlotQueue = false;
    BGFreeSlotQueueType& bgFreeSlot = sBattleGroundMgr.BGFreeSlotQueue[m_TypeID];
    for (BGFreeSlotQueueType::iterator itr = bgFreeSlot.begin(); itr != bgFreeSlot.end(); ++itr)
    {
        if ((*itr)->GetInstanceID() == GetInstanceID())
        {
            bgFreeSlot.erase(itr);
            return;
        }
    }
}

/// <summary>
/// get the number of free slots for team.
/// </summary>
/// <param name="team">The team.</param>
/// <returns>returns the number how many players can join battleground to MaxPlayersPerTeam</returns>
uint32 BattleGround::GetFreeSlotsForTeam(Team team) const
{
    // return free slot count to MaxPlayerPerTeam
    if (GetStatus() == STATUS_WAIT_JOIN || GetStatus() == STATUS_IN_PROGRESS)
    {
        return (GetInvitedCount(team) < GetMaxPlayersPerTeam()) ? GetMaxPlayersPerTeam() - GetInvitedCount(team) : 0;
    }

    return 0;
}

/// <summary>
/// Determines whether [has free slots].
/// </summary>
/// <returns></returns>
bool BattleGround::HasFreeSlots() const
{
    return GetPlayersSize() < GetMaxPlayers();
}

/// <summary>
/// Updates the player score.
/// </summary>
/// <param name="Source">The source.</param>
/// <param name="type">The type.</param>
/// <param name="value">The value.</param>
void BattleGround::UpdatePlayerScore(Player* Source, uint32 type, uint32 value)
{
    // this procedure is called from virtual function implemented in bg subclass
    BattleGroundScoreMap::const_iterator itr = m_PlayerScores.find(Source->GetObjectGuid());

    if (itr == m_PlayerScores.end())                        // player not found...
    {
        return;
    }

    switch (type)
    {
        case SCORE_KILLING_BLOWS:                           // Killing blows
            itr->second->KillingBlows += value;
            break;
        case SCORE_DEATHS:                                  // Deaths
            itr->second->Deaths += value;
            break;
        case SCORE_HONORABLE_KILLS:                         // Honorable kills
            itr->second->HonorableKills += value;
            break;
        case SCORE_BONUS_HONOR:                             // Honor bonus
            // do not add honor in arenas
            if (isBattleGround())
            {
                // reward honor instantly
                if (Source->RewardHonor(NULL, 1, (float)value))
                {
                    itr->second->BonusHonor += value;
                }
            }
            break;
            // used only in EY, but in MSG_PVP_LOG_DATA opcode
        case SCORE_DAMAGE_DONE:                             // Damage Done
            itr->second->DamageDone += value;
            break;
        case SCORE_HEALING_DONE:                            // Healing Done
            itr->second->HealingDone += value;
            break;
        default:
            sLog.outError("BattleGround: Unknown player score type %u", type);
            break;
    }
}











/// <summary>
/// Sends the message to all.
/// </summary>
/// <param name="entry">The entry.</param>
/// <param name="type">The type.</param>
/// <param name="source">The source.</param>
void BattleGround::SendMessageToAll(int32 entry, ChatMsg type, Player const* source)
{
    MaNGOS::BattleGroundChatBuilder bg_builder(type, entry, source);
    MaNGOS::LocalizedPacketDo<MaNGOS::BattleGroundChatBuilder> bg_do(bg_builder);
    BroadcastWorker(bg_do);
}

/// <summary>
/// Sends the yell to all.
/// </summary>
/// <param name="entry">The entry.</param>
/// <param name="language">The language.</param>
/// <param name="guid">The GUID.</param>
void BattleGround::SendYellToAll(int32 entry, uint32 language, ObjectGuid guid)
{
    Creature* source = GetBgMap()->GetCreature(guid);
    if (!source)
    {
        return;
    }
    MaNGOS::BattleGroundYellBuilder bg_builder(Language(language), entry, source);
    MaNGOS::LocalizedPacketDo<MaNGOS::BattleGroundYellBuilder> bg_do(bg_builder);
    BroadcastWorker(bg_do);
}

/// <summary>
/// Ps the send message to all.
/// </summary>
/// <param name="entry">The entry.</param>
/// <param name="type">The type.</param>
/// <param name="source">The source.</param>
/// <param name="">The .</param>
void BattleGround::PSendMessageToAll(int32 entry, ChatMsg type, Player const* source, ...)
{
    va_list ap;
    va_start(ap, source);

    MaNGOS::BattleGroundChatBuilder bg_builder(type, entry, source, &ap);
    MaNGOS::LocalizedPacketDo<MaNGOS::BattleGroundChatBuilder> bg_do(bg_builder);
    BroadcastWorker(bg_do);

    va_end(ap);
}

/// <summary>
/// Sends the message2 to all.
/// </summary>
/// <param name="entry">The entry.</param>
/// <param name="type">The type.</param>
/// <param name="source">The source.</param>
/// <param name="arg1">The arg1.</param>
/// <param name="arg2">The arg2.</param>
void BattleGround::SendMessage2ToAll(int32 entry, ChatMsg type, Player const* source, int32 arg1, int32 arg2)
{
    MaNGOS::BattleGround2ChatBuilder bg_builder(type, entry, source, arg1, arg2);
    MaNGOS::LocalizedPacketDo<MaNGOS::BattleGround2ChatBuilder> bg_do(bg_builder);
    BroadcastWorker(bg_do);
}

/// <summary>
/// Sends the yell2 to all.
/// </summary>
/// <param name="entry">The entry.</param>
/// <param name="language">The language.</param>
/// <param name="guid">The GUID.</param>
/// <param name="arg1">The arg1.</param>
/// <param name="arg2">The arg2.</param>
void BattleGround::SendYell2ToAll(int32 entry, uint32 language, ObjectGuid guid, int32 arg1, int32 arg2)
{
    Creature* source = GetBgMap()->GetCreature(guid);
    if (!source)
    {
        return;
    }
    MaNGOS::BattleGround2YellBuilder bg_builder(language, entry, source, arg1, arg2);
    MaNGOS::LocalizedPacketDo<MaNGOS::BattleGround2YellBuilder> bg_do(bg_builder);
    BroadcastWorker(bg_do);
}

/// <summary>
/// Ends the now.
/// </summary>
void BattleGround::EndNow()
{
    RemoveFromBGFreeSlotQueue();
    SetStatus(STATUS_WAIT_LEAVE);
    SetEndTime(0);
}

/*
*/
/// <summary>
/// important notice:
/// buffs aren't spawned/despawned when players captures anything
/// buffs are in their positions when battleground starts
/// </summary>
/// <param name="go_guid">The go_guid.</param>
void BattleGround::HandleTriggerBuff(ObjectGuid go_guid)
{
    GameObject* obj = GetBgMap()->GetGameObject(go_guid);
    if (!obj || obj->GetGoType() != GAMEOBJECT_TYPE_TRAP || !obj->isSpawned())
    {
        return;
    }

    obj->SetLootState(GO_JUST_DEACTIVATED);             // can be despawned or destroyed
    return;
}

/// <summary>
/// Handles the kill player.
/// </summary>
/// <param name="player">The player.</param>
/// <param name="killer">The killer.</param>
void BattleGround::HandleKillPlayer(Player* player, Player* killer)
{
    // add +1 deaths
    UpdatePlayerScore(player, SCORE_DEATHS, 1);

    // add +1 kills to group and +1 killing_blows to killer
    if (killer)
    {
        UpdatePlayerScore(killer, SCORE_HONORABLE_KILLS, 1);
        UpdatePlayerScore(killer, SCORE_KILLING_BLOWS, 1);

        for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
        {
            Player* plr = sObjectMgr.GetPlayer(itr->first);

            if (!plr || plr == killer)
            {
                continue;
            }

            if (plr->GetTeam() == killer->GetTeam() && plr->IsAtGroupRewardDistance(player))
            {
                UpdatePlayerScore(plr, SCORE_HONORABLE_KILLS, 1);
            }
        }
    }

    // to be able to remove insignia -- ONLY IN BattleGrounds
    if (!isArena())
    {
        player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
    }
}

/// <summary>
/// return the player's team based on battlegroundplayer info
/// used in same faction arena matches mainly
/// </summary>
/// <param name="guid">The GUID.</param>
/// <returns></returns>
Team BattleGround::GetPlayerTeam(ObjectGuid guid)
{
    BattleGroundPlayerMap::const_iterator itr = m_Players.find(guid);
    if (itr != m_Players.end())
    {
        return itr->second.PlayerTeam;
    }
    return TEAM_NONE;
}

/// <summary>
/// Determines whether [is player in battle ground] [the specified GUID].
/// </summary>
/// <param name="guid">The GUID.</param>
/// <returns></returns>
bool BattleGround::IsPlayerInBattleGround(ObjectGuid guid)
{
    BattleGroundPlayerMap::const_iterator itr = m_Players.find(guid);
    if (itr != m_Players.end())
    {
        return true;
    }
    return false;
}

/// <summary>
/// Players the added to BG check if BG is running.
/// </summary>
/// <param name="plr">The PLR.</param>
void BattleGround::PlayerAddedToBGCheckIfBGIsRunning(Player* plr)
{
    if (GetStatus() != STATUS_WAIT_LEAVE)
    {
        return;
    }

    WorldPacket data;
    BattleGroundQueueTypeId bgQueueTypeId = BattleGroundMgr::BGQueueTypeId(GetTypeID(), GetArenaType());

    BlockMovement(plr);

    sBattleGroundMgr.BuildPvpLogDataPacket(&data, this);
    plr->GetSession()->SendPacket(&data);

    sBattleGroundMgr.BuildBattleGroundStatusPacket(&data, this, plr->GetBattleGroundQueueIndex(bgQueueTypeId), STATUS_IN_PROGRESS, GetEndTime(), GetStartTime(), GetArenaType(), plr->GetBGTeam());
    plr->GetSession()->SendPacket(&data);
}

/// <summary>
/// Gets the alive players count by team.
/// </summary>
/// <param name="team">The team.</param>
/// <returns></returns>
uint32 BattleGround::GetAlivePlayersCountByTeam(Team team) const
{
    int count = 0;
    for (BattleGroundPlayerMap::const_iterator itr = m_Players.begin(); itr != m_Players.end(); ++itr)
    {
        if (itr->second.PlayerTeam == team)
        {
            Player* pl = sObjectMgr.GetPlayer(itr->first);
            if (pl && pl->IsAlive())
            {
                ++count;
            }
        }
    }
    return count;
}

void BattleGround::CheckArenaWinConditions()
{
    if (!GetAlivePlayersCountByTeam(ALLIANCE) && GetPlayersCountByTeam(HORDE))
    {
        EndBattleGround(HORDE);
    }
    else if (GetPlayersCountByTeam(ALLIANCE) && !GetAlivePlayersCountByTeam(HORDE))
    {
        EndBattleGround(ALLIANCE);
    }
}

/// <summary>
/// Sets the bg raid.
/// </summary>
/// <param name="team">The team.</param>
/// <param name="bg_raid">The bg_raid.</param>
void BattleGround::SetBgRaid(Team team, Group* bg_raid)
{
    Group*& old_raid = m_BgRaids[GetTeamIndexByTeamId(team)];

    if (old_raid)
    {
        old_raid->SetBattlegroundGroup(NULL);
    }

    if (bg_raid)
    {
        bg_raid->SetBattlegroundGroup(this);
    }

    old_raid = bg_raid;
}

/// <summary>
/// Gets the closest grave yard.
/// </summary>
/// <param name="player">The player.</param>
/// <returns></returns>
WorldSafeLocsEntry const* BattleGround::GetClosestGraveYard(Player* player)
{
    return sObjectMgr.GetClosestGraveYard(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetMapId(), player->GetTeam());
}

bool BattleGround::IsTeamScoreInRange(Team team, uint32 minScore, uint32 maxScore) const
{
    PvpTeamIndex team_idx = GetTeamIndexByTeamId(team);
    uint32 score = (m_TeamScores[team_idx] < 0) ? 0 : uint32(m_TeamScores[team_idx]);
    return score >= minScore && score <= maxScore;
}

void BattleGround::SetBracket(PvPDifficultyEntry const* bracketEntry)
{
    m_BracketId  = bracketEntry->GetBracketId();
    SetLevelRange(bracketEntry->MinLevel, bracketEntry->MaxLevel);
}

/// <summary>
/// Gets the winner in case of premature finish of the BG.
/// Different BG's may have different criteria for choosing the winner besides simple player accounting
/// </summary>
/// <returns>The winner team</returns>
Team BattleGround::GetPrematureWinner()
{
    uint32 hordePlayers = GetPlayersCountByTeam(HORDE);
    uint32 alliancePlayers = GetPlayersCountByTeam(ALLIANCE);

    if (alliancePlayers > hordePlayers)
    {
        return ALLIANCE;
    }

    if (hordePlayers > alliancePlayers)
    {
        return HORDE;
    }

    return TEAM_NONE;
}
