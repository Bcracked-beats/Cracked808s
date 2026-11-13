# In-place visualizer and Advanced sidebar

VISUAL ONLY now shrinks the existing host editor to 720×420. There is no second native window or duplicate analyzer. Independent dimensions are permitted from 320×200 to 3840×2160; a host/display may impose tighter limits. FULL UI restores the previous normal size and Advanced state.

Advanced occupies the right side at a logical 420px width. Two-column controls, page descriptions, a viewport and four-column modulation steps replace the bottom strip. Expansion fits the available display when possible. Parameter IDs and audio/state behavior are unchanged.

Regression tests cover same-editor ownership, no new desktop window, restored size, large/free resizing, hidden-host suspension and containment across all seven pages. Ten new integration assertions failed against 2.5.2 before implementation; the new-layout integration run passed.

Context7 supplied JUCE resizing/ownership documentation; exact APIs were checked against vendored headers. Caveman skills were applied as instruction workflows, not a cost-measurement runtime. No measured token savings are claimed.

The previously reported intermittent FL Studio exit crash remains deferred and unresolved. No audio-engine changes were made in this UI update.
