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

#include <utility>
#include <memory>
#include <mutex>
#include "Common/Locales.h"
#include <algorithm>
#include "Common/ServerDefines.h"
#include "WorldGateway.h"

#include "DBCStores.h"
#include "Database/DatabaseEnv.h"
#include "Log/Log.h"
#include "SharedDefines.h"
#include "SessionMailbox.h"
#include "World.h"
#include "WorldSession.h"

#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif

#include <string>

namespace
{
    /**
     * @brief The account row, read once during LookupAccount and reused by Attach.
     *
     * This is exactly the set of columns the old WorldSocket pulled out of the
     * login database and then had to carry, as loose locals, through four hundred
     * lines of interleaved policy. Naming it makes the hand-off explicit and stops
     * the row being fetched twice.
     */
    struct AccountRow : public proto::AuthContext
    {
        uint32         id        = 0;
        AccountTypes   security  = SEC_PLAYER;
        uint8          expansion = 0;
        time_t         muteTime  = 0;
        LocaleConstant locale    = LOCALE_enUS;
        std::string    os;
        BigNumber      sessionKey;
    };

    /**
     * @brief Register the calling thread with the database client library.
     *
     * The protocol layer calls LookupAccount()/Attach() on one of the network
     * engine's worker threads, and those threads query LoginDatabase and
     * CharacterDatabase. The MySQL client keeps per-thread state that every such
     * thread has to set up and tear down, or it corrupts and leaks it -- a
     * failure that shows up far from its cause and only under load.
     *
     * The registration lives here rather than in the network engine on purpose:
     * net must not know a database exists. A function-local thread_local runs the
     * guard's constructor the first time a given worker thread reaches this code
     * and its destructor when that thread exits, so the transport stays oblivious
     * while every thread that touches the database is still accounted for.
     *
     * One call covers the thread for the whole client library -- mysql_thread_init
     * is per-thread, not per-connection -- so a single guard suffices for both
     * databases used below.
     */
    void EnsureDbThreadRegistered()
    {
        static thread_local DbThreadGuard guard(&LoginDatabase);
        (void)guard;
    }
}

WorldGateway::WorldGateway()
    : m_nextId(1)
{
}

WorldGateway::~WorldGateway()
{
}

proto::AuthLookup WorldGateway::LookupAccount(const proto::AuthRequest& request)
{
    EnsureDbThreadRegistered();

    proto::AuthLookup result;

    // ---- Client build ----------------------------------------------------
    if (!IsAcceptableClientBuild(request.build))
    {
        result.status = proto::AuthStatus::VersionMismatch;
        return result;
    }

    // ---- Account row -----------------------------------------------------
    std::string safeAccount = request.account;
    LoginDatabase.escape_string(safeAccount);

    QueryResult* queryResult =
        LoginDatabase.PQuery("SELECT "
                             "`id`, "          // 0
                             "`gmlevel`, "     // 1
                             "`sessionkey`, "  // 2
                             "`last_ip`, "     // 3
                             "`locked`, "      // 4
                             "`expansion`, "   // 5
                             "`mutetime`, "    // 6
                             "`locale`, "      // 7
                             "`os` "           // 8
                             "FROM `account` WHERE `username` = '%s'",
                             safeAccount.c_str());

    if (!queryResult)
    {
        result.status = proto::AuthStatus::UnknownAccount;
        return result;
    }

    const Field* fields = queryResult->Fetch();

    std::shared_ptr<AccountRow> row = std::make_shared<AccountRow>();

    row->id = fields[0].GetUInt32();

    // Clamp rather than trust: a bad gmlevel in the database must not hand out
    // more authority than the server has levels for.
    uint32 security = fields[1].GetUInt16();
    if (security > SEC_ADMINISTRATOR)
    {
        security = SEC_ADMINISTRATOR;
    }
    row->security = AccountTypes(security);

    row->sessionKey.SetHexStr(fields[2].GetString());

    const std::string lastIp = fields[3].GetString();
    const bool        locked = fields[4].GetUInt8() == 1;

    row->expansion = uint8(std::min<uint32>(sWorld.getConfig(CONFIG_UINT32_EXPANSION),
                                            fields[5].GetUInt8()));
    row->muteTime  = time_t(fields[6].GetUInt64());

    const uint8 rawLocale = fields[7].GetUInt8();
    row->locale = rawLocale >= MAX_LOCALE ? LOCALE_enUS : LocaleConstant(rawLocale);

    row->os = fields[8].GetString();

    delete queryResult;

    // ---- IP lock ---------------------------------------------------------
    if (locked && lastIp != request.peerAddress)
    {
        sLog.outBasic("WorldGateway: account '%s' is IP locked to %s but connected from %s",
                      request.account.c_str(), lastIp.c_str(),
                      request.peerAddress.c_str());
        result.status = proto::AuthStatus::Failed;
        return result;
    }

    // ---- Bans ------------------------------------------------------------
    QueryResult* banResult =
        LoginDatabase.PQuery("SELECT 1 FROM `account_banned` WHERE `id` = %u AND `active` = 1 "
                             "AND (`unbandate` > UNIX_TIMESTAMP() OR `unbandate` = `bandate`) "
                             "UNION "
                             "SELECT 1 FROM `ip_banned` WHERE (`unbandate` = `bandate` OR "
                             "`unbandate` > UNIX_TIMESTAMP()) AND `ip` = '%s'",
                             row->id, request.peerAddress.c_str());

    if (banResult)
    {
        delete banResult;
        sLog.outBasic("WorldGateway: banned account '%s' tried to connect from %s",
                      request.account.c_str(), request.peerAddress.c_str());
        result.status = proto::AuthStatus::Banned;
        return result;
    }

    // ---- Security floor (server closed to ordinary players) --------------
    const AccountTypes allowed = sWorld.GetPlayerSecurityLimit();
    if (allowed > SEC_PLAYER && row->security < allowed)
    {
        result.status = proto::AuthStatus::Unavailable;
        return result;
    }

    // ---- Warden's client OS rule -----------------------------------------
    const bool wardenActive = sWorld.getConfig(CONFIG_BOOL_WARDEN_WIN_ENABLED)
                           || sWorld.getConfig(CONFIG_BOOL_WARDEN_OSX_ENABLED);

    if (wardenActive && row->os != "Win" && row->os != "OSX")
    {
        sLog.outError("WorldGateway: client %s reported invalid OS '%s'",
                      request.peerAddress.c_str(), row->os.c_str());
        result.status = proto::AuthStatus::Reject;
        return result;
    }

    result.status     = proto::AuthStatus::Ok;
    result.sessionKey = row->sessionKey;
    result.context    = row;
    return result;
}

