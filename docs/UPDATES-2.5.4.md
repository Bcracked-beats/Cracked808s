# 2.5.4 candidate: optional updates and release trust

Unsigned candidate only. Public stable remains 2.5.3 until a separate release is approved. Signing service/repository setup is not yet complete. See CODE_SIGNING.md and SECURITY.md.

Advanced > Settings contains CHECK FOR UPDATES and Check daily (optional), off by default. A newer public stable version reveals a first-party download-page link without adding another permanent main-menu control. There are no automatic downloads/installations. Existing APVTS state, presets, DSP, quality defaults and IDs are unchanged. The deeper black/white UI and centered Pan readout are included.

The separately bundled Cracked808sUpdate.exe uses normal Windows HTTPS validation, fixed first-party metadata, no redirects/cookies/auth, bounded response/timeouts, strict numeric version/product/schema/page validation and a per-user cache/mutex. No audio, preset, project or unique device identifier is sent. Ordinary network metadata is visible to the host. Successful and failed attempts are throttled for 24 hours; manual checking bypasses age, not concurrency exclusion. Auto-check preferences are independent of presets. If the helper is absent or a connection fails, audio continues offline.

The plugin launches the helper hidden with handle inheritance disabled. It owns no networking thread; editor/module teardown only closes a process handle and never waits for the helper. This design is not a claim that the separate intermittent FL Studio exit crash has been solved. The helper may finish a request after an editor closes.

Publish public/updates/cracked808s.json only for already available releases and keep it consistent with the website release version. Requests use DNT and GPC, and are not counted as plugin downloads. TLS checks must never be disabled. Any signer, provider acceptance or malware-scan claim requires actual independent evidence before publication.
