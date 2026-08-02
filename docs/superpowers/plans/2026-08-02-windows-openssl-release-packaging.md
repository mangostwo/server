# Windows OpenSSL Release Packaging Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the Windows release ZIP self-contained so direct `realmd.exe`, `mangosd.exe`, and Windows-service startup load the bundled OpenSSL 3 legacy provider without a global OpenSSL installation or user-set environment variable.

**Architecture:** AppVeyor deterministically packages matching OpenSSL runtime DLLs beside the daemons and only `legacy.dll` under `ossl-modules`. Before constructing the legacy provider on Windows, the core preserves a non-empty administrator override or configures OpenSSL's default library context to search the executable-relative module directory. Existing provider validation remains fail-closed.

**Tech Stack:** PowerShell 5.1, Windows batch-free ZIP layout, CMake 4.x/MSVC multi-config builds, C++17, Win32 path/environment APIs, OpenSSL 3 provider API, existing MaNGOS test harness.

## Global Constraints

- Do not push or merge to master; work only in `E:\Mangos\WIP\Two\LFDSystemsRepair\server` and the external files under `E:\Mangos\appveyor\Two`.
- Execute this plan inline in the existing isolated worktree; do not spawn subagents.
- Do not start or stop `realmd` or `mangosd`; the user will perform the release runtime smoke test.
- Preserve every non-empty administrator-provided `OPENSSL_MODULES` value.
- Do not modify the process environment in the core fallback.
- On non-Windows platforms, retain current provider discovery exactly.
- Package only `legacy.dll`; do not package `default.dll` or unrelated providers.
- No database or configuration-file changes.
- Use targeted checks during implementation; run the full build and CTest suite once at phase end.
- Keep SWE reviews limited to the OpenSSL/AppVeyor changes introduced by this plan.

---

### Task 1: Make AppVeyor packaging fail closed

**Files:**
- Modify: `E:\Mangos\appveyor\Two\install_script.txt`
- Modify: `E:\Mangos\appveyor\Two\build_script.txt`
- Test: temporary PowerShell contract command; no repository test file

**Interfaces:**
- Consumes: one selected x64 OpenSSL root passed to CMake as `OPENSSL_ROOT_DIR`.
- Produces: `server\libcrypto-3-x64.dll`, `server\libssl-3-x64.dll`, and `server\ossl-modules\legacy.dll`; throws before archive creation if any are unavailable.

- [ ] **Step 1: Run the failing packaging contract**

Run from `E:\Mangos\appveyor\Two`:

```powershell
$buildText = Get-Content -Raw .\build_script.txt
$installText = Get-Content -Raw .\install_script.txt
$failures = @()
if ($buildText -notmatch 'Join-Path \$DestinationDirectory "ossl-modules"') { $failures += 'standard ossl-modules destination missing' }
if ($buildText -match 'Write-Warning "No OpenSSL provider modules') { $failures += 'provider is still optional' }
if ($buildText -notmatch '\$serverStage\\ossl-modules\\legacy\.dll') { $failures += 'staged legacy provider path missing' }
if ($buildText -notmatch 'Assert-PathExists \$requiredReleasePath') { $failures += 'staged release assertions missing' }
if ($buildText -match 'default\.dll') { $failures += 'default provider is still packaged' }
if ($installText -notmatch 'legacy\.dll') { $failures += 'install dependency validation omits legacy provider' }
if ($failures.Count -ne 0) { throw ($failures -join '; ') }
```

Expected: FAIL with at least `standard ossl-modules destination missing`, `provider is still optional`, and `install dependency validation omits legacy provider`.

- [ ] **Step 2: Add deterministic OpenSSL artifact resolution**

In both scripts, add a PowerShell helper with this contract:

```powershell
function Resolve-OpenSslArtifact {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string[]]$RelativePaths,
        [Parameter(Mandatory = $true)][string]$Description
    )

    foreach ($relativePath in $RelativePaths) {
        $candidate = Join-Path $Root $relativePath
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Get-Item -LiteralPath $candidate).FullName
        }
    }

    throw "$Description was not found under $Root. Checked: $($RelativePaths -join ', ')"
}
```

