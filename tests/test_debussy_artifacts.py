#!/usr/bin/env python3
"""Prove that committed Debussy CSV/MIDI listening artifacts are reproducible."""

from __future__ import annotations

import csv
import io
import pathlib
import struct
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURE_DIR = ROOT / "tests" / "fixtures" / "debussy"
EXPORTER = ROOT / "tests" / "debussy_csv_to_midi.py"
FIXTURES = ("still_plant_3m", "touch_arc_3m", "light_arc_3m")
SEEDS = {"seed-a": "0x4d595df4", "seed-b": "0x9e3779b9"}
TICKS_PER_BEAT = 480
TRACE_BEATS = 180


def read_variable_length(data: bytes, position: int) -> tuple[int, int]:
    value = 0
    while True:
        byte = data[position]
        position += 1
        value = (value << 7) | (byte & 0x7F)
        if not byte & 0x80:
            return value, position


def assert_balanced_three_voice_midi(data: bytes) -> None:
    assert data[:4] == b"MThd"
    header_length, format_number, tracks, division = struct.unpack(
        ">IHHH", data[4:14]
    )
    assert (header_length, format_number, tracks, division) == (6, 0, 1, 480)
    assert data[14:18] == b"MTrk"
    track_length = struct.unpack(">I", data[18:22])[0]
    assert len(data) == 22 + track_length

    position = 22
    tick = 0
    active: set[tuple[int, int]] = set()
    maximum_voices = 0
    while position < len(data):
        delta, position = read_variable_length(data, position)
        tick += delta
        status = data[position]
        position += 1
        if status == 0xFF:
            position += 1  # meta event type
            length, position = read_variable_length(data, position)
            position += length
            continue

        kind = status & 0xF0
        channel = status & 0x0F
        assert kind in (0x80, 0x90, 0xC0)
        data_length = 1 if kind == 0xC0 else 2
        message = data[position : position + data_length]
        position += data_length
        if kind == 0x90 and message[1] > 0:
            identity = (channel, message[0])
            assert identity not in active
            active.add(identity)
            maximum_voices = max(maximum_voices, len(active))
        elif kind in (0x80, 0x90):
            identity = (channel, message[0])
            assert identity in active
            active.remove(identity)

    assert not active
    assert maximum_voices <= 3
    assert tick == TRACE_BEATS * TICKS_PER_BEAT


def rows_from(data: bytes) -> list[dict[str, str]]:
    return list(csv.DictReader(io.StringIO(data.decode("utf-8"))))


def assert_fixture_semantics(fixture: str, rows: list[dict[str, str]]) -> None:
    assert len(rows) == TRACE_BEATS
    assert [int(row["beat"]) for row in rows] == list(range(TRACE_BEATS))
    assert all(int(row["active_voices"]) <= 3 for row in rows)
    assert all(
        row["phrase_boundary"] == "1"
        for row in rows
        if row["colour_changed"] == "1"
    )

    accents = [int(row["beat"]) for row in rows if row["accent"] == "1"]
    changes = [row for row in rows if row["colour_changed"] == "1"]
    if fixture == "still_plant_3m":
        assert accents == []
        assert len(changes) == 1
        assert sum(bool(row["melody_note"]) for row in rows[64:]) <= 52
    elif fixture == "touch_arc_3m":
        assert accents == [42, 66, 90]
        assert {row["phase"] for row in rows} == {
            "calm", "grow", "crest", "release"
        }
        assert rows[-1]["phase"] == "calm"
    else:
        assert accents == []
        assert [row["scene"] for row in changes] == [
            "veils", "cathedral", "pagodas", "cathedral", "veils"
        ]


def main() -> None:
    assert len(sys.argv) == 2, "generator executable required"
    generator = pathlib.Path(sys.argv[1])
    generated_by_name: dict[str, bytes] = {}

    with tempfile.TemporaryDirectory(prefix="debussy-artifacts-") as temp_name:
        temp = pathlib.Path(temp_name)
        for fixture in FIXTURES:
            for seed_name, seed in SEEDS.items():
                stem = f"{fixture}-{seed_name}"
                generated = subprocess.check_output([generator, fixture, seed])
                generated_by_name[stem] = generated
                assert generated == (FIXTURE_DIR / f"{stem}.csv").read_bytes()
                rows = rows_from(generated)
                assert_fixture_semantics(fixture, rows)

                csv_path = temp / f"{stem}.csv"
                midi_path = temp / f"{stem}.mid"
                csv_path.write_bytes(generated)
                subprocess.run(
                    [sys.executable, EXPORTER, csv_path, midi_path], check=True
                )
                midi = midi_path.read_bytes()
                assert midi == (FIXTURE_DIR / f"{stem}.mid").read_bytes()
                assert_balanced_three_voice_midi(midi)

    for fixture in FIXTURES:
        assert generated_by_name[f"{fixture}-seed-a"] != generated_by_name[
            f"{fixture}-seed-b"
        ]
    print("debussy_artifacts: 3 traces x 2 seeds and balanced MIDI passed")


if __name__ == "__main__":
    main()
