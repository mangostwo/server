# CLAUDE.md

Context for AI assistants — the Claude GitHub App (`@claude`) and contributors using Claude — working in
this repo.

## The rules

These four override anything else in this file, and anything in `doc/CodingStandard.md` that contradicts
them. They are listed in the order they matter.

### Rule zero: DECOUPLING

Not a preference. The first real boundary in this codebase is `src/proto/`, and it is enforced by the
linker: `game` links `proto`, `proto` links neither `game` nor the database. A socket cannot reach `sWorld`
because there is nothing to reach it *through* — the seam cannot silently close the first time someone finds
it inconvenient.

Hold that line, and cut new ones the same way:

- Two components talk through an interface, not through each other's headers. `IWorldGateway` (connection to
  world) and `IClientLink` (world to connection) are the shape to copy: symmetric, narrow, and neither side
  names the other's type.
- A dependency that only exists because a header happened to be included is not a dependency, it is an
  accident. Delete it.
- Game-agnostic code belongs in `shared`. If it names a spell, a creature or a player, it does not.
- If you cannot decouple something, say so and explain what blocks it. Do not route around it with a
  `#include`, a global, or a friend declaration.

The submodules are subject to the same rule from the other side: **they are never modified here.** Anything
they need in order to build against this fork lives in `src/shared/Compat/<name>/` and is attached from the
outside — see `cmake/SubmoduleCompat.cmake`. A local commit inside a submodule makes the parent reference an
object that exists on no remote, and a fresh clone cannot resolve it.

#### Aboard a transport, the world does not exist

For **crew, pets and minions on a global transport** (ship, zeppelin — a vehicle is a different mechanism
and is exempt), using world coordinates or any world/global mechanism is **absolutely forbidden**. They are
worked in **local (deck) coordinates only**.

- No local→world composition. Not for placement, not for login, not for a create block. A passenger's world
  position is never computed, because it is never true and never sent: the create block carries `(0,0,0)`
  plus the transport GUID and the deck offset, and the client does the transform itself.
- No world grid. A passenger is in no cell and is visited by no grid searcher — not by the visibility
  notifier, not by an AoE sweep. On-deck searches read the vessel's own crew container directly.
- No world distance test decides anything about a passenger.

**The crew are PERMANENTLY visible to the players on the vessel, for as long as those players are aboard.**
Not conditionally, not by range, not by a visibility pass — being aboard is the whole test. Observers
*ashore* see the crew too, but through the vessel: the ship announces and retains them, and only the ship
has an (approximate) world position at all, used solely to find those observers.

Only the vessel is allowed a world position, and only because someone has to find it.

### Rule one: the code aligns to C++, not C++ to the code

This is C++17, strict. When old MaNGOS code and the language disagree, **the code changes.** Do not
reintroduce a 1990s idiom to spare a rewrite, and do not "adapt" a modern construct until it fits the old
shape.

- `std::thread`, `std::mutex`, `std::shared_mutex`, `std::atomic`, `thread_local`, RAII. ACE is gone and
  stays gone; so is `Common.h`.
- Meyers singletons (`MaNGOS::Singleton<T>`, with the template befriended so private destructors work).
- Prefer the standard library call that makes a mistake unwritable over the one that merely avoids it today.
  `BN_bn2binpad` instead of `BN_bn2bin` plus manual padding is the canonical example: one of them cannot be
  got wrong.
- Undefined behaviour is a defect, not a style question. Taking the address of the first element of an empty
  vector is a bug even where it currently works.

### Rule two: CamelCase, never snake_case

`FindMapLocked`, not `find_map_locked`. Types and functions in CamelCase, members with the `m_` prefix this
tree already uses. Match the surrounding code for everything the rules do not cover.

### Rule three: unit tests

Tests live in `src/tests/`, behind `-DWITH_TESTS=ON` (off by default), and use the dependency-free harness in
`TestHarness.h` — no framework is vendored.

Test what is invisible in ordinary use: pure functions over bytes, framing and reassembly under
fragmentation, anything whose failure mode is rare enough to look like bad luck. The padding bug that shows
up in one login out of 256 is the reason this rule exists.

**A test you have never seen fail is not evidence.** Revert the fix and watch it go red before you trust it.
Cover the bad path explicitly: malformed input, hostile lengths, truncated streams, boundary sizes. For
anything on a socket, rejection must be the worst outcome — never a crash, a hang, or an allocation the peer
controls.