Use ordered candidates:

```powershell
$headerPaths = @('include\openssl\ssl.h')
$opensslExePaths = @('bin\openssl.exe', 'openssl.exe')
$cryptoRuntimePaths = @('bin\libcrypto-3-x64.dll', 'libcrypto-3-x64.dll')
$sslRuntimePaths = @('bin\libssl-3-x64.dll', 'libssl-3-x64.dll')
$legacyProviderPaths = @(
    'bin\ossl-modules\legacy.dll',
    'bin\legacy.dll',
    'lib\ossl-modules\legacy.dll',
    'ossl-modules\legacy.dll'
)
$cryptoImportPaths = @(
    'lib\VC\x64\MD\libcrypto.lib',
    'lib\VC\libcrypto64MD.lib',
    'lib\libcrypto.lib'
)
$sslImportPaths = @(
    'lib\VC\x64\MD\libssl.lib',
    'lib\VC\libssl64MD.lib',
    'lib\libssl.lib'
)
```

In `install_script.txt`, resolve the header plus both import libraries, both
runtime DLLs, `legacy.dll`, and `openssl.exe` inside the existing-installation
branch. If any resolution throws, treat that root as unusable and perform the
existing clean fallback install; after fallback installation, resolve all seven
artifacts again without catching the exception so the install phase fails
immediately when incomplete.

In `build_script.txt`, iterate `$openSslRootCandidates` in order and call
`Resolve-OpenSslArtifact` for all seven artifacts inside a `try` block. Select a
root only after every call succeeds, retaining the resolved paths in an object
used by `Copy-OpenSslRuntime`. If no candidate passes, throw with every checked
root. Store them as a `[pscustomobject]` with `Header`, `OpenSslExe`,
`CryptoRuntime`, `SslRuntime`, `LegacyProvider`, `CryptoImport`, and `SslImport`
properties. This replaces the current header-plus-recursive-libcrypto root test and
recursive first-match selection for packaged files.

- [ ] **Step 3: Stage only the required files**

Change `Copy-OpenSslRuntime` in `build_script.txt` to accept
`[pscustomobject]$Artifacts` and `[string]$DestinationDirectory`. Copy
`$Artifacts.CryptoRuntime` and `$Artifacts.SslRuntime` to the destination, use
`Join-Path $DestinationDirectory "ossl-modules"` for the provider directory,
and copy only `$Artifacts.LegacyProvider` there. Delete the broad recursive
provider search, `default.dll` handling, and warning-only branch.

Before staging, prove the selected runtime supplies the default provider
internally: create a unique directory under `$env:TEMP`, copy only the resolved
`legacy.dll` into it, temporarily set the process `OPENSSL_MODULES` to that
directory, and run the resolved `openssl.exe list -providers -provider default
-provider legacy` through `Invoke-Native`. Restore the prior environment value
in `finally` and remove only that explicit temporary directory. A non-zero exit
must abort packaging; this prevents silently relying on an unshipped
`default.dll`.

After copying `$out` into `$serverStage`, add exact assertions before 7-Zip:

```powershell
foreach ($requiredReleasePath in @(
    "$serverStage\mangosd.exe",
    "$serverStage\realmd.exe",
    "$serverStage\libcrypto-3-x64.dll",
    "$serverStage\libssl-3-x64.dll",
    "$serverStage\ossl-modules\legacy.dll"
)) {
    Assert-PathExists $requiredReleasePath "Required release artifact"
}
```

- [ ] **Step 4: Verify PowerShell syntax and the green contract**

Run:

```powershell
foreach ($script in @('.\install_script.txt', '.\build_script.txt')) {
    $tokens = $null
    $errors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile(
        (Resolve-Path $script), [ref]$tokens, [ref]$errors)
    if ($errors.Count -ne 0) { throw ($errors | Out-String) }
}
```

Then rerun Step 1. Expected: both syntax checks PASS and the contract exits without throwing.

- [ ] **Step 5: Report AppVeyor handoff**

Report the two exact local paths and the required resulting ZIP layout to the user. These external files are not in a Git repository, so do not invent a commit or push step.

