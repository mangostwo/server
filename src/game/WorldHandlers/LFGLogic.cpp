/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 */

#include "LFGLogic.h"

#include <algorithm>
#include <iterator>

namespace
{
    std::uint8_t const CombatRoles = LFGLogic::RoleTank |
        LFGLogic::RoleHealer | LFGLogic::RoleDamage;

    std::uint8_t RoleCount(std::uint8_t roles)
    {
        std::uint8_t count = 0;
        for (std::uint8_t role : {LFGLogic::RoleTank, LFGLogic::RoleHealer,
            LFGLogic::RoleDamage})
        {
            if ((roles & role) != 0)
            {
                ++count;
            }
        }
        return count;
    }

    bool HasCapacity(std::uint8_t role, LFGLogic::RoleNeeds const& capacity)
    {
        if (role == LFGLogic::RoleTank)
        {
            return capacity.tanks != 0;
        }
        if (role == LFGLogic::RoleHealer)
        {
            return capacity.healers != 0;
        }
        return role == LFGLogic::RoleDamage && capacity.damage != 0;
    }

    void Consume(std::uint8_t role, LFGLogic::RoleNeeds& capacity)
    {
        if (role == LFGLogic::RoleTank)
        {
            --capacity.tanks;
        }
        else if (role == LFGLogic::RoleHealer)
        {
            --capacity.healers;
        }
        else
        {
            --capacity.damage;
        }
    }

    void Restore(std::uint8_t role, LFGLogic::RoleNeeds& capacity)
    {
        if (role == LFGLogic::RoleTank)
        {
            ++capacity.tanks;
        }
        else if (role == LFGLogic::RoleHealer)
        {
            ++capacity.healers;
        }
        else
        {
            ++capacity.damage;
        }
    }

    bool Assign(std::vector<LFGLogic::RoleRequest> const& requests,
        std::size_t index, LFGLogic::RoleNeeds& capacity,
        std::vector<LFGLogic::RoleAssignment>& assignments)
    {
        if (index == requests.size())
        {
            return true;
        }

        LFGLogic::RoleRequest const& request = requests[index];
        for (std::uint8_t role : {LFGLogic::RoleTank, LFGLogic::RoleHealer,
            LFGLogic::RoleDamage})
        {
            if ((request.selectedRoles & role) == 0 ||
                !HasCapacity(role, capacity))
            {
                continue;
            }

            Consume(role, capacity);
            assignments.push_back({request.playerGuid, role});
            if (Assign(requests, index + 1, capacity, assignments))
            {
                return true;
            }
            assignments.pop_back();
            Restore(role, capacity);
        }
        return false;
    }
}

std::set<std::uint32_t> LFGLogic::FilterRandomCandidates(
    std::uint32_t groupId, std::vector<DungeonCandidate> const& dungeons)
{
    std::set<std::uint32_t> result;
    for (DungeonCandidate const& dungeon : dungeons)
    {
        if (dungeon.groupId == groupId && !dungeon.category &&
            dungeon.mapId != 0 && dungeon.fivePlayerDungeon)
        {
            result.insert(dungeon.id);
        }
    }
    return result;
}

bool LFGLogic::SelectCandidate(std::set<std::uint32_t> const& candidates,
    std::size_t index, std::uint32_t& selectedDungeonId)
{
    if (index >= candidates.size())
    {
        return false;
    }

    std::set<std::uint32_t>::const_iterator itr = candidates.begin();
    std::advance(itr, index);
    selectedDungeonId = *itr;
    return true;
}

std::uint32_t LFGLogic::FirstFailure(
    std::vector<std::uint32_t> const& results, std::uint32_t okResult)
{
    for (std::uint32_t result : results)
    {
        if (result != okResult)
        {
            return result;
        }
    }
    return okResult;
}

bool LFGLogic::AllRolesAnswered(std::vector<RoleRequest> const& requests)
{
    if (requests.empty())
    {
        return false;
    }

    for (RoleRequest const& request : requests)
    {
        if ((request.selectedRoles & CombatRoles) == 0)
        {
            return false;
        }
    }
    return true;
}

std::vector<std::uint8_t> LFGLogic::SelectedCombatRoles(
    std::uint8_t selectedRoles)
{
    std::vector<std::uint8_t> result;
    for (std::uint8_t role : {RoleTank, RoleHealer, RoleDamage})
    {
        if ((selectedRoles & role) != 0)
        {
            result.push_back(role);
        }
    }
    return result;
}

