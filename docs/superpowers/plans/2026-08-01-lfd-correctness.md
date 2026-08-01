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

**Tech Stack:** C++17, Mango Two game/server APIs, existing dependency-free test
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
- Follow red-green-refactor for every decision expressible through the approved
  dependency-free `LFGLogic` API. The existing test executable deliberately
  does not link `game.lib` or construct `Player`, `Group`, `WorldSession`, DBC,
  or object-manager state. Adapter/lifecycle behavior outside that seam receives
  a targeted `game` compile after each task and the explicit phase-end runtime
  smoke checklist; do not claim it is covered by `LFGLogicTest.cpp`.
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

Add `LFGLogicTest.cpp` cases for the declared pure API only. Prefix every
`TEST(...)` name with `LFG_` so `-only LFG` selects it. Cover:

- category filtering (Random Classic group 1 keeps real five-player normal or
  heroic rows and drops category/map-0/raid/BG/arena rows);
- first/last indexed selection and empty/out-of-range selection;
- first non-OK join result;
- empty, zero-role, leader-only, multi-role, valid 1/1/3, impossible, and
  deterministic role-assignment inputs;
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
cmake -S . -B E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  -G "Visual Studio 18 2026" -A x64
```

Expected red: configure fails because the newly listed `LFGLogic.cpp` does not
exist. This confirms the new test/build edge is active rather than silently
using the old Visual Studio project.

**Step 3: Implement the pure seam**

Implement exactly the standard-library-only API from the approved design.
`ResolveRoles` uses bounded deterministic backtracking over at most five
players, returning one combat role per player and remaining 1/1/3 capacity.
`FilterRandomCandidates` and `IsTeleportTarget` enforce real five-player maps;
the latter also enforces group difficulty. Time helpers use signed seconds and
clamp before conversion.

**Step 4: Verify and commit**

```powershell
cmake -S . -B E:/Mangos/WIP/Two/LFDSystemsRepair/build-msvc `
  -G "Visual Studio 18 2026" -A x64
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

**Step 1: Re-run the applicable pure contract**

Run the existing `LFG_FilterRandomCandidates` and selection/failure cases from
Task 1. They cover the pure boundary: DBC-shaped normal/heroic/category/map
inputs arrive only after the live adapter has rejected unknown entries and
inactive seasonal rows.

This step is a green characterization check, not a claimed adapter test. The
subsequent `game` build verifies the live adapter compiles; queue publication,
season filtering, and provenance are integration-smoke items.

**Step 2: Add the queue data model**

In `LFGMgr.h` add `playerDungeonMap`, `ownerProposalMap`, `LFGQueueSource`, and
`queueSourceMap`. Extend `LFGPlayers`, `LFGRoleCheck`, `LFGProposal`, and
`LFGGroupStatus` with the approved initialized fields and constructors. Add
`m_playerQueueOwners`, `m_ownerProposalIds`, proposal expiry, captured `TeamId`,
and random category maps. Before correcting `LFG_TIME_ROLECHECK` to 45 seconds,
run `rg -n "LFG_TIME_ROLECHECK" src` and verify every use compares or adds
`time_t` seconds; update any inconsistent use in the same task.

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
  src/game/WorldHandlers/LFGMgrProposal.cpp
git commit -m "fix: preserve LFD queue provenance"
```

Expected green: LFG tests pass and the game target compiles.

## Task 3: Make matching and proposal transitions atomic

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGMgr.cpp`
- Modify: `src/game/WorldHandlers/LFGMgrProposal.cpp`

**Step 1: Re-run matching primitives**

Run the Task 1 `ResolveRoles`, `IsProposalReady`, `SelectCandidate`, time, and
expired-owner tests. These cover only the pure matching inputs and key
collection. Candidate intersection, deterministic leader selection, immutable
source ownership, reverse indexes, and unwind-once behavior remain explicit
integration-smoke checklist items because they are not in the approved pure API.

Do not add source-shaped fixtures to `LFGLogicTest.cpp`; they would test a model
that production `LFGMgr` does not use.

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
  src/game/WorldHandlers/LFGMgrProposal.cpp
git commit -m "fix: make LFD proposals atomic"
```