### Task 2: Stage a realistic failing provider test

**Files:**
- Modify: `src/tests/CMakeLists.txt:91-181`
- Modify: `src/tests/OpenSSLProviderTest.cpp:25-69`
- Test: `mangos_tests`

**Interfaces:**
- Consumes: `MANGOS_TEST_OPENSSL_LEGACY_DLL`, the selected OpenSSL runtime DLLs, and `$<TARGET_FILE_DIR:mangos_tests>`.
- Produces: a test layout matching the release ZIP while `OPENSSL_MODULES` is empty.

- [ ] **Step 1: Change CMake test staging before production code**

Find `libcrypto` with the existing names and find `libssl` using `NAMES
libssl-3-x64.dll libssl-3.dll libssl.dll`, both from `MANGOS_SSL_HINTS`. Copy
both beside `mangos_tests`. Find `legacy.dll` using `PATH_SUFFIXES ""
bin/ossl-modules ossl-modules lib/ossl-modules`, then add a post-build command
that creates `$<TARGET_FILE_DIR:mangos_tests>/ossl-modules` and copies it there
as `legacy.dll`. Replace the current test environment path with:

```cmake
set_tests_properties(mangos_tests PROPERTIES ENVIRONMENT "OPENSSL_MODULES=")
```

Keep warning behavior at configure time when a developer machine lacks the optional runtime artifacts; CI/AppVeyor packaging remains the release gate.

- [ ] **Step 2: Assert the test environment remains empty**

In `OpenSSLProviderTest.cpp`, include `<cstdlib>` and add to the provider initialization test around singleton construction:

```cpp
const char* modules = std::getenv("OPENSSL_MODULES");
std::string modulesBefore = modules ? modules : "";
REQUIRE(modulesBefore.empty());

OpenSSLProviderManager& manager =
    OpenSSLProviderManager::Instance();

const char* modulesAfter = std::getenv("OPENSSL_MODULES");
CHECK_STR(std::string(modulesAfter ? modulesAfter : ""), modulesBefore);
```

This proves both the CTest premise and that manager initialization does not
mutate the environment. Remove the test's existing duplicate manager
declaration when inserting the block.

- [ ] **Step 3: Build and verify RED**

Run:

```powershell
cmake --build E:\Mangos\WIP\Two\LFDSystemsRepair\build-msvc --target mangos_tests --config RelWithDebInfo --parallel
ctest --test-dir E:\Mangos\WIP\Two\LFDSystemsRepair\build-msvc -C RelWithDebInfo -R '^mangos_tests$' --output-on-failure
```

Expected: build succeeds, the staged `ossl-modules\legacy.dll` exists, and `mangos_tests` FAILS because `OSSL_PROVIDER_load("legacy")` still searches OpenSSL's compiled module directory.

### Task 3: Configure executable-relative provider discovery

**Files:**
- Modify: `src/shared/Auth/OpenSSLProvider.cpp:32-260`
- Test: `src/tests/OpenSSLProviderTest.cpp`

**Interfaces:**
- Consumes: `OPENSSL_MODULES`, the running executable path, `<executable-directory>\ossl-modules\legacy.dll`, and the default `OSSL_LIB_CTX`.
- Produces: an `OpenSSLProvider` for `legacy` after search-path preparation; no process-environment mutation.

- [ ] **Step 1: Add Windows-only path and environment helpers**

Under the existing OpenSSL 3 anonymous namespace, add Windows-only helpers that:

```cpp
#ifdef WIN32
std::wstring ReadWindowsEnvironment(const wchar_t* name);
std::filesystem::path GetExecutableDirectory();
bool ConvertWideToAnsi(const std::wstring& value, std::string& converted);
bool ConvertForOpenSSL(const std::filesystem::path& directory,
                       std::string& converted);
void ConfigureBundledProviderSearchPath();
#endif
OpenSSLProvider LoadLegacyProvider();
```

Implementation requirements:

