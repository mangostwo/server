# LFD Correctness and Solo Smoke Mode Design

## Context

Mango Two currently treats a random-dungeon category as both the player's
request and the dungeon assigned to the group. For example, DBC entry 258
(`Random Classic Dungeon`) is a category with map 0, while entry 12
(`Stormwind Stockade`) is an actual dungeon on map 34. The queue initially
expands the category to compatible dungeons, then replaces that set with entry
258 before matching. Proposal creation selects the first set element, so the
category reaches teleport code and a generic map-0 area trigger can place the
party in the outdoor Stockades tower.

The stock 3.3.5a client also expects LFD group state that Mango Two does not
currently supply. Its minimap sends `LFGTeleport(false)` to enter and
`LFGTeleport(true)` to leave. `IsInLFGDungeon()` relies on the LFD dungeon entry
from `SMSG_GROUP_LIST` matching the current map and difficulty. Mango Two sends
zero for the LFD role, state, and dungeon fields, implements only teleport-out,
and deletes the group dungeon status when completion rewards are issued.

This design repairs the existing Mango Two LFD manager. It does not port the
CMaNGOS LFG subsystem or add a world-database teleport table.

## Goals

- A random queue request must resolve to an eligible actual dungeon before a
  proposal is sent; a category row must never be used as a teleport target.
- Preserve the player's requested random category independently from the
  selected actual dungeon so client display and rewards remain correct.
- Make solo and premade queue, role-check, proposal, dungeon, completion,
  teleport, leave, kick, and cleanup transitions internally consistent.
- Send the stock 3.3.5a group-list LFD fields the client uses for roles,
  dungeon detection, and completion state.
- Implement both directions of `CMSG_LFG_TELEPORT` without losing the saved
  return location at dungeon completion.
- Reject malformed selections and aggregate party eligibility failures without
  allowing a later valid member to overwrite an earlier failure.
- Provide a temporary administrator-only `.debug lfd` command that allows a
  valid single player to exercise the complete LFD proposal and dungeon flow.
- Add regression tests for pure selection, validation, state, packet-value,
  and testing-mode decisions before changing production behavior.

## Non-goals

- No new `lfg_dungeon_template` table or other database migration.
- No client patch or addon change.
- No rewrite or wholesale import of the CMaNGOS WotLK LFG subsystem.
- No changes to dungeon eligibility, deserter, cooldown, lockout, expansion,
  level, or faction rules except correcting their aggregation and validation.
- No permanent reduction of the normal five-player role requirements.
- No deployment, data installation, or server startup as part of this change.
- The `.debug lfd` command is temporary and will be removed after the user's
  live smoke test in a follow-up change.

## Core invariants

1. `requestedRandomDungeonId` is either zero or a valid random/seasonal
   category selected by the player.
2. A queue candidate set contains only validated actual dungeon IDs. Random
   category rows are never candidates.
3. `selectedDungeonId` is an actual member of the compatible candidate set.
4. A proposal and an in-dungeon group carry exactly one actual selected
   dungeon ID.
5. Random-category identity is retained per player because a random-queue
   player can match a player who selected the resulting dungeon specifically.
6. Teleport code accepts only an actual dungeon whose map is instanceable and
   whose difficulty matches the group's configured difficulty.
7. Normal matching requires the existing five-player role composition. LFD
   testing mode may bypass only the match-size and role-composition readiness
   check for a queue unit containing one player.
8. Completion changes state and rewards players but does not destroy the
   selected dungeon or saved return-location identity needed to leave.
9. Each LFD state transition updates manager state and client-visible state as
   one operation; cleanup is idempotent.
10. Queue matching never dereferences a live `Player*`. Queue ownership records
    the team and reverse player-to-owner mapping needed to match or cancel a
    disconnected player safely.
11. A selected-role mask may contain multiple roles, but every proposal and
    in-dungeon group assigns each player exactly one role while satisfying the
    one-tank, one-healer, three-damage composition for both normal and heroic
    five-player dungeons.

## Data model

### Queue data

`LFGPlayers::dungeonList` becomes the compatible actual-dungeon candidate set.
It must not be repurposed as a client display list. Add
`playerDungeonMap randomDungeonByPlayer`, mapping each player GUID to a
requested random category ID, with zero representing a specific request. Add a
captured `TeamId team` and set `isGroup` correctly in both constructors: false
for a solo owner and true for a premade owner. `currentRoles` holds each
player's selected role mask while queued. When queue units merge, their
candidate sets are intersected and their per-player random maps and role masks
are merged.