## Traps that have already cost time

Every item here is something that actually happened in this tree, not a general caution. They share a shape:
**the build stays green and the mistake survives.**

**A build that reports success may not have compiled your change.** `ninja: no work to do` is not
confirmation — it means the tool believes the object is current, which is exactly what a bad sync or a
preserved mtime produces. Before trusting a one-file verification, confirm the source on the machine
contains your edit and that the object was actually rebuilt (`touch` it and watch it compile).

**When a test fails, first ask how often.** A test that fails *every* iteration is almost always testing the
wrong thing — a wrong endianness, a wrong model of the API. A test that fails rarely, near 1 in 256, is the
code. Both happened here on the same afternoon: four total failures were my tests, one 1-in-256 failure was a
real padding bug. Do not debug the code until the failure rate says it is the code.

**A quoted `#include` searches the including file's own directory first.** A compat header named
`ElunaCompat.h` was placed on the include path; Eluna ships a file of that name, so its own header won every
time and the shim was never included at all — with no error anywhere. If a header you added seems to have no
effect, check the preprocessor output (`-E`) for a sentinel from it rather than assuming the include path is
right. Give compat headers a path-qualified include, or a name the submodule cannot own.

**Two files with the same include guard cancel each other silently.** SD3's `pch.h` and
`include/precompiled.h` both used `SC_PRECOMPILED_H`; whichever the compiler saw first suppressed the other.
Because the two lists had drifted, `-DPCH=1` and `-DPCH=0` compiled the scripts against *different sets of
headers*. Any build-time flag that changes which declarations a source sees is a defect, not a
configuration.

**When you delete a class, audit what it called, not only what called it.** Removing `WorldSocketMgr` also
removed the only call to `InitializeOpcodes()`. The function still compiled, still linked, and was invoked by
nobody; `opcodeTable` would have stayed zero-initialised and the server would have started cleanly and died
on the first packet. Nothing in the toolchain catches an orphaned initialiser.

**The three toolchains disagree, so all three are the gate.** Not redundancy — each caught something the
others did not this session. FreeBSD killed a process with `SIGPIPE` where Linux survived; MSVC and clang
differ on typo-correction; a most-vexing-parse (`std::vector<uint8> v(size_t(n));` declares a function)
produced three unrecognisably different errors. A change is verified when MSVC, GCC and Clang have all
compiled it.

**Some files here are CRLF and some are Windows-1252.** Scripted search-and-replace that assumes `\n`, or
that re-encodes on write, will either silently match nothing or corrupt bytes. Check `git diff` after any
bulk edit.

## Project

**MangosTwo** — The Wrath of the Lich King World of Warcraft **3.3.5a** server (C++, MySQL/MariaDB).
Compatibility target is **3.3.5a only**; do **not** introduce 4.x/Cata or later-expansion assumptions.

- **Database changes go in the separate `mangostwo/database` repo**, not here — as transactional, idempotent
  `Rel##_##_###_*.sql` migrations that chain via `db_version`.
- Clone/update **recursively**: `dep`, `src/realmd`, `src/modules/{SD3,Eluna}` and `win` are submodules. Never shallow-update a submodule to a non-tip pinned SHA.
- Client data is baked by `src/tools/extractor` (`mangos-extractor`) into `<DataDir>/tiles`,
  `<DataDir>/gomodels` and `<DataDir>/dbc`. It always builds; there is no vmap or .map step.
- Less-obvious locations: scripting in `src/modules/` (Eluna = Lua, SD3 = C++). The `src/game/` tree is under
  an ongoing **decomp cohesion-split** (large classes like `Player`/`ObjectMgr` are being broken into
  topical `*.cpp` files, e.g. `PlayerDuel.cpp`, `ObjectMgrCreatures.cpp`); locate code by symbol/string, not a
  fixed file, because methods move between files.

## Build & test

**C++17** — strict (`-std=c++17`, GNU extensions off); C code is C11. CMake ≥ 3.18; GCC/Clang
(Linux/macOS/BSD) or MSVC ≥ 2015 (Windows). **OpenSSL 3.x is mandatory.**

