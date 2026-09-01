# Biotron 1.9.8 beta-08 — organic full-board light

Internal Fibonacci/A06–A08 visual candidate only. It is not flashed, released
or approved for customers.

## Exact identity

- Source commit: `7471707`
- Pico SDK: `2.3.0`
- ARM GNU toolchain: `13.2.Rel1` (`arm-none-eabi-gcc 13.2.1`)
- Settings compatibility ID: `1765723554` (unchanged)
- Music + Pulse LED engine: `ON`
- UF2: `/Users/andreymanirko/ProjectData/playtronica-firmware/biotron/1.9.8-beta08/biotron-1.9.8-beta08-organic-led.uf2`
- UF2 SHA-256: `38c7fd35ef5e456d86b03f50f380d499519835fa84b7516fbd7b1cd1012b91da`
- ELF SHA-256: `76f2f9e9a086337d2b735059f8ecc2e6c31031b96028643a82119632f6af8c2f`

## Intended experience

- A note begins in its pitch zone instead of flashing the whole board.
- After one 4 ms engine tick, lower-energy light spreads to neighbouring zones
  and the opposite green arc, like reflected light.
- All six green zones participate while the played source and pitch remain the
  brightest readable location.
- The three blue zones receive a quiet note-density breath. Full beat pulses
  remain stronger, so rhythm is still legible.
- Faster notes accumulate smoothly and make the board fuller rather than
  producing binary flashes.

## Safety and compatibility

- MIDI bytes, settings, USB descriptors, protocol IDs and flash layout are
  unchanged.
- Rendering remains main-loop-only; there is no heap, flash or USB work per
  LED frame.
- The six green outputs share a `90000` logical-level frame budget. Repeated or
  dense notes cannot drive every green output to full duty at once.
- The feature remains compile-time scoped to A06–A08/Fibonacci. Do not flash
  A03–A05 with this artifact.

## Automated evidence

- All 17 production-linked host groups pass in sanitizer and optimized lanes.
- One-million-event LED stress remains bounded.
- Tests prove origin → 4 ms spread → decay, all nine logical zones responding,
  spatial pitch ordering, velocity/rate response, mute and A08 GPIO polarity.
- Pinned ARM Release build passes; `picotool info` identifies an RP2040 UF2,
  Pico SDK 2.3.0 and program `biotron`.

## Physical gates still required

1. Verify the connected board is the recorded Fibonacci/A08 unit and preserve
   the current settings plus rollback artifact.
2. Flash only after explicit owner approval and read back version `1.9.8`.
3. Compare slow low/mid/high notes: the origin must be clear and the spread
   gradual, with no dead zones or harsh flash.
4. Play increasingly fast for one minute: the board should become fuller and
   settle naturally without flicker, stuck light, USB loss or silent music.
5. Check brightness, current and temperature before any team handoff. Visual
   taste and electrical safety remain human/physical gates.