Add `playerGroupMap m_playerQueueOwners`, mapping every queued, role-check, or
proposal player to the solo or group queue owner. It is published and removed
with the queue transition, supplies deterministic logout cleanup, and avoids
searching live `Player` objects merely to discover ownership.

For a premade role check, `LFGRoleCheck::dungeonList` holds the same actual
candidate set. Its `currentRoles` map must be completely initialized before the
role check is inserted into `m_roleCheckMap`. `PerformRoleCheck` must mutate the
stored object, not a detached copy. Successful, aborted, failed, and timed-out
checks all have explicit removal paths.

`LFGPlayerStatus::dungeonList` remains the client-facing request list. Random
players see their category entry; specific players see their selected dungeon
entries. This keeps queue UI semantics separate from match candidates.

### Proposal data

`LFGProposal::dungeonID` is always the selected actual dungeon ID. Add
`playerDungeonMap randomDungeonByPlayer`. `currentRoles` contains the resolved
single role assigned to every player, not the original multi-role mask.
Proposal creation selects uniformly from the compatible actual candidate set
using the core random-number helper. Empty candidate sets cannot create
proposals.

The proposal carries the per-player requested-random map. Decline, timeout, or
group-creation failure restores or clears each source queue consistently and
does not lose the request identity required by later rewards.

### In-dungeon group data

`LFGGroupStatus::dungeonID` is the actual selected dungeon ID. Add
`playerDungeonMap randomDungeonByPlayer`; its existing `playerRoles` contains
the assigned single roles. The record survives
the transition from `LFG_STATE_IN_DUNGEON` to
`LFG_STATE_FINISHED_DUNGEON` and is removed only when the LFD group is fully
cleaned up after member leave or disband. Teleporting out alone does not remove
the record because the stock client permits teleporting back into an active LFD
group.

Read-only manager queries expose the actual dungeon entry, per-player role, and
finished state needed by `Group::SendUpdate()` without exposing mutable map
storage to `Group`.

The concrete packet seam is:

```cpp
struct LFGGroupUpdateData
{
    uint8 role;
    uint8 state;
    uint32 dungeonEntry;
};

bool LFGMgr::GetGroupUpdateData(ObjectGuid groupGuid,
    ObjectGuid playerGuid, LFGGroupUpdateData& data) const;
```

It returns false when the group status or player role is absent. `Group` owns a
separate `uint32 m_updateCounter`, initialized to zero, because the packet
sequence belongs to the group rather than LFD manager state.

## Queue and role-check flow

1. Reject an empty request before reading its first element.
2. Resolve every requested entry before dereferencing it. Reject unknown IDs,
   incompatible mixed types, and multiple selections containing a random
   category.
3. For a random request, record the category separately, expand by its DBC
   group, and keep only non-category dungeon/heroic rows.
4. Apply every party member's lock and eligibility filters to the actual
   candidates. Failure leaves the category intact for the join-result UI but
   does not enqueue it.
5. For premades, initialize all member role slots, candidate dungeons, and
   random-request identities before publishing the role check.
6. `PerformRoleCheck` takes a reference to the object stored in
   `m_roleCheckMap`. A single bounded scan checks every member exactly once and
   returns pending as soon as it sees `PLAYER_ROLE_NONE`; it never uses the
   current unbounded `do-while` loop. After successful role confirmation,
   enqueue the actual candidates and persist the confirmed roles. Successful,
   failed, aborted, and timed-out checks erase the stored role-check record.
   `RemoveOldRoleChecks` first collects expired group GUIDs, then processes and
   erases each record by key outside iteration. It never erases the active
   unordered-map iterator.
7. Matching intersects actual candidate sets and combines per-player selected
   role masks and request identities. A bounded backtracking resolver assigns
   each player one selected role without exceeding one tank, one healer, and
   three damage. It handles multi-role masks rather than switching on the whole
   mask. A partial unit is compatible when every current player can be assigned
   within those capacities; a five-player unit is ready only when the exact
   1/1/3 composition is resolved. The resolved single roles are copied into the
   proposal and group status.
8. Proposal creation randomly selects one actual compatible dungeon and
   carries request identity forward unchanged.

`UpdateNeededRoles` derives missing roles from the resolver for every supported
five-player candidate, including normal and heroic difficulties. It does not
read `*dungeonList.begin()` or special-case normal difficulty. An empty
candidate set fails before this function. Wait-time accounting treats a player
as eligible for every role bit selected until the proposal fixes one assignment.

