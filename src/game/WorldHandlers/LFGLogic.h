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

#ifndef MANGOS_LFGLOGIC_H
#define MANGOS_LFGLOGIC_H

#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

namespace LFGLogic
{
    static std::uint8_t const RoleLeader = 0x01;
    static std::uint8_t const RoleTank = 0x02;
    static std::uint8_t const RoleHealer = 0x04;
    static std::uint8_t const RoleDamage = 0x08;

    struct DungeonCandidate
    {
        std::uint32_t id;
        std::uint32_t groupId;
        std::int32_t mapId;
        std::uint32_t difficulty;
        bool category;
        bool fivePlayerDungeon;
        bool instanceable;
    };

    struct RoleRequest
    {
        std::uint64_t playerGuid;
        std::uint8_t selectedRoles;
    };

    struct RoleAssignment
    {
        std::uint64_t playerGuid;
        std::uint8_t assignedRole;
    };

    struct RoleNeeds
    {
        std::uint8_t tanks;
        std::uint8_t healers;
        std::uint8_t damage;
    };

    struct GroupPacketValues
    {
        std::uint8_t role;
        std::uint8_t state;
        std::uint32_t dungeonEntry;
    };

    struct TimedOwner
    {
        std::uint64_t ownerGuid;
        std::int64_t deadline;
    };

    std::set<std::uint32_t> FilterRandomCandidates(
        std::uint32_t groupId,
        std::vector<DungeonCandidate> const& dungeons);
    bool SelectCandidate(std::set<std::uint32_t> const& candidates,
        std::size_t index, std::uint32_t& selectedDungeonId);
    std::uint32_t FirstFailure(std::vector<std::uint32_t> const& results,
        std::uint32_t okResult);
    bool AllRolesAnswered(std::vector<RoleRequest> const& requests);
    std::vector<std::uint8_t> SelectedCombatRoles(
        std::uint8_t selectedRoles);
    bool ResolveRoles(std::vector<RoleRequest> const& requests,
        std::vector<RoleAssignment>& assignments, RoleNeeds& needs);
    bool IsProposalReady(std::size_t playerCount, RoleNeeds const& needs);
    std::set<std::uint32_t> RequeueDungeons(
        std::set<std::uint32_t> const& requestedDungeons,
        std::uint32_t activeDungeonId);
    std::uint32_t RequeueRandomDungeon(bool hasActiveProvenance,
        std::uint32_t activeRandomDungeonId,
        std::uint32_t requestedRandomDungeonId);
    bool ShouldReturnFromCompletedDungeon(bool finished,
        std::int32_t currentMapId, std::int32_t previousDungeonMapId);
    bool ShouldRestoreActiveGroupStatus(bool terminal, bool queued,
        bool hasActiveGroupStatus);
    bool IsQueueOwnerPublished(bool queued, bool proposing);
    bool ShouldReplacePendingQueueSource(bool requestValid,
        bool hasPendingSource);
    GroupPacketValues MakeGroupPacketValues(std::uint8_t role,
        bool finished, std::uint32_t dungeonEntry);
    std::uint8_t GroupHeaderRole(bool lfd, bool battleground,
        std::uint8_t assignedRole);
    bool IsTeleportTarget(DungeonCandidate const& dungeon,
        std::uint32_t groupDifficulty);
    bool ShouldVoteKick(std::uint64_t targetGuid,
        std::uint64_t kickerGuid);
    std::int64_t ElapsedSeconds(std::int64_t start,
        std::int64_t now);
    std::int64_t RemainingSeconds(std::int64_t start,
        std::int64_t duration, std::int64_t now);
    std::vector<std::uint64_t> CollectExpiredOwners(
        std::vector<TimedOwner> const& owners, std::int64_t now);
}

#endif
