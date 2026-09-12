# Debussy listening fixtures

Each CSV is the 180-beat golden event table for a three-minute trace at the
MIDI exporter's fixed 60 BPM. The matching MIDI uses channel 1 for the plant
melody and channel 2 for the pedal/colour pair. Both channels use the same
General MIDI piano program so seed A/B comparisons do not change timbre.
During a sensor response, the pedal holds the harmonic field and the colour
voice shadows each melodic move from two to nine semitones on either side.
Stable input is real silence: each continuing hand movement advances the
melody, while stopping leaves one short resolving echo before all three voices
return to rest. Light selects the preferred interval and scene slowly, so it
steers one melody without becoming a second random voice.

- `still_plant_3m`: low plant energy and slow, sub-threshold light drift; every
  beat is silent.
- `touch_arc_3m`: three approach–touch–release gestures, roughly one phrase
  apart, followed by a long release to calm.
- `light_arc_3m`: stable plant while a hand moves continuously over the light
  sensor, dark → bright → dark; every above-Wake-Up movement advances a note.
- Seed A: `0x4d595df4`; seed B: `0x9e3779b9`.

The CSV, not the MIDI serialization, is the musical golden truth. Regenerate
only after reviewing a deliberate rule change and updating its invariant test.

## First listening gate

Use `touch_arc_3m-seed-a.mid` as A and `touch_arc_3m-seed-b.mid` as B. Play
both through the same piano or soft-string patch, without changing tempo,
gain, reverb or channel balance. The sensor trace is byte-identical: touches
occur at beats 42, 66 and 90, approximately one phrase apart, then the input
settles for the rest of the sample.

For each seed, answer only:

1. Is the motif recognisable when it returns?
2. Do the three touches create a clear calm → grow → crest → release arc?
3. Is any repetition annoying, or any leap chaotic?
4. Which should continue: A, B, both, or neither?

Passing invariants and balanced MIDI do not decide whether the result is
musically good. No scheduler, protocol, persistence, UI or hardware work starts
until this listening gate passes.

## Elegance revision

The 2026-09-12 listening gate rejected beat-by-beat probability as too random.
These fixtures now use repeatable phrase masks, restart the motif at each phrase,
keep the root as a common tone across collection changes, and limit every melodic
leap to seven semitones. Random state may change only at phrase boundaries.
