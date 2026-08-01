# Mango Two LFD Correctness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> `superpowers:executing-plans` to execute this plan task by task. Do not use
> subagents in this workspace.

**Goal:** Repair Mango Two's WotLK Dungeon Finder so random queues select and
teleport to real instances, the stock 3.3.5a client receives the state needed
for enter/leave, and queue, proposal, completion, logout, and boot lifecycles do
not strand players. Add a temporary administrator-only one-player smoke mode.

**Architecture:** Keep `LFGMgr` as the lifecycle owner, but separate an actual
dungeon candidate from each player's client-facing random category. Preserve
immutable queue-source snapshots across merges and proposals, use reverse owner
indexes for deterministic cleanup, and route primitive decisions through a
dependency-free `LFGLogic` unit tested by `mangos_tests`. Live adapters retain
all `Player`, `Group`, DBC, packet, and teleport side effects.

**Tech Stack:** C++11, Mango Two game/server APIs, existing dependency-free test
harness, CMake, MSVC 2026. Client packet behavior is grounded in the local
3.3.5a build-12340 UI and read-only decompilation evidence recorded in the
approved design.

**Design:**
`docs/superpowers/specs/2026-08-01-lfd-correctness-design.md`

## Global constraints

- Work only in `E:\Mangos\WIP\Two\LFDSystemsRepair\server` on
  `codex/lfd-systems-repair`; do not modify the source checkout, database repo,
  client extraction, or IDA database.
- No database migration. Stockades map 34 continues to use the installed
  `areatrigger_teleport` row 101.
- Follow red-green-refactor. For each task, add or extend the focused test,
  observe the stated failure, make the minimum production change, and rerun the
  focused test before committing.
- Do not start `mangosd`, `realmd`, MariaDB, or any deployed service. The user
  performs the live `.debug lfd` smoke test after the build handoff.
- Preserve non-LFD group packet behavior and all unrelated queue behavior.
- The temporary smoke bypass applies only to a queued solo owner with one
  player and a valid combat-role selection. It never bypasses normal
  eligibility, proposal acceptance, teleport safety, rewards, or cleanup.
- Run external review only on this plan file before implementation. A later
  implementation review receives only the implementation diff, completed
  checks, and known risks.

## Task 1: Establish the pure LFD decision seam

**Files:**

- Create: `src/game/WorldHandlers/LFGLogic.h`
- Create: `src/game/WorldHandlers/LFGLogic.cpp`
- Create: `src/tests/LFGLogicTest.cpp`
- Modify: `src/tests/CMakeLists.txt`

**Step 1: Configure and verify the clean baseline**

Configure a dedicated build/install pair, reusing the known dependency roots:

```powershell
cmake -S . -B E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_INSTALL_PREFIX=E:/Mangos/WIP/Two/LFDSystemsRepair/install-msvc `
  -DOPENSSL_ROOT_DIR=C:/OpenSSL-Win64-401 `
  -DMySQL_INCLUDE_DIR="C:/Program Files/MariaDB 10.11/include/mysql" `
  -DMySQL_LIBRARY="C:/Program Files/MariaDB 10.11/lib/libmariadb.lib" `
  -DBUILD_TOOLS=1 -DBUILD_MANGOSD=1 -DBUILD_REALMD=1 `
  -DWITH_TESTS=1 -DWITH_NET_TESTS=0 -DSOAP=1 `
  -DSCRIPT_LIB_ELUNA=1 -DSCRIPT_LIB_SD3=1 -DUSE_STORMLIB=1 -DPCH=1
cmake --build E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  --config Release --target mangos_tests --parallel
ctest --test-dir E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  -C Release --output-on-failure
```

Expected: configure, existing test build, and existing suite pass before any
production edit. If configure fails, resolve only the local dependency path;
do not change repository build logic to accommodate this machine.

**Step 2: Write the failing decision tests**

Add `LFGLogicTest.cpp` cases for:

- category filtering (Random Classic group 1 keeps real five-player normal or
  heroic rows and drops category/map-0/raid/BG/arena rows);
- first/last indexed selection and empty/out-of-range selection;
- first non-OK join result;
- empty, zero-role, leader-only, multi-role, valid 1/1/3, impossible, and
  deterministic leader/assignment inputs;
