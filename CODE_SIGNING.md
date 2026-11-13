# Code signing policy

Status (2026-11-13): **not active**. Public 2.5.3 is unsigned. Local 2.5.4 is an unsigned test candidate, not a signed public release. SignPath Foundation has not approved this project, and no certificate has been issued. No warning-free or malware-free claim is made.

Public source repository: https://github.com/Bcracked-beats/Cracked808s . The Windows candidate-check workflow is `.github/workflows/windows-ci.yml`; it builds/tests unsigned candidates only and does not publish releases. Its first hosted run and signing-provider approval must be verified separately. Protected signing/release permissions are not claimed to be configured yet.

Cracked Vault retains copyright. Bcracked is the maintainer, reviewer and release approver. Before signing becomes active, this policy must name the confirmed public repository and its protected release workflow, and repository/provider accounts must use MFA. External contributions require review; every signing request requires the owner's explicit approval.

## Required release evidence

1. Reviewed public source tag/commit, vendored dependencies, licences and corresponding build files.
2. Passing DSP, sound, UI, VST3 load/unload/exit and installer tests; no unreviewed downloads or post-build scripts in the installer.
3. Sign the VST3, helper and standalone before packaging; sign the NSIS-generated uninstaller and final installer. The signing provider must verify build provenance. Never use a self-signed certificate as a substitute for public trust.
4. Verify the expected certificate thumbprint, trusted Authenticode status and timestamp for all six managed files using `scripts/Verify-ReleaseTrust.ps1`. The uninstaller argument must be the actual extracted/installed uninstaller, not the installer again.
5. Compute final installer/ZIP/source SHA-256 checksums after all signing. Publish immutable versioned artifacts and matching source together. Update stable version metadata only after release availability is verified. Never overwrite an existing version with different bytes.

SignPath is the preferred free application route, subject to eligibility and approval: https://signpath.org/terms . Only after actual acceptance and a verified signed release may the site display “Free code signing provided by SignPath.io, certificate by SignPath Foundation.” Until then, do not use that sentence as a claim of sponsorship or acceptance. The JUCE AGPL distribution and vendored third-party licences must be disclosed to the provider.

## Privacy and installation

Update checks are off by default, manual or opt-in daily. The bundled helper requests only `https://www.crackedvault.workers.dev/updates/cracked808s.json`; ordinary network metadata is visible to the host, but no audio, project, preset, unique device ID or usage telemetry is sent. There are no cookies, redirects, automatic downloads or installations. No user-controlled response URL is launched. Requests cannot keep a plugin DLL worker alive during host shutdown. See https://www.crackedvault.workers.dev/security .

Setup installs the intact VST3 and optional standalone app, includes notices/uninstall support, and preserves presets. User preferences/cache live in the per-user Cracked Vault/Cracked808s/Updates folder and contain no credentials. Clearing this folder resets update preferences, not presets. A SmartScreen reputation warning is not a named malware finding; a named detection must be investigated rather than bypassed. Microsoft documents that signing does not guarantee immediate SmartScreen reputation: https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/smartscreen-reputation .