- `ReadWindowsEnvironment` grows a `std::vector<wchar_t>` around `GetEnvironmentVariableW` and returns a copied value.
- `GetExecutableDirectory` grows a `std::vector<wchar_t>` around `GetModuleFileNameW`; it never uses the current directory. Filesystem path construction and the provider-file probe use `std::error_code` overloads where available and catch `std::filesystem::filesystem_error`, logging and returning instead of escaping manager construction.
- `ConvertWideToAnsi` produces an owned narrow string with `WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, ...)` and rejects `usedDefaultChar`; use it for both OpenSSL paths and Windows diagnostic text.
- `ConvertForOpenSSL` passes `directory.native()` (the Windows `std::wstring`) to `ConvertWideToAnsi`, then checks the resulting narrow byte length including appended `\legacy.dll` against `MAX_PATH`. On conversion failure or narrow-length overflow it retries with `GetShortPathNameW`, converts that wide short path, and checks the final narrow byte length again.
- `ConfigureBundledProviderSearchPath` returns immediately for a non-empty override and verifies `ossl-modules\legacy.dll` is a regular file. If `OSSL_PROVIDER_set_default_search_path(nullptr, converted.c_str()) != 1`, it logs the failure and returns without throwing; the following provider load then retains existing fail-closed behavior.
- `LoadLegacyProvider` calls configuration first and returns `OpenSSLProvider("legacy")`.
- Wrap Windows helper definitions and calls in `#ifdef WIN32`; the single cross-platform `LoadLegacyProvider` calls `ConfigureBundledProviderSearchPath()` only inside that guard and always returns `OpenSSLProvider("legacy")`.

Add `<filesystem>` and `<vector>` plus `#ifdef WIN32`-guarded `<windows.h>` to `OpenSSLProvider.cpp`.

- [ ] **Step 2: Guarantee initialization order**

Replace:

```cpp
: m_legacyProvider("legacy"), m_defaultProvider("default"), m_initialized(false)
```

with:

```cpp
: m_legacyProvider(LoadLegacyProvider()),
  m_defaultProvider("default"),
  m_initialized(false)
```

The factory configures the search path before constructing the first provider; the manager constructor body remains validation-only.

- [ ] **Step 3: Correct failure diagnostics**

Replace the hard-coded `C:\OpenSSL-Win64\bin` advice with a message that expects `ossl-modules\legacy.dll` beside the daemon or an explicit `OPENSSL_MODULES` directory containing `legacy.dll`. On Windows, convert the copied `ReadWindowsEnvironment(L"OPENSSL_MODULES")` value through `ConvertWideToAnsi` before passing its owned `std::string::c_str()` to `%s`; if conversion fails, log `<unrepresentable>`. Under `#else`, copy `std::getenv` immediately into `std::string`. Never pass a `wchar_t*` to `sLog`.

- [ ] **Step 4: Build and verify GREEN**

Run the Task 2 build and CTest commands again. Expected: `mangos_tests` PASS, both providers report major version 3, RC4 fetch succeeds, and the environment assertion remains true.

- [ ] **Step 5: Run focused compile coverage**

Build the two production targets:

```powershell
cmake --build E:\Mangos\WIP\Two\LFDSystemsRepair\build-msvc --target realmd mangosd --config RelWithDebInfo --parallel
```

Expected: both targets compile and link successfully.

- [ ] **Step 6: Commit the focused core change**

```powershell
git add src/shared/Auth/OpenSSLProvider.cpp src/tests/CMakeLists.txt src/tests/OpenSSLProviderTest.cpp
git diff --cached --check
git commit -m "fix: load bundled OpenSSL provider on Windows"
```

Do not include external AppVeyor files in the commit because they are outside the repository.

### Task 4: Review and phase-end verification

**Files:**
- Review: only the Task 1 external script diffs and Task 2-3 core commit
- Verify: existing build/test/install outputs

**Interfaces:**
- Consumes: targeted green checks and the exact scoped diffs.
- Produces: reviewed AppVeyor handoff plus rebuilt, installed, and deployed feature-branch artifacts; no master push.

- [ ] **Step 1: Obtain one focused SWE-1.7 code review**