Queue publication, merge, and proposal transitions use one manager helper to
update the internal queue record and every affected `LFGPlayerStatus`. The
status retains that player's original client-facing selection, while its state,
update type, comment, and queue membership change atomically with the internal
candidate intersection. A merge must not leave `CMSG_LFG_GET_STATUS` describing
the pre-merge state.

## Join validation

Join validation starts from `ERR_LFG_OK` and returns immediately on a decisive
failure. For a solo player, deserter, cooldown, battleground, arena, and level
checks cannot be overwritten by a final unconditional success. For a party,
each member is checked in a stable order; the first failure is retained, and a
later valid member cannot reset it. Offline-member and group-size failures are
reported after the checks needed to establish those conditions.

Selection validation is defense in depth: the join handler validates input,
proposal creation validates the chosen row, and teleport validates the stored
actual dungeon and instanceable map. Invalid internal state produces an LFG
error and cleanup rather than a crash or map-0 teleport.

Every function that currently reads `*dungeons.begin()` or
`*dungeonList.begin()` (`JoinLFG`, `UpdateNeededRoles`, queue-status generation,
and `SendDungeonProposal`) receives an explicit empty-set guard immediately
before the read. Unknown DBC rows and empty role maps likewise return a join or
internal-state error rather than being dereferenced.

## Client packet contract

For `SMSG_GROUP_LIST`, an LFD group sends:

- The player's confirmed LFG role in the existing fourth header byte.
- LFD state byte `0` while active and `2` after dungeon completion.
- The packed actual LFG dungeon entry for the selected dungeon.
- A monotonically increasing group-update counter rather than a constant zero.

For an LFD group the player's role replaces the existing fourth header byte,
followed immediately by the LFD state byte and packed dungeon entry. For a
non-LFD group the fourth byte retains the existing battleground indicator and
no LFD state/dungeon fields are appended. Each packet emitted by
`Group::SendUpdate()` writes `m_updateCounter++`. If an LFD group status record
is unexpectedly absent, the packet uses safe zero values and logs the invariant
violation; it must not invent a category or map.

The actual dungeon entry enables the stock client's `IsInLFGDungeon()` to
compare its map and difficulty with the player's current instance. Client-facing
queue update packets continue to use the player's original request entry.

Each other-member tuple in `SMSG_GROUP_LIST` also ends with that member's
assigned LFG role byte. `Group::SendUpdate()` calls `GetGroupUpdateData` for the
recipient header and for every listed member; offline members use the role
retained in `LFGGroupStatus::playerRoles`, not a hard-coded zero.

## Teleport behavior

`TeleportPlayer(player, false)` enters the selected actual dungeon:

- Require a current LFD group and valid group status.
- Reject dead, falling, fatigued, vehicle, charming, and combat-invalid states
  using the existing LFG teleport errors where available.
- Resolve the actual dungeon row and reject category rows, map 0, or a
  non-instanceable map.
- For automatic teleport immediately after proposal acceptance, use an
  existing group member already on the selected map as the destination;
  otherwise use `GetMapEntranceTrigger(actualMapId)`.
- For opcode-driven `CMSG_LFG_TELEPORT(false)`, always use the selected
  dungeon's entrance trigger rather than another member's live position.
- Save the player's battleground-style entry point only when entering from a
  non-instance world location.

`TeleportPlayer(player, true)` leaves the dungeon:

- Require the player to be on the selected actual dungeon map.
- Restore the saved entry point.
- Keep LFD group identity intact so the player can teleport back in while the
  LFD group remains active, including after completion until group cleanup.

Automatic teleport after proposal acceptance uses the same actual-dungeon
validation as opcode-driven teleport-in, with the destination-policy difference
passed explicitly (automatic group formation versus player-requested re-entry)
instead of duplicating the safety checks.

`GetMapEntranceTrigger` remains the coordinate source in this server-only
repair. If no trigger exists, teleport fails safely. Maps with multiple valid
entrances cannot be disambiguated from `LFGDungeons.dbc` alone; exact
per-dungeon coordinates require the deliberately deferred database enhancement
and are not claimed as solved here.

## Completion, leave, kick, and cleanup

Boss completion is idempotent. If the group status is already
`LFG_STATE_FINISHED_DUNGEON`, `HandleBossKilled` returns before calculating or
sending any reward. Otherwise it changes the group and players to finished,
sends a refreshed group list, and rewards each player using that player's
requested random category plus the group's actual dungeon. After issuing the
first applicable reward it calls `RegisterPlayerDaily` for that player and
dungeon type, so `HasPlayerDoneDaily` changes for subsequent runs. It does not
erase the group status immediately.