## Task 4: Match the 3.3.5a packet contract

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGHandler.cpp`
- Modify: `src/game/WorldHandlers/LFGMgrProposal.cpp`
- Modify: `src/game/WorldHandlers/Group.h`
- Modify: `src/game/WorldHandlers/Group.cpp`

**Step 1: Re-run packet-value primitives**

Run the Task 1 group packet value and header-role cases. They cover
active/finished state, assigned roles, and preservation of the existing non-LFD
battleground header byte.

Do not claim the pure test covers `Group::SendUpdate` loop sequencing. Capturing
one counter before the recipient loop and incrementing once after it is a
compile-reviewed integration change and a packet/runtime smoke item.

**Step 2: Expose read-only group update data**

Add `LFGGroupUpdateData` and
`GetGroupUpdateData(groupGuid, playerGuid, data) const`. Return the assigned
role, active/finished byte, and `GetDungeonEntry(actualId)`; fail cleanly if
group/player state is absent. Make `GetDungeonEntry(uint32) const` so the query
does not require mutable manager access.

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
- proposal: serialize the selected actual packed entry and resolved roles; set
  `silent = !proposal.isNew &&
  proposal.groupRawGuid == recipientOriginalGroup` exactly as the client-facing
  design specifies (therefore every new proposal writes zero and shows).
- join result: retain packed `partyForbidden` entries.
- rewards: write both dungeon values packed.

**Step 5: Verify and commit**

Build `mangos_tests` and `game`, run `mangos_tests.exe -only LFG`, run
`git diff --check`, then commit:

```powershell
git add src/game/WorldHandlers/LFGMgr.h `
  src/game/WorldHandlers/LFGHandler.cpp `
  src/game/WorldHandlers/LFGMgrProposal.cpp `
  src/game/WorldHandlers/Group.h src/game/WorldHandlers/Group.cpp
git commit -m "fix: send complete LFD client state"
```

## Task 5: Implement safe enter, leave, and re-entry

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGMgrProposal.cpp`
- Modify: `src/game/WorldHandlers/LFGHandler.cpp`

**Step 1: Re-run teleport eligibility primitives**

Run the Task 1 `IsTeleportTarget` cases covering actual Stockades map 34,
random category/map 0, raid/BG/arena shapes, normal/heroic mismatch, and
matching difficulty. These validate the production predicate; live destination
choice, saved entry point, and both teleport directions are runtime-smoke items.

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
  src/game/WorldHandlers/LFGHandler.cpp
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

**Step 1: Re-run lifecycle primitives and enumerate smoke-only behavior**

Run the Task 1 self-leave/kick, expired-owner, and remaining-time tests.
Completion idempotence, per-player random reward selection, leader-role
replacement, group cleanup, and logout retention are live `LFGMgr` integration
behavior outside the pure API; keep them on the phase-end smoke checklist rather
than adding non-production fixtures.

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
successful removal. After `_removeMember` auto-promotes, call
`sLFGMgr.OnGroupLeaderChanged(GetObjectGuid(), m_memberSlots.front().guid)`;
after explicit `_setLeader`, pass that new leader GUID as well. Call disband
cleanup once before member slots are cleared.

**Step 4: Repair direct leave and logout**

Direct self-leave removes the player/group normally; only an attempt to remove
another member starts a boot. Add `bool OnPlayerLogout(Player*)` before registry
and generic non-raid removal. In `WorldSession::LogoutPlayer`, capture
`bool retainLfgGroup = sLFGMgr.OnPlayerLogout(_player)` and execute the existing
generic `_player->RemoveFromGroup()` block only when `!retainLfgGroup`. The hook
cancels pre-dungeon owners/proposals, returns true for in-dungeon/boot/finished
LFD membership, and leaves a pending offline boot vote for normal timeout.

**Step 5: Complete boot lifecycle**

Add `LFGState previousState` to `LFGBoot` and initialize it from the group status
when a vote starts. Add `FinishBootVote(groupGuid, passed)` and
`RemoveOldBoots()`. Success removes the target once; failure/timeout restores
every remaining member and group to `previousState` and erases the vote.
Completion during boot fails the vote first. `SendLfgBootUpdate` uses clamped
seconds with no `/1000` conversion. Call boot cleanup from `Update()` before
matching.

**Step 6: Verify and commit**

Build `mangos_tests` and `game`, run focused LFG tests, then commit all listed
files with:

```powershell
git add src/game/WorldHandlers/LFGMgr.h `
  src/game/WorldHandlers/LFGMgr.cpp `
  src/game/WorldHandlers/LFGMgrQueue.cpp `
  src/game/WorldHandlers/LFGMgrProposal.cpp `
  src/game/WorldHandlers/Group.cpp src/game/Server/WorldSession.cpp `
  src/game/Object/PlayerGroup.cpp
git commit -m "fix: close LFD lifecycle cleanup gaps"
```

## Task 7: Add the temporary one-player smoke mode

**Files:**

