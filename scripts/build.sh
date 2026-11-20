#!/usr/bin/env bash
# Build one firmware image with CMake and print the path of its .bin.
#
# Usage: scripts/build.sh <target>
#   <target> is rosbot or rosbot_xl.
#
# Output: build/release/<board>.bin (path printed on stdout, build
# log on stderr). Requires cmake >= 3.25, ninja, arm-none-eabi-gcc on PATH
# (Ubuntu: sudo apt install cmake ninja-build gcc-arm-none-eabi
# libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib).

set -euo pipefail

TARGET="${1:-}"
case "$TARGET" in
    rosbot | rosbot_xl) BOARD="$TARGET" ;;
    *)
        echo "Usage: $0 {rosbot|rosbot_xl}" >&2
        exit 2
        ;;
esac

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ ! -f build/release/CMakeCache.txt ]]; then
    cmake -S cmake --preset release >&2
fi
cmake --build build/release --target "$BOARD" >&2

echo "$ROOT/build/release/$BOARD.bin"
