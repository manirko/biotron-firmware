# Biotron 1.9.7 beta-05 — scheduler recovery

Internal test candidate only. Not released and not approved for customers.

## Exact identity

- Source commit: `9aad87e27c789f49e1e4489282b04041448e1cda`
- Pico SDK: `2.3.0`
- ARM GNU toolchain: `15.3.rel1`
- Settings compatibility ID: `1765723554` (unchanged)
- Music + Pulse LED engine: `ON` (A06–A08/Fibonacci test unit only)
- UF2 SHA-256: `ce3a15053c7c6cc16cc7a8757085b080173d32f2516b31690d933f17edebe336`
- ELF SHA-256: `b34f1fa1e3006b75059199f9d6790ed54fca2d692d298c4c71836e5a320cbff7`

## User-visible change

- Repeated or interrupted legacy Settings batches can no longer leave an
  otherwise responsive Biotron permanently silent. Their boundary command now
  restarts the internal music scheduler idempotently instead of toggling it.
- If an alarm allocation is transiently cancelled or fails, an Active device
  starts the music scheduler again on the next sensor sample.
- The quiet calibration and LED behaviour from beta-04 are unchanged.

## Incident evidence

- The connected 1.9.4 lab unit remained mounted and answered version, settings
  and health queries while producing no musical notes.
- Raw plant and light sensor values continued changing and all parser/USB/TX
  error counters were zero.
- An explicit recalibration restored 13 balanced Note On/Note Off pairs. This
  is consistent with a stopped scheduler, but does not by itself prove a single
  historical cause.

## Automated evidence

- Full production-linked host suite: PASS in ASan/UBSan and optimized lanes.
- ARM release build: PASS.
- Scheduler contract covers repeated batch boundaries, transient scheduling
  failure, automatic recovery and resumed balanced note generation.

## Still required

- Flash only the recorded Fibonacci/A06–A08 lab unit.
- Confirm firmware query returns `1.9.7` and presets survive the update.
- Reproduce the former failure path: play, change timing repeatedly in Settings,
  switch between Play and Settings, and recalibrate.
- Require continued notes/LED response, no stuck notes, USB loss or reset for
  ten minutes. Failure means rollback; it is not release evidence.
