# CLAUDE.md

Context for AI assistants — the Claude GitHub App (`@claude`) and contributors using Claude — working in
this repo. Humans: also read [`doc/CodingStandard.md`](doc/CodingStandard.md).

## Project

**MangosTwo** — The Wrath of the Lich King World of Warcraft **3.3.5a** server (C++, MySQL/MariaDB).
Compatibility target is **3.3.5a only**; do **not** introduce 4.x/Cata or later-expansion assumptions.

- **Database changes go in the separate `mangostwo/database` repo**, not here — as transactional, idempotent
  `Rel##_##_###_*.sql` migrations that chain via `db_version`.
- Clone/update **recursively**: `dep`, `src/realmd`, `src/modules/{SD3,Eluna}`, `src/tools/Extractor_projects`
  and `win` are submodules. Never shallow-update a submodule to a non-tip pinned SHA.
- Less-obvious locations: scripting in `src/modules/` (Eluna = Lua, SD3 = C++). The `src/game/` tree is under
  an ongoing **decomp cohesion-split** (large classes like `Player`/`ObjectMgr` are being broken into
  topical `*.cpp` files, e.g. `PlayerDuel.cpp`, `ObjectMgrCreatures.cpp`); locate code by symbol/string, not a
  fixed file, because methods move between files.

## Build & test

**C++17** — strict (`-std=c++17`, GNU extensions off); C code is C11. CMake ≥ 3.18; GCC/Clang
(Linux/macOS/BSD) or MSVC ≥ 2015 (Windows). The exact flags CI builds with:

```sh
git clone --recursive https://github.com/mangostwo/server.git && cd server
sudo apt-get install -y git cmake make build-essential \
  libssl-dev libbz2-dev default-libmysqlclient-dev libace-dev libreadline-dev   # Debian/Ubuntu deps
mkdir -p _build _install && cd _build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../_install \
  -DBUILD_TOOLS=1 -DBUILD_MANGOSD=1 -DBUILD_REALMD=1 -DSOAP=1 \
  -DSCRIPT_LIB_ELUNA=1 -DSCRIPT_LIB_SD3=1 -DPLAYERBOTS=0 \
  -DUSE_STORMLIB=1 -DPCH=0
make -j"$(nproc)" && make install -j"$(nproc)"
```

Windows: use the EasyBuild helper in `win/`. **A PR MUST keep CI green:** the Linux build compiles with
**both** GCC and Clang, Windows builds on AppVeyor, and Codacy/CodeFactor gate quality.

> **Playerbots stay OFF on Two.** The playerbots module is broken on this fork — build with `-DPLAYERBOTS=0`
> and do **not** enable it in any build config or CI job.

## Code style

Source of truth: [`doc/CodingStandard.md`](doc/CodingStandard.md). Non-default rules:

- **4-space indent, never tabs**; ~80-column lines.
- **Allman braces**, and **YOU MUST brace single-statement blocks** — even one-line `if`/`for`/`while`. Do
  not de-brace existing ones. (Exception: do not brace `switch`/`case` bodies.)
- **One space before `(`, none inside**: `if (x)`, not `if( x )`.
- Doxygen: `///` above a member, `///<` trailing, `/** ... */` multi-line.
- These `.cpp`/`.h` are **Windows-1252** encoded; preserve byte-for-byte. Tools that re-encode to UTF-8
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

## Review focus (for `@claude`)

Prioritise: **(1)** correctness/safety in `src/game/` handlers and anything touching live world/DB state;
**(2)** coding-standard conformance above (including the Windows-1252 byte-preservation rule); **(3)** build/CI
impact (GCC *and* Clang, Windows/AppVeyor); **(4)** DB-migration correctness (use the `mangostwo/database`
pattern). Keep feedback concrete and minimal-diff; flag correctness/standard issues, not style preferences
the standard doesn't cover.