```sh
git clone --recursive https://github.com/mangostwo/server.git && cd server
sudo apt-get install -y git cmake make build-essential \
  libssl-dev libbz2-dev default-libmysqlclient-dev libreadline-dev   # Debian/Ubuntu deps
cmake -S . -B ../build-two -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../install-two \
  -DBUILD_TOOLS=1 -DBUILD_MANGOSD=1 -DBUILD_REALMD=1 -DSOAP=1 \
  -DSCRIPT_LIB_ELUNA=1 -DSCRIPT_LIB_SD3=1 \
  -DUSE_STORMLIB=1 -DPCH=1 -DWITH_TESTS=0
cmake --build ../build-two -j"$(nproc)" && cmake --install ../build-two
```

**Build out-of-source, outside the working tree.** Not hygiene — a test. Sources here still carry relative
includes like `#include "../../dep/foo.h"`; an in-source build tree can make those resolve by accident, so the
build passes and the broken include survives. An include that only resolves in-source is a bug to fix, not a
reason to move the build back in.

Optional: `-DWITH_IO_URING=1` selects the io_uring network backend instead of epoll (Linux only).
`-DWITH_TESTS=1` builds `mangos_tests`.

Windows: use the EasyBuild helper in `win/`. **A PR MUST keep CI green:** the Linux build compiles with
**both** GCC and Clang, and Windows builds on AppVeyor.

## Code style

- **4-space indent, never tabs**; ~80-column lines.
- **Allman braces**, and **brace single-statement blocks** — even one-line `if`/`for`/`while`. Do not de-brace
  existing ones. (Exception: do not brace `switch`/`case` bodies.)
- **One space before `(`, none inside**: `if (x)`, not `if( x )`.
- Doxygen: `///` above a member, `///<` trailing, `/** ... */` multi-line.
- Many `.cpp`/`.h` here are **Windows-1252** encoded; preserve byte-for-byte. Tools that re-encode to UTF-8
  corrupt non-ASCII bytes — use byte-preserving edits and check `git diff` after.

## Logging

Console output is rendered on a dedicated off-thread writer (`src/shared/Log/ConsoleLogWriter`) so the
world/map-update threads never block on console I/O. Two rules follow:

- **Never write to stdout directly** (`printf`/`fprintf`, progress bars, ad-hoc notices) for console output —
  route it through `Log::ConsoleEmitRaw` so stdout has a single owner and lines can't tear against, or
  overtake, the writer's output.
- **Gate high-volume runtime debug** with `DEBUG_FILTER_LOG(LOG_FILTER_*, …)` (or `DETAIL_`/`BASIC_`),
  reusing an existing `LogFilters` bit where one fits (e.g. `LOG_FILTER_GRID_ADD`, `LOG_FILTER_DB_SCRIPTS`,
  `LOG_FILTER_MAP_LOADING`). All filters ship **default-on (suppressed)**; set a `LogFilter_*` key to `0` to
  see a category. **Never filter `outError`/`outErrorDb`** — errors must always show.

Recommended runtime mode: `LogLevel=1` (quiet console) + `LogFileLevel=3` (buffered full file). Packet
logging is opt-in via `PacketLoggingEnabled` (off by default).

## Threading

Any thread that touches the database must call `mysql_thread_init` on entry and `mysql_thread_end` on exit —
use the `DbThreadGuard` RAII wrapper, not bare calls. Map-update threads run delayed SQL and are the ones
that get this wrong; a thread that skips it corrupts MySQL's per-thread state rather than failing cleanly.

## Review focus (for `@claude`)

Prioritise: **(1)** the four rules above, decoupling first; **(2)** correctness/safety in `src/game/` handlers
and anything touching live world/DB state; **(3)** build/CI impact (GCC *and* Clang, Windows/AppVeyor);
**(4)** DB-migration correctness (use the `mangostwo/database` pattern).

Keep feedback concrete and minimal-diff. Flag correctness and rule violations, not style preferences the
rules do not cover. If something is a real defect, say so plainly and show the failing case — a review that
hedges on a bug is worse than no review.

## What is built here (transport, movement, the offline baker) — and how to port it

A map of the non-obvious subsystems this fork has reworked, and the invariants a sibling core
(CMaNGOS, or the getMangos line — mangoszero/one/three) must satisfy to carry the same work across. These
forks are **divergent and byte-similar in places but not identical**: name the target and re-check, never
assume a change ported because the file looked the same.

### Passenger transport (global vessels: ships, zeppelins)

The rule is Rule zero's subsection above — *aboard a transport the world does not exist*. What implements it:

