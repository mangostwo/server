# Windows OpenSSL Release Packaging Design

## Problem

The Windows release archive contains the OpenSSL crypto runtime DLL but does not
guarantee a usable OpenSSL 3 legacy provider. The AppVeyor script treats a
missing `legacy.dll` as a warning, stages modules under the non-standard
`openssl-modules` name, and publishes the archive without proving that the
extracted server can locate the provider. OpenSSL then falls back to its
compiled installation directory or an administrator-provided
`OPENSSL_MODULES`, neither of which is valid for a clean extracted release.

## Outcome

An extracted Windows release contains every OpenSSL artifact MaNGOS requires
and automatically selects the bundled provider without requiring OpenSSL to be
installed globally or `OPENSSL_MODULES` to be configured by the user. Direct
daemon and Windows-service startup both work from the extracted release.

## Design

### Installed layout

Windows dynamic-OpenSSL installs and AppVeyor release archives will contain:

```text
server/
  mangosd.exe
  realmd.exe
  libcrypto-3-x64.dll
  ossl-modules/
    legacy.dll
```

The files must come from the same OpenSSL installation used to link the
binaries. A missing runtime DLL or `legacy.dll` is a packaging failure, not a
warning. The default provider is not packaged because OpenSSL 3 supplies it
internally on the supported distribution; only the dynamically loaded legacy
provider is required. AppVeyor proves this for the selected runtime by running
its `openssl.exe` with a temporary module directory containing only the chosen
`legacy.dll` and requiring both `default` and `legacy` providers to load before
the archive is created.

### Runtime discovery

On Windows, before loading either provider, MaNGOS will:

1. Preserve an explicitly configured, non-empty `OPENSSL_MODULES` value.
2. Otherwise resolve the running executable's directory rather than relying on
   the process working directory.
3. If `ossl-modules/legacy.dll` exists beside the executable, set OpenSSL's
   default provider search path to that `ossl-modules` directory before loading
   the legacy and default providers.
4. If the bundled file is absent, retain OpenSSL's normal provider discovery
   and report both supported remedies: use a complete release or explicitly set
   `OPENSSL_MODULES` to the directory containing `legacy.dll`.

This does not modify the process environment, does not override administrator
configuration, and does not change non-Windows provider discovery.

The ordering is explicit: a small factory used to initialize
`m_legacyProvider` first configures the search path and then constructs the
legacy-provider wrapper. `m_defaultProvider` is constructed afterward. Search
path configuration therefore occurs before either call to
`OSSL_PROVIDER_load`, rather than in the manager constructor body after member
initialization has already completed.

Executable-path resolution uses `GetModuleFileNameW` with a dynamically grown
buffer. It constructs
`<executable-directory>\ossl-modules` as a wide filesystem path, checks
`legacy.dll` without consulting the current working directory, and converts the
directory to the active Windows ANSI code page for OpenSSL's `LoadLibraryA`
provider loader. Conversion rejects best-fit substitutions. If the path cannot
be represented or exceeds the narrow loader's practical path limit, MaNGOS
tries the directory's existing 8.3 short path; if neither form is usable it
retains fail-closed provider loading and logs the unsupported path. A non-empty
override is detected through the Windows environment API and copied for all
diagnostics, including version mismatch; no borrowed environment pointer
survives initialization.

### Build and release packaging

The external AppVeyor script will copy the crypto runtime DLL and `legacy.dll` from
the same OpenSSL root supplied to CMake. It will use `ossl-modules` and verify
all staged archive inputs before invoking 7-Zip. This prevents a stale or
minimal AppVeyor OpenSSL cache from publishing a broken archive.

Artifact lookup uses an ordered list of supported paths beneath that selected
x64 OpenSSL root: `bin` for runtime DLLs, followed by the root for distributions
that place them there; and `bin\ossl-modules`, `bin`,
`lib\ossl-modules`, then `ossl-modules` for `legacy.dll`. It does not
recursively select an arbitrary first match. The dependency check also requires
both crypto and SSL import libraries before a root is considered usable. Only
`legacy.dll` is staged; `default.dll` and unrelated provider modules are not
packaged.

The AppVeyor install script will validate `legacy.dll` as well as the runtime
and development libraries so a bad dependency cache fails before compilation.

## Failure handling

- AppVeyor fails before archive creation if any required artifact is absent.
- The core does not override an administrator-provided `OPENSSL_MODULES`.
- If executable-path resolution, provider-file validation, or OpenSSL search
  path configuration fails, provider loading retains its existing fail-closed
  behavior and logs an actionable diagnostic.

No database changes or configuration-file migration are required.

## Verification

1. Before editing, the Windows provider test must fail with `OPENSSL_MODULES`
   absent even when a matching `ossl-modules/legacy.dll` is staged beside the
   test executable.
2. After the core change, the same test must load the legacy provider and fetch
   RC4 without modifying the process environment. The test also asserts that
   `OPENSSL_MODULES` remains absent or empty.
3. Before editing, a focused AppVeyor contract check must fail because the
   current scripts use `openssl-modules` and allow the provider to be absent.
4. Both AppVeyor PowerShell scripts are syntax-checked, and the contract check
   confirms the runtime/provider files, standard directory name, and
   pre-archive assertions.
5. The next AppVeyor run must publish the exact layout above. An extracted ZIP
   smoke test starts each daemon directly with `OPENSSL_MODULES` absent and
   confirms the legacy provider loads.

The Windows failure diagnostic names the release-relative `ossl-modules`
remedy and says an explicit `OPENSSL_MODULES` must point to the directory that
contains `legacy.dll`; it no longer assumes `C:\OpenSSL-Win64\bin`.

The Windows test CMake rules copy the selected `legacy.dll` into
`$<TARGET_FILE_DIR:mangos_tests>/ossl-modules`, co-locate the matching OpenSSL
crypto runtime DLL, and run `mangos_tests` with `OPENSSL_MODULES` empty. They no longer
make the test pass by setting `OPENSSL_MODULES` to the machine installation.

## Scope

This repair changes only Windows provider discovery and external AppVeyor
dependency packaging. It does not change cryptographic algorithms,
authentication protocol behavior, database schemas, or the completed LFD
behavior.
