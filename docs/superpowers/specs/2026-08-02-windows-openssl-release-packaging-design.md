# Windows OpenSSL Release Packaging Design

## Problem

The Windows release archive contains the OpenSSL runtime DLLs but does not
guarantee a usable OpenSSL 3 legacy provider. The AppVeyor script treats a
missing `legacy.dll` as a warning, stages modules under the non-standard
`openssl-modules` name, and publishes the archive without proving that the
extracted server can locate the provider. OpenSSL then falls back to its
compiled installation directory or an administrator-provided
`OPENSSL_MODULES`, neither of which is valid for a clean extracted release.

## Phase-one outcome

An extracted Windows release contains every OpenSSL artifact MaNGOS requires
and supplies launchers that select the bundled provider without requiring
OpenSSL to be installed globally or `OPENSSL_MODULES` to be configured by the
user. This first phase changes only the external AppVeyor scripts.

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
  Start-Mangosd.cmd
  Start-Realmd.cmd
```

The files must come from the same OpenSSL installation used to link the
binaries. A missing runtime DLL or `legacy.dll` is a packaging failure, not a
warning. The default provider is not packaged because OpenSSL 3 supplies it
internally on the supported distribution; only the dynamically loaded legacy
provider is required.

### Runtime discovery

Each launcher will:

1. Preserve an explicitly configured, non-empty `OPENSSL_MODULES` value.
2. Otherwise set `OPENSSL_MODULES` to the executable-relative `ossl-modules`
   directory for the child process only.
3. Start the corresponding daemon from the extracted server directory and
   forward all command-line arguments and the daemon's exit code.

The existing daemon binaries are unchanged. Launching an `.exe` directly, or
starting it as a Windows service, still requires a suitable externally defined
`OPENSSL_MODULES`. Executable-relative discovery for those entry points is a
possible later core change, outside this AppVeyor-first phase.

### Build and release packaging

The external AppVeyor script will copy the runtime DLLs and `legacy.dll` from
the same OpenSSL root supplied to CMake. It will use `ossl-modules`, create the
two launchers, and verify all staged archive inputs before invoking 7-Zip. This
prevents a stale or minimal AppVeyor OpenSSL cache from publishing a broken
archive.

The AppVeyor install script will validate `legacy.dll` as well as the runtime
and development libraries so a bad dependency cache fails before compilation.

## Failure handling

- AppVeyor fails before archive creation if any required artifact is absent.
- The launchers do not overwrite an administrator-provided `OPENSSL_MODULES`.
- Direct executable and Windows-service use retain the existing runtime
  diagnostic when no provider path is configured.

No database changes or configuration-file migration are required.

## Verification

1. Before editing, a focused contract check must fail because the current
   scripts use `openssl-modules`, allow the provider to be absent, and create no
   launchers.
2. Both AppVeyor PowerShell scripts are syntax-checked after editing.
3. A focused contract check confirms the required runtime/provider files,
   standard directory name, launcher behavior, and pre-archive assertions.
4. The next AppVeyor run is the integration gate: it must complete staging and
   publish a ZIP containing the exact layout above.
5. An extracted-release smoke test should launch each daemon through its
   launcher with `OPENSSL_MODULES` absent and confirm the legacy provider loads.

## Scope

This phase changes only the external Windows AppVeyor dependency and packaging
scripts. It does not modify MaNGOS source, cryptographic algorithms,
authentication protocol behavior, database schemas, or the completed LFD
behavior.
