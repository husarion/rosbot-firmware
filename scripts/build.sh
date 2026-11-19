#!/usr/bin/env bash
# Build one firmware image with CMake and print the path of its .bin.
#
# Usage: scripts/build.sh <target>
#   <target> is rosbot, rosbot_xl, rosbot_release or rosbot_xl_release:
#   the board, plus _release for the Release preset (default: Debug).
#
# Output: build/<debug|release>/<board>.bin (path printed on stdout, build
# log on stderr). Requires cmake >= 3.25, ninja, arm-none-eabi-gcc on PATH
# (Ubuntu: sudo apt install cmake ninja-build gcc-arm-none-eabi
# libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib).

set -euo pipefail

TARGET="${1:-}"
case "$TARGET" in
    rosbot | rosbot_xl) PRESET="debug"; BOARD="$TARGET" ;;
    rosbot_release | rosbot_xl_release) PRESET="release"; BOARD="${TARGET%_release}" ;;
    *)
        echo "Usage: $0 {rosbot|rosbot_xl|rosbot_release|rosbot_xl_release}" >&2
        exit 2
        ;;
esac

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ ! -f "build/$PRESET/CMakeCache.txt" ]]; then
    cmake -S cmake --preset "$PRESET" >&2
fi
cmake --build "build/$PRESET" --target "$BOARD" >&2

echo "$ROOT/build/$PRESET/$BOARD.bin"
