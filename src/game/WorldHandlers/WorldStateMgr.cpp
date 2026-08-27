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

#include "WorldStateMgr.h"

#include "DBCStores.h"
#include "DBCStructure.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
    /// Returned by reference for a state no landmark uses, so callers need no null check.
    std::vector<AreaPOIEntry const*> const s_noPois;

    /// The highest icon slot a row carries. The client clamps anything above this to
    /// Icon[0] rather than reading past the row, so a larger value is not a crash --
    /// it is just an icon nobody meant.
    const uint32 MAX_POI_ICON_SLOT = 9;
}

void WorldStateMgr::Initialize()
{
    m_poiByState.clear();

    uint32 gated = 0;
    for (uint32 i = 0; i < sAreaPOIStore.GetNumRows(); ++i)
    {
        AreaPOIEntry const* poi = sAreaPOIStore.LookupEntry(i);
        if (!poi)
        {
            continue;
        }

        // A row with no world state is drawn unconditionally and the server has no say
        // in it whatsoever -- there is nothing to index and nothing to send.
        if (!poi->WorldStateID)
        {
            continue;
        }

        m_poiByState[poi->WorldStateID].push_back(poi);
        ++gated;
    }

    sLog.outString(">> Indexed %u map landmark(s) driven by a world state, over %u state(s)",
                   gated, uint32(m_poiByState.size()));
}

uint32 WorldStateMgr::GetState(uint32 zoneId, uint32 stateId) const
{
    std::shared_lock<std::shared_mutex> guard(m_lock);

    StateZoneMap::const_iterator zone = m_states.find(zoneId);
    if (zone == m_states.end())
    {
        return 0;
    }

    StateValueMap::const_iterator state = zone->second.find(stateId);
    return state != zone->second.end() ? state->second : 0;
}

bool WorldStateMgr::SetState(uint32 zoneId, uint32 stateId, uint32 value)
{
    if (!stateId)
    {
        return false;
    }

    {
        std::unique_lock<std::shared_mutex> guard(m_lock);

        uint32& stored = m_states[zoneId][stateId];
        if (stored == value)
        {
            return false;
        }

        stored = value;
    }

    // Outside the lock. Broadcast walks every session and writes to sockets; holding a
    // write lock across that would stall every map thread reading a landmark's state
    // for as long as the slowest client's send buffer takes.
    Broadcast(zoneId, stateId, value);
    return true;
}

bool WorldStateMgr::SetPoiState(uint32 poiId, uint32 value)
{
    AreaPOIEntry const* poi = sAreaPOIStore.LookupEntry(poiId);
    if (!poi)
    {
        sLog.outError("WorldStateMgr: no AreaPOI row %u", poiId);
        return false;
    }

    if (!poi->WorldStateID)
    {
        sLog.outError("WorldStateMgr: AreaPOI %u carries no world state -- the client "
                      "draws it unconditionally and the server cannot change it", poiId);
        return false;
    }

    if (value > MAX_POI_ICON_SLOT)
    {
        sLog.outError("WorldStateMgr: AreaPOI %u given icon slot %u; the row holds %u, "
                      "and the client clamps anything past it back to the first icon",
                      poiId, value, MAX_POI_ICON_SLOT);
    }

    return SetState(ZoneOfPoi(*poi), poi->WorldStateID, value);
}

void WorldStateMgr::FillInitialStates(uint32 zoneId, ByteBuffer& data, uint32& count) const
{
    std::shared_lock<std::shared_mutex> guard(m_lock);

    // Zone 0 first, then the zone's own: a zone may deliberately override a global
    // default, and the client keeps whichever value it was told last.
    const uint32 keys[2] = { 0, zoneId };
    for (int i = 0; i < 2; ++i)
    {
        if (i == 1 && zoneId == 0)
        {
            break;
        }

        StateZoneMap::const_iterator zone = m_states.find(keys[i]);
        if (zone == m_states.end())
        {
            continue;
        }

        for (StateValueMap::const_iterator itr = zone->second.begin();
             itr != zone->second.end(); ++itr)
        {
            data << uint32(itr->first);
            data << uint32(itr->second);
            ++count;
        }
    }
}

std::vector<AreaPOIEntry const*> const& WorldStateMgr::PoisOfState(uint32 stateId) const
{
    std::unordered_map<uint32, std::vector<AreaPOIEntry const*> >::const_iterator itr =
        m_poiByState.find(stateId);

    return itr != m_poiByState.end() ? itr->second : s_noPois;
}

std::vector<AreaPOIEntry const*> WorldStateMgr::PoisOfZone(uint32 zoneId) const
{
    std::vector<AreaPOIEntry const*> found;

    for (std::unordered_map<uint32, std::vector<AreaPOIEntry const*> >::const_iterator itr =
             m_poiByState.begin(); itr != m_poiByState.end(); ++itr)
    {
        for (AreaPOIEntry const* poi : itr->second)
        {
            if (ZoneOfPoi(*poi) == zoneId)
            {
                found.push_back(poi);
            }
        }
    }

    return found;
}

uint32 WorldStateMgr::ZoneOfPoi(AreaPOIEntry const& poi)
{
    // AreaPOI names an AREA, and an area is often a sub-zone -- "Wintergrasp Fortress"
    // rather than "Wintergrasp". The client is told world states per ZONE, so the walk up
    // to the parent is what makes the key match what the player will be standing in.
    AreaTableEntry const* area = GetAreaEntryByAreaID(poi.AreaID);
    if (!area)
    {
        return 0;
    }

    return area->ParentAreaID ? area->ParentAreaID : area->ID;
}

void WorldStateMgr::Broadcast(uint32 zoneId, uint32 stateId, uint32 value) const
{
    WorldPacket data(SMSG_UPDATE_WORLD_STATE, 4 + 4);
    data << uint32(stateId);
    data << uint32(value);

    SessionMap const& sessions = sWorld.GetAllSessions();
    for (SessionMap::const_iterator itr = sessions.begin(); itr != sessions.end(); ++itr)
    {
        WorldSession* session = itr->second;
        if (!session)
        {
            continue;
        }

        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
        {
            continue;
        }

        // Zone 0 is everyone; anything else only reaches the players who are being told
        // about that zone in the first place, and who will be told again on the way in.
        if (zoneId && player->GetCachedZoneId() != zoneId)
        {
            continue;
        }

        session->SendPacket(&data);
    }
}
