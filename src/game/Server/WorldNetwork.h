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

#ifndef MANGOS_H_WORLDNETWORK
#define MANGOS_H_WORLDNETWORK

#include "Listener.h"
#include "Policies/Singleton.h"
#include "WorldGateway.h"

#include <string>

/**
 * @brief Owns the world server's listening socket and its side of the seam.
 *
 * Two objects, in the right order: the gateway (the world's implementation of
 * the protocol contract) and the listener that hands every accepted connection
 * to it. Nothing else about networking survives at this level -- accepting,
 * worker threads, buffering, backpressure and teardown all belong to the shared
 * engine underneath.
 *
 * This exists as a separate object, rather than living in whatever currently
 * starts the server, so that the ownership and shutdown order stay put while
 * mangosd is rewritten around it.
 */
class WorldNetwork : public MaNGOS::Singleton<WorldNetwork>
{
        friend class MaNGOS::Singleton<WorldNetwork>;

    public:

        /**
         * @brief Bind and start accepting world connections.
         *
         * @param port   TCP port to listen on.
         * @param bindIp Interface to bind, or empty for all interfaces.
         * @return false if the port could not be bound.
         */
        bool Start(uint16 port, const std::string& bindIp);

        /// Stop accepting and tear down every live connection.
        void Stop();

    private:

        WorldNetwork();
        ~WorldNetwork();

        // Declaration order matters: the listener holds a reference to the
        // gateway, so the gateway must be constructed first and destroyed last.
        WorldGateway    m_gateway;
        proto::Listener m_listener;
};

#define sWorldNetwork MaNGOS::Singleton<WorldNetwork>::Instance()

#endif