proto::SessionId WorldGateway::Attach(const proto::AuthRequest& request,
                                      const std::shared_ptr<proto::IClientLink>& link,
                                      const std::shared_ptr<proto::AuthContext>& context)
{
    EnsureDbThreadRegistered();

    std::shared_ptr<AccountRow> row =
        std::dynamic_pointer_cast<AccountRow>(context);
    if (!link || !row)
    {
        return proto::INVALID_SESSION_ID;
    }

    // The proof has already been verified by this point, so recording the address
    // here is recording an authenticated login rather than an attempt.
    static SqlStatementID updAccount;
    SqlStatement stmt = LoginDatabase.CreateStatement(
        updAccount, "UPDATE `account` SET `last_ip` = ? WHERE `username` = ?");
    stmt.PExecute(request.peerAddress.c_str(), request.account.c_str());

    std::shared_ptr<SessionMailbox> mailbox =
        std::make_shared<SessionMailbox>();
    std::unique_ptr<WorldSession> session =
        std::make_unique<WorldSession>(
            row->id, link, mailbox, row->security, row->expansion,
            row->muteTime, row->locale, row->sessionKey);

    session->LoadGlobalAccountData();
    session->LoadTutorialsData();

    // The addon block was left opaque by the protocol layer precisely so it could
    // be parsed here, where the addon registry lives.
    WorldPacket addonPacket(CMSG_AUTH_SESSION, request.addonData.size());
    if (!request.addonData.empty())
    {
        addonPacket.append(request.addonData.data(), request.addonData.size());
    }
    session->ReadAddonsInfo(addonPacket);

    const bool wardenActive = sWorld.getConfig(CONFIG_BOOL_WARDEN_WIN_ENABLED)
                           || sWorld.getConfig(CONFIG_BOOL_WARDEN_OSX_ENABLED);
    if (wardenActive)
    {
        session->InitWarden(uint16(request.build), &row->sessionKey, row->os);
    }

    if (link->IsClosed())
    {
        return proto::INVALID_SESSION_ID;
    }

    proto::SessionId id;
    {
        std::lock_guard<std::mutex> lock(m_lock);
        do
        {
            id = m_nextId++;
            if (id == proto::INVALID_SESSION_ID)
            {
                id = m_nextId++;
            }
        }
        while (m_routes.find(id) != m_routes.end());
        m_routes.emplace(id, mailbox);
    }

    // AddSession answers the client itself, with either AUTH_OK or a queue
    // position. The cipher was armed before we were called, so that reply goes
    // out encrypted -- which is the one ordering constraint across this seam.
    // AddSession also answers the addon block, via SendAddonsInfo() over the
    // list ReadAddonsInfo() just parsed -- so nothing more is owed here.
    WorldSession* published = session.get();
    try
    {
        sWorld.AddSession(published);
    }
    catch (...)
    {
        Detach(id);
        throw;
    }
    (void)session.release();

    return id;
}

void WorldGateway::Deliver(proto::SessionId session, WorldPacket&& packet)
{
    std::shared_ptr<SessionMailbox> mailbox;
    {
        std::lock_guard<std::mutex> lock(m_lock);
        auto route = m_routes.find(session);
        if (route == m_routes.end())
        {
            return;
        }
        mailbox = route->second;
    }

    mailbox->Enqueue(std::make_unique<WorldPacket>(std::move(packet)));
}

void WorldGateway::Detach(proto::SessionId session)
{
    std::shared_ptr<SessionMailbox> mailbox;
    {
        std::lock_guard<std::mutex> lock(m_lock);

        auto route = m_routes.find(session);
        if (route == m_routes.end())
        {
            return;
        }
        mailbox = route->second;
        m_routes.erase(route);
    }

    mailbox->Close();
}