- normal five-player readiness versus the single-player testing exception;
- selected-role expansion into tank/healer/damage bits;
- active/finished group packet values and LFD/non-LFD/BG header role values;
- teleport target category/map/difficulty rules;
- self-leave versus vote-kick;
- elapsed/remaining time clamping and expired-owner collection.

Append `LFGLogicTest.cpp` and
`${CMAKE_SOURCE_DIR}/src/game/WorldHandlers/LFGLogic.cpp` to `SRC_GRP_TESTS`.

Run:

```powershell
cmake --build E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  --config Release --target mangos_tests --parallel
```

Expected red: compile fails because `LFGLogic.h/.cpp` do not exist.

**Step 3: Implement the pure seam**

Implement exactly the standard-library-only API from the approved design.
`ResolveRoles` uses bounded deterministic backtracking over at most five
players, returning one combat role per player and remaining 1/1/3 capacity.
`FilterRandomCandidates` and `IsTeleportTarget` enforce real five-player maps;
the latter also enforces group difficulty. Time helpers use signed seconds and
clamp before conversion.

**Step 4: Verify and commit**

```powershell
cmake --build E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  --config Release --target mangos_tests --parallel
E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc/src/tests/Release/mangos_tests.exe `
  -only LFG
git diff --check
git add src/game/WorldHandlers/LFGLogic.h `
  src/game/WorldHandlers/LFGLogic.cpp src/tests/LFGLogicTest.cpp `
  src/tests/CMakeLists.txt
git commit -m "test: add LFD decision coverage"
```

Expected green: all selected LFG tests pass.

## Task 2: Separate queue candidates from request provenance

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGMgr.cpp`
- Modify: `src/game/WorldHandlers/LFGMgrQueue.cpp`
- Modify: `src/game/WorldHandlers/LFGMgrProposal.cpp`
- Modify: `src/tests/LFGLogicTest.cpp`

**Step 1: Add failing adapter-oriented pure cases**

Extend tests with DBC-shaped candidates covering an inactive seasonal row,
unknown/empty selections, and normal/heroic rows sharing a random group. Verify
the pure result contains only actual IDs and that request category identity is
not present in the candidate set.

Expected red: current queue code has no usable actual/category separation and
the new adapter expectations are not yet represented.

**Step 2: Add the queue data model**

In `LFGMgr.h` add `playerDungeonMap`, `ownerProposalMap`, `LFGQueueSource`, and
`queueSourceMap`. Extend `LFGPlayers`, `LFGRoleCheck`, `LFGProposal`, and
`LFGGroupStatus` with the approved initialized fields and constructors. Add
`m_playerQueueOwners`, `m_ownerProposalIds`, proposal expiry, captured `TeamId`,
and random category maps. Correct `LFG_TIME_ROLECHECK` to 45 seconds.

**Step 3: Make join and role check publish complete units**

In `JoinLFG`:

- guard empty/unknown/mixed selections before dereference;
- keep the requested random entry only in per-player/UI provenance;
- adapt DBC rows into `LFGLogic::DungeonCandidate`, prefilter inactive seasonal
  rows, and expand to real non-raid dungeon candidates;
- enforce solo level 15 and retain the first party/solo failure;
- capture team and initialize all premade member roles/random identities before
  inserting the role check;
- publish one immutable source snapshot and every reverse owner mapping.

In `PerformRoleCheck`, mutate the stored check, use the bounded answered scan
and role resolver, and erase success/failure/abort records. In
`RemoveOldRoleChecks`, collect keys before erasing.

**Step 4: Synchronize queue and player state**

Add `TransitionQueueUnit(owner, state, updateType)` plus an explicitly
player-only status helper for proposal/group phases. Make add/remove/role-check
paths use the appropriate helper. `AddToWaitMap` records every selected combat
role, and `UpdateNeededRoles` uses the role resolver without difficulty or
empty-set special cases.

**Step 5: Verify and commit**

```powershell
cmake --build E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  --config Release --target mangos_tests game --parallel
E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc/src/tests/Release/mangos_tests.exe `
  -only LFG
git diff --check
git add src/game/WorldHandlers/LFGMgr.h `
  src/game/WorldHandlers/LFGMgr.cpp `
  src/game/WorldHandlers/LFGMgrQueue.cpp `
  src/game/WorldHandlers/LFGMgrProposal.cpp src/tests/LFGLogicTest.cpp
git commit -m "fix: preserve LFD queue provenance"
```

Expected green: LFG tests pass and the game target compiles.

