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

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>
#include <mutex>
#include "ClientConnection.h"

#include "Opcodes.h"

#include "Auth/BigNumber.h"
#include "Auth/Sha1.h"
#include "Log/Log.h"
#include "Utilities/ByteBuffer.h"

#include <cstring>

namespace proto
{
    namespace
    {
        /// Number of bytes in the client's SHA-1 login proof.
        const size_t AUTH_DIGEST_SIZE = 20;

        /**
         * @brief Draw the server authentication nonce from the cryptographic RNG.
         *
         * The old code used the general-purpose PRNG for this. The value is hashed
         * into the client's proof, so a predictable seed narrows the search space
         * for anyone replaying a captured login.
         */
        uint32 MakeAuthSeed()
        {
            BigNumber seed;
            seed.SetRand(32);
            return seed.AsDword();
        }
    }

    ClientConnection::ClientConnection(IWorldGateway& gateway)
        : m_gateway(gateway),
          m_codec(),
          m_seed(MakeAuthSeed()),
          m_session(INVALID_SESSION_ID),
          m_closed(false),
          m_hadPing(false),
          m_fastPingRun(0)
    {
    }

    ClientConnection::~ClientConnection()
    {
    }

    std::vector<uint8_t> ClientConnection::onConnect()
    {
        // SMSG_AUTH_CHALLENGE: one-slot shuffle marker, the server nonce, and two
        // 16-byte seeds the 3.3.5a client folds into its own crypt setup.
        WorldPacket packet(SMSG_AUTH_CHALLENGE, 40);
        packet << uint32(1);
        packet << m_seed;

        BigNumber seed1;
        seed1.SetRand(16 * 8);
        packet.append(seed1.AsByteArray(16), 16);

        BigNumber seed2;
        seed2.SetRand(16 * 8);
        packet.append(seed2.AsByteArray(16), 16);

        // The crypt is not armed yet, so this goes out in clear text -- which is
        // exactly right: the client cannot key its cipher until it has this packet.
        return PacketCodec::Encode(packet, PacketCodec::HeaderEncryptor());
    }

    std::vector<uint8_t> ClientConnection::onData(const uint8_t* data, size_t len)
    {
        std::vector<WorldPacket> packets;

        if (m_codec.Feed(data, len, packets) == DecodeStatus::Malformed)
        {
            sLog.outError("proto: malformed packet framing from %s, dropping",
                          m_address.c_str());
            Close();
            return std::vector<uint8_t>();
        }

        // A short read anywhere below unwinds onto a network worker thread, where
        // nothing else would catch it and the process would abort. Dropping the
        // peer has to be the worst a malformed packet can do.
        try
        {
            for (size_t i = 0; i < packets.size(); ++i)
            {
                if (!HandlePacket(std::move(packets[i])))
                {
                    Close();
                    break;
                }
            }
        }
        catch (ByteBufferException&)
        {
            sLog.outError("proto: short read handling packet from %s, dropping",
                          m_address.c_str());
            Close();
        }

        // Everything this class sends goes through SendPacket() (and therefore the
        // transport's Sender), because a reply may be produced on the world thread
        // long after this call returned. Nothing is ever returned inline.
        return std::vector<uint8_t>();
    }

    bool ClientConnection::HandlePacket(WorldPacket&& packet)
    {
        const uint16 opcode = uint16(packet.GetOpcode());

        switch (opcode)
        {
            case CMSG_AUTH_SESSION:
                if (m_session != INVALID_SESSION_ID)
                {
                    sLog.outError("proto: repeated CMSG_AUTH_SESSION from %s",
                                  m_address.c_str());
                    return false;
                }
                return HandleAuthSession(packet);

            case CMSG_PING:
                return HandlePing(packet);

            default:
                break;
        }

        if (m_session == INVALID_SESSION_ID)
        {
            sLog.outError("proto: opcode %u from unauthenticated peer %s",
                          opcode, m_address.c_str());
            return false;
        }

        m_gateway.Deliver(m_session, std::move(packet));
        return true;
    }

