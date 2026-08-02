/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 */

#include "TestHarness.h"
#include "LFGLogic.h"

#include <cstdint>
#include <set>
#include <vector>

namespace
{
    LFGLogic::DungeonCandidate Candidate(std::uint32_t id, std::uint32_t groupId,
        std::int32_t mapId, std::uint32_t difficulty, bool category,
        bool fivePlayerDungeon, bool instanceable = true)
    {
        return LFGLogic::DungeonCandidate{id, groupId, mapId, difficulty,
            category, fivePlayerDungeon, instanceable};
    }

    std::uint8_t AssignedRole(
        std::vector<LFGLogic::RoleAssignment> const& assignments,
        std::uint64_t playerGuid)
    {
        for (LFGLogic::RoleAssignment const& assignment : assignments)
        {
            if (assignment.playerGuid == playerGuid)
            {
                return assignment.assignedRole;
            }
        }
        return 0;
    }
}

TEST(LFG_FilterRandomCandidatesRejectsCategoriesAndNonDungeons)
{
    std::vector<LFGLogic::DungeonCandidate> const rows = {
        Candidate(10, 1, 34, 0, false, true),
        Candidate(11, 1, 36, 1, false, true),
        Candidate(258, 1, 0, 0, true, false, false),
        Candidate(12, 2, 33, 0, false, true),
        Candidate(13, 1, 533, 0, false, false),
        Candidate(14, 1, 30, 0, false, false),
        Candidate(15, 1, 0, 0, false, true)
    };

    std::set<std::uint32_t> const filtered =
        LFGLogic::FilterRandomCandidates(1, rows);

    CHECK_EQ(filtered.size(), std::size_t(2));
    CHECK(filtered.count(10) == 1);
    CHECK(filtered.count(11) == 1);
    CHECK(filtered.count(258) == 0);
}

TEST(LFG_SelectCandidateUsesStableIndexBounds)
{
    std::set<std::uint32_t> const candidates = {12, 44};
    std::uint32_t selected = 99;

    CHECK(LFGLogic::SelectCandidate(candidates, 0, selected));
    CHECK_EQ(selected, std::uint32_t(12));
    CHECK(LFGLogic::SelectCandidate(candidates, 1, selected));
    CHECK_EQ(selected, std::uint32_t(44));
    CHECK(!LFGLogic::SelectCandidate(candidates, 2, selected));

    std::set<std::uint32_t> const empty;
    CHECK(!LFGLogic::SelectCandidate(empty, 0, selected));
}

TEST(LFG_FirstFailureCannotBeOverwrittenByLaterSuccess)
{
    std::vector<std::uint32_t> const results = {0, 12, 0, 5};
    CHECK_EQ(LFGLogic::FirstFailure(results, 0), std::uint32_t(12));
    CHECK_EQ(LFGLogic::FirstFailure({}, 0), std::uint32_t(0));
}

TEST(LFG_AllRolesAnsweredRequiresACombatRole)
{
    CHECK(!LFGLogic::AllRolesAnswered({}));
    CHECK(!LFGLogic::AllRolesAnswered({{1, LFGLogic::RoleLeader}}));
    CHECK(!LFGLogic::AllRolesAnswered({{1, LFGLogic::RoleTank}, {2, 0}}));
    CHECK(LFGLogic::AllRolesAnswered({
        {1, std::uint8_t(LFGLogic::RoleLeader | LFGLogic::RoleTank)},
        {2, std::uint8_t(LFGLogic::RoleHealer | LFGLogic::RoleDamage)}}));
}

TEST(LFG_SelectedCombatRolesExpandsMultiRoleMasks)
{
    std::vector<std::uint8_t> const roles = LFGLogic::SelectedCombatRoles(
        std::uint8_t(LFGLogic::RoleLeader | LFGLogic::RoleTank |
            LFGLogic::RoleHealer | LFGLogic::RoleDamage));

    REQUIRE(roles.size() == 3);
    CHECK_EQ(roles[0], LFGLogic::RoleTank);
    CHECK_EQ(roles[1], LFGLogic::RoleHealer);
    CHECK_EQ(roles[2], LFGLogic::RoleDamage);
}

