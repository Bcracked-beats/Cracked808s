# Cracked808s 2.5.4 candidate — Cracked Vault

**Unsigned candidate, not a signed public release.** The public website still distributes 2.5.3. Signing/provider approval is pending. Do not treat this candidate as a SmartScreen-warning fix. Read CODE_SIGNING.md and SECURITY.md.

New in this candidate: deeper monochrome UI, centered Pan track/readout, and optional update checking in Advanced > Settings. Check daily starts off. The bundled helper only requests first-party version metadata; no audio/project/preset identifiers are sent and updates are not downloaded or installed automatically. Keep Cracked808sUpdate.exe inside the intact VST3 bundle or beside the standalone. See source docs/UPDATES-2.5.4.md. The historical install/feature notes below describe public 2.5.3; candidate package filenames use 2.5.4.

Free, open-source 808 saturation and stereo widening for Windows x64. VST3 and standalone public-beta builds. Copyright (C) 2026 Cracked Vault. Licensed AGPL-3.0-only; ownership is retained. See LICENSE.txt and NOTICE.md. Provided without warranty.

## Install

1. Close your DAW before installing or updating.
2. Recommended: run `Cracked808s-2.5.3-Windows-x64-Setup.exe`, approve Windows permission, and follow Setup. It installs the intact VST3 bundle automatically. Optional standalone app and uninstaller are in `C:\Program Files\Cracked Vault\Cracked808s`. Installation is offline and does not touch user presets.
3. Manual ZIP alternative: extract this ZIP. Copy the entire `VST3/Cracked808s.vst3` folder into `C:\Program Files\Common Files\VST3`. Keep the bundle intact; do not copy only its inner file. Windows may request administrator permission.
4. Reopen your DAW and rescan plugins. In FL Studio, use Options > Manage plugins > Find installed plugins. Load Cracked808s on an audio track.
5. Start with Clean Weight or Warm Tube. Adjust Drive/Tone/Mix and Width. Use stereo monitoring, with Mono Check off, to hear width. Save a user preset when you find a sound.

For standalone use, launch `Standalone/Cracked808s.exe` and configure its audio device separately. Only Windows x64 is included; macOS, AU and AAX builds are not supplied. This is an unsigned beta. Verify the published SHA-256 checksums and use trusted downloads; do not disable your system's security protections.

## New in 2.5.3

Click **VISUAL ONLY** to shrink the same plugin panel to its spectrum, particles or PNG. No second window opens. Resize freely from 320×200 up to 3840×2160, subject to your host/display. Click **FULL UI** to restore controls and the previous panel size. Advanced now opens on the right with two-column controls and a scrollable body. Every parameter remains available.

The 2.5.2 processing optimizations are retained. This update changes UI, not audio algorithms, quality defaults, state, plugin identity or latency. High oversampling and large visualizers still cost CPU.

**Known issue:** an intermittent FL Studio close crash has been reported. A blank-project native close test and the lifecycle probes passed, so the report is not considered fixed. Investigation was deferred at the owner's request. Save projects normally and report a reproducible test project if it recurs.

## Graphics cleanup introduced in 2.5.1

Fixed a Windows shutdown freeze after opening the plugin UI. Shared logo graphics are released when the last editor closes instead of during DLL teardown. Audio, controls, original VST3 identity, presets and project state remain unchanged. Install over the old version with your DAW closed; do not remove the plugin from your projects or delete presets.

## Included from 2.5.0

Pan is on the main panel below Input / Output. Drag left/right, enter L/R percentages, or double-click to center. Center preserves the original level; endpoints mute the opposite channel, like stereo balance in a mixer. Pan is smoothed and affects the complete wet/dry output. Existing Stereo Balance automation and presets remain compatible. Use a stereo output to hear placement; Mono Check / mono host outputs fold the result to mono. Global bypass ignores Pan.

The installer provides Windows Apps uninstall support. Close your DAW before upgrading or uninstalling. Uninstall removes managed plugin/app files but keeps user presets and unrelated files. Factory presets still reset sound settings, including Pan, to their defaults.

## Visual color options

Open Customize (or Advanced), then the Colors tab. Enable Color and choose:

- Palette Flow: the original timbre/hit-driven Aurora, Prism, Ice or Ember palettes.
- Random Bass Hits: a new full-spectrum hue on separated bass attacks. Treble and sustained notes do not repeatedly retrigger it.
- Full Spectrum: music-driven flow across the full hue wheel.
- Fixed Hue: hold a selected hue, with optional spatial spread.

Adjust bass sensitivity, minimum hit gap, transition time, saturation, hue and spread. Reduce transition time for snappier changes; increase it for smooth blends. Color and visual changes do not change the audio. Reduced Motion freezes automatic geometry/color changes. All settings travel in DAW/user-preset state. Default remains silver; the saturation/widening algorithms are unchanged; 2.5 adds smoothed final-output Pan. Random hues use a repeatable pseudorandom sequence from a fixed seed on editor creation, not a saved sequence position.

## Sound and visuals

23 saturation styles, up to six bands, Drive/Tone/parallel Mix, stereo generation from mono, low-frequency protection, 21 factory presets, user preset saving, A/B and undo/redo. Spectrum, particle field and embedded custom PNG visualizers follow real audio. Output safety is a stereo-linked sample-peak limiter, not a true-peak mastering limiter. It cannot repair audio clipped before the plugin. High Drive intentionally distorts; for a cleaner boost use Output with protection enabled.

Normal 2x quality is the real-time starting point. Superb32x/six-band processing may exceed one CPU core and is best reserved for offline work. Host-reported latency remains256 samples.

## Corresponding source

The matching `Cracked808s-2.5.1-source.zip` is provided alongside the Windows download. It includes all project sources, tests, assets, vendored JUCE modules/build tools and build instructions. Original and third-party notices are retained. Read the license before redistributing or modifying. No trademark or endorsement rights are granted.

Donations are optional: https://paypal.me/bdoggonecrazy . No donation unlocks additional features.