    bool ClientConnection::HandleAuthSession(WorldPacket& packet)
    {
        AuthRequest request;
        request.peerAddress = m_address;

        try
        {
            packet >> request.build;
            packet.read_skip<uint32>();
            packet >> request.account;
            packet.read_skip<uint32>();
            packet >> request.clientSeed;
            packet.read_skip<uint32>();
            packet.read_skip<uint32>();
            packet.read_skip<uint32>();
            packet.read_skip<uint64>();
            packet.read(request.digest, AUTH_DIGEST_SIZE);
        }
        catch (ByteBufferException&)
        {
            sLog.outError("proto: truncated CMSG_AUTH_SESSION from %s",
                          m_address.c_str());
            return false;
        }

        // Whatever is left is the addon block. It is opaque here -- the world
        // parses it, because the reply depends on the addon registry it owns.
        if (packet.rpos() < packet.size())
        {
            request.addonData.assign(packet.contents() + packet.rpos(),
                                     packet.contents() + packet.size());
        }

        // Policy and persistence: account row, bans, IP lock, allowed build,
        // security level, client OS. None of it belongs on this side of the seam.
        const AuthLookup lookup = m_gateway.LookupAccount(request);

        if (lookup.status != AuthStatus::Ok)
        {
            SendAuthStatus(lookup.status);
            sLog.outError("proto: login refused for account '%s' from %s (code %u)",
                          request.account.c_str(), m_address.c_str(),
                          uint32(lookup.status));
            return false;
        }

        // Cryptography stays on this side. The client proves it holds the same
        // session key realmd handed it, over both halves of the nonce.
        BigNumber sessionKey = lookup.sessionKey;
        const uint8 zero[4] = { 0, 0, 0, 0 };
        const uint32 clientSeed = request.clientSeed;
        const uint32 serverSeed = m_seed;

        Sha1Hash sha;
        sha.UpdateData(request.account);
        sha.UpdateData(zero, 4);
        sha.UpdateData(reinterpret_cast<const uint8*>(&clientSeed), 4);
        sha.UpdateData(reinterpret_cast<const uint8*>(&serverSeed), 4);
        sha.UpdateBigNumbers(&sessionKey, NULL);
        sha.Finalize();

        if (std::memcmp(sha.GetDigest(), request.digest, AUTH_DIGEST_SIZE) != 0)
        {
            SendAuthStatus(AuthStatus::Failed);
            sLog.outError("proto: bad login proof for account '%s' from %s",
                          request.account.c_str(), m_address.c_str());
            return false;
        }

        // Arm the cipher BEFORE the world is told, because the world answers with
        // SMSG_AUTH_RESPONSE (or a queue position) the moment it accepts the
        // session, and that reply must already be encrypted.
        m_crypt.Init(&sessionKey);
        m_codec.SetHeaderDecryptor(
            [this](uint8* header, size_t len) { m_crypt.DecryptRecv(header, len); });

        // Hand the world a share of our own lifetime. net::ISession is held by
        // shared_ptr from the moment the transport accepts, so this is well
        // formed here, and it is what allows a WorldSession to outlive its socket
        // long enough to unwind the player from the map.
        std::shared_ptr<IClientLink> link =
            std::static_pointer_cast<ClientConnection>(shared_from_this());

        const SessionId session = m_gateway.Attach(request, link, lookup.context);
        if (session == INVALID_SESSION_ID)
        {
            SendAuthStatus(AuthStatus::SystemError);
            return false;
        }

        m_session = session;

        DEBUG_LOG("proto: account '%s' authenticated from %s",
                  request.account.c_str(), m_address.c_str());
        return true;
    }

    bool ClientConnection::HandlePing(WorldPacket& packet)
    {
        uint32 ping    = 0;
        uint32 latency = 0;

        try
        {
            packet >> ping;
            packet >> latency;
        }
        catch (ByteBufferException&)
        {
            sLog.outError("proto: truncated CMSG_PING from %s", m_address.c_str());
            return false;
        }

        // The 3.3.5a client pings roughly every 30 seconds. Anything materially
        // faster is either a broken client or someone probing, so count the run;
        // a single early ping is jitter and must not be treated as either.
        static const std::chrono::seconds MIN_PING_INTERVAL(27);

        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();

        if (m_hadPing)
        {
            if (now - m_lastPing < MIN_PING_INTERVAL)
            {
                ++m_fastPingRun;
            }
            else
            {
                m_fastPingRun = 0;
            }
        }
        m_lastPing = now;
        m_hadPing  = true;

        if (!m_gateway.OnPing(m_session, latency, m_fastPingRun))
        {
            return false;
        }

        WorldPacket pong(SMSG_PONG, 4);
        pong << ping;
        SendPacket(pong);
        return true;
    }

    void ClientConnection::SendAuthStatus(AuthStatus status)
    {
        WorldPacket packet(SMSG_AUTH_RESPONSE, 1);
        packet << uint8(status);
        SendPacket(packet);
    }

    void ClientConnection::SendPacket(const WorldPacket& packet)
    {
        if (m_closed.load(std::memory_order_acquire) || !m_sender)
        {
            return;
        }

        std::vector<uint8_t> wire;
        {
            // The cipher is a stream: two threads encrypting headers concurrently
            // would interleave the keystream and desynchronise the client for good.
            std::lock_guard<std::mutex> lock(m_cryptSendLock);
            wire = PacketCodec::Encode(packet,
                [this](uint8* header, size_t len)
                {
                    if (m_crypt.IsInitialized())
                    {
                        m_crypt.EncryptSend(header, len);
                    }
                });
        }

        m_sender(wire.data(), wire.size());
    }

    void ClientConnection::Close()
    {
        m_closed.store(true, std::memory_order_release);
        if (m_closer)
        {
            m_closer();
        }
    }

    void ClientConnection::onClose()
    {
        m_closed.store(true, std::memory_order_release);

        if (m_session != INVALID_SESSION_ID)
        {
            m_gateway.Detach(m_session);
            m_session = INVALID_SESSION_ID;
        }
    }
}
