# Cracked808s — Cracked Vault

Free, open-source 808 saturation, stereo widening and visualizers for Windows x64. Built by Bcracked for the music community. Cracked Vault retains copyright; the code and original artwork are AGPL-3.0-only. Third-party licences and notices remain in place.

[Official downloads and version history](https://www.crackedvault.workers.dev/plugins/cracked808s) · [Download verification](https://www.crackedvault.workers.dev/security) · [Creator/contact](https://www.crackedvault.workers.dev/about)

## Status

This source builds **2.5.4, an unsigned development candidate**. Public stable is still 2.5.3. Code signing is in preparation, not approved or active. Neither open source nor a matching hash guarantees safety. Do not disable security protections. See [Code signing policy](CODE_SIGNING.md) and [Security reporting](SECURITY.md).

The candidate preserves the sound engine and adds optional first-party update checks, darker monochrome styling and aligned Pan. Update checks default off. No audio, projects, presets, device identifiers or telemetry are sent; ordinary network metadata is visible to the hosting provider. No automatic downloads/installations. The separate bundled helper has no network thread inside the plugin DLL and does not inherit DAW handles. A reported intermittent FL Studio exit issue remains unresolved.

## Build

Use Windows x64, Visual Studio C++ build tools, CMake 3.22+ and Ninja or the Visual Studio generator. JUCE 8.0.15 build sources are vendored; no framework fetch is needed.

```sh
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

The optional manual Windows CI workflow pins its actions, tests the real bundles, and produces **unsigned candidate artifacts only**. It never publishes a release or claims signing. NSIS 3.12 is required for installer packaging. All installer files must pass the trusted-signature gate after real signing and before a signed public release. Signing keys, credentials, visitor data and local build outputs do not belong in this repository.

Install the entire VST3 bundle, including Cracked808sUpdate.exe. Close the DAW before installation. User presets are preserved by managed uninstall. See [candidate update notes](docs/UPDATES-2.5.4.md), [licence](LICENSE.txt), [notices](NOTICE.md) and [third-party licences](THIRD-PARTY-LICENSES.md).
