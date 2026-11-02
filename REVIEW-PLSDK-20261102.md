# PLSDK integration review — 2 November 2026

Scope: Biotron internal1.10.11/a773904 plus today's descriptor-only6a26781.
The public web/test packet e02b11e/Windows r2 is a separate, unchanged component.
No device, settings write, calibration, BOOT or flash was used for this review.

## Fixed before tester candidate

1. USB CDC initialized before TinyUSB. Pico SDK2.3.0 stdio_usb_init requires
   tud_inited() when the app links tinyusb_device. main.setup called stdio_init_all
   before init_midi. Release NDEBUG hid the assertion; Debug can assert at startup.
   Fix: read settings, initialize MIDI/USB, then CDC stdio. No settings ABI change.
   Existing release contract fails on old order and passes on the repaired order.
2. Public PLSDK.h used bool without stdbool. A standalone C consumer fails to
   compile. Fix: include stdbool. Existing host gate now compiles that header alone.
3. tusb_config defined LIB_TINYUSB_HOST although CMake links tinyusb_device.
   This is a CMake linkage marker, not device configuration. Fix: remove the
   local definition. A source contract rejects overriding either linkage marker.
   IMPORTANT: the initial hypothesis that today's CDC is disabled was disproved
   by the real pinned-SDK preprocessor. CFG_TUH_RPI_PIO_USB is defined as0 by
   TinyUSB; Pico's defined() condition still includes CDC. No physical root cause
   or missing CDC output is claimed. Removing the false marker prevents wrong
   feature detection; the direct observed fault is startup/header contract.

## Further PLSDK requirements (not silently changed)

- PLSDK is a tracked vendor snapshot, not a git submodule: add_subdirectory and
  PLSDK_lib link the source actually shipped. The local VERSION2.0.0 is not proof
  of an upstream release. Keep provenance and product-specific patches with the
  SDK owner's review; do not replace it with a newer upstream wholesale.
- Separate product identity/configuration from reusable SDK implementation. Today's
  MIDI-first/EF0201 change must be carried to sibling products only with their
  own binary descriptor, factory CDC and physical acceptance checks. Do not copy
  Biotron IDs, settings identity or cable roles to another instrument.
- CFG_TUD_MIDI=2 is the number of TinyUSB MIDI function instances; this descriptor
  has one MIDI function with two cables. Changing it also changes the computed
  PID0x3011, so freeze VID/PID for this test. A future cleanup must decouple the
  historical PID from instance count and prove host/cache compatibility first.
- High-speed descriptor is inherited, RP2040 full-speed code does not use it.
  Its CDC-first order and single-cable macro disagree with the current two-cable
  full-speed declaration. Require a matching descriptor or explicit unsupported
  target guard before porting this SDK to a high-speed MCU. Not a current RP2040
  interoperability root-cause claim.
- Main loop owns tud_task, bounded RX/TX, persistence and sensor callbacks. IRQs
  defer musical work. Do not add a second USB task/core or change this ownership.
  Keep ASan/UBSan/optimized contract tests and actual ARM link proof together.
- Preserve exact parameter ranges/nonces, both reply cables, Note Off priority,
  bounded queues and settings identity1765723554. The full existing host suite
  covers these; software proof does not certify host timing, CDC factory use,
  OP-XY hearing, phone/Windows native routes or actual recovery.

## Latest feedback and smallest tester handoff

Telegram read2Nov:30Oct complete web e02b11e + Windows r2 was delivered at
https://t.me/c/1797899795/33543. The older958/v5 task is superseded. Sergey2Nov
reports no major immediately-fixable issue and asks for repository hygiene and
working together; this is not a Windows/full hardware acceptance result.27Oct
Limit4/near-identical timbres/calibration velocity items were addressed in the
blogger branch; do not redo the old redesign or claim new listening acceptance.

For the next packet keep validated public web e02b11e and Windows r2 immutable;
add this exact firmware source, audited UF2 and optional board-specific test card.
Do not insert internal1.10.11 into the public-stable updater. Stable1.8.2 remains
separate, and entering BOOT on that old firmware can clear presets. No mandatory
upgrade to solve a web issue. Native tests remain one step at a time after exact
board A06–A08, ownership, backup and rollback prerequisites.

Hardware task <=4steps after gates: identify/save original device state; listen
and reconnect on the chosen host; verify CDC/settings/approved recovery path;
report exact hash, expected/actual and restore result. No new auto-flash runner.
Do not call USB Fix or software gate a physical PASS.

Primary code references: PLSDK/CMakeLists.txt, PLSDK/include/tusb_config.h,
PLSDK/include/PLSDK.h, main.c, PLSDK/src/PLSDK.c, commands.c, midi_parser.c,
midi_tx.c and usb_descriptors.c; Pico SDK2.3.0 commit98a542c1, TinyUSB86ad6e56.
Official web reader could not fetch the two pinned GitHub pages (cache miss);
claims above use the actual pinned local source and ARM preprocessor, not snippets.
