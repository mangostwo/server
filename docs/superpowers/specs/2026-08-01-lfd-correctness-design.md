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
captured `TeamId team`. The default constructor remains solo/default state; the
parameterized constructor receives an explicit `bool isGroup` and every call
site passes false for a solo owner or true for a premade owner. `currentRoles`
holds each player's selected role mask while queued. When queue units merge,
their candidate sets are intersected and their per-player random maps and role
masks are merged.

The new aliases in `LFGMgr.h` are concrete:

```cpp
typedef std::unordered_map<ObjectGuid, uint32> playerDungeonMap;
typedef std::unordered_map<ObjectGuid, uint32> ownerProposalMap;
```

`m_playerQueueOwners` uses the existing `playerGroupMap` alias; its value is the
solo-player or premade-group owner GUID. `m_ownerProposalIds` uses
`ownerProposalMap`; its value is the active proposal ID.

Queue units also preserve their pre-merge ownership boundary:

```cpp
struct LFGQueueSource
{
    ObjectGuid ownerGuid;
    std::set<uint32> dungeonList;
    playerDungeonMap randomDungeonByPlayer;
    roleMap selectedRoles;
    std::string comment;
    TeamId team;
    bool isGroup;
    time_t joinedTime;
};

typedef std::unordered_map<ObjectGuid, LFGQueueSource> queueSourceMap;
```

Every newly published solo or premade `LFGPlayers` unit starts with one
immutable `sourceUnits` entry containing the original actual candidates,
per-player random identities, selected multi-role masks, comment, team,
ownership kind, and queue time. `MergeGroups` unions these source entries
without rewriting them while its top-level candidate set and roles become the
compatible merged view. This is the information boundary required to undo a
failed proposal.

`LFGPlayers` explicitly adds `queueSourceMap sourceUnits` alongside
`randomDungeonByPlayer` and `team`. Its parameterized constructor accepts
`playerDungeonMap const&`, `TeamId`, and `queueSourceMap const&` and initializes
all three members. Initial solo/premade publication constructs a one-entry map
keyed by its owner; merged construction passes the union.

Add `playerGroupMap m_playerQueueOwners`, mapping every queued, role-check, or
proposal player to the solo or group queue owner. It is published and removed
with the queue transition, supplies deterministic logout cleanup, and avoids
searching live `Player` objects merely to discover ownership.

Its value is phase-specific but never ambiguous: during role check or queue it
is the current top-level `m_playerData` owner, and a merge rewrites every merged
member to the surviving aggregate owner. `BeginProposal` uses `sourceUnits` to
rewrite each player to that player's immutable source owner; that owner then
resolves through `m_ownerProposalIds`. Restore republishes the source owner as
the top-level queue owner, while success/discard clears the player mapping.

Add `ownerProposalMap m_ownerProposalIds`, mapping every source queue owner
(solo player or premade group) to its active proposal ID. Proposal creation and
unwind maintain it atomically with `m_proposalMap`; group cleanup can therefore
find its proposal directly rather than linearly scanning all proposals.

For a premade role check, `LFGRoleCheck::dungeonList` holds the same actual
candidate set and the struct adds
`playerDungeonMap randomDungeonByPlayer`. Before insertion into
`m_roleCheckMap`, `JoinLFG` enumerates all online group members and completely
initializes both `currentRoles` (leader's submitted mask, zero for members still
pending) and `randomDungeonByPlayer`
(the request category for random queueing, zero for a specific request). Only
then is the role check published. `PerformRoleCheck` mutates that stored object,
not a detached copy. On success it copies the stored actual candidates,
per-player random map, and selected roles into `LFGPlayers` before publishing
the queue unit. Successful, aborted, failed, and timed-out checks all have
explicit removal paths.

`LFGPlayerStatus::dungeonList` remains the client-facing request list. Random
players see their category entry; specific players see their selected dungeon
entries. This keeps queue UI semantics separate from match candidates.

### Proposal data

`LFGProposal::dungeonID` is always the selected actual dungeon ID. Add
`playerDungeonMap randomDungeonByPlayer` and
`queueSourceMap sourceUnits`. The latter snapshots every solo-player or
premade-group queue source consumed by this proposal, because one proposal may
combine multiple owners and the resolved `currentRoles` cannot reconstruct the
original multi-role selections. `BeginProposal` copies the merged queue unit's
source map before erasing queue ownership and publishes
`m_ownerProposalIds[owner]` for every source key. `currentRoles` contains the
resolved protocol role mask assigned to every player, not the original
multi-role selection.
Proposal creation selects uniformly from the compatible actual candidate set
using the core random-number helper. Empty candidate sets cannot create
proposals. Add `time_t expiresAt`, set to `time(NULL) + LFG_TIME_PROPOSAL`.