## Task 3: Make matching and proposal transitions atomic

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGMgr.cpp`
- Modify: `src/game/WorldHandlers/LFGMgrProposal.cpp`
- Modify: `src/tests/LFGLogicTest.cpp`

**Step 1: Add failing matching/lifecycle decision cases**

Add cases for multi-role partial compatibility, exact five-player readiness,
deterministic leader selection, zero-second wait samples, and proposal expiry
key collection. Use source-shaped fixtures to assert that intersecting candidate
sets never selects the random category and that failed/surviving source
classification is owner-atomic.

Expected red: current matching uses cached needs, live unordered-set iterators,
and proposal data cannot reconstruct original queue sources.

**Step 2: Repair matching and merging**

- Iterate snapshots in `FindQueueMatches` and `FindSpecificQueueMatches`,
  rechecking owner/state before use and returning immediately after mutation.
- Make `RoleMapsAreCompatible` rerun `ResolveRoles` over combined masks and
  compare captured teams without live `Player*` dereferences.
- In `MergeGroups`, intersect actual candidates, union immutable `sourceUnits`,
  roles, and random maps, erase the consumed queue owner/data, and rewrite all
  member reverse mappings to the survivor.
- Make `UpdateWaitMap` accept zero, update the assigned role map, and always
  update the actual dungeon average.

**Step 3: Add one proposal entry/unwind path**

Implement `BeginProposal(owner)` to own the aggregate snapshot, resolve one
role per player and one leader bit, select an actual compatible dungeon, erase
queue state, publish proposal/source reverse indexes, and send the proposal.
Implement `UnwindProposal(id, failedPlayers)` to move/erase the proposal first,
clear reverse mappings once, discard a failed solo/premade source as a unit,
and revalidate/restore each agreed source once. Use it for decline, timeout, and
group-creation failure. Success clears the same reverse indexes without restore.

Add `RemoveOldProposals()` to `Update()` before matching. All proposal times are
seconds, and `SendDungeonProposal` validates a nonempty actual candidate set.

**Step 4: Verify and commit**

Run the Task 2 focused build/test commands, then:

```powershell
git diff --check
git add src/game/WorldHandlers/LFGMgr.h `
  src/game/WorldHandlers/LFGMgr.cpp `
  src/game/WorldHandlers/LFGMgrProposal.cpp src/tests/LFGLogicTest.cpp
git commit -m "fix: make LFD proposals atomic"
```

## Task 4: Match the 3.3.5a packet contract

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGHandler.cpp`
- Modify: `src/game/WorldHandlers/LFGMgrProposal.cpp`
- Modify: `src/game/WorldHandlers/Group.h`
- Modify: `src/game/WorldHandlers/Group.cpp`
- Modify: `src/tests/LFGLogicTest.cpp`

**Step 1: Add failing packet-value cases**

Test packed entry values, active/finished LFD state, assigned recipient/member
roles, and preservation of the existing non-LFD battleground header byte. Add a
counter helper fixture demonstrating one captured value for all recipients and
one increment per logical update.

Expected red: the current group packet writes the battleground byte and zero
LFD state/dungeon fields; several LFG packets serialize raw IDs.

**Step 2: Expose read-only group update data**

Add `LFGGroupUpdateData` and
`GetGroupUpdateData(groupGuid, playerGuid, data) const`. Return the assigned
role, active/finished byte, and `GetDungeonEntry(actualId)`; fail cleanly if
group/player state is absent.

**Step 3: Repair `SMSG_GROUP_LIST`**

Initialize `Group::m_updateCounter` in every constructor. In `SendUpdate`,
capture it before the recipient loop, use `GroupHeaderRole` so LFD recipients
get assigned roles while non-LFD/BG packets keep their existing byte, append
conditional LFD state/packed actual entry, write each other member's assigned
role, and increment once after all recipients.

**Step 4: Repair LFG packet values without changing layout**

- `SendLfgUpdate`: preserve the IDA-confirmed player/party field order and call
  `GetDungeonEntry` inside the session serializer for each status ID.
- role-check and queue status: display each recipient's packed request entry but
  keep wait maps keyed by the actual dungeon ID.
- proposal: serialize the selected actual packed entry, resolved roles, and
  `silent = 0` for a new proposal.
- join result: retain packed `partyForbidden` entries.
- rewards: write both dungeon values packed.

**Step 5: Verify and commit**

