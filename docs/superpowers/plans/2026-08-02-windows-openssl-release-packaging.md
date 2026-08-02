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
if ($buildText -notmatch 'Assert-PathExists "\$serverStage\\ossl-modules\\legacy\.dll"') { $failures += 'staged legacy provider assertion missing' }
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

An OpenSSL root is usable only when its header and all five artifact categories resolve. This replaces recursive first-match selection for packaged files.

- [ ] **Step 3: Stage only the required files**

Change `Copy-OpenSslRuntime` in `build_script.txt` to copy the resolved crypto and SSL runtime DLLs to `$out`, create `$out\ossl-modules`, and copy only the resolved `legacy.dll` there. Delete the broad recursive provider search, `default.dll` handling, and warning-only branch.

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

Find both `libcrypto-3-x64.dll` and `libssl-3-x64.dll` from the existing `MANGOS_SSL_HINTS`. Copy both beside `mangos_tests`. Find `legacy.dll`, then add a post-build command that creates `$<TARGET_FILE_DIR:mangos_tests>/ossl-modules` and copies it there as `legacy.dll`. Replace the current test environment path with:

```cmake
set_tests_properties(mangos_tests PROPERTIES ENVIRONMENT "OPENSSL_MODULES=")
```

Keep warning behavior at configure time when a developer machine lacks the optional runtime artifacts; CI/AppVeyor packaging remains the release gate.

- [ ] **Step 2: Assert the test environment remains empty**

In `OpenSSLProviderTest.cpp`, include `<cstdlib>` and add to the provider initialization test before obtaining the singleton:

```cpp
const char* modules = std::getenv("OPENSSL_MODULES");
CHECK(modules == nullptr || modules[0] == '\0');
```

This protects the requirement that the fallback does not mutate the environment.

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
std::wstring ReadWindowsEnvironment(const wchar_t* name);
std::filesystem::path GetExecutableDirectory();
bool ConvertForOpenSSL(const std::filesystem::path& directory,
                       std::string& converted);
void ConfigureBundledProviderSearchPath();
OpenSSLProvider LoadLegacyProvider();
```

Implementation requirements:

- `ReadWindowsEnvironment` grows a `std::vector<wchar_t>` around `GetEnvironmentVariableW` and returns a copied value.
- `GetExecutableDirectory` grows a `std::vector<wchar_t>` around `GetModuleFileNameW`; it never uses the current directory.
- `ConvertForOpenSSL` uses `WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, ...)`, rejects `usedDefaultChar`, and rejects a provider path whose appended `\legacy.dll` reaches `MAX_PATH`. On failure it retries with `GetShortPathNameW` and the same checks.
- `ConfigureBundledProviderSearchPath` returns immediately for a non-empty override, verifies `ossl-modules\legacy.dll` is a regular file, then requires `OSSL_PROVIDER_set_default_search_path(nullptr, converted.c_str()) == 1`.
- `LoadLegacyProvider` calls configuration first and returns `OpenSSLProvider("legacy")`.
- Non-Windows compilation keeps `LoadLegacyProvider` as a direct wrapper returning `OpenSSLProvider("legacy")`.

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

Replace the hard-coded `C:\OpenSSL-Win64\bin` advice with a message that expects `ossl-modules\legacy.dll` beside the daemon or an explicit `OPENSSL_MODULES` directory containing `legacy.dll`. On Windows, use a copied `ReadWindowsEnvironment(L"OPENSSL_MODULES")` value for the version-mismatch diagnostic; on other platforms copy `std::getenv` immediately into `std::string`.

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

Run the existing MSVC full build and CTest suite, then the existing clang-cl compilation gate used by the LFD branch. Record exact commands, exit codes, test counts, and binary revision metadata.

- [ ] **Step 4: Refresh install and Testing deployment**

Install from the rebased feature build into `E:\Mangos\WIP\Two\LFDSystemsRepair\install-msvc`, include the exact AppVeyor OpenSSL layout, and copy the resulting production-clean files to `E:\Mangos\WIP\Two\Testing\server_install`. Verify source/install/deployment hashes and confirm no temporary solo-LFD command is present.

- [ ] **Step 5: Stop short of publishing**

Report the branch HEAD, modified external AppVeyor files, verification evidence, AppVeyor upload instructions, and remaining user smoke test. Do not push, merge, or start either daemon.