The proposal carries the per-player requested-random map. Decline and timeout
classify a source as failed when it contains a player who declined or remained
pending at expiry; that complete solo/premade source leaves LFD. Sources whose
members agreed are restored once from their immutable snapshots after
revalidating that all members are online and available. Revalidation starts
from the snapshot's actual candidates, intersects them with every member's
current `GetJoinResult` and `FindRandomDungeonsNotForPlayer` eligibility, and
publishes only that current nonempty intersection; if any member fails or the
intersection becomes empty, the entire source is discarded. A group-creation
failure restores every still-valid source. Every
path removes the proposal and all of its reverse owner mappings exactly once.
Thus restoration retains original selected masks, source boundaries, candidate
provenance, random identities, team, comment, ownership kind, and joined time,
while its actual candidates reflect current eligibility.

### In-dungeon group data

`LFGGroupStatus::dungeonID` is the actual selected dungeon ID. Add
`playerDungeonMap randomDungeonByPlayer`; its existing `playerRoles` contains
the assigned protocol role masks. A mask contains exactly one combat-role bit
(tank, healer, or damage) and additionally `PLAYER_ROLE_LEADER` for the one
player selected as group leader. Its parameterized constructor adds a
`playerDungeonMap const&` argument and initializes the new member;
`CreateDungeonGroup` passes `proposal.randomDungeonByPlayer`. The record survives
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

It returns false when the group status or player role is absent. `Group.h` adds
a separate `uint32 m_updateCounter` member, initialized to zero by every group
constructor, because the packet sequence belongs to the group rather than LFD
manager state. On success, `GetGroupUpdateData` converts the stored raw
`status->dungeonID` with `GetDungeonEntry` and returns that packed value in
`dungeonEntry`; `Group::SendUpdate` serializes it unchanged.

## Queue and role-check flow

1. Reject an empty request before reading its first element.
2. Resolve every requested entry before dereferencing it. Reject unknown IDs,
   incompatible mixed types, and multiple selections containing a random
   category.
3. For a random request, record the category separately, expand by its DBC
   group, and keep only non-category dungeon/heroic rows whose map satisfies
   `MapEntry::IsNonRaidDungeon()`. `Instanceable()` alone is insufficient
   because it also accepts raids, battlegrounds, and arenas.
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
   unordered-map iterator. Because the field is compared with `time(NULL)`,
   `LFG_TIME_ROLECHECK` is corrected from `45 * IN_MILLISECONDS` to 45 seconds.
7. Matching intersects actual candidate sets and combines per-player selected
   role masks and request identities. A bounded backtracking resolver assigns
   each player one selected role without exceeding one tank, one healer, and
   three damage. It handles multi-role masks rather than switching on the whole
   mask. A partial unit is compatible when every current player can be assigned
   within those capacities; a five-player unit is ready only when the exact
   1/1/3 composition is resolved. The solver returns one combat-role bit per
   player. Before proposal publication, leader selection preserves the leader
   of a reused premade group; otherwise it chooses the lowest-GUID player who
   requested `PLAYER_ROLE_LEADER`, falling back to the lowest GUID. Exactly that
   player's combat role is ORed with `PLAYER_ROLE_LEADER`. These final protocol
   masks are copied into the proposal and group status and are used by proposal
   and group-list packets.
8. Proposal creation randomly selects one actual compatible dungeon and
   carries request identity forward unchanged.

`UpdateNeededRoles` derives missing roles from the resolver for every supported
five-player candidate, including normal and heroic difficulties. It does not
read `*dungeonList.begin()` or special-case normal difficulty. An empty
candidate set fails before this function. Wait-time accounting treats a player
as eligible for every role bit selected until the proposal fixes one assignment.
`AddToWaitMap` iterates the ordered combat-role bits returned by
`LFGLogic::SelectedCombatRoles(mask)` and inserts the player in each selected
role map; it never switches on the whole multi-role mask. After acceptance,
`UpdateWaitMap` receives only the player's resolved single combat role. It
accepts a zero-second sample, updates that role's wait map, and always updates
the actual dungeon's `m_avgWaitTime` with the same sample; a shared internal
sample helper replaces the current role `switch` default that updates only one
map.
The stored `neededTanks`, `neededHealers`, and `neededDps` are display/wait-time
data only. `RoleMapsAreCompatible` concatenates the two units' selected-role
masks and reruns `ResolveRoles` on the combined unit; it never accepts or rejects
a merge by comparing the cached needs from two independently chosen partial
assignments.

