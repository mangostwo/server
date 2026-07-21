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

#ifndef MANGOS_H_GUILDMGR
#define MANGOS_H_GUILDMGR

#include "Utilities/UnorderedMapSet.h"
#include "Platform/Define.h"
#include <string>
#include "Policies/Singleton.h"

#include <mutex>

class Guild;
class ObjectGuid;

class GuildMgr
{
        typedef UNORDERED_MAP<uint32, Guild*> GuildMap;

        GuildMap m_GuildMap;
        // Serializes structural access to m_GuildMap. Required because
        // Map workers (e.g. AchievementMgr::SendAchievementEarned during
        // Player::Update -> Unit aura tick) read this singleton while
        // WorldThread can erase entries via Guild::Disband ->
        // sGuildMgr.RemoveGuild. Mutable so const Get* methods can lock.
        // Note: protects only the map itself; Guild* lifetime across
        // concurrent Get / RemoveGuild callers is a separate concern.
        mutable std::mutex m_GuildMapLock;
    public:
        GuildMgr();
        ~GuildMgr();

        void AddGuild(Guild* guild);
        void RemoveGuild(uint32 guildId);

        Guild* GetGuildById(uint32 guildId) const;
        Guild* GetGuildByName(std::string const& name) const;
        Guild* GetGuildByLeader(ObjectGuid const& guid) const;
        std::string GetGuildNameById(uint32 guildId) const;

        void LoadGuilds();
};

#define sGuildMgr MaNGOS::Singleton<GuildMgr>::Instance()

#endif // _GUILDMGR_H