bool LFGLogic::ResolveRoles(std::vector<RoleRequest> const& requests,
    std::vector<RoleAssignment>& assignments, RoleNeeds& needs)
{
    assignments.clear();
    needs = {1, 1, 3};
    if (requests.empty() || requests.size() > 5 ||
        !AllRolesAnswered(requests))
    {
        return false;
    }

    std::vector<RoleRequest> ordered = requests;
    for (RoleRequest& request : ordered)
    {
        request.selectedRoles &= CombatRoles;
    }
    std::sort(ordered.begin(), ordered.end(),
        [](RoleRequest const& left, RoleRequest const& right)
        {
            std::uint8_t const leftCount = RoleCount(left.selectedRoles);
            std::uint8_t const rightCount = RoleCount(right.selectedRoles);
            return leftCount != rightCount ? leftCount < rightCount :
                left.playerGuid < right.playerGuid;
        });

    if (!Assign(ordered, 0, needs, assignments))
    {
        assignments.clear();
        needs = {1, 1, 3};
        return false;
    }
    return true;
}

bool LFGLogic::IsProposalReady(std::size_t playerCount, RoleNeeds const& needs)
{
    return playerCount == 5 && needs.tanks == 0 && needs.healers == 0 &&
        needs.damage == 0;
}

std::set<std::uint32_t> LFGLogic::RequeueDungeons(
    std::set<std::uint32_t> const& requestedDungeons,
    std::uint32_t activeDungeonId)
{
    return activeDungeonId == 0 ? requestedDungeons :
        std::set<std::uint32_t>{activeDungeonId};
}

std::uint32_t LFGLogic::RequeueRandomDungeon(bool hasActiveProvenance,
    std::uint32_t activeRandomDungeonId,
    std::uint32_t requestedRandomDungeonId)
{
    return hasActiveProvenance ? activeRandomDungeonId :
        requestedRandomDungeonId;
}

bool LFGLogic::ShouldReturnFromCompletedDungeon(bool finished,
    std::int32_t currentMapId, std::int32_t previousDungeonMapId)
{
    return finished && previousDungeonMapId > 0 &&
        currentMapId == previousDungeonMapId;
}

bool LFGLogic::ShouldRestoreActiveGroupStatus(bool terminal, bool queued,
    bool hasActiveGroupStatus)
{
    return terminal && !queued && hasActiveGroupStatus;
}

bool LFGLogic::IsQueueOwnerPublished(bool queued, bool proposing)
{
    return queued || proposing;
}

bool LFGLogic::ShouldReplacePendingQueueSource(bool requestValid,
    bool hasPendingSource)
{
    return requestValid && hasPendingSource;
}

LFGLogic::GroupPacketValues LFGLogic::MakeGroupPacketValues(std::uint8_t role,
    bool finished, std::uint32_t dungeonEntry)
{
    return GroupPacketValues{role, std::uint8_t(finished ? 2 : 0),
        dungeonEntry};
}

std::uint8_t LFGLogic::GroupHeaderRole(bool lfd, bool battleground,
    std::uint8_t assignedRole)
{
    return lfd ? assignedRole : std::uint8_t(battleground ? 1 : 0);
}

bool LFGLogic::IsTeleportTarget(DungeonCandidate const& dungeon,
    std::uint32_t groupDifficulty)
{
    return !dungeon.category && dungeon.mapId != 0 &&
        dungeon.fivePlayerDungeon && dungeon.difficulty == groupDifficulty;
}

bool LFGLogic::ShouldVoteKick(std::uint64_t targetGuid,
    std::uint64_t kickerGuid)
{
    return targetGuid != kickerGuid;
}

std::int64_t LFGLogic::ElapsedSeconds(std::int64_t start, std::int64_t now)
{
    return now > start ? now - start : 0;
}

std::int64_t LFGLogic::RemainingSeconds(std::int64_t start,
    std::int64_t duration, std::int64_t now)
{
    if (duration <= 0)
    {
        return 0;
    }

    std::int64_t const elapsed = ElapsedSeconds(start, now);
    return elapsed >= duration ? 0 : duration - elapsed;
}

std::vector<std::uint64_t> LFGLogic::CollectExpiredOwners(
    std::vector<TimedOwner> const& owners, std::int64_t now)
{
    std::vector<std::uint64_t> result;
    for (TimedOwner const& owner : owners)
    {
        if (owner.deadline <= now)
        {
            result.push_back(owner.ownerGuid);
        }
    }
    return result;
}
