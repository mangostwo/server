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

## Data model

### Queue data

`LFGPlayers::dungeonList` becomes the compatible actual-dungeon candidate set.
It must not be repurposed as a client display list. Add a per-player map from
player GUID to requested random category ID, with zero representing a specific
dungeon request. When queue units merge, their candidate sets are intersected
and their per-player request maps are merged alongside their roles.

For a premade role check, `LFGRoleCheck::dungeonList` holds the same actual
candidate set. Its `currentRoles` map must be completely initialized before the
role check is inserted into `m_roleCheckMap`. `PerformRoleCheck` must mutate the
stored object, not a detached copy. Successful, aborted, failed, and timed-out
checks all have explicit removal paths.

`LFGPlayerStatus::dungeonList` remains the client-facing request list. Random
players see their category entry; specific players see their selected dungeon
entries. This keeps queue UI semantics separate from match candidates.

### Proposal data

`LFGProposal::dungeonID` is always the selected actual dungeon ID. Proposal
creation selects uniformly from the compatible actual candidate set using the
core random-number helper. Empty candidate sets cannot create proposals.

The proposal carries the per-player requested-random map. Decline, timeout, or
group-creation failure restores or clears each source queue consistently and
does not lose the request identity required by later rewards.

### In-dungeon group data

`LFGGroupStatus::dungeonID` is the actual selected dungeon ID. It also carries
the per-player requested-random map and the assigned roles. The record survives
the transition from `LFG_STATE_IN_DUNGEON` to
`LFG_STATE_FINISHED_DUNGEON` and is removed only when the LFD group is fully
cleaned up after exit, leave, or disband.

Read-only manager queries expose the actual dungeon entry, per-player role, and
finished state needed by `Group::SendUpdate()` without exposing mutable map
storage to `Group`.

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
6. After successful role confirmation, enqueue the actual candidates and
   persist the confirmed roles. Remove the role-check record.
7. Matching intersects actual candidate sets and combines per-player roles and
   request identities. Normal readiness remains one tank, one healer, and three
   damage roles.
8. Proposal creation randomly selects one actual compatible dungeon and
   carries request identity forward unchanged.

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

## Client packet contract

For `SMSG_GROUP_LIST`, an LFD group sends:

- The player's confirmed LFG role in the existing fourth header byte.
- LFD state byte `0` while active and `2` after dungeon completion.
- The packed actual LFG dungeon entry for the selected dungeon.
- A monotonically increasing group-update counter rather than a constant zero.

Non-LFD and battleground group packet behavior remains unchanged. If an LFD
group status record is unexpectedly absent, the packet uses safe zero values
and logs the invariant violation; it must not invent a category or map.

The actual dungeon entry enables the stock client's `IsInLFGDungeon()` to
compare its map and difficulty with the player's current instance. Client-facing
queue update packets continue to use the player's original request entry.

## Teleport behavior

`TeleportPlayer(player, false)` enters the selected actual dungeon:

- Require a current LFD group and valid group status.
- Reject dead, falling, fatigued, vehicle, charming, and combat-invalid states
  using the existing LFG teleport errors where available.
- Resolve the actual dungeon row and reject category rows, map 0, or a
  non-instanceable map.
- Use an existing group member already on the selected map as the destination
  when appropriate; otherwise use `GetMapEntranceTrigger(actualMapId)`.
- Save the player's battleground-style entry point only when entering from a
  non-instance world location.

`TeleportPlayer(player, true)` leaves the dungeon:

- Require the player to be on the selected actual dungeon map.
- Restore the saved entry point.
- Keep LFD group identity intact so the player can teleport back in while the
  LFD group remains active, including after completion until group cleanup.

Automatic teleport after proposal acceptance uses the same actual-dungeon
validation and destination resolution as opcode-driven teleport-in, avoiding
two divergent implementations.

## Completion, leave, kick, and cleanup

Boss completion is idempotent. It changes the group and players to finished,
sends a refreshed group list, and rewards each player using that player's
requested random category plus the group's actual dungeon. It does not erase
the group status immediately.

Queue cancellation remains `CMSG_LFG_LEAVE` behavior for role check, queue, and
proposal states. In-dungeon exit continues to use `CMSG_LFG_TELEPORT(true)`.

An LFD member selecting ordinary Leave Party removes that member rather than
starting a vote against themselves. Kicking a different member still uses the
vote-kick flow. The removal path clears the departing player's LFD status and
request identity, teleports or homebinds them through existing group removal
rules, and cleans the group status when the group is disbanded or no LFD
members remain. Repeated cleanup calls are harmless.

Logout does not manufacture a new LFD state. Existing group membership remains
authoritative, and stale queue, proposal, role-check, boot, player-status, and
group-status records are removed at the corresponding cancellation or group
destruction boundary.

## Temporary `.debug lfd` smoke mode

Add `.debug lfd` beside `.debug bg`, restricted to `SEC_ADMINISTRATOR`. It
toggles a manager-wide `m_testing` flag initialized to false and prints or
broadcasts whether one-player LFD testing is enabled.

When enabled, a queue unit containing exactly one player may be considered
ready without satisfying the five-player role composition. It still must pass
normal join validation, have a non-empty actual candidate set, accept a normal
proposal, create an LFD group, and execute the production packet, teleport,
completion, reward, teleport-out, and leave paths. Premade groups and merged
multi-player queue units continue to use normal readiness rules.

The testing flag never persists to the database or configuration and defaults
off on every server start. The command and flag are deliberately isolated so a
follow-up removal deletes the command registration, handler, flag, and one
readiness branch without changing production matching.

## Test strategy

Introduce a focused LFD logic test target/file under the existing C++ test
harness. Extract only pure decisions needed for deterministic coverage rather
than constructing `Player`, `Group`, `WorldSession`, or map services in unit
tests.

Required regression cases:

- Random Classic entry 258 expands to actual group-1 dungeon IDs and excludes
  entry 258 and every other category row.
- A selected proposal dungeon is a member of the compatible candidate set;
  deterministic injected/random-index boundaries cover first and last entries.
- Empty and unknown selections fail without dereference.
- Solo and party eligibility failures cannot be overwritten by later success.
- Merging queue units intersects candidates and preserves each player's random
  category independently.
- Premade role-check initialization contains every member before publication,
  and success/failure/timeout cleanup removes the stored check.
- Group packet value helpers return actual role, packed actual dungeon entry,
  active/finished state bytes, and a changing update counter.
- Teleport eligibility rejects category/map-0/non-instance destinations and
  permits the actual Stockades row on map 34.
- Completion retains actual dungeon state and selects rewards using the
  per-player random category.
- Self-leave is distinguished from kicking another member.
- Testing mode allows exactly one solo queue unit to become proposal-ready;
  normal mode and all multi-player units retain the production role rules.

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
  self-leave, kick, completion, and cleanup have deterministic regression
  coverage.
- `.debug lfd` enables the user to complete the normal proposal and dungeon
  path alone, is administrator-only, defaults off, and is visibly temporary.
- Targeted tests and the phase-end Windows server build succeed from the
  isolated worktree without changing the source checkout or database repo.
