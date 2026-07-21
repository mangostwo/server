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

#ifndef MANGOS_H_WORLDGATEWAY
#define MANGOS_H_WORLDGATEWAY

#include "IWorldGateway.h"

#include <memory>
#include <mutex>
#include <unordered_map>

class WorldSession;

/**
 * @brief The world's side of the protocol seam.
 *
 * Everything the old WorldSocket used to reach for -- the login database, ban and
 * IP-lock checks, the allowed client build, the security floor, Warden's client-OS
 * rule, the scripting hook, and the WorldSession object itself -- lives here, on
 * the far side of an interface the protocol library knows nothing about.
 *
 * The protocol layer refers to a session only by an opaque SessionId. That
 * indirection is not ceremony: it means a connection can never be handed a
 * WorldSession pointer it might outlive, and the registry below is the single
 * place where the mapping is resolved under a lock.
 */
class WorldGateway : public proto::IWorldGateway
{
    public:

        WorldGateway();
        ~WorldGateway() override;

        // --- proto::IWorldGateway -----------------------------------------

        proto::AuthLookup LookupAccount(const proto::AuthRequest& request) override;

        proto::SessionId Attach(const proto::AuthRequest& request,
                                const std::shared_ptr<proto::IClientLink>& link,
                                const std::shared_ptr<proto::AuthContext>& context) override;

        void Deliver(proto::SessionId session, WorldPacket&& packet) override;

        bool OnPing(proto::SessionId session, uint32 latency,
                    uint32 fastPingRun) override;

        void Detach(proto::SessionId session) override;

    private:

        /// Resolve a handle to a live session, or NULL. Caller must hold m_lock.
        WorldSession* Find(proto::SessionId session) const;

        mutable std::mutex m_lock;

        /// Handles are drawn from a counter rather than reusing account ids, so a
        /// stale handle from a torn-down connection can never resolve onto the
        /// session of a player who has since logged back in.
        proto::SessionId m_nextId;

        std::unordered_map<proto::SessionId, WorldSession*> m_sessions;
};

#endif