Build `mangos_tests` and `game`, run `mangos_tests.exe -only LFG`, run
`git diff --check`, then commit:

```powershell
git add src/game/WorldHandlers/LFGMgr.h `
  src/game/WorldHandlers/LFGHandler.cpp `
  src/game/WorldHandlers/LFGMgrProposal.cpp `
  src/game/WorldHandlers/Group.h src/game/WorldHandlers/Group.cpp `
  src/tests/LFGLogicTest.cpp
git commit -m "fix: send complete LFD client state"
```

## Task 5: Implement safe enter, leave, and re-entry

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGMgrProposal.cpp`
- Modify: `src/game/WorldHandlers/LFGHandler.cpp`
- Modify: `src/tests/LFGLogicTest.cpp`

**Step 1: Add failing teleport eligibility cases**

Cover actual Stockades map 34, random category/map 0, raid/BG/arena shapes,
normal/heroic mismatch, and matching difficulty. Expected red: current
teleport code trusts the stored row and implements only teleport-out.

**Step 2: Implement one validated teleport function**

Change the interface to `TeleportPlayer(Player*, bool out, bool automatic)`.
The proposal path passes `automatic=true`; `CMSG_LFG_TELEPORT` passes false.
Resolve the group status and actual row, require a real non-raid dungeon and
matching group difficulty for both directions, and return the existing LFG
error for invalid player/location state.

For entry, an already-on-map player is a no-op. Automatic formation prefers the
online leader, then the first online member already on the selected map, then
the map entrance. Opcode re-entry always uses the entrance. Save the return
point only when entering from a non-instance world location.

For exit, require the actual selected map and restore the saved entry point
without deleting LFD state, including after completion.

**Step 3: Verify and commit**

Run the focused tests and game build, then commit:

```powershell
git add src/game/WorldHandlers/LFGMgr.h `
  src/game/WorldHandlers/LFGMgrProposal.cpp `
  src/game/WorldHandlers/LFGHandler.cpp src/tests/LFGLogicTest.cpp
git commit -m "fix: implement safe LFD teleport lifecycle"
```

## Task 6: Make completion, leave, logout, leader, and boot cleanup deterministic

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGMgr.cpp`
- Modify: `src/game/WorldHandlers/LFGMgrQueue.cpp`
- Modify: `src/game/WorldHandlers/LFGMgrProposal.cpp`
- Modify: `src/game/WorldHandlers/Group.cpp`
- Modify: `src/game/Server/WorldSession.cpp`
- Modify: `src/game/Object/PlayerGroup.cpp`
- Modify: `src/tests/LFGLogicTest.cpp`

**Step 1: Add failing lifecycle decision cases**

Cover self-leave versus kick, completion idempotence, per-player random reward
selection, expired boot collection, boot remaining time, and leader-role
replacement. Expected red: the current completion erases group status, self
leave enters vote-kick, logout removes dungeon members, and boot records do not
expire reliably.

**Step 2: Preserve completion and reward provenance**

Make `HandleBossKilled` idempotent, fail any active boot first, transition group
and members to finished without erasing group status/dungeon, register the
daily run, and choose rewards from each player's random category (zero means a
specific queue).

**Step 3: Add idempotent group lifecycle hooks**

Implement `OnGroupMemberRemoved`, `OnGroupDisband`, and
`OnGroupLeaderChanged`. Clear every owned role check, queue unit, proposal,
reverse map, boot, status, group status, and group-set entry exactly once.
Update `leaderGuid` and the sole leader bit without sending; existing
`Group::SendUpdate()` remains the single packet send. Call member cleanup after
successful removal, call leader cleanup after auto-promotion and explicit
`_setLeader`, and call disband cleanup once before member slots are cleared.

**Step 4: Repair direct leave and logout**

Direct self-leave removes the player/group normally; only an attempt to remove
another member starts a boot. Add `OnPlayerLogout(Player*)` before registry and
generic non-raid removal. It cancels pre-dungeon owners/proposals, retains
in-dungeon/boot/finished LFD membership, and leaves a pending offline boot vote
for normal timeout.

**Step 5: Complete boot lifecycle**

Add `FinishBootVote(groupGuid, passed)` and `RemoveOldBoots()`. Success removes
the target once; failure/timeout restores every remaining member and group to
the prior dungeon state and erases the vote. Completion during boot fails the
vote first. `SendLfgBootUpdate` uses clamped seconds with no `/1000` conversion.
Call boot cleanup from `Update()` before matching.

