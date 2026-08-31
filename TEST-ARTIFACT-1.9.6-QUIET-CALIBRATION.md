# Biotron 1.9.6 beta-04 — quiet calibration

Internal test candidate only. Not released and not approved for customers.

## Exact identity

- Source commit: `df05841759b39d2e04b1ed615e6eff64599aeb03`
- Pico SDK: `2.3.0`
- ARM GNU toolchain: `15.3.rel1`
- Settings compatibility ID: `1765723554` (unchanged)
- Music + Pulse LED engine: `ON` (A06–A08/Fibonacci test unit only)
- UF2 SHA-256: `4b77d70376be85d68d78751794222627f4f80f7a6eef85c829426610a0ef055a`
- ELF SHA-256: `4b0dd45142b42d64170533542cac96b327ff679f9072db3cf4ed0dc55f1758b7`

## User-visible change

- Calibration keeps the eight-note cue but sends MIDI velocity `24` instead of
  `64`. Ordinary plant, light, button and DAW notes are unchanged.
- Automatic and browser-requested calibration now report waiting/measuring/
  ready state. Automatic calibration uses nonce `0`; an explicit request keeps
  its request nonce. This lets compatible Settings builds attenuate calibration
  without guessing from notes.

## Automated evidence

- Full production-linked host suite: PASS in ASan/UBSan and optimized lanes.
- ARM release build: PASS.
- Scheduler contract covers explicit calibration, automatic nonce-zero state,
  exact cue notes/velocity, matching Note Off and sensor-loss interruption.

## Still required

- Flash only the recorded Fibonacci/A06–A08 lab unit.
- Confirm firmware query returns `1.9.6` and presets survive the update.
- Compare calibration with normal play in Chrome and a DAW/external synth.
- Confirm the cue is quiet but still audible, no stuck notes/USB loss, and LED
  behaviour remains correct. Failure means rollback; it is not release evidence.
