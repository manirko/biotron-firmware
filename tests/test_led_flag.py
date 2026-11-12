"""The build must supply the LED flag, and the header must preserve it."""
import os
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
command = [os.environ.get("CC", "cc"), "-std=c11", "-Werror", "-fsyntax-only",
           "-I", str(ROOT / "include"), "-x", "c", "-"]
missing = subprocess.run(command, input='#include "leds.h"\n', text=True,
                         capture_output=True)
assert missing.returncode != 0, "leds.h silently accepts a missing CMake flag"
assert "BIOTRON_LED_MUSIC_PULSE" in missing.stderr
for flag in (0, 1):
    probe = f"""#if BIOTRON_LED_MUSIC_PULSE != {flag}
#error flag differs before include
#endif
#include "leds.h"
#if BIOTRON_LED_MUSIC_PULSE != {flag}
#error header changed the build flag
#endif
led_engine_t engine;
"""
    subprocess.run(command + [f"-DBIOTRON_LED_MUSIC_PULSE={flag}"],
                   input=probe, text=True, check=True)
print("led_flag: missing flag rejected; OFF=0 / ON=1 and unconditional include passed")
