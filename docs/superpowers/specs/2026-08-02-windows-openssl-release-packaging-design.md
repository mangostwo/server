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

### Build and release packaging

The external AppVeyor script will copy the runtime DLLs and `legacy.dll` from
the same OpenSSL root supplied to CMake. It will use `ossl-modules` and verify
all staged archive inputs before invoking 7-Zip. This prevents a stale or
minimal AppVeyor OpenSSL cache from publishing a broken archive.

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
   RC4 without modifying the process environment.
3. Before editing, a focused AppVeyor contract check must fail because the
   current scripts use `openssl-modules` and allow the provider to be absent.
4. Both AppVeyor PowerShell scripts are syntax-checked, and the contract check
   confirms the runtime/provider files, standard directory name, and
   pre-archive assertions.
5. The next AppVeyor run must publish the exact layout above. An extracted ZIP
   smoke test starts each daemon directly with `OPENSSL_MODULES` absent and
   confirms the legacy provider loads.

## Scope

This repair changes only Windows provider discovery and external AppVeyor
dependency packaging. It does not change cryptographic algorithms,
authentication protocol behavior, database schemas, or the completed LFD
behavior.
