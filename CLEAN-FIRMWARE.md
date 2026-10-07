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

## Bench evidence

Source cleanup: 303aafc. UF2 SHA-256:
`823d044374268462d39b13c0e65dc2676cda2fb3d5162edba78aedccb0a09f3d`.
On user-confirmed Fibonacci/A08, verified native write and exact 1.10.9
version/topology passed. Readback passed on both cables; settings vector and
plant BPM match the initial 1.10.8 baseline. Read-only flash-save delta is zero.
Thirty-second Paul/P07 mixed stress passed. At the earlier bench checkpoint,
localhost web loaded settings and confirmed `Saved on Biotron.` That older
updater targeted 1.9.8; it was not a browser-update acceptance result for 1.10.9.

Later on 7 October, native 1.10.9 → 1.9.8 → the same exact 1.10.9 completed.
Persisted settings on both cables and plant BPM matched at all three stages;
the comparison is preserved in ProjectData under
`biotron/2026-10-07-garden-release-qa/cycle-settings-comparison.json`.
The internal web runtime D (`30d978514f67`) now pins the clean 1.10.9 UF2.
Full browser directory-picker write/readback and the browser rollback/reflash
cycle remain unverified/BLOCKED; native evidence does not close them.

The original exact 1.10.8 passed source host tests, mixed stress and a
30-second capture (5 Note On / 5 Note Off, no active or orphan notes).
Native 1.10.8 → 1.9.8 → exact 1.10.8 completed before clean-candidate testing.
This later result does not erase the previous 1.9.8 orphan Note Off finding.