Provide only the goal, constraints, AppVeyor before/after diffs, core commit diff, completed targeted tests, and known limitation that the actual AppVeyor job and user daemon smoke test are external. Require BLOCKING/IMPORTANT/MINOR findings and a verdict. Permit at most one re-review after concrete fixes.

- [ ] **Step 2: Address verified blocking or important findings**

Apply only correctness, regression, security, or compatibility fixes that reproduce against the changed files. Add a test only for a realistic failure path. Re-run the narrowest affected checks and amend with a new commit rather than rewriting unrelated LFD history.

- [ ] **Step 3: Run phase-end build and tests once**

Run:

```powershell
cmake --build E:\Mangos\WIP\Two\LFDSystemsRepair\build-msvc --config RelWithDebInfo --parallel
ctest --test-dir E:\Mangos\WIP\Two\LFDSystemsRepair\build-msvc -C RelWithDebInfo --output-on-failure
cmake --build E:\Mangos\WIP\Two\LFDSystemsRepair\build-clangcl --parallel
```

Expected: all three commands exit zero. Record the CTest count and binary
revision metadata.

- [ ] **Step 4: Refresh install and Testing deployment**

Run:

```powershell
$cacheLine = Get-Content E:\Mangos\WIP\Two\LFDSystemsRepair\build-msvc\CMakeCache.txt |
    Where-Object { $_ -match '^OPENSSL_ROOT_DIR:' } | Select-Object -First 1
$cacheMatch = [regex]::Match($cacheLine, '^[^=]+=(.+)$')
if (-not $cacheMatch.Success) { throw 'OPENSSL_ROOT_DIR is missing from the MSVC CMake cache' }
$localOpenSslRoot = $cacheMatch.Groups[1].Value -replace '/', '\'

function Resolve-LocalOpenSslArtifact([string[]]$RelativePaths) {
    foreach ($relativePath in $RelativePaths) {
        $candidate = Join-Path $localOpenSslRoot $relativePath
        if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
    }
    throw "OpenSSL artifact missing under ${localOpenSslRoot}: $($RelativePaths -join ', ')"
}

$localCrypto = Resolve-LocalOpenSslArtifact @('bin\libcrypto-3-x64.dll', 'libcrypto-3-x64.dll')
$localSsl = Resolve-LocalOpenSslArtifact @('bin\libssl-3-x64.dll', 'libssl-3-x64.dll')
$localLegacy = Resolve-LocalOpenSslArtifact @(
    'bin\ossl-modules\legacy.dll', 'bin\legacy.dll',
    'lib\ossl-modules\legacy.dll', 'ossl-modules\legacy.dll')

cmake --install E:\Mangos\WIP\Two\LFDSystemsRepair\build-msvc --config RelWithDebInfo
New-Item -ItemType Directory -Force E:\Mangos\WIP\Two\LFDSystemsRepair\install-msvc\ossl-modules | Out-Null
Copy-Item -LiteralPath $localCrypto -Destination E:\Mangos\WIP\Two\LFDSystemsRepair\install-msvc -Force
Copy-Item -LiteralPath $localSsl -Destination E:\Mangos\WIP\Two\LFDSystemsRepair\install-msvc -Force
Copy-Item -LiteralPath $localLegacy -Destination E:\Mangos\WIP\Two\LFDSystemsRepair\install-msvc\ossl-modules\legacy.dll -Force
Copy-Item -Path E:\Mangos\WIP\Two\LFDSystemsRepair\install-msvc\* -Destination E:\Mangos\WIP\Two\Testing\server_install -Recurse -Force
```

Then enumerate every installed file, derive its relative path, and require the
SHA256 at the corresponding Testing path to match. Verify the deployed
`mangosd.exe`, `realmd.exe`, runtime DLLs, and
`ossl-modules\legacy.dll` explicitly, and confirm the temporary solo-LFD command
is absent from source before reporting.