TEST(LFG_ResolveRolesBuildsExactFivePlayerComposition)
{
    std::vector<LFGLogic::RoleRequest> const requests = {
        {5, LFGLogic::RoleDamage},
        {2, LFGLogic::RoleHealer},
        {1, LFGLogic::RoleTank},
        {4, LFGLogic::RoleDamage},
        {3, LFGLogic::RoleDamage}
    };
    std::vector<LFGLogic::RoleAssignment> assignments;
    LFGLogic::RoleNeeds needs = {9, 9, 9};

    CHECK(LFGLogic::ResolveRoles(requests, assignments, needs));
    CHECK_EQ(assignments.size(), std::size_t(5));
    CHECK_EQ(AssignedRole(assignments, 1), LFGLogic::RoleTank);
    CHECK_EQ(AssignedRole(assignments, 2), LFGLogic::RoleHealer);
    CHECK_EQ(AssignedRole(assignments, 3), LFGLogic::RoleDamage);
    CHECK_EQ(AssignedRole(assignments, 4), LFGLogic::RoleDamage);
    CHECK_EQ(AssignedRole(assignments, 5), LFGLogic::RoleDamage);
    CHECK_EQ(needs.tanks, std::uint8_t(0));
    CHECK_EQ(needs.healers, std::uint8_t(0));
    CHECK_EQ(needs.damage, std::uint8_t(0));
}

TEST(LFG_ResolveRolesBacktracksAcrossMultiRoleSelections)
{
    std::vector<LFGLogic::RoleRequest> const requests = {
        {1, std::uint8_t(LFGLogic::RoleTank | LFGLogic::RoleHealer)},
        {2, LFGLogic::RoleTank},
        {3, std::uint8_t(LFGLogic::RoleHealer | LFGLogic::RoleDamage)},
        {4, LFGLogic::RoleDamage},
        {5, LFGLogic::RoleDamage}
    };
    std::vector<LFGLogic::RoleAssignment> assignments;
    LFGLogic::RoleNeeds needs = {0, 0, 0};

    CHECK(LFGLogic::ResolveRoles(requests, assignments, needs));
    CHECK_EQ(AssignedRole(assignments, 1), LFGLogic::RoleHealer);
    CHECK_EQ(AssignedRole(assignments, 2), LFGLogic::RoleTank);
    CHECK_EQ(AssignedRole(assignments, 3), LFGLogic::RoleDamage);
}

TEST(LFG_ResolveRolesReportsPartialNeedsAndRejectsImpossibleGroups)
{
    std::vector<LFGLogic::RoleAssignment> assignments;
    LFGLogic::RoleNeeds needs = {0, 0, 0};

    CHECK(LFGLogic::ResolveRoles({{1, LFGLogic::RoleTank},
        {2, LFGLogic::RoleDamage}}, assignments, needs));
    CHECK_EQ(needs.tanks, std::uint8_t(0));
    CHECK_EQ(needs.healers, std::uint8_t(1));
    CHECK_EQ(needs.damage, std::uint8_t(2));

    CHECK(!LFGLogic::ResolveRoles({
        {1, LFGLogic::RoleDamage}, {2, LFGLogic::RoleDamage},
        {3, LFGLogic::RoleDamage}, {4, LFGLogic::RoleDamage},
        {5, LFGLogic::RoleDamage}}, assignments, needs));
    CHECK(!LFGLogic::ResolveRoles({
        {1, LFGLogic::RoleTank}, {2, LFGLogic::RoleHealer},
        {3, LFGLogic::RoleDamage}, {4, LFGLogic::RoleDamage},
        {5, LFGLogic::RoleDamage}, {6, LFGLogic::RoleDamage}},
        assignments, needs));
}

TEST(LFG_ProposalReadinessRequiresCompleteFivePlayerComposition)
{
    LFGLogic::RoleNeeds const complete = {0, 0, 0};
    LFGLogic::RoleNeeds const incomplete = {0, 1, 3};

    CHECK(LFGLogic::IsProposalReady(5, complete));
    CHECK(!LFGLogic::IsProposalReady(4, complete));
    CHECK(!LFGLogic::IsProposalReady(5, incomplete));
}

TEST(LFG_RequeuePreservesTheActiveDungeon)
{
    CHECK(LFGLogic::RequeueDungeons({10, 11}, 34) ==
        std::set<std::uint32_t>({34}));
    CHECK(LFGLogic::RequeueDungeons({10, 11}, 0) ==
        std::set<std::uint32_t>({10, 11}));
}

TEST(LFG_ReplacementQueuePreservesRandomDungeonProvenance)
{
    CHECK_EQ(LFGLogic::RequeueRandomDungeon(true, 258, 0),
        std::uint32_t(258));
    CHECK_EQ(LFGLogic::RequeueRandomDungeon(true, 0, 258),
        std::uint32_t(0));
    CHECK_EQ(LFGLogic::RequeueRandomDungeon(false, 0, 258),
        std::uint32_t(258));
}