Queue publication, merge, and proposal transitions use one manager helper to
update the internal queue record and every affected `LFGPlayerStatus`. The
status retains that player's original client-facing selection, while its state,
update type, comment, and queue membership change atomically with the internal
candidate intersection. A merge must not leave `CMSG_LFG_GET_STATUS` describing
the pre-merge state.

Concretely, `TransitionQueueUnit(ownerGuid, state, updateType)` resolves the
live aggregate through `m_playerData`, sets its `currentState`, and updates the
state/update type of every member status in the same operation. Queue and role-
check code stops calling the current player-only `SetPlayerState` and
`SetPlayerUpdateType` helpers. After `BeginProposal` has erased the aggregate,
proposal/group paths use an explicitly player-only status helper because no
queue record remains to synchronize. Missing owner or member status aborts and
cleans the transition rather than partially updating one side.

`FindQueueMatches` iterates a snapshot of owner GUIDs rather than live
`m_queueSet` iterators. `FindSpecificQueueMatches` likewise snapshots candidate
owners and rechecks each owner still exists and is queued before use. After a
merge or `BeginProposal` it returns immediately because either transition can
erase queue-set entries and invalidate queue-data pointers.

`MergeGroups` erases the consumed buffer owner from `m_queueSet` before erasing
`m_playerData[bufferOwner]`, then rewrites all merged player reverse mappings to
the surviving aggregate owner. `BeginProposal` copies `sourceUnits` and all
proposal data first, erases the aggregate owner from both `m_queueSet` and
`m_playerData`, and only then publishes proposal/reverse state. Neither success
nor unwind can therefore expose a stale aggregate queue record.

`RemoveOldProposals`, called from `LFGMgr::Update`, collects expired proposal IDs
before modifying maps. A pending proposal expires after 45 seconds and uses the
same decline/unwind path as a negative response: player statuses are updated,
reverse proposal ownership is removed, compatible surviving source units are
restored at most once, and disconnected/stale units are cleaned. Thus a missing
client response cannot leave an indefinite proposal.

Decline, timeout, and group-creation failure call one
`UnwindProposal(proposalId, failedPlayers)` helper. It moves a proposal snapshot
out of `m_proposalMap` before restoration, removes each `sourceUnits` key from
`m_ownerProposalIds`, clears every source member's stale
`m_playerQueueOwners`, and then either republishes or discards each source once.
Successful group creation performs the same reverse-map cleanup without
restoration before erasing the proposal. No path retains an owner-to-proposal
or player-to-queue mapping after that owner has been consumed into an LFD
group.

All LFD lifecycle timestamps use `time_t` seconds. In addition to correcting
`LFG_TIME_ROLECHECK`, proposal wait accounting uses
`time(NULL) - joinTime` directly and never divides that value by
`IN_MILLISECONDS`. `SendLfgBootUpdate` sends
`LFGLogic::RemainingSeconds(boot.startTime, LFG_TIME_BOOT, time(NULL))` and
never divides the already-seconds value by 1000. Role-check, proposal, and boot
constants are therefore consistently 45, 45, and 120 seconds at their
comparison and packet boundaries.

Each `LFGMgr::Update` tick calls `RemoveOldRoleChecks()`,
`RemoveOldProposals()`, and `RemoveOldBoots()` before queue matching, then runs
`FindQueueMatches()` and `SendQueueStatus()`. Each cleanup helper independently
collects keys before erasing manager maps.

## Join validation

Join validation starts from `ERR_LFG_OK` and returns immediately on a decisive
failure. For a solo player, deserter, cooldown, battleground, arena, and level
checks cannot be overwritten by a final unconditional success. For a party,
each member is checked in a stable order; the first failure is retained, and a
later valid member cannot reset it. Offline-member and group-size failures are
reported after the checks needed to establish those conditions.

The minimum level 15 rule is applied to the solo player as well as every party
member before queue publication.

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

