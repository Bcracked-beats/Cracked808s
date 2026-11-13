# Cracked808s 2.5.4 candidate — corresponding source

This source builds the unsigned 2.5.4 candidate with optional update notices. Public stable remains 2.5.3 while signing approval is pending. Keep the separately built update helper in each VST3/standalone bundle. See CODE_SIGNING.md, SECURITY.md and docs/UPDATES-2.5.4.md. Historical release notes below are retained for context.

Copyright (C) 2026 Cracked Vault. AGPL-3.0-only; ownership retained. Read LICENSE.txt, NOTICE.md and THIRD-PARTY-LICENSES.md. This is the matching source for the Windows x64 public-beta release, not an audio recording or a skin-only mockup.

## Contents

- src/: complete plugin/UI/DSP/state code.
- tests/: DSP, sound-quality, state/UI and real VST3 module-load checks.
- assets/: original logo, embedded Space Grotesk and its license.
- third_party/JUCE/: all vendored framework modules and CMake/juceaide build sources used for this release, with original third-party notices. Unused examples, documentation, standalone extras and precompiled demo/Projucer executables are omitted; they are not required to build this project. No network fetch is needed for JUCE.
- CMakeLists.txt: VST3, standalone and validation targets. Published manufacturer/plugin IDs Bcrk/Ck8s remain stable.

## Build on Windows x64

Install Visual Studio Build Tools/Community with Desktop development with C++, CMake >=3.22 and Ninja. Open an x64 Native Tools Command Prompt. From this extracted source directory:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

The tested environment used MSVC/Visual Studio18, its bundled CMake and Ninja. Linux/macOS toolchains are not validated for this release. The code may be portable, but this release does not include or promise macOS/AU/AAX binaries.

Outputs are under build/Cracked808s_artefacts/Release/VST3 and Standalone. Copy the entire Cracked808s.vst3 bundle into the system VST3 folder after closing your DAW. Audio latency is reported as256 samples. Default Normal2x is practical for real-time use; Superb32x/six-band operation may exceed one core and is intended for offline work.

## Verify and customize

Cracked808sValidation can optionally take an absolute output-directory argument to render the real editor, export factory presets and generate synthetic stereo WAV examples. It does not use your audio files or open a DAW. No real customer reviews, telemetry or private source paths are required to build.

Visual settings are display-only and saved in host/user-preset state. Advanced > Colors adds Palette Flow, Random Bass Hits, Full Spectrum and Fixed Hue, with bass sensitivity, minimum hit interval, transition, saturation and hue spread. Reduced Motion freezes automatic geometry/color changes. Random color sequences are pseudorandom and repeat from a fixed seed on editor creation; the current sequence position is not a saved parameter.

## Build the installer and release

After the Release CMake build, install or extract NSIS 3.12 or newer from its official distribution. NSIS 3.12 ZIP SHA-256: 56581f90db321581c5381193d796fffcf2d24b2f8fed2160a6c6a3baa67f2c4f (verify against https://nsis-dev.github.io/release-data/versions.json).

Run `pwsh -File scripts/Build-Release.ps1 -BuildDir <absolute-build-directory> -Makensis <absolute-path-to-makensis.exe>` from the source checkout. Staging directories must be new. The script packages the VST3, standalone, offline Setup, manual ZIP, and complete corresponding-source ZIP. Then run `pwsh -File tests/InstallerTests.ps1 -BuildDir <absolute-build-directory> -Makensis <absolute-path-to-makensis.exe>`. QA uses a disposable temporary destination and a separate HKCU uninstall key, never the real shared VST3 folder. Setup's production manifest requires administrator permission. No code-signing certificate is provided; do not claim the binaries are signed.