- Modify: `src/game/WorldHandlers/LFGMgr.h`
- Modify: `src/game/WorldHandlers/LFGMgr.cpp`
- Modify: `src/game/WorldHandlers/Chat.h`
- Modify: `src/game/WorldHandlers/Chat.cpp`
- Modify: `src/game/ChatCommands/DebugCommands.cpp`

**Step 1: Re-run the readiness test**

Run the Task 1 `IsProposalReady` cases. They assert testing accepts exactly one
resolved player while production one-player, zero-role, leader-only, and
incomplete multi-player inputs remain unready. The live branch additionally
checks `!isGroup`, so premade exclusion is an integration-smoke item.

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
failure prominently, leave `m_testing` disabled, and return; do not hard-code
that point into teleport behavior.

**Step 4: Verify and commit**

Build `mangos_tests` and `game`, run focused LFG tests, then commit:

```powershell
git add src/game/WorldHandlers/LFGMgr.h `
  src/game/WorldHandlers/LFGMgr.cpp src/game/WorldHandlers/Chat.h `
  src/game/WorldHandlers/Chat.cpp src/game/ChatCommands/DebugCommands.cpp
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

From a Developer PowerShell for Visual Studio 2026, run:

```powershell
cmake -S . -B E:/Mangos/WIP/Two/LFDSystemsRepair/build-clangcl `
  -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MAKE_PROGRAM="E:/Program Files/Microsoft Visual Studio/18/Enterprise/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe" `
  -DCMAKE_C_COMPILER="E:/Program Files/Microsoft Visual Studio/18/Enterprise/VC/Tools/Llvm/x64/bin/clang-cl.exe" `
  -DCMAKE_CXX_COMPILER="E:/Program Files/Microsoft Visual Studio/18/Enterprise/VC/Tools/Llvm/x64/bin/clang-cl.exe" `
  -DCMAKE_INSTALL_PREFIX=E:/Mangos/WIP/Two/LFDSystemsRepair/install-clangcl `
  -DOPENSSL_ROOT_DIR=C:/OpenSSL-Win64-401 `
  -DMySQL_INCLUDE_DIR="C:/Program Files/MariaDB 10.11/include/mysql" `
  -DMySQL_LIBRARY="C:/Program Files/MariaDB 10.11/lib/libmariadb.lib" `
  -DBUILD_TOOLS=1 -DBUILD_MANGOSD=1 -DBUILD_REALMD=1 `
  -DWITH_TESTS=1 -DWITH_NET_TESTS=0 -DSOAP=1 `
  -DSCRIPT_LIB_ELUNA=1 -DSCRIPT_LIB_SD3=1 -DUSE_STORMLIB=1 -DPCH=0
cmake --build E:/Mangos/WIP/Two/LFDSystemsRepair/build-clangcl `
  --target mangos_tests game --parallel
ctest --test-dir E:/Mangos/WIP/Two/LFDSystemsRepair/build-clangcl `
  --output-on-failure
```

**Step 3: Preserve the integration smoke checklist**

Do not start a server during this implementation session. Give the user this
explicit runtime checklist for the built artifacts:

- enabling `.debug lfd` reports whether `GetMapEntranceTrigger(34)` matches row
  101 including orientation;
- a solo Random Classic request with one valid combat role enters exactly one
  visible proposal and shows the original random category in queue UI;
- acceptance forms one LFD group, selects an actual dungeon, and Stockades
  selection enters instance map 34 rather than the outdoor Stormwind tower;
- group UI shows the resolved role and correct active/finished dungeon state;
- Teleport Out and Teleport To Dungeon work before completion, and both remain
  available after completion until group cleanup;
- completion rewards once, records the daily run, and a repeated completion
  callback does not reward twice;
- self-leave exits directly, while kicking someone else remains a boot vote;
- a later multi-player regression pass covers premade role-check abort/timeout,
  proposal decline/timeout source restoration, logout retention, boot timeout,
  leader auto-promotion, and disband cleanup; these are not represented as pure
  unit tests;
- running `.debug lfd` again disables the temporary bypass.

**Step 4: Review only the implementation diff**

Give the external reviewer the design goal, commits after
`4f6d7a18629939bc5526af757b2be0d15c5f6f29`, diff, exact test/build results,
and unresolved runtime risk. Forbid edits and broad repository review. Fix only
BLOCKING/IMPORTANT correctness, regression, security, or compatibility findings
with one targeted check and at most one focused re-review.

Use Devin SWE-1.7 free for this diff-only review, matching the user's requested
reviewer. Do not ask it to re-review the design or plan.

**Step 5: Verify handoff state**

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