The active/completed values are not speculative: the maintained local CMaNGOS
WotLK implementation writes `0` for an active LFD group and `2` when its player
state is `LFG_STATE_FINISHED_DUNGEON`. Read-only IDA inspection provides the
authoritative 12340 layout: the `SMSG_GROUP_LIST` reader at `0x6D8870` reads the
group-type byte, subgroup byte, flags byte, and recipient role byte; when
group-type bit `0x08` (`GROUPTYPE_LFD`) is set it then reads an LFD state byte
and dungeon-entry dword before the group GUID and counter. It reads one role
byte at the end of every other-member tuple. The parser passes that conditional
dungeon dword through `sub_52C0A0` into `dword_BD19A4`, and
`IsInLFGDungeon()` at `0x555660` resolves that entry and compares its map and
difficulty with the current instance. The user's smoke test remains the runtime
UI confirmation, but no fork-derived packet assumption is needed to choose the
layout or values.

The current fourth-header-byte comment is wrong: this field is consumed as the
recipient role. This is verified for both packet branches because `0x6D8870`
unconditionally reads the byte into `byte_BD198A`, while only the following
state and dungeon fields are conditional on `GROUPTYPE_LFD`. The client helper
at `0x52C860` returns `byte_BD198A` for the local player, and its caller at
`0x60C810` tests bits `0x02`, `0x04`, and `0x08` as tank, healer, and damage.
For an LFD group the server therefore writes the assigned LFD role, followed
immediately by the LFD state byte and packed dungeon entry. For a non-LFD
group, including a battleground group, the server preserves the existing
fourth-byte value (`isBGGroup() ? 1 : 0`) and appends no LFD state/dungeon
fields; this avoids changing an unrelated legacy packet path even though the
12340 client does not interpret that value as a combat role. Each packet
emitted by `Group::SendUpdate()` writes
the call's current `m_updateCounter`. The method increments the counter once
after sending that update to all recipients, so every recipient gets the same
sequence value for one logical update and observes a monotonically increasing
value across later calls. If an LFD group status record is unexpectedly absent,
the packet uses safe zero values and logs the invariant violation; it must not
invent a category or map.

Implementation captures `uint32 counter = m_updateCounter` before the recipient
loop, writes `counter` into every packet built in that loop, and executes
`++m_updateCounter` once after the loop. Every `Group` constructor initializes
the member to zero.

The actual dungeon entry enables the stock client's `IsInLFGDungeon()` to
compare its map and difficulty with the player's current instance. Proposal
packets already carry the actual selected dungeon; after group creation the
conditional group-list field is the continuing source of that actual dungeon.
Client-facing queue update packets therefore continue to use each player's
original request entry, preserving random-category UI identity rather than
overwriting it with the selected dungeon.

`SendLfgUpdate` retains the stock layout confirmed by the 12340 client handler
at `0x55BDC0`: case 871 reads player update type, joined, then (when joined)
queued, two flag bytes, dungeon count, dungeon dwords, and comment; case 872
reads party update type, joined, then LFG-joined, queued, two flag bytes, three
party bytes, dungeon count, dungeon dwords, and comment. The server must,
however, convert every ID in `LFGPlayerStatus::dungeonList` with
`GetDungeonEntry(id)` before writing it; sending bare IDs is not the packed
client slot representation. The conversion occurs inside
`WorldSession::SendLfgUpdate`, immediately before each dungeon dword is appended;
the manager-owned status list remains raw.

`SMSG_LFG_QUEUE_STATUS` follows the same packed-slot rule. Manager queue state
continues to store raw actual candidate IDs for matching, but status is chosen
per recipient: a random player uses
`queueInfo->randomDungeonByPlayer[playerGuid]`, while a specific-queue player
uses the guarded first entry of that player's client-facing
`LFGPlayerStatus::dungeonList`. `SendQueueStatus` converts that request ID with
`GetDungeonEntry(id)` before assigning `LFGQueueStatus::dungeonID`;
`WorldSession::SendLfgQueueStatus` then serializes that packed value unchanged.
`SendQueueStatus` uses two explicitly named variables: `requestDungeonEntry`
for that client field and a separately guarded raw `actualDungeonId` from
`queueInfo->dungeonList` for all tank/healer/damage/average wait-map lookups.
It never indexes wait maps with the packed request entry or a random category.

