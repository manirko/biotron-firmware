#!/bin/sh
# Build only; never contacts a connected device.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if [ "$#" -ne 2 ]; then
  echo 'Usage: tools/build.sh stable|candidate /absolute/new-build-directory' >&2
  exit 2
fi
case "$1" in
  stable) minor=8; patch=2; base=67920c21d51866576075308529a055f10be164dc ;;
  candidate) minor=10; patch=11; base=a7739040e07ba7ca04767682755eec8589c15234 ;;
  *) exit 2 ;;
esac
case "$2" in /*) ;; *) echo 'Build directory must be absolute' >&2; exit 2 ;; esac
[ ! -e "$2/CMakeCache.txt" ] || { echo 'Use a new build directory' >&2; exit 2; }
cd "$root"
git merge-base --is-ancestor "$base" HEAD
# Keep a candidate source tree from accidentally announcing the stable version.
if [ "$1" = stable ]; then
  ! git cat-file -e HEAD:include/settings_readback.h 2>/dev/null
else
  git cat-file -e HEAD:include/settings_readback.h
fi
export PICO_SDK_PATH="${PICO_SDK_PATH:-$HOME/ProjectData/toolchains/pico-sdk-2.3.0}"
export PICO_TOOLCHAIN_PATH="${PICO_TOOLCHAIN_PATH:-$HOME/ProjectData/toolchains/arm-gnu-toolchain-13.2.Rel1-darwin-arm64-arm-none-eabi/bin}"
[ "$(git -C "$PICO_SDK_PATH" rev-parse HEAD)" = 98a542c1a62fb549ffb5d66a3e5892b06276b670 ]
"$PICO_TOOLCHAIN_PATH/arm-none-eabi-gcc" -dumpfullversion | grep -qx 13.2.1
export FLASH_ID_STARTUP=1765723554 MAJOR_VERSION=1 MINOR_VERSION="$minor" PATCH_VERSION="$patch"
export SOURCE_DATE_EPOCH=1793577600 DEBUG_BUILD=false
if [ "$1" = candidate ]; then
  set -- "$2" -DBIOTRON_FLASH_ID=1765723554 -DBIOTRON_VERSION_MAJOR=1 \
    -DBIOTRON_VERSION_MINOR=10 -DBIOTRON_VERSION_PATCH=11 -DBIOTRON_LED_MUSIC_PULSE=ON
else
  set -- "$2"
fi
build_dir=$1
shift
cmake -S "$root" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DPICO_BOARD=pico -DPICO_PLATFORM=rp2040 -DPICO_TOOLCHAIN_PATH="$PICO_TOOLCHAIN_PATH" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$@"
cmake --build "$build_dir" --parallel 4
shasum -a 256 "$build_dir/biotron.uf2"
