---
name: build
description: How to build and install mangos_two. Use for ANY build, install, or deploy of this server — on ficom, licom, or Windows. Covers which box, which generator, which flags, and the cleanup that must follow. Invoke before compiling anything.
---

# Build

The user runs and tests the server on **ficom** (FreeBSD/clang) and **licom** (Linux/GCC).
Windows/MSVC is not a test target. A build that was not installed on the box the user runs
proves nothing to them.

## 1. Sources arrive → clean build, always

When sources are brought to ficom or licom, **delete the previous build directory and
configure fresh**. Never reuse it.

```sh
rm -rf /mangos/build-two && cmake -S /mangos/mangos_two -B /mangos/build-two -G Ninja …
```

The reason is not tidiness. `tar` preserves mtimes, and a tarball made on Windows carries
Windows timestamps: extracted onto a box whose objects were built later in wall-clock terms,
ninja decides the sources are older than the objects and **silently skips them**. The build
reports success, the install reports success, and the binary does not contain the change.
That has already happened here and cost hours. A fresh directory makes it unwritable.

If a build tree is ever reused anyway, `find src cmake CMakeLists.txt -type f -exec touch {} +`
first — and still verify the binary, e.g. `strings -a <binary> | grep <a new literal>`.

## 2. Iteration build — ficom / licom

```sh
cmake -S /mangos/mangos_two -B /mangos/build-two -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/mangos/server2 \
  -DPCH=1 -DSCRIPT_LIB_SD3=0 -DSCRIPT_LIB_ELUNA=0 -DWITH_TESTS=0
ninja -j4 > /tmp/build.log 2>&1 && ninja install >> /tmp/build.log 2>&1
```

Ninja, `PCH=1`, scripts off, no tests. Check ninja's **own** exit code — chaining
`ninja ; ninja install ; echo $?` reports the installer's status and hides a failed build.

## 3. Push build — only when the user asks for it

Only when the user says the build is **for push/commit**: full build, **`PCH=0`**, on all
three toolchains — ficom, licom, and MSVC. That is the only time Windows is compiled.

## 4. Never build or test on Windows otherwise

Not as a "quick syntax check", not to save a round trip. It is delay, and it verifies a
binary nobody runs. Rule 3 is the sole exception.

## 5. Build logs live in /tmp. Nowhere else.

Every build/configure/install log goes to `/tmp` — **never** into `/mangos`, never into the
source tree, never anywhere else on the box.

## 6. The build tree may stay — but prove it recompiled

`/mangos/build-two` may be kept between iterations; deleting it every time is not required.
Tarballs, core files and anything else the build spat out still go.

**The cost of keeping it is rule 1's hazard.** `tar` preserves Windows mtimes, so extracted
sources can look older than the objects built from them and ninja skips them in silence.
Whenever the tree is reused, force the timestamps forward and check the work was real:

```sh
find /mangos/mangos_two/src /mangos/mangos_two/cmake -type f -exec touch {} +
ninja -j4 > /tmp/build.log 2>&1 && grep -c '^\[' /tmp/build.log
```

`ninja: no work to do` after an edit is a FAILED verification, not a fast one. Confirm the
binary carries the change (`strings -a <binary> | grep <a new literal>`) before believing an
install that compiled nothing.
