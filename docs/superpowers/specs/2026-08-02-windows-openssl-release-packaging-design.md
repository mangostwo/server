# Windows OpenSSL Release Packaging Design

## Problem

The Windows release archive contains the OpenSSL runtime DLLs but does not
guarantee a usable OpenSSL 3 legacy provider. The AppVeyor script treats a
missing `legacy.dll` as a warning, stages modules under the non-standard
`openssl-modules` name, and publishes the archive without proving that the
extracted server can locate the provider. OpenSSL then falls back to its
compiled installation directory or an administrator-provided
`OPENSSL_MODULES`, neither of which is valid for a clean extracted release.

## Outcome

An extracted Windows release is self-contained for the OpenSSL functionality
MaNGOS requires. Starting `realmd.exe` or `mangosd.exe` directly must load the
matching legacy provider without requiring OpenSSL to be installed globally or
`OPENSSL_MODULES` to be configured.

## Design

### Installed layout

Windows dynamic-OpenSSL installs and AppVeyor release archives will contain:

```text
server/
  mangosd.exe
  realmd.exe
  libcrypto-3-x64.dll
  libssl-3-x64.dll
  ossl-modules/
    legacy.dll
```

The files must come from the same OpenSSL installation used to link the
binaries. A missing runtime DLL or `legacy.dll` is a packaging failure, not a
warning. The default provider is not packaged because OpenSSL 3 supplies it
internally on the supported distribution; only the dynamically loaded legacy
provider is required.

### Runtime discovery

On Windows, before providers are loaded, MaNGOS will:

1. Preserve an explicitly configured, non-empty `OPENSSL_MODULES` value.
2. Otherwise resolve the running executable's directory.
3. If `ossl-modules/legacy.dll` exists beside that executable, set OpenSSL's
   default provider search path to the `ossl-modules` directory for the default
   library context.
4. If the bundled provider is absent, retain OpenSSL's normal discovery and
   report a diagnostic that names both supported remedies.

This fallback is Windows-only. It does not change Linux provider discovery or
override administrator configuration.

### Build and release packaging

The core CMake install rules will locate the runtime DLLs and `legacy.dll` from
the OpenSSL installation selected by `find_package(OpenSSL)`, then install them
to the layout above. This makes local `cmake --install` output and release
packaging consistent.

The external AppVeyor script will retain a defensive packaging check. It will
use `ossl-modules`, require exactly the needed legacy provider, and verify the
staged archive inputs before invoking 7-Zip. This prevents a stale or minimal
AppVeyor OpenSSL cache from publishing a broken archive.

## Failure handling

- CMake configuration or installation fails on Windows dynamic builds if the
  required OpenSSL runtime/provider artifacts cannot be located.
- AppVeyor fails before archive creation if any required artifact is absent.
- Runtime diagnostics report the bundled directory attempted and the explicit
  `OPENSSL_MODULES` override when provider loading still fails.

No database changes or configuration-file migration are required.

## Verification

1. A focused provider test runs with `OPENSSL_MODULES` unset and a matching
   `ossl-modules/legacy.dll` beside the test executable. RC4 acquisition must
   succeed through the same provider manager used by both daemons.
2. CMake install output is checked for both OpenSSL runtime DLLs and
   `ossl-modules/legacy.dll`.
3. The AppVeyor PowerShell script is syntax-checked and its staging assertions
   are inspected against the required archive layout.
4. The rebased branch receives the existing targeted tests, one phase-end full
   build/CTest run, and an extracted-install provider smoke check with
   `OPENSSL_MODULES` removed from the process environment.

## Scope

This repair changes Windows dependency packaging and provider discovery only.
It does not change cryptographic algorithms, authentication protocol behavior,
database schemas, or the completed LFD behavior.