```powershell
$installRoot = (Resolve-Path E:\Mangos\WIP\Two\LFDSystemsRepair\install-msvc).Path.TrimEnd('\')
$deployRoot = (Resolve-Path E:\Mangos\WIP\Two\Testing\server_install).Path.TrimEnd('\')
$mismatches = @()
foreach ($sourceFile in Get-ChildItem -LiteralPath $installRoot -File -Recurse) {
    $relative = $sourceFile.FullName.Substring($installRoot.Length).TrimStart('\')
    $deployed = Join-Path $deployRoot $relative
    if (-not (Test-Path -LiteralPath $deployed -PathType Leaf)) {
        $mismatches += "missing: $relative"
        continue
    }
    if ((Get-FileHash -Algorithm SHA256 -LiteralPath $sourceFile.FullName).Hash -ne
        (Get-FileHash -Algorithm SHA256 -LiteralPath $deployed).Hash) {
        $mismatches += "hash: $relative"
    }
}
if ($mismatches.Count -ne 0) { throw ($mismatches -join '; ') }

foreach ($required in @(
    'mangosd.exe', 'realmd.exe', 'libcrypto-3-x64.dll',
    'libssl-3-x64.dll', 'ossl-modules\legacy.dll'
)) {
    if (-not (Test-Path -LiteralPath (Join-Path $deployRoot $required) -PathType Leaf)) {
        throw "Missing deployed release artifact: $required"
    }
}

$temporaryLfdMatches = rg -n 'HandleDebugLfd|debug lfd|solo LFD' src
if ($LASTEXITCODE -eq 0) { throw "Temporary LFD smoke code remains:`n$temporaryLfdMatches" }
if ($LASTEXITCODE -ne 1) { throw "rg failed with exit code $LASTEXITCODE" }
```

- [ ] **Step 5: Stop short of publishing**

Report the branch HEAD, modified external AppVeyor files, verification evidence, AppVeyor upload instructions, and remaining user smoke test. Do not push, merge, or start either daemon.

### Task 5: Audit and port the verified repair across MaNGOS Zero-Four

**Files:**
- Audit: `E:\Mangos\Repos\Zero\server`, `One\server`, `Three\server`, and the active Four server checkout
- Modify as proven: `E:\Mangos\appveyor\Zero`, `One`, `Three`, and `Four`
- Worktrees: one isolated `codex/openssl-release-repair` branch per affected core

**Interfaces:**
- Consumes: the reviewed Two implementation and each core's current live `origin/master`.
- Produces: equivalent self-contained Windows release behavior only where the same OpenSSL 3 provider contract exists; no pushes.

- [ ] **Step 1: Refresh and classify each core read-only**

Fetch `origin/master` for Zero, One, Three, and the active Four repository.
Compare `OpenSSLProvider.{h,cpp}`, provider tests/CMake, daemon startup, and
AppVeyor scripts against the verified Two implementation. Record whether each
is an exact port, a build-only packaging port, or a distinct design.

- [ ] **Step 2: Port exact matches in isolated worktrees**

Zero, One, and Three currently show the same eager
`OpenSSLProviderManager`/hard-coded `OPENSSL_MODULES` pattern. Create clean
worktrees from their refreshed `origin/master`, apply the smallest compatible
provider fallback and realistic test staging available in each repository, run
the narrowest existing provider/build check, and commit separately per core.
Do not add a new test framework where a core lacks the Two harness.

- [ ] **Step 3: Update matching AppVeyor scripts**

Apply the verified fail-closed artifact resolver, default-provider probe,
`ossl-modules` layout, and staged-file assertions to Zero, One, Three, and Four.
Run the PowerShell parser and the focused packaging contract separately in each
directory so per-core root/stage names remain intact.

- [ ] **Step 4: Treat Four as a separate compatibility gate**

Confirm which Four repository the AppVeyor project builds before editing core
source. The local `mangosfour/server` checkout uses legacy `EVP_rc4()` and has no
provider manager, while `NewMangosFour` has the eager provider-manager pattern;
do not assume they are interchangeable. If AppVeyor targets the former, stop at
the packaging update and write a focused Four-specific provider-loading design
before changing its authentication code.

- [ ] **Step 5: Review only cross-core ports and report**

Give SWE one bounded diff-only review of the mechanical cross-core ports after
their targeted checks. Report each branch/worktree HEAD, scripts changed,
checks run, and any Four-specific blocker. Do not push or merge any repository.
