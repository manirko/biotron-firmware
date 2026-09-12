#!/usr/bin/env python3
"""Render a deterministic, neutral listening preview from a Debussy trace."""

from __future__ import annotations

import argparse
import array
import csv
import math
import pathlib
import wave


SAMPLE_RATE = 44_100
BEAT_SECONDS = 1.0
MASTER_GAIN = 2.5


def frequency(note: int) -> float:
    return 440.0 * 2.0 ** ((note - 69) / 12.0)


def soft_clip(value: float) -> float:
    return value / (1.0 + abs(value))


def render(source: pathlib.Path, destination: pathlib.Path) -> None:
    with source.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))

    melody = [
        (
            int(row["beat"]) * BEAT_SECONDS,
            int(row["melody_note"]),
            int(row["velocity"]) / 127.0,
            max(1, int(row["duration_beats"])) * BEAT_SECONDS,
            row["surprise"] == "1",
        )
        for row in rows
        if row["melody_note"]
    ]
    frame_count = int(len(rows) * BEAT_SECONDS * SAMPLE_RATE)
    pcm = array.array("h")

    for frame in range(frame_count):
        time = frame / SAMPLE_RATE
        row = rows[min(len(rows) - 1, int(time / BEAT_SECONDS))]
        pedal = frequency(int(row["pedal_note"]))
        colour = frequency(int(row["colour_note"]))
        breath = 0.86 + 0.14 * math.sin(2.0 * math.pi * 0.083 * time)
        left = breath * (
            0.050 * math.sin(2.0 * math.pi * pedal * time)
            + 0.018 * math.sin(2.0 * math.pi * pedal * 2.0 * time)
            + 0.033 * math.sin(2.0 * math.pi * colour * time)
        )
        right = breath * (
            0.047 * math.sin(2.0 * math.pi * pedal * time + 0.08)
            + 0.020 * math.sin(2.0 * math.pi * pedal * 2.0 * time + 0.12)
            + 0.036 * math.sin(2.0 * math.pi * colour * time + 0.05)
        )

        for onset, note, velocity, duration, surprise in melody:
            age = time - onset
            tail = max(duration, 1.5) + (1.0 if surprise else 0.35)
            if age < 0.0 or age >= tail:
                continue
            attack = min(1.0, age / 0.018)
            envelope = attack * math.exp(-age * (1.45 if surprise else 2.1))
            fundamental = frequency(note)
            tone = velocity * envelope * (
                0.32 * math.sin(2.0 * math.pi * fundamental * age)
                + 0.13 * math.sin(2.0 * math.pi * fundamental * 2.01 * age)
                + 0.055 * math.sin(2.0 * math.pi * fundamental * 3.99 * age)
            )
            pan = max(-0.55, min(0.55, (note - 66) / 24.0))
            left += tone * (0.72 - 0.24 * pan)
            right += tone * (0.72 + 0.24 * pan)

        edge = min(time, len(rows) * BEAT_SECONDS - time)
        fade = min(1.0, max(0.0, edge / 0.012))
        pcm.append(
            int(max(-1.0, min(1.0, soft_clip(left) * MASTER_GAIN * fade)) * 32767)
        )
        pcm.append(
            int(max(-1.0, min(1.0, soft_clip(right) * MASTER_GAIN * fade)) * 32767)
        )

    with wave.open(str(destination), "wb") as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(pcm.tobytes())


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("destination", type=pathlib.Path)
    args = parser.parse_args()
    render(args.source, args.destination)


if __name__ == "__main__":
    main()
