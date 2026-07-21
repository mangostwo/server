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

#include <string>
#include "WorldNetwork.h"

#include "Log/Log.h"
#include "OpcodeTable.h"

INSTANTIATE_SINGLETON_1(WorldNetwork);

WorldNetwork::WorldNetwork()
    : m_gateway(),
      m_listener(m_gateway)
{
}

WorldNetwork::~WorldNetwork()
{
    Stop();
}

bool WorldNetwork::Start(uint16 port, const std::string& bindIp)
{
    // Must happen before the listener opens: opcodeTable is a plain array with
    // static storage, so until this runs every entry is name = nullptr,
    // handler = nullptr. A connection arriving first would dispatch through a
    // null handler.
    //
    // WorldSocketMgr::StartNetwork used to make this call. That class is gone,
    // and its replacement (proto::Listener) sits on the far side of the
    // networking boundary and must not know game opcodes exist -- so the call
    // belongs here, on the game side, which is the last place that owns both.
    InitializeOpcodes();

    if (!m_listener.Start(port, bindIp))
    {
        sLog.outError("Failed to bind the world listener to %s:%u",
                      bindIp.empty() ? "0.0.0.0" : bindIp.c_str(), uint32(port));
        return false;
    }

    return true;
}

void WorldNetwork::Stop()
{
    m_listener.Stop();
}
