# Version log

## 2.5.4 candidate — Optional updates and release verification (2026-11-13)
- Candidate only, not a signed public release. Public stable remains 2.5.3 pending release approval.
- Manual or opt-in daily first-party version checks under Advanced > Settings; no audio/project data or automatic installations. A link appears only for a newer public version.
- A separate 400-KiB-class helper keeps network lifetime outside the plugin DLL, with no inherited DAW handles. Existing DSP, parameters, IDs, presets and latency are unchanged.
- Neutral black/white UI and aligned Pan track/readout. Added transparent signing/verification policy and a trusted-release signature gate.
- The intermittent FL Studio exit crash remains unresolved; these checks are not a claim that it is fixed.

## 2.5.3 — One panel. More room. (2026-11-12)
- Replaced the detached window with in-place VISUAL ONLY mode. The existing host panel shrinks, freely resizes up to 4K, and returns through FULL UI.
- Moved Advanced to the right: two-column controls, page headings, a scrollable body and four-column modulation steps.
- Preserved parameter bindings, presets, IDs, latency, audio DSP and hidden-window analysis gates. No duplicate analysis consumer.
- The intermittent FL Studio exit crash remains unresolved; this is not a confirmed crash fix.

## 2.5.2 — Less repeated work. A separate view. (2026-11-11)
- Cached input/output gain targets and modulation envelope coefficients instead of recalculating identical powers/exponentials at every oversampled sample; sample-by-sample smoothing is retained.
- Skip zero-feedback nonlinear work, unused visual analysis without a visible consumer, settled visualizer repaints, and unchanged knob/background repaints. No lower audio-quality default or oversampling downgrade.
- Added POP OUT / DOCK for one resizable visualizer-only window. The same analyzer is moved, never duplicated. It closes with the plugin editor.
- Original plugin IDs, parameters, state/presets, limiter and reported latency are retained.
- Additional worker-unload, process-exit, modal teardown, pop-out lifetime and analysis scheduling tests. The intermittent FL Studio close problem was not reproduced in a blank-project test and remains unresolved; this release does not claim to fix it.

## 2.5.1 — Shutdown stability (2026-11-11)
- Fixed a Windows DAW-exit freeze caused by static GPU-backed logo images surviving editor teardown.
- Logo caches are now shared only while editors exist and released before DLL unload.
- Added a native VST3 regression that opens/paints/removes the real editor, unloads the module and completes COM shutdown under a timeout.
- Audio processing, parameters, presets, saved-project identity and latency are unchanged from 2.5.0.

## 2.4.0
- Added Palette Flow, Random Bass Hits, Full Spectrum and Fixed Hue behaviors.
- Bass-specific source-time detection; customizable sensitivity, hit gap, transition, saturation, hue and spread.
- Added Advanced > Colors; existing visual and audio state IDs preserved.
- Open-source AGPL-3.0-only public-beta release with corresponding source. Copyright retained by Cracked Vault.
- Audio DSP unchanged from2.3.1. Four test suites and the original-ID VST3 module probe pass.

## 2.3.1
- Replaced looping particle motion with real loudness, transient, spectrum and waveform-driven shape.
- Matched short-hit response at30/60FPS and stabilized bass waveform triggering through192kHz.
- Smaller logos and left-aligned title/subtitle.

## 2.3.0
- Corrected pitch-dependent stereo pull and improved center stability.
- Particle sphere, optional color palettes and Advanced visual customization.

## 2.2.0
- Stereo-linked lookahead output safety replaced hard clipping.
- Refined Drive/Tone, broader width, knob glow, PNG and spectrum presentation.

## 2.1.0
- Original brand artwork, embedded Space Grotesk and enhanced saturation/level compensation.
# 2.5.0 — Pan & easy installation (2026-10-29)

- Main-panel Pan with L/C/R readout and double-click centering. Smoothed 20ms stereo balance after global wet/dry mixing; center stays unity and bypass stays unaffected.
- Preserves the original `balance` parameter ID, project/user preset compatibility, VST3 identity and 256-sample latency.
- Offline Windows x64 Setup automatically installs the complete bundle into the standard VST3 folder, with optional standalone app and Windows uninstall support. User presets and unrelated files are preserved.
- Manual ZIP, corresponding AGPL source and checksums remain available. Unsigned public beta.

