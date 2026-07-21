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

#ifndef MANGOS_PROTO_CLIENTCONNECTION_H
#define MANGOS_PROTO_CLIENTCONNECTION_H

#include <cstdint>
#include <utility>
#include "IClientLink.h"
#include "IWorldGateway.h"
#include "PacketCodec.h"

#include "Auth/AuthCrypt.h"
#include "net/ISession.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace proto
{
    /**
     * @brief One client connection, speaking the 3.3.5a world protocol.
     *
     * This is the whole of what used to be WorldSocket, minus everything that was
     * never protocol: no database, no configuration, no session object, no Warden,
     * no scripting hooks. What is left is exactly three jobs -- frame the stream,
     * run the header cipher, and prove the client is who it claims -- plus handing
     * the results to the world through IWorldGateway.
     *
     * Threading: the transport calls OnConnect/OnData/OnClose on a network thread,
     * one connection at a time. SendPacket() may be called from any thread (the
     * world update thread does, constantly), so header encryption is serialised and
     * the byte hand-off goes through net::Sender, which the transport disarms at
     * teardown -- a world thread still ticking a dying session merely sends into a
     * no-op rather than touching a freed socket.
     */
    class ClientConnection : public net::ISession, public IClientLink
    {
        public:

            explicit ClientConnection(IWorldGateway& gateway);
            ~ClientConnection() override;

            // --- net::ISession ------------------------------------------------

            void setPeerAddress(const std::string& address) override
            {
                m_address = address;
            }

            void setSender(net::Sender sender) override
            {
                m_sender = std::move(sender);
            }

            void setCloser(net::Closer closer) override
            {
                m_closer = std::move(closer);
            }

            std::vector<uint8_t> onConnect() override;
            std::vector<uint8_t> onData(const uint8_t* data, size_t len) override;
            void onClose() override;

            bool closed() const override
            {
                return m_closed.load(std::memory_order_acquire);
            }

            // --- IClientLink (what the world may do to us) --------------------

            /// Encode, encrypt and queue a packet. Safe from any thread, and a
            /// no-op once the peer is gone.
            void SendPacket(const WorldPacket& packet) override;

            /// Mark the connection dead and ask the transport to tear it down.
            void Close() override;

            const std::string& GetRemoteAddress() const override { return m_address; }

            bool IsClosed() const override { return closed(); }

        private:

            /// Dispatch one fully decoded packet. Returns false to drop the peer.
            bool HandlePacket(WorldPacket&& packet);

            bool HandleAuthSession(WorldPacket& packet);
            bool HandlePing(WorldPacket& packet);

            /// Send a bare SMSG_AUTH_RESPONSE carrying only a status byte.
            void SendAuthStatus(AuthStatus status);

            IWorldGateway& m_gateway;

            std::string m_address;

            PacketCodec m_codec;

            AuthCrypt  m_crypt;
            std::mutex m_cryptSendLock; ///< serialises header encryption on send

            /// Server half of the authentication nonce, drawn from the OpenSSL RNG
            /// rather than the general-purpose PRNG: it is an input to the client's
            /// SHA-1 proof, so a predictable value weakens the handshake.
            uint32 m_seed;

            SessionId m_session;

            std::atomic<bool> m_closed;

            net::Sender m_sender;
            net::Closer m_closer;

            /// When the previous CMSG_PING arrived; valid once m_hadPing is set.
            std::chrono::steady_clock::time_point m_lastPing;
            bool m_hadPing;

            /// Consecutive pings that arrived faster than a real client sends
            /// them. Reset the moment the cadence returns to normal, so ordinary
            /// network jitter cannot accumulate into a kick.
            uint32 m_fastPingRun;
    };
}

#endif
