#!/usr/bin/env python3
"""Convert a Debussy decision CSV into a balanced, three-voice MIDI file."""

from __future__ import annotations

import argparse
import csv
import pathlib
import struct
from collections.abc import Iterable


TICKS_PER_BEAT = 480


def variable_length(value: int) -> bytes:
    encoded = [value & 0x7F]
    value >>= 7
    while value:
        encoded.append(0x80 | (value & 0x7F))
        value >>= 7
    return bytes(reversed(encoded))


def event_track(rows: Iterable[dict[str, str]]) -> bytes:
    source = list(rows)
    events: list[tuple[int, int, bytes]] = [
        (0, 0, b"\xff\x03\x0cDebussy Mode"),
        (0, 0, b"\xff\x51\x03\x0f\x42\x40"),  # 60 BPM
        (0, 0, bytes((0xC0, 0))),
        (0, 0, bytes((0xC1, 0))),
    ]
    melody_note: int | None = None
    melody_off_tick = 0
    harmony_notes: tuple[int, int] | None = None

    for row in source:
        tick = int(row["beat"]) * TICKS_PER_BEAT
        if melody_note is not None and melody_off_tick <= tick:
            events.append((melody_off_tick, 1, bytes((0x80, melody_note, 0))))
            melody_note = None

        requested_harmony = (int(row["pedal_note"]), int(row["colour_note"]))
        if harmony_notes is None or row["colour_changed"] == "1":
            if harmony_notes is not None:
                for note in harmony_notes:
                    events.append((tick, 1, bytes((0x81, note, 0))))
            for note in requested_harmony:
                events.append((tick, 2, bytes((0x91, note, 46))))
            harmony_notes = requested_harmony

        if row["melody_note"]:
            if melody_note is not None:
                events.append((tick, 1, bytes((0x80, melody_note, 0))))
            melody_note = int(row["melody_note"])
            velocity = int(row["velocity"])
            duration = int(row["duration_beats"])
            events.append((tick, 2, bytes((0x90, melody_note, velocity))))
            melody_off_tick = tick + duration * TICKS_PER_BEAT

    end_tick = len(source) * TICKS_PER_BEAT
    if melody_note is not None:
        events.append((min(melody_off_tick, end_tick), 1,
                       bytes((0x80, melody_note, 0))))
    if harmony_notes is not None:
        for note in harmony_notes:
            events.append((end_tick, 1, bytes((0x81, note, 0))))

    track = bytearray()
    previous_tick = 0
    for tick, _, message in sorted(events, key=lambda item: (item[0], item[1])):
        track.extend(variable_length(tick - previous_tick))
        track.extend(message)
        previous_tick = tick
    track.extend(b"\x00\xff\x2f\x00")
    return bytes(track)


def convert(source: pathlib.Path, destination: pathlib.Path) -> None:
    with source.open(newline="", encoding="utf-8") as handle:
        track = event_track(csv.DictReader(handle))
    header = b"MThd" + struct.pack(">IHHH", 6, 0, 1, TICKS_PER_BEAT)
    destination.write_bytes(header + b"MTrk" + struct.pack(">I", len(track)) + track)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("destination", type=pathlib.Path)
    args = parser.parse_args()
    convert(args.source, args.destination)


if __name__ == "__main__":
    main()
