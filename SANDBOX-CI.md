# Fork-only firmware candidate builder

This fork control branch exists only to build a test candidate from an explicit
`sandbox/*` ref. It does not create a release or update `biotron-releases`.

The workflow runs parser host tests, builds with pinned Pico SDK 2.3.0 and
uploads UF2/ELF plus source/toolchain commits and SHA-256 sums for 14 days.

Do not flash the artifact until the tester has recorded the PCB revision,
current firmware, known-good recovery image and approval from the firmware
owner. A successful CI build is not physical acceptance.