`SendLfgRoleCheckUpdate` likewise stops reading the removed scalar
`randomDungeonID`. It uses the leader's nonzero
`roleCheck.randomDungeonByPlayer[leaderGuid]` as the single displayed random
category; otherwise it sends the role check's specific request dungeon set.
Every emitted entry is converted with `GetDungeonEntry`.

`partyForbidden` remains the client-bound exception to raw internal dungeon
keys: `FindRandomDungeonsNotForPlayer` stores packed `dungeon->Entry()` keys and
`SMSG_LFG_JOIN_RESULT` serializes them unchanged. Join filtering masks each key
to its raw 24-bit ID only for removal from the internal actual-candidate set; it
does not rewrite the stored locked-dungeon key to a bare ID.

In `SMSG_LFG_PROPOSAL_UPDATE`, the byte currently named `showProposal` is the
opposite: the 12340 handler logs it as `silent`, and client `sub_552900` emits
`LFG_PROPOSAL_SHOW` only when that value is zero. Rename the local to `silent`.
Normal newly matched proposals set `LFGProposal::isNew = true`, making
`silent = !proposal.isNew && proposal.groupRawGuid == recipientOriginalGroup`
false so the proposal window is shown. The field must not be inverted to one
for new proposals. Backfill/continue semantics are outside this repair.

Each other-member tuple in `SMSG_GROUP_LIST` also ends with that member's
assigned LFG role byte. `Group::SendUpdate()` calls `GetGroupUpdateData` for the
recipient header and for every listed member; offline members use the role
retained in `LFGGroupStatus::playerRoles`, not a hard-coded zero.

## Teleport behavior

The manager interface becomes
`void TeleportPlayer(Player* player, bool out, bool automatic)`. The
post-proposal `TeleportToDungeon` path passes `automatic = true`; the
`CMSG_LFG_TELEPORT` handler passes `automatic = false` for both directions.
Teleport-out ignores the destination-policy flag.

`TeleportPlayer(player, false, automatic)` enters the selected actual dungeon:

- Require a current LFD group and valid group status.
- Reject dead, falling, fatigued, vehicle, charming, and combat-invalid states
  using the existing LFG teleport errors where available.
- Resolve the actual dungeon row and reject category rows, map 0, or a
  map that is not `MapEntry::IsNonRaidDungeon()`. This rejects raids,
  battlegrounds, and arenas even though all can be `Instanceable()`.
- Require `dungeon->Difficulty == pGroup->GetDungeonDifficulty()`; a mismatch
  fails with `LFG_TELEPORTERROR_INVALID_LOCATION` before choosing a destination.
- If the player is already on the selected actual map, return successfully
  without relocating or overwriting the saved entry point.
- For automatic teleport immediately after proposal acceptance, use an
  online group leader already on the selected map as the destination; otherwise
  use the first online member on that map in stable group-member order, then
  fall back to `GetMapEntranceTrigger(actualMapId)`.
- For opcode-driven `CMSG_LFG_TELEPORT(false)`, always use the selected
  dungeon's entrance trigger rather than another member's live position.
- Save the player's battleground-style entry point only when entering from a
  non-instance world location.

`TeleportPlayer(player, true, false)` leaves the dungeon:

- Resolve the same actual dungeon row and require its difficulty to match
  `pGroup->GetDungeonDifficulty()`; a mismatch fails with
  `LFG_TELEPORTERROR_INVALID_LOCATION` rather than trusting inconsistent group
  state.
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

The in-scope Two database was checked before implementation:
`World/Setup/FullDB/areatrigger_teleport.sql` row 101 is "Stormwind Stockades
Entrance" and targets map 34 at `(54.23, 0.28, -18.34, 6.26)`. Because
`GetMapEntranceTrigger` selects by target map and requirement ordering rather
than by trigger ID, the smoke-test preflight calls
`sObjectMgr.GetMapEntranceTrigger(34)` and compares the returned map and target
coordinates and orientation (with normal floating-point tolerance) to row 101.
A missing row or a different selected trigger is reported as a
data-install/selection problem rather than worked around with an outdoor or
hard-coded destination; no database migration is required by this repair.

## Completion, leave, kick, and cleanup

Boss completion is idempotent. If the group status is already
`LFG_STATE_FINISHED_DUNGEON`, `HandleBossKilled` returns before calculating or
sending any reward. Otherwise it changes the group and players to finished,
sends a refreshed group list, and rewards each player using that player's
requested random category plus the group's actual dungeon. After issuing the
first applicable reward it calls `RegisterPlayerDaily` for that player and
dungeon type, so `HasPlayerDoneDaily` changes for subsequent runs. It does not
erase the group status immediately.