Queue cancellation remains `CMSG_LFG_LEAVE` behavior for role check, queue, and
proposal states. In-dungeon exit continues to use `CMSG_LFG_TELEPORT(true)`.

An LFD member selecting ordinary Leave Party removes that member rather than
starting a vote against themselves. In `Player::RemoveFromGroup`, an LFD call
where `guid == kicker` calls `Group::RemoveMember(guid, 0)` and performs the
same object-manager deletion used by an ordinary group when the remaining count
requires it. Only `guid != kicker` routes to `AttemptToKickPlayer`. Kicking a
different member therefore retains the vote-kick flow.

The lifecycle hooks are explicit:

```cpp
void LFGMgr::OnGroupMemberRemoved(ObjectGuid groupGuid,
    ObjectGuid playerGuid);
void LFGMgr::OnGroupDisband(ObjectGuid groupGuid);
```

`Group::RemoveMember` calls `OnGroupMemberRemoved` after a successful direct
member removal. `Group::Disband` calls `OnGroupDisband` once before clearing its
member slots. The member hook clears that player's status and reverse queue
ownership and erases the player from both
`LFGGroupStatus::randomDungeonByPlayer` and
`LFGGroupStatus::playerRoles`; when no tracked LFD members remain it performs
group cleanup. The disband hook clears every role check, queue unit, proposal
membership, boot vote, player status, reverse queue owner, group status, and
group-set record owned by the group. The hooks tolerate already-removed records,
so the two-member path that enters `Disband` does not double-clean or
dereference invalid state. Existing teleport or homebind behavior remains in
the group removal code.

Add `void LFGMgr::OnPlayerLogout(ObjectGuid playerGuid)`, called from
`WorldSession::LogoutPlayer` before the player is removed from the registry. It
uses `m_playerQueueOwners`: a solo queue is cancelled; a premade role check or
queue is cancelled for the whole premade because its required member is no
longer available; a pending proposal is declined through the normal proposal
unwind; an already in-dungeon group retains its group record and offline member
role until ordinary leave/disband cleanup. `MatchesAreOfSameTeam` compares the
captured `LFGPlayers::team` values and never dereferences registry players. A
missing owner/state makes units incompatible and schedules their stale queue
records for cleanup.

## Temporary `.debug lfd` smoke mode

Add `.debug lfd` beside `.debug bg`, restricted to `SEC_ADMINISTRATOR`. It
toggles a manager-wide `m_testing` flag initialized to false and prints or
broadcasts whether one-player LFD testing is enabled.

When enabled, `AddToQueue` publishes a valid queue unit normally. At the start
of `FindSpecificQueueMatches`, before comparing it with peer units, an explicit
`currentState == LFG_STATE_QUEUED && m_testing &&
currentRoles.size() == 1 && !isGroup` branch verifies that its actual candidate
set is non-empty and calls one shared `BeginProposal(ownerGuid)` transition.
`BeginProposal` erases the owner from `m_queueSet`, changes the queue record and
every player status to proposal, and only then calls `SendDungeonProposal`.
Normal full-group matching uses the same transition. Repeated update ticks see
neither a queued state nor a queue-set member and cannot create duplicates. The
solo unit still must pass normal join
validation, accept a normal proposal, create an LFD group, and execute the
production packet, teleport, completion, reward, teleport-out, and leave paths.
Premade groups and merged multi-player queue units continue to use normal
readiness rules.

The testing flag never persists to the database or configuration and defaults
off on every server start. The command and flag are deliberately isolated so a
follow-up removal deletes the command registration, handler, flag, and one
readiness branch without changing production matching.

## Test strategy

Introduce `src/game/WorldHandlers/LFGLogic.h` and `LFGLogic.cpp` as a dependency-
free unit containing the primitive/container decisions for category filtering,
indexed selection, first-failure aggregation, role-answer completion, queue
readiness, packet values, teleport-row eligibility, and self-leave versus
vote-kick. The game build already includes this file through its
`WorldHandlers/*.cpp` source glob. `src/tests/CMakeLists.txt` explicitly compiles
the same `LFGLogic.cpp` into `mangos_tests`, and `src/tests/LFGLogicTest.cpp`
tests it through the existing harness without linking `game.lib` or constructing
`Player`, `Group`, `WorldSession`, `ObjectMgr`, or DBC stores. Integration code
adapts live objects into the pure inputs and owns side effects.

