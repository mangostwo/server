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

#ifndef MANGOS_PROTO_IWORLDGATEWAY_H
#define MANGOS_PROTO_IWORLDGATEWAY_H

#include "IClientLink.h"

#include "Auth/BigNumber.h"
#include "Platform/Define.h"
#include "Utilities/WorldPacket.h"

#include <memory>
#include <string>
#include <vector>

namespace proto
{
    /**
     * @brief Opaque handle the world hands back for an attached session.
     *
     * The protocol layer never dereferences it; it only quotes it back on
     * Deliver()/Detach(). Zero means "no session".
     */
    typedef uint32 SessionId;

    static const SessionId INVALID_SESSION_ID = 0;

    /**
     * @brief Verdict codes as they go out in SMSG_AUTH_RESPONSE.
     *
     * These are wire values, so they belong to the protocol rather than to the
     * game. The world returns one of them and the protocol layer serialises it;
     * neither side needs the other's headers to agree on the meaning.
     */
    enum class AuthStatus : uint8
    {
        Ok              = 0x0C,
        Failed          = 0x0D,
        Reject          = 0x0E,
        BadServerProof  = 0x0F,
        Unavailable     = 0x10,
        SystemError     = 0x11,
        VersionMismatch = 0x14,
        UnknownAccount  = 0x15,
        WaitQueue       = 0x1B,
        Banned          = 0x1C,
        Suspended       = 0x20
    };

    /**
     * @brief Everything the client claimed in CMSG_AUTH_SESSION.
     *
     * Purely what was on the wire -- nothing here has been trusted yet.
     */
    struct AuthRequest
    {
        uint32      build;       ///< client build number
        std::string account;     ///< account name, as sent (upper-case)
        uint32      clientSeed;  ///< client's half of the proof nonce
        uint8       digest[20];  ///< the client's SHA-1 proof
        std::vector<uint8> addonData; ///< compressed addon block, opaque here
        std::string peerAddress; ///< remote IP, for IP locking and ban checks
    };

    /**
     * @brief World-side state carried from LookupAccount() to Attach().
     *
     * The protocol layer never looks inside this; it only holds it across the
     * proof step and hands it back. That is what keeps the account row -- security
     * level, expansion, mute time, locale, client OS -- from having to be fetched
     * twice, without any of those fields appearing in an interface that has no
     * business knowing they exist.
     *
     * The world derives whatever it likes from this and owns the lifetime.
     */
    struct AuthContext
    {
        virtual ~AuthContext() {}
    };

    /**
     * @brief What the world knows about an account, once it has looked it up.
     */
    struct AuthLookup
    {
        AuthStatus status;      ///< Ok only if the account may proceed to the proof
        BigNumber  sessionKey;  ///< meaningful only when status == Ok

        /// Opaque, world-owned. Returned to Attach() untouched.
        std::shared_ptr<AuthContext> context;

        AuthLookup() : status(AuthStatus::UnknownAccount) {}
    };

    /**
     * @brief The one and only seam between the protocol layer and the world.
     *
     * Everything above this line is wire format and cryptography; everything below
     * it is policy and persistence. The split is deliberate and total:
     *
     *   - The world performs every database query, every ban and IP-lock check,
     *     every configuration decision, and owns the WorldSession object.
     *   - The protocol layer performs every byte-level operation, owns the framing
     *     and the header cipher, and verifies the SHA-1 proof -- for which it needs
     *     the session key, and nothing else, from the world.
     *
     * No decision is taken on both sides. The protocol library therefore links
     * neither the game nor the database, which is what makes the boundary real
     * rather than a naming convention.
     *
     * Threading: LookupAccount() and Attach() are called on a network thread, one
     * connection at a time. Deliver() may be called concurrently for distinct
     * sessions and must be safe against the world thread draining them.
     */
    class IWorldGateway
    {
        public:

            virtual ~IWorldGateway() {}

            /**
             * @brief Resolve an account and decide whether it may log in at all.
             *
             * This is where the account row, bans, IP lock, allowed build, security
             * level and client OS are checked -- all of it world and database
             * business. Returning anything other than AuthStatus::Ok makes the
             * protocol layer send that code and drop the connection.
             *
             * @param request What the client claimed. Untrusted.
             * @return The verdict, plus the session key when the account may proceed.
             */
            virtual AuthLookup LookupAccount(const AuthRequest& request) = 0;

            /**
             * @brief Build the world-side session for a client that proved itself.
             *
             * Called only after LookupAccount() returned Ok and the SHA-1 proof
             * verified, so the account is authentic by the time the world commits
             * any state to it.
             *
             * @param request The (now trusted) login request.
             * @param link    How to talk back to this client. The world keeps it
             *                for the life of the session; it is shared rather than
             *                raw so that a session outliving its socket -- which
             *                happens whenever a player disconnects mid-tick -- sends
             *                into a disarmed link instead of freed memory.
             * @param context Whatever LookupAccount() attached, handed straight
             *                back so the account row need not be re-read.
             * @return A handle for later Deliver()/Detach(), or INVALID_SESSION_ID
             *         if the world declined to create the session after all.
             */
            virtual SessionId Attach(const AuthRequest& request,
                                     const std::shared_ptr<IClientLink>& link,
                                     const std::shared_ptr<AuthContext>& context) = 0;

            /**
             * @brief Hand a decoded packet to an attached session.
             *
             * The protocol layer has already framed and decrypted it and knows
             * nothing about what it means.
             */
            virtual void Deliver(SessionId session, WorldPacket&& packet) = 0;

            /**
             * @brief The connection is gone; release the session.
             *
             * The world may keep the session alive past this call to unwind game
             * state, which is what net::ISession::reapable() exists to express.
             */
            virtual void Detach(SessionId session) = 0;
    };
}

#endif