TEST(LFG_CompletedDungeonTransitionReturnsMembersFromThePreviousMap)
{
    CHECK(LFGLogic::ShouldReturnFromCompletedDungeon(true, 34, 34));
    CHECK(!LFGLogic::ShouldReturnFromCompletedDungeon(false, 34, 34));
    CHECK(!LFGLogic::ShouldReturnFromCompletedDungeon(true, 0, 34));
    CHECK(!LFGLogic::ShouldReturnFromCompletedDungeon(true, 36, 34));
}

TEST(LFG_QueueOwnerCanBePublishedByAnImmediateProposal)
{
    CHECK(LFGLogic::IsQueueOwnerPublished(true, false));
    CHECK(LFGLogic::IsQueueOwnerPublished(false, true));
    CHECK(!LFGLogic::IsQueueOwnerPublished(false, false));
}

TEST(LFG_GroupPacketValuesPreserveRoleStateAndPackedEntry)
{
    LFGLogic::GroupPacketValues const active =
        LFGLogic::MakeGroupPacketValues(LFGLogic::RoleHealer, false, 0x0500000C);
    CHECK_EQ(active.role, LFGLogic::RoleHealer);
    CHECK_EQ(active.state, std::uint8_t(0));
    CHECK_EQ(active.dungeonEntry, std::uint32_t(0x0500000C));

    LFGLogic::GroupPacketValues const finished =
        LFGLogic::MakeGroupPacketValues(LFGLogic::RoleDamage, true, 0x01000005);
    CHECK_EQ(finished.state, std::uint8_t(2));
    CHECK_EQ(finished.dungeonEntry, std::uint32_t(0x01000005));
}

TEST(LFG_GroupHeaderRolePreservesNonLfdBattlegroundValue)
{
    CHECK_EQ(LFGLogic::GroupHeaderRole(true, false, LFGLogic::RoleTank),
        LFGLogic::RoleTank);
    CHECK_EQ(LFGLogic::GroupHeaderRole(false, true, LFGLogic::RoleDamage),
        std::uint8_t(1));
    CHECK_EQ(LFGLogic::GroupHeaderRole(false, false, LFGLogic::RoleDamage),
        std::uint8_t(0));
}

TEST(LFG_TeleportTargetRequiresAnActualDungeonAndMatchingDifficulty)
{
    CHECK(LFGLogic::IsTeleportTarget(
        Candidate(12, 1, 34, 0, false, true), 0));
    CHECK(!LFGLogic::IsTeleportTarget(
        Candidate(258, 1, 0, 0, true, false, false), 0));
    CHECK(!LFGLogic::IsTeleportTarget(
        Candidate(13, 1, 533, 0, false, false), 0));
    CHECK(!LFGLogic::IsTeleportTarget(
        Candidate(14, 1, 34, 1, false, true), 0));
}

TEST(LFG_SelfLeaveDoesNotBecomeAVoteKick)
{
    CHECK(!LFGLogic::ShouldVoteKick(123, 123));
    CHECK(LFGLogic::ShouldVoteKick(123, 456));
}

TEST(LFG_TimeHelpersUseClampedSeconds)
{
    CHECK_EQ(LFGLogic::ElapsedSeconds(100, 145), std::int64_t(45));
    CHECK_EQ(LFGLogic::ElapsedSeconds(145, 100), std::int64_t(0));
    CHECK_EQ(LFGLogic::RemainingSeconds(100, 120, 100), std::int64_t(120));
    CHECK_EQ(LFGLogic::RemainingSeconds(100, 120, 160), std::int64_t(60));
    CHECK_EQ(LFGLogic::RemainingSeconds(100, 120, 221), std::int64_t(0));
}

TEST(LFG_ExpiredOwnersAreCollectedWithoutMutatingTheInput)
{
    std::vector<LFGLogic::TimedOwner> const owners = {
        {1, 100}, {2, 101}, {3, 99}
    };
    std::vector<std::uint64_t> const expired =
        LFGLogic::CollectExpiredOwners(owners, 100);

    REQUIRE(expired.size() == 2);
    CHECK_EQ(expired[0], std::uint64_t(1));
    CHECK_EQ(expired[1], std::uint64_t(3));
    CHECK_EQ(owners.size(), std::size_t(3));
}