- **`Transports.{h,cpp}`** — the game object, and it is a CLOCK and a RELAY and nothing else: the route
  (`GenerateWaypoints`, `m_timer`, `TeleportTransport` — whose only decision is *when* the ship changes
  world map), `PinRouteGrids` (the world grids the route crosses, pinned at start-up), `GetPathProgress`
  (the one number the client needs; the create block carries position `(0,0,0)` and always will), and
  `AsMap()`. It knows nothing about passengers. There is no pose recovery, no transform, no composition.
- **`TransportMap.{h,cpp}`** — the vessel AS A MAP, and where her whole life is kept: crew (`EnlistCrew`),
  passengers (`Embark`/`Disembark`), minions (`UpdateMinions`), the hull as terrain (`SurfaceAt`,
  `IsBlocked`, `FreeSpotNear`, `HullRadius`), the announcement channel (`AnnounceVessel`/`RetractVessel`,
  `CollectRelaySources`, `GatherObservers`) and both sides of the seam (`VesselLeavingWorld`/
  `VesselEnteredWorld`). `Map::AsTransport()` is the one way to ask; `Transport::VesselOf` derives being
  aboard from the map alone and never stores it.
- **`Vehicle.{h,cpp}`** — `VehicleInfo`/`TransportInfo`, the vehicle's own and shared with nothing. A
  vehicle is the ONLY thing here that composes a seat offset into a world position, legitimately, because
  the server owns its pose. The old `TransportBase` that a ship and a tank both inherited is gone.
- **`ObjectUpdate.cpp`** — an `ONTRANSPORT` unit serialises world position `(0,0,0)`; a MO_TRANSPORT's
  `UPDATEFLAG_TRANSPORT` sends `GetPathProgress()`, **not** a wall clock.
- **`Map.cpp`** — `SendInitTransports` is map-scoped through `SendCreateTo`; `SendInitSelf` sends the
  boarder's own vessel crew.
- **`ObjectMgr` static-GUID pool** — `GenerateStaticCreatureLowGuid`/`FreeStaticCreatureLowGuid` with a free
  list, so carrying crew (not respawning) no longer leaks the bounded static range.
- **`Unit::RemoveFromWorld`** unboards a boarded minion before the base class forgets it.

**The load-bearing invariant for any core.** Visibility is *elimination-by-leftovers*: `VisibleNotifier`
copies `m_clientGUIDs`, erases whatever the cell visit re-finds, and destroys the remainder as out-of-range.
So the vessel and its crew **must never be inserted into `m_clientGUIDs`** — put them there and the sweep
destroys them every tick. Visibility is instead a **map-membership channel**, edge-triggered on four events
(player/vessel enters/leaves the map), never a distance poll. If the target core's notifier differs, find
its equivalent of this sweep first; that is where a port breaks.

**MO_TRANSPORT vs. lift.** A ship is `GAMEOBJECT_TYPE_MO_TRANSPORT` (type 15, `transports` table), **never
`AddToWorld`'d** (`IsInWorld == false` by design), only `SetMap`; the server *estimates* its pose from
waypoints, so the crew's world position is a lie. A lift/tram/elevator is `GAMEOBJECT_TYPE_TRANSPORT` (type
11, `gameobject` table): an ordinary grid GameObject, client-animated along a fixed path, **server-static at
a fixed anchor** — so there local↔world is **exact, not a lie**, and passengers stay ordinary grid creatures
seen through the normal grid. Pets ride a type-11 lift beside their master via an event-driven heartbeat off
the master's movement packet (`Player::UpdateLiftMinions`); the local-mesh upgrade is the open TODO.

### The spatial component: an object HAS a placement

`Geometry::Placement` (`src/shared/Geometry/Placement.h`) holds a **frame**, a pose in it, and an extent.
Nothing in the object hierarchy owns geometry any more: `WorldObject` exposes `Where()` (read) and `Place()`
(mutate) and has no `GetPositionX`, no `GetDistance`, no `HasInArc`. Ask the component:
`obj->Where().DistanceTo(other->Where())`.

- **`Geometry::Frame`** is a map instance — a vessel is one like any other — **or** a vehicle
  seat (`Frame::Deck(vehicleGuid)`, the last thing still spelled that way), and
  **cross-frame answers fail closed** — infinite distance, never in reach, never in arc. Forgetting the map
  check is unwritable, which is why `IsWithinDist`/`IsWithinDistInMap` collapsed into one call.
