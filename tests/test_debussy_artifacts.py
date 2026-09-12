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
import wave


ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXTURE_DIR = ROOT / "tests" / "fixtures" / "debussy"
EXPORTER = ROOT / "tests" / "debussy_csv_to_midi.py"
RENDERER = ROOT / "tests" / "debussy_csv_to_wav.py"
FIXTURES = {
    "still_plant_3m": 180,
    "touch_arc_3m": 180,
    "light_arc_3m": 180,
    "viral_touch_15s": 15,
}
SEEDS = {"seed-a": "0x4d595df4", "seed-b": "0x9e3779b9"}
TICKS_PER_BEAT = 480


def read_variable_length(data: bytes, position: int) -> tuple[int, int]:
    value = 0
    while True:
        byte = data[position]
        position += 1
        value = (value << 7) | (byte & 0x7F)
        if not byte & 0x80:
            return value, position


def assert_balanced_three_voice_midi(data: bytes, trace_beats: int) -> None:
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
    assert tick == trace_beats * TICKS_PER_BEAT


def rows_from(data: bytes) -> list[dict[str, str]]:
    return list(csv.DictReader(io.StringIO(data.decode("utf-8"))))


def assert_fixture_semantics(fixture: str, rows: list[dict[str, str]]) -> None:
    trace_beats = FIXTURES[fixture]
    assert len(rows) == trace_beats
    assert [int(row["beat"]) for row in rows] == list(range(trace_beats))
    assert all(int(row["active_voices"]) <= 3 for row in rows)
    scene_changes = [rows[0]] + [
        row for previous, row in zip(rows, rows[1:])
        if row["scene"] != previous["scene"]
    ]
    assert all(row["phrase_boundary"] == "1" for row in scene_changes[1:])

    accents = [int(row["beat"]) for row in rows if row["accent"] == "1"]
    structural_responses = [
        int(row["beat"]) for row in rows
        if row["structural_response"] == "1"
    ]
    surprises = [
        int(row["beat"]) for row in rows if row["surprise"] == "1"
    ]
    if fixture == "still_plant_3m":
        assert accents == []
        assert structural_responses == []
        assert surprises == []
        assert len(scene_changes) == 1
        assert sum(bool(row["melody_note"]) for row in rows[64:]) <= 52
    elif fixture == "touch_arc_3m":
        assert accents == [42, 66, 90]
        assert structural_responses == [42, 66, 90]
        assert surprises == [46, 70, 94]
        for touch in structural_responses:
            reveal = rows[touch + 4]
            returning = rows[touch + 8]
            assert reveal["arc_stage"] == "reveal"
            assert reveal["melody_note"] == reveal["landmark_note"]
            assert returning["arc_stage"] == "return"
            assert abs(
                int(returning["melody_note"]) - int(rows[touch]["melody_note"])
            ) <= 3
        assert {row["phase"] for row in rows} == {
            "calm", "grow", "crest", "release"
        }
        assert rows[-1]["phase"] == "calm"
    elif fixture == "viral_touch_15s":
        assert accents == [2]
        assert structural_responses == [2]
        assert surprises == [6]
        assert rows[6]["arc_stage"] == "reveal"
        assert rows[6]["melody_note"] == rows[6]["landmark_note"]
        assert rows[10]["arc_stage"] == "return"
        assert abs(
            int(rows[10]["melody_note"]) - int(rows[2]["melody_note"])
        ) <= 3
        assert rows[14]["melody_note"] == rows[10]["melody_note"]
        assert rows[14]["collection"] == rows[0]["collection"]
        assert rows[14]["pedal_note"] == rows[0]["pedal_note"]
    else:
        assert accents == []
        assert structural_responses == []
        assert surprises == []
        assert [row["scene"] for row in scene_changes] == [
            "veils", "cathedral", "pagodas", "cathedral", "veils"
        ]


def main() -> None:
    assert len(sys.argv) == 2, "generator executable required"
    generator = pathlib.Path(sys.argv[1])
    generated_by_name: dict[str, bytes] = {}

    with tempfile.TemporaryDirectory(prefix="debussy-artifacts-") as temp_name:
        temp = pathlib.Path(temp_name)
        for fixture, trace_beats in FIXTURES.items():
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
                assert_balanced_three_voice_midi(midi, trace_beats)

                if fixture == "viral_touch_15s":
                    wav_path = temp / f"{stem}.wav"
                    subprocess.run(
                        [sys.executable, RENDERER, csv_path, wav_path], check=True
                    )
                    with wave.open(str(wav_path), "rb") as preview:
                        assert preview.getnchannels() == 2
                        assert preview.getsampwidth() == 2
                        assert preview.getframerate() == 44_100
                        assert preview.getnframes() == trace_beats * 44_100

    for fixture in FIXTURES:
        assert generated_by_name[f"{fixture}-seed-a"] != generated_by_name[
            f"{fixture}-seed-b"
        ]
    print(
        f"debussy_artifacts: {len(FIXTURES)} traces x "
        f"{len(SEEDS)} seeds and balanced MIDI passed"
    )


if __name__ == "__main__":
    main()
