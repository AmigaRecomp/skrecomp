#!/usr/bin/env bash
# Builds Soccer Kid on macOS, Linux or Windows (MSYS2 UCRT64 shell). With a directory of your five disk images it also
# extracts the game data and builds it into the program (and, on macOS, packs Soccer Kid.app).
#   scripts/build.sh                 the program (asks for --roms / --data until data/soccerkid.dat exists)
#   scripts/build.sh ../SourceROMS   step 1 skextract, step 2 the program with the game inside
set -euo pipefail
cd "$(dirname "$0")/.."
case "$(uname -s)" in
  Darwin) preset=macos ;;
  Linux) preset=linux ;;
  MINGW*|MSYS*|CYGWIN*) preset=windows ;;
  *) echo "build.sh: unknown system $(uname -s)" >&2; exit 2 ;;
esac
cmake --preset "$preset"
cmake --build --preset "$preset"
if [ $# -ge 1 ]; then
  "build/$preset/skextract" "$1"
  cmake --build --preset "$preset"                       # again: soccerkid with data/soccerkid.dat inside
fi
if [ "$preset" = macos ]; then
  cmake --build --preset macos-app
  echo "build.sh: build/macos/Soccer Kid.app"
else
  echo "build.sh: build/$preset/soccerkid"
fi