`SMSG_LFG_PLAYER_REWARD` receives packed entries, not bare IDs. A nonzero
per-player random category is converted with `GetDungeonEntry(randomId)` and the
actual group dungeon with `GetDungeonEntry(status->dungeonID)` before building
`LFGRewards`; a specific-queue player's random entry remains zero.

Queue cancellation remains `CMSG_LFG_LEAVE` behavior for role check, queue, and
proposal states. In-dungeon exit continues to use `CMSG_LFG_TELEPORT(true)`.

An LFD member selecting ordinary Leave Party removes that member rather than
starting a vote against themselves. In `Player::RemoveFromGroup`, an LFD call
where `guid == kicker` calls `Group::RemoveMember(guid, 0)` and performs the
same object-manager deletion used by an ordinary group when the remaining count
requires it. Only `guid != kicker` routes to `AttemptToKickPlayer`. Kicking a
different member therefore retains the vote-kick flow.

Boot votes have a bounded lifecycle rather than leaving the group in
`LFG_STATE_BOOT`. `AttemptToKickPlayer` accepts only an
`LFG_STATE_IN_DUNGEON` group with no active boot and valid distinct target and
kicker members; finished groups cannot start a vote. `LFGBoot` retains its
seconds-based `startTime = time(NULL)` and adds the previous group state;
expiration is always `startTime + LFG_TIME_BOOT`. `CastVote`
accepts a response only for a pending voter. Success, mathematical failure, or
timeout calls one `FinishBootVote(groupGuid, succeeded)` helper, which sends the
final boot packet, restores every surviving member and the group to the saved
state, performs the successful kick if applicable, and erases
`m_bootStatusMap[groupGuid]` exactly once. `RemoveOldBoots`, called by
`LFGMgr::Update`, first collects expired group GUIDs and then fails each vote
outside map iteration. If dungeon completion occurs during a boot, it first
finishes the vote as failed and then performs the idempotent completion
transition.

The lifecycle hooks are explicit:

```cpp
void LFGMgr::OnGroupMemberRemoved(ObjectGuid groupGuid,
    ObjectGuid playerGuid);
void LFGMgr::OnGroupDisband(ObjectGuid groupGuid);
void LFGMgr::OnGroupLeaderChanged(ObjectGuid groupGuid,
    ObjectGuid newLeaderGuid);
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
the group removal code. Both `Group::ChangeLeader` and the automatic promotion
branch of `Group::RemoveMember` call the leader hook after `_setLeader` has
updated `m_leaderGuid`; the removal path passes the newly promoted
`m_memberSlots.front().guid`. In the removal path,
`OnGroupMemberRemoved` runs first, then `OnGroupLeaderChanged`, both before the
existing `SendUpdate()`. The leader hook updates `LFGGroupStatus::leaderGuid`,
clears `PLAYER_ROLE_LEADER` from every saved LFD role mask, and sets it on the
new leader's assigned combat role. It does not send a packet: the existing
single `SendUpdate()` at the end of `Group::RemoveMember` and
`Group::ChangeLeader` serializes the refreshed state without a duplicate
`SMSG_GROUP_LIST`.

Add `bool LFGMgr::OnPlayerLogout(Player* player)`, called from
`WorldSession::LogoutPlayer` before the player is removed from the registry and
before its normal non-raid group-removal branch. The hook first examines the
live `player->GetGroup()` and `m_groupStatusMap`; it returns true only when the
player belongs to an LFD group whose status is `LFG_STATE_IN_DUNGEON`,
`LFG_STATE_BOOT`, or `LFG_STATE_FINISHED_DUNGEON`. `LogoutPlayer` uses that return value as
`retainLfgGroup` and skips its generic `RemoveFromGroup()` path when true. This
preserves the offline member and LFD group on explicit logout; clean disconnect
already skips that generic path, while the hook still performs the LFD state
cleanup described below. Neither path depends on queue-only reverse ownership
to recognize an in-dungeon group.

During `LFG_STATE_BOOT`, logout retains group membership and any already-cast
vote; a pending offline vote remains pending and the normal 120-second expiry
guarantees resolution. It does not cancel or silently pass the vote.

For pre-dungeon states the hook returns false after using
`m_playerQueueOwners`: a solo queue is cancelled; a premade role check or queue
is cancelled for the whole premade because its required member is no longer
available; and a pending proposal is declined through the normal proposal
unwind. An in-dungeon or finished group retains its group record, player role,
and per-player random category until ordinary leave/disband cleanup.
`MatchesAreOfSameTeam` compares captured `LFGPlayers::team` values and never
dereferences registry players. A missing owner/state makes units incompatible
and schedules their stale queue records for cleanup.

## Temporary `.debug lfd` smoke mode

Add `.debug lfd` beside `.debug bg`, restricted to `SEC_ADMINISTRATOR`:
`Chat.cpp` registers `lfd` in `debugCommandTable`, `Chat.h` declares
`HandleDebugLfdCommand`, and the handler toggles an `LFGMgr::m_testing` flag
initialized to false and prints or broadcasts whether one-player LFD testing
is enabled. Public `bool IsTesting() const` and `void SetTesting(bool)` methods
are the command's only access to the private flag.

When enabled, `AddToQueue` publishes a valid queue unit normally. At the start
of `FindSpecificQueueMatches`, before comparing it with peer units, an explicit
`currentState == LFG_STATE_QUEUED && m_testing &&
currentRoles.size() == 1 && !isGroup` branch verifies that its actual candidate
set is non-empty, `AllRolesAnswered` accepts the selected mask, and
`ResolveRoles` yields one combat-role assignment. It then calls one shared
`BeginProposal(ownerGuid)` transition. Testing bypasses only the missing four
players; a zero or leader-only mask cannot enter a proposal.
`BeginProposal` snapshots the queue data, erases the aggregate queue record and
queue-set owner, changes every player status to proposal, and only then calls
`SendDungeonProposal` with the owned snapshot.
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
    bool IsProposalReady(std::size_t playerCount, bool testing,
        RoleNeeds const& needs);
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
```

