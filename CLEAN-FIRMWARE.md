# Clean firmware candidate — 7 October 2026

Internal candidate 1.10.9 removes the abandoned composition experiment from
1.10.8 beta21. It restores the production tree at 1669c62, preserving all
changes through runtime calibration controls. USB identity, settings layout,
compatibility ID 1765723554, both MIDI cables and classic music remain intact.

Removed: composition engine/adapter, runtime controls and query 122, vendor
SysEx actions 28–37, gesture themes, renderers and audio/MIDI fixtures. These
IDs are unused; they must not silently become another settings action.

Retained since 1.9.8: both-cable CC regression, normalized calibration defaults,
movement-gated light notes, safe light pitch-mode transitions, calibration
metrics and runtime calibration controls. The Music + Pulse LED engine is ON
for this A06–A08/Fibonacci candidate; it is independent of composition mode.

Build: Pico SDK 2.3.0, ARM GNU 13.2.Rel1, Release, version 1.10.9. Host tests
run sanitizer and optimized lanes. Raw device evidence and restoration images
belong in ProjectData/playtronica-firmware, never in Git.

This is not a customer release. Browser flashing, musical/LED assessment,
power-cycle and other-platform acceptance remain separate release gates.
