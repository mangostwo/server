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
 * @file MiscHandlerInstance.cpp
 * @brief Cohesion split of MiscHandler.cpp -- instance-difficulty opcodes:
 *        reset instances, set dungeon difficulty, set raid difficulty.
 *        Same `WorldSession` handlers; no behaviour change.
 */

#include <zlib.h>
#include "Platform/Define.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Database/DatabaseImpl.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "World.h"
#include "CinematicFlyover.h"
#include "GuildMgr.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "Auth/BigNumber.h"
#include "Auth/Sha1.h"
#include "UpdateData.h"
#include "LootMgr.h"
#include "Chat.h"
#include "ScriptMgr.h"
#include "Object.h"
#include "BattleGround/BattleGround.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "Pet.h"
#include "SocialMgr.h"
#include "DBCEnums.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Resets the player's or group's saved instances.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleResetInstancesOpcode(WorldPacket& /*recv_data*/)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_RESET_INSTANCES");

    if (Group* pGroup = _player->GetGroup())
    {
        if (pGroup->IsLeader(_player->GetObjectGuid()))
        {
            pGroup->ResetInstances(INSTANCE_RESET_ALL, false, _player);
        }
    }
    else
    {
        _player->ResetInstances(INSTANCE_RESET_ALL, false);
    }
}

void WorldSession::HandleSetDungeonDifficultyOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode MSG_SET_DUNGEON_DIFFICULTY");

    uint32 mode;
    recv_data >> mode;

    if (mode >= MAX_DUNGEON_DIFFICULTY)
    {
        sLog.outError("WorldSession::HandleSetDungeonDifficultyOpcode: player %d sent an invalid instance mode %d!", _player->GetGUIDLow(), mode);
        return;
    }

    if (Difficulty(mode) == _player->GetDungeonDifficulty())
    {
        return;
    }

    // cannot reset while in an instance
    Map* map = _player->GetMap();
    if (map && map->IsDungeon())
    {
        sLog.outError("WorldSession::HandleSetDungeonDifficultyOpcode: player %d tried to reset the instance while inside!", _player->GetGUIDLow());
        return;
    }

    // Exception to set mode to normal for low-level players
    if (_player->getLevel() < LEVELREQUIREMENT_HEROIC && mode > REGULAR_DIFFICULTY)
    {
        return;
    }

    if (Group* pGroup = _player->GetGroup())
    {
        if (pGroup->IsLeader(_player->GetObjectGuid()))
        {
            // the difficulty is set even if the instances can't be reset
            //_player->SendDungeonDifficulty(true);
            pGroup->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, false, _player);
            pGroup->SetDungeonDifficulty(Difficulty(mode));
        }
    }
    else
    {
        _player->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, false);
        _player->SetDungeonDifficulty(Difficulty(mode));
    }
}

void WorldSession::HandleSetRaidDifficultyOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode MSG_SET_RAID_DIFFICULTY");

    uint32 mode;
    recv_data >> mode;

    if (mode >= MAX_RAID_DIFFICULTY)
    {
        sLog.outError("WorldSession::HandleSetRaidDifficultyOpcode: player %d sent an invalid instance mode %d!", _player->GetGUIDLow(), mode);
        return;
    }

    if (Difficulty(mode) == _player->GetRaidDifficulty())
    {
        return;
    }

    // cannot reset while in an instance
    Map* map = _player->GetMap();
    if (map && map->IsDungeon())
    {
        sLog.outError("WorldSession::HandleSetRaidDifficultyOpcode: player %d tried to reset the instance while inside!", _player->GetGUIDLow());
        return;
    }

    // Exception to set mode to normal for low-level players
    if (_player->getLevel() < LEVELREQUIREMENT_HEROIC && mode > REGULAR_DIFFICULTY)
    {
        return;
    }

    if (Group* pGroup = _player->GetGroup())
    {
        if (pGroup->IsLeader(_player->GetObjectGuid()))
        {
            // the difficulty is set even if the instances can't be reset
            //_player->SendDungeonDifficulty(true);
            pGroup->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true, _player);
            pGroup->SetRaidDifficulty(Difficulty(mode));
        }
    }
    else
    {
        _player->ResetInstances(INSTANCE_RESET_CHANGE_DIFFICULTY, true);
        _player->SetRaidDifficulty(Difficulty(mode));
    }
}