The role constants are local numeric bit values matching the 3.3.5 protocol
(`leader=0x01`, `tank=0x02`, `healer=0x04`, `damage=0x08`).
Live adapters set `DungeonCandidate::fivePlayerDungeon` from
`MapEntry::IsNonRaidDungeon()`, not `MaxPlayers`, and both
`FilterRandomCandidates` and `IsTeleportTarget` require it.
They set `DungeonCandidate::difficulty` directly from
`LfgDungeonsEntry::Difficulty`.
They set `DungeonCandidate::category` when `LfgDungeonsEntry::TypeID` is neither
`LFG_TYPE_DUNGEON` nor `LFG_TYPE_HEROIC_DUNGEON`; a seasonal flag by itself
does not turn an otherwise actual dungeon row into a category.
Before constructing the dependency-free candidate vector, the manager adapter
uses `IsSeasonActive` to omit any row whose seasonal flag is inactive; the pure
`FilterRandomCandidates` never attempts to query world-event state.
`GroupHeaderRole` returns the assigned role for LFD groups and preserves the
legacy `1`/`0` battleground value for non-LFD groups. `IsTeleportTarget` is
exactly `!dungeon.category && dungeon.mapId != 0 &&
dungeon.fivePlayerDungeon && dungeon.difficulty == groupDifficulty`.
`AllRolesAnswered` requires at least one combat-role bit for every request; a
zero mask or leader-only mask is invalid rather than complete.
`SelectedCombatRoles` returns selected tank, healer, and damage bits in that
stable order and ignores the leader bit.
`ResolveRoles` strips the leader bit, orders the search deterministically by
fewest selected roles and then GUID, and uses bounded backtracking over at most
five players. It succeeds only when every player has one assignment within the
1/1/3 capacities and returns the remaining capacities as `RoleNeeds`.
`IsProposalReady` returns true for production only at five players with zero
remaining needs. With `testing == true`, it additionally accepts one player
after the caller has enforced the debug-solo conditions and successful
`AllRolesAnswered`/`ResolveRoles`; it does not encode premade state.
`ElapsedSeconds(start, now)` returns `max(0, now - start)` and
`RemainingSeconds(start, duration, now)` clamps to the inclusive range
`[0, duration]`. Queue status casts the elapsed result to `uint32` only after
that clamp, so a backward wall-clock adjustment cannot underflow.
`src/tests/CMakeLists.txt` appends
`${CMAKE_SOURCE_DIR}/src/game/WorldHandlers/LFGLogic.cpp` and
`LFGLogicTest.cpp` to `SRC_GRP_TESTS`; otherwise the pure implementation would
not be linked into `mangos_tests`.