**Step 6: Verify and commit**

Build `mangos_tests` and `game`, run focused LFG tests, then commit all listed
files with:

```powershell
git commit -m "fix: close LFD lifecycle cleanup gaps"
```

## Task 7: Add the temporary one-player smoke mode

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGMgr.cpp`
- Modify: `src/game/WorldHandlers/Chat.h`
- Modify: `src/game/WorldHandlers/Chat.cpp`
- Modify: `src/game/ChatCommands/DebugCommands.cpp`
- Modify: `src/tests/LFGLogicTest.cpp`

**Step 1: Add the failing readiness test**

Assert testing accepts exactly one solo queue unit with a resolved combat role,
while production one-player, premade one-player, zero-role, leader-only, and
multi-player incomplete units remain unready.

**Step 2: Add the isolated temporary switch**

Add `m_testing=false`, `IsTesting() const`, and `SetTesting(bool)`. Register
`.debug lfd` beside `.debug bg` at `SEC_ADMINISTRATOR`; the handler toggles the
manager flag and reports enabled/disabled. Mark the command, flag, accessors,
and readiness branch with `TEMPORARY LFD SMOKE TEST` comments for later removal.

At the beginning of `FindSpecificQueueMatches`, allow only a queued non-premade
owner with one player, nonempty actual candidates, answered roles, and a valid
resolved assignment to call the same `BeginProposal` as production. Erasing the
queue unit before proposal publication prevents repeated proposals.

**Step 3: Add Stockades preflight output**

When enabling the command, query `GetMapEntranceTrigger(34)` and compare map,
X/Y/Z, and orientation with row 101 `(34, 54.23, 0.28, -18.34, 6.26)` using
normal float tolerance. If it differs or is absent, report the data/selection
failure prominently; do not hard-code that point into teleport behavior.

**Step 4: Verify and commit**

Build `mangos_tests` and `game`, run focused LFG tests, then commit:

```powershell
git add src/game/WorldHandlers/LFGMgr.h `
  src/game/WorldHandlers/LFGMgr.cpp src/game/WorldHandlers/Chat.h `
  src/game/WorldHandlers/Chat.cpp src/game/ChatCommands/DebugCommands.cpp `
  src/tests/LFGLogicTest.cpp
git commit -m "feat: add temporary solo LFD smoke mode"
```

## Task 8: Phase-end verification and build handoff

**Files:**

- Modify only if a test exposes a correctness defect in the files already in
  scope.

**Step 1: Run repository checks and the full MSVC build**

```powershell
git diff --check
cmake --build E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  --config Release --parallel
ctest --test-dir E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  -C Release --output-on-failure
cmake --install E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  --config Release
```

Expected: `mangos_tests`, `game`, `mangosd`, `realmd`, tools, and scripts build;
all non-network tests pass; install output is isolated under `install-msvc`.

**Step 2: Run an independent compiler check where locally available**

This machine has `clang-cl` but no WSL, container runtime, GCC, or Unix Clang.
Configure `build-clangcl` with Ninja from the Visual Studio developer
environment, `clang-cl` for C and C++, `PCH=0`, and otherwise the same options;
build `mangos_tests` and `game`, then run the tests. Record Linux GCC/Clang CI
commands as not locally runnable rather than claiming they passed.

**Step 3: Review only the implementation diff**

Give the external reviewer the design goal, commits after
`4f6d7a18629939bc5526af757b2be0d15c5f6f29`, diff, exact test/build results,
and unresolved runtime risk. Forbid edits and broad repository review. Fix only
BLOCKING/IMPORTANT correctness, regression, security, or compatibility findings
with one targeted check and at most one focused re-review.

**Step 4: Verify handoff state**

```powershell
git status --short
git log --oneline 4f6d7a186..HEAD
```

Report:

- branch, worktree, final commits, build/install directories;
- exact tests/builds and results, including unavailable compiler gates;
- no DB migration and no service start/deploy;
- smoke instructions: log in as administrator, run `.debug lfd`, queue Random
  Classic with one valid combat role, accept the proposal, confirm Stockades is
  map 34, use Teleport Out and Teleport To Dungeon before/after completion, then
  run `.debug lfd` again to turn testing off;
- the temporary command must be removed after the user's smoke test.
