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

#ifndef MANGOS_H_WORLDSTATEMGR
#define MANGOS_H_WORLDSTATEMGR

#include "Platform/Define.h"
#include "Policies/Singleton.h"

#include <map>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

class ByteBuffer;
struct AreaPOIEntry;

/**
 * @brief The world state store -- the one number a map landmark is drawn from.
 *
 * A LANDMARK IS NOT AN OBJECT. The fixed points the client draws on a zone map -- the
 * Wintergrasp towers, the walls, the gates, the workshops -- are not creatures, not
 * gameobjects and not anything the server can move. They are rows in AreaPOI.dbc, which
 * the server never sends and the client already has. All the server ever contributes is
 * ONE `uint32` per row:
 *
 *   value 0        the landmark is not drawn at all
 *   value 1..9     the landmark is drawn with Icon[value - 1] from its own row
 *
 * The client clamps the index to 0..8 and recomputes it on every query, so a change of
 * state repaints the icon with no list rebuild; only crossing zero adds or removes the
 * landmark. That is the whole mechanism. It costs two packets:
 * `SMSG_INIT_WORLD_STATES` when a player enters a zone, and `SMSG_UPDATE_WORLD_STATE`
 * whenever a value moves after that.
 *
 * THE ZONE IS PART OF THE KEY. The client keeps world states in one flat table with no
 * zone in it, but it is told them per zone and it is told them again on every zone
 * change -- so a state seeded for Wintergrasp must not follow a player to Icecrown, or
 * the two would overwrite each other in that flat table. Zone 0 is the exception and
 * means "everywhere": the arena season, the Wintergrasp countdown, anything the client
 * is expected to know regardless of where it is standing.
 *
 * This store is DELIBERATELY not the battleground one. A BattleGround fills its own
 * states from its own live state on demand, because they change per match; these are
 * durable world facts with no owner to ask, so they are kept here and pushed.
 */
class WorldStateMgr
{
    public:
        WorldStateMgr() {}
        ~WorldStateMgr() {}

        /// Index AreaPOI.dbc by world state. Call once, after the DBCs are loaded.
        void Initialize();

        /// Current value, or 0 when nothing has ever set it -- which is also "not drawn".
        uint32 GetState(uint32 zoneId, uint32 stateId) const;

        /**
         * @brief Store a world state and push it to everyone who can see it.
         * @return true when the value actually changed; false when it already held it.
         *
         * The push is edge-triggered on purpose: re-sending an unchanged value costs a
         * packet per player and buys nothing, since the client stores what it was told.
         */
        bool SetState(uint32 zoneId, uint32 stateId, uint32 value);

        /**
         * @brief Set the landmark's icon by AreaPOI id, resolving zone and state from the row.
         * @param poiId AreaPOI.dbc row id.
         * @param value 0 hides the landmark, 1..9 selects Icon[value - 1].
         * @return false when the row is unknown or carries no world state to move.
         */
        bool SetPoiState(uint32 poiId, uint32 value);

        /// Append every state known for this zone (plus the global ones) to an init packet.
        void FillInitialStates(uint32 zoneId, ByteBuffer& data, uint32& count) const;

        /// The AreaPOI rows a world state drives, empty when it drives none.
        std::vector<AreaPOIEntry const*> const& PoisOfState(uint32 stateId) const;

        /// Every AreaPOI row that carries a world state and sits in this zone.
        std::vector<AreaPOIEntry const*> PoisOfZone(uint32 zoneId) const;

        /// The zone a landmark belongs to -- AreaPOI names an area, which may be a sub-zone.
        static uint32 ZoneOfPoi(AreaPOIEntry const& poi);

    private:
        typedef std::map<uint32, uint32> StateValueMap;      ///< stateId -> value, ordered so
                                                             ///< the init packet is stable
        typedef std::unordered_map<uint32, StateValueMap> StateZoneMap;

        /// Send one SMSG_UPDATE_WORLD_STATE to every player the zone key covers.
        void Broadcast(uint32 zoneId, uint32 stateId, uint32 value) const;

        /// Reads come from anywhere, including map threads; only the push is the world
        /// thread's, because it walks the session map.
        mutable std::shared_mutex m_lock;
        StateZoneMap m_states;

        /// Built once at start-up from AreaPOI.dbc and then only read, so it needs no lock.
        std::unordered_map<uint32, std::vector<AreaPOIEntry const*> > m_poiByState;
};

/**
 * @brief Global world state store instance
 */
#define sWorldStateMgr MaNGOS::Singleton<WorldStateMgr>::Instance()

#endif