Required regression cases:

- Random Classic entry 258 expands to actual group-1 dungeon IDs and excludes
  entry 258, every other category row, raids, battlegrounds, and arenas.
- A selected proposal dungeon is a member of the compatible candidate set;
  deterministic injected/random-index boundaries cover first and last entries.
- Empty and unknown selections fail without dereference.
- Solo and party eligibility failures cannot be overwritten by later success.
- A logged-out solo or premade member is removed from active matching before
  registry removal, and team comparison uses captured queue data.
- Merging queue units intersects candidates and preserves each player's random
  category and immutable source snapshot independently; decline/timeout removes
  the failed source and restores each agreed source with its original selected
  masks and candidates exactly once.
- Premade role-check initialization contains every member before publication,
  the all-answered scan terminates on pending roles, and
  success/failure/timeout cleanup removes the stored check.
- Group packet value helpers return actual role, packed actual dungeon entry,
  active/finished state bytes, a changing update counter, and assigned roles
  for every other-member tuple. Header fixtures cover an LFD recipient role and
  preservation of the existing non-LFD battleground byte; non-LFD packets omit
  the conditional state/dungeon fields.
- Player and party LFG update fixtures preserve the IDA-confirmed field order
  and write packed request entries for both random and specific queues.
- Role-check and queue-status packets show a random recipient's packed category
  while internal matching and wait maps remain keyed by a separate actual
  dungeon ID.
- New proposals serialize `silent = 0` and cause the stock client to emit
  `LFG_PROPOSAL_SHOW`; queue-unit transitions leave manager and player status in
  the same state.
- Teleport eligibility rejects category/map-0/non-instance destinations and a
  dungeon/group difficulty mismatch, and permits the actual Stockades row on
  map 34. Automatic formation may use an existing member while opcode re-entry
  always uses the entrance trigger. Smoke preflight verifies that
  `GetMapEntranceTrigger(34)` resolves row 101's target coordinates and
  orientation.
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
- Multi-role queue entries are added to every selected combat-role wait map;
  the resolved proposal mask has one combat role, and exactly the selected
  group leader also has the leader bit.
- Queue-status and player/party update dungeon values are packed entries, not
  raw IDs.
- Expired role-check owners are collected before manager erasure, preventing
  active-iterator invalidation.
- Wait time from 100 to 145 is 45 seconds, and a 120-second boot begun at 100
  reports 120 at time 100, 60 at time 160, and clamps to zero after expiry.
- Zero-second matches update both the resolved role wait map and average map.
- Boot success, failure, timeout, completion-during-boot, and logout-during-boot
  restore deterministic state and erase the boot record. Explicit leader
  change and member-removal auto-promotion each move the sole leader bit to the
  new leader, update `LFGGroupStatus::leaderGuid`, and produce one group-list
  update rather than two.
- Group member-removal and disband hooks remove their complete LFD ownership
  sets, including `m_ownerProposalIds`, and are safe when called more than once.
- Explicit logout cancels pre-dungeon ownership but retains an in-dungeon or
  finished LFD group by bypassing the generic non-raid group-removal path.

Each production behavior change follows red-green-refactor: add one failing
test, observe the expected failure, implement the minimum change, and rerun the
focused test before proceeding. At phase end, run the complete available C++
test target and the repository's supported compiler gates: a Windows MSVC
server build plus Linux GCC and Clang configure/build/test equivalents of the
CI workflows, each from a fresh or source-verified build directory. No live
runtime claim is made until the user performs the one-player smoke test.

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
  logout, self-leave, kick/boot timeout, leader change, completion, daily
  registration, and cleanup have deterministic regression coverage.
- `.debug lfd` enables the user to complete the normal proposal and dungeon
  path alone, is administrator-only, defaults off, and is visibly temporary.
- Targeted tests and phase-end MSVC, GCC, and Clang server gates succeed from
  the isolated worktree without changing the source checkout or database repo.

## Existing completion trigger

`HandleBossKilled` is currently invoked by dungeon-category completion in
`AchievementMgr::CompletedCriteriaFor`, despite its name. This repair preserves
that trigger and makes its state/reward handling idempotent; replacing it with a
new boss-death or instance-script completion system is outside scope.