`LFGLogic.h` includes only standard headers (`<cstddef>`, `<cstdint>`,
`<vector>`, and `<set>`) and declares this dependency-free interface; it must
not include `LFGMgr.h`, `Player.h`, `Group.h`, DBC headers, or any other game
header:

```cpp
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
    bool ResolveRoles(std::vector<RoleRequest> const& requests,
        std::vector<RoleAssignment>& assignments, RoleNeeds& needs);
    bool IsProposalReady(std::size_t playerCount, bool isPremade,
        bool testing, RoleNeeds const& needs);
    GroupPacketValues MakeGroupPacketValues(std::uint8_t role,
        bool finished, std::uint32_t dungeonEntry);
    bool IsTeleportTarget(DungeonCandidate const& dungeon);
    bool ShouldVoteKick(std::uint64_t targetGuid,
        std::uint64_t kickerGuid);
    std::vector<std::uint64_t> CollectExpiredOwners(
        std::vector<TimedOwner> const& owners, std::int64_t now);
}
```

The role constants are local numeric bit values matching the 3.3.5 protocol
(`leader=0x01`, `tank=0x02`, `healer=0x04`, `damage=0x08`).
`ResolveRoles` strips the leader bit, orders the search deterministically by
fewest selected roles and then GUID, and uses bounded backtracking over at most
five players. It succeeds only when every player has one assignment within the
1/1/3 capacities and returns the remaining capacities as `RoleNeeds`.

Required regression cases:

- Random Classic entry 258 expands to actual group-1 dungeon IDs and excludes
  entry 258 and every other category row.
- A selected proposal dungeon is a member of the compatible candidate set;
  deterministic injected/random-index boundaries cover first and last entries.
- Empty and unknown selections fail without dereference.
- Solo and party eligibility failures cannot be overwritten by later success.
- A logged-out solo or premade member is removed from active matching before
  registry removal, and team comparison uses captured queue data.
- Merging queue units intersects candidates and preserves each player's random
  category independently.
- Premade role-check initialization contains every member before publication,
  the all-answered scan terminates on pending roles, and
  success/failure/timeout cleanup removes the stored check.
- Group packet value helpers return actual role, packed actual dungeon entry,
  active/finished state bytes, a changing update counter, and assigned roles
  for every other-member tuple.
- Teleport eligibility rejects category/map-0/non-instance destinations and
  permits the actual Stockades row on map 34.
- Completion retains actual dungeon state and selects rewards using the
  per-player random category; a second completion callback grants nothing, and
  the first completion registers the player's daily run.
- Self-leave is distinguished from kicking another member.
- Testing mode allows exactly one solo queue unit to become proposal-ready; it
  leaves the queue before proposal creation, while normal mode and all
  multi-player units retain the production role rules.
- Multi-role selections resolve to a valid single-role assignment when one
  exists for normal and heroic five-player candidates, and reject impossible
  compositions.
- Expired role-check owners are collected before manager erasure, preventing
  active-iterator invalidation.
- Group member-removal and disband hooks remove their complete LFD ownership
  sets and are safe when called more than once.

Each production behavior change follows red-green-refactor: add one failing
test, observe the expected failure, implement the minimum change, and rerun the
focused test before proceeding. At phase end, run the complete available C++
test target and build the server from the isolated worktree using a fresh or
verified build directory. No live runtime claim is made until the user performs
the one-player smoke test.

## Acceptance criteria

- Random mode never stores or teleports to a category row and cannot select map
  0 for Random Classic Dungeon.
- Actual Stockades selection teleports to map 34 using the existing entrance
  trigger.
- The stock client receives enough LFD group metadata to show Teleport Out
  inside the selected dungeon and Teleport To Dungeon outside it.
- Both teleport directions work before completion, and teleport-out still works
  after completion.
- Random rewards retain the original category while specific-queue players do
  not receive random-category credit.
- Solo and premade restrictions, role checks, proposals, decline/timeout,
  logout, self-leave, kick, completion, daily registration, and cleanup have
  deterministic regression coverage.
- `.debug lfd` enables the user to complete the normal proposal and dungeon
  path alone, is administrator-only, defaults off, and is visibly temporary.
- Targeted tests and the phase-end Windows server build succeed from the
  isolated worktree without changing the source checkout or database repo.

## Existing completion trigger

`HandleBossKilled` is currently invoked by dungeon-category completion in
`AchievementMgr::CompletedCriteriaFor`, despite its name. This repair preserves
that trigger and makes its state/reward handling idempotent; replacing it with a
new boss-death or instance-script completion system is outside scope.