- **Terrain-agnostic by construction.** Ground height, water level, line of sight and the free-spot sweep are
  not the component's: they are free functions (`DropToGround`, `ClampToAllowedZ`, `HasLineOfSight`,
  `FindFreeSpotNear`, `PointNear`, `RandomGroundPointNear`) that compose a placement with the engine that
  owns the answer. Phase and world membership likewise: `SharesWorld`, `InReach`, `InFrontPhased`.
- **The extent lives in the component.** `ComputeBoundingRadius()` is the per-class formula, pushed in by
  `RefreshBoundingRadius()` when a model or scale changes — rarely — and read back as `Where().Extent()`.
- **Transforms are `Geometry::Transform`.** `Placement::Basis()` is the bridge; a vehicle seat composes
  through it legitimately (the server owns a vehicle's pose), a global transport's hull never does.
  `TransportInfo` holds a VEHICLE rider's pose; a ship's passengers have no such record, because they
  are simply on her map.
- Anchors are placements too: `Creature::Spawn()` (respawn pose) and `CombatAnchor()` (leash point).
  `MovementInfo::Reported()` is the CLIENT's claimed pose, kept because the anticheat and fall damage want
  it — not a mirror of the placement.
- The seam is enforced: `ctest -R spatial_boundary` (`src/tests/CheckSpatialBoundary.cmake`) fails the build
  if any of those names reappears as a member of `Object` or a descendant. `src/tests/PlacementTest.cpp`
  pins the behaviour on **real world-database rows** (Stormwind spawns by guid, a named zeppelin by entry),
  not on invented coordinates.

### Movement frame

`Motion::IMotionFrame` with `FrameFor(mover)` → `WorldFrame` or `TransportFrame` (chosen by
`Transport::VesselOf`). Chase/Follow/random/AoE resolve in the mover's frame, so on-deck movement is done in
deck coordinates without the caller knowing. Porting the transport visibility without this frame will
reintroduce world composition on the deck.

### Client-side interpolation and DBC coupling (cannot be fixed server-side)

- A vessel's position is **interpolated by the client** from `time % period` against `TransportAnimation.dbc`
  / `TransportRotation.dbc`. The **server never loads those DBCs**; it only sends TIME + GUID + world
  `(0,0,0)`. Every map's clock must yield the same `time % period` — verify the target core's map clock is
  monotonic and shared, or vessels desync across a seam.
- The **tram 90° rotation** is a data/DBC coupling: the GO base rotation drives the animation direction and
  is bound to `TransportRotation.dbc`. It is **not** server-fixable — aligning the DB quaternion made the
  cars slide through walls. Don't chase it in core code.
- Field name: this fork spells the quaternion field **`GAMEOBJECT_ROTATION`** (`OBJECT_END + 0x04`, 4 floats
  x,y,z,w). The 3.3.5 emulator community usually calls it `GAMEOBJECT_PARENTROTATION` — a convention, **not**
  a Blizzard-official name. A sibling core may use either spelling for the same offset.

### Offline baker (extractor) and navmesh

- One pass over the client MPQs bakes `tiles`, `gomodels`, `dbc`; the navmesh is baked **from the tiles,
  never the MPQs**, so the pathfinder walks exactly the surface the collision engine answers with.
- **WMO-only maps** (a WDT with no ADT grid — Deeprun Tram, map 369) emit a single `w_<map>.tile`. The nav
  bake (`NavMeshBuilder::BakeAll`) enumerates both `t_<map>_<x>_<y>.tile` grids **and** those `w_` tiles;
  `GlobalWmoGrids` derives the grid cells the WMO's world footprint spans, and each bakes from the one shared
  tile (no terrain/liquid/neighbours). A core that only scans `t_` tiles gives those maps no navmesh.
- Console feedback is two narrow callbacks the game-agnostic builder exposes: `ProgressFn` (live per-tile
  header, terminal only) and `MapDoneFn` (one durable line per map, survives a pipe). Keep them decoupled —
  the builder must not learn about the console.

### Known cross-core defect

`BigNumber::AsByteArray` pads at the **wrong end** on the whole getMangos line (mangoszero/one/three and
this tree's ancestor), so ~1 SRP login in 256 fails at random. Prefer `BN_bn2binpad`. If you port auth or
crypto here from a sibling core, assume it carries the bug until you have read the bytes.
