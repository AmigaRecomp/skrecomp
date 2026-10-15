# Soccer Kid

A native port of **Soccer Kid** (Krisalis Software, 1993), the Amiga AGA version, for macOS, Linux and Windows.

It is not an emulator. The original 68000 program was disassembled and its game logic rewritten by hand in C, routine by
routine (levels, objects, bosses, the ball, the scoreboard, the world map, the title, the intro, the music replay). The
pictures, sounds, music, levels and texts are read from your own copy of the game: **no game data is in this
repository**.

This port was written with the assistance of **Claude Opus** (Anthropic): the reverse engineering of the original
program, the C code and its tests were done in a long working session between a person and the model.

## What you need

- The five disks of Soccer Kid for the Amiga 1200 (AGA), as `.adf` images or `.zip` files holding one `.adf` each.
  They are recognised by their volume names (`SKDisk0` .. `SKDisk4`), not by file name.
- CMake 3.24+, Ninja, a C compiler, SDL3 and zlib:
  - macOS: `brew install sdl3 ninja cmake`
  - Linux: your distribution's SDL3 and zlib development packages, `cmake`, `ninja`
  - Windows: MSYS2, then in its UCRT64 shell `pacman -S mingw-w64-ucrt-x86_64-{gcc,SDL3,zlib,cmake,ninja}`

## Build

```sh
scripts/build.sh /path/to/your/disks
```

This does two steps:

1. **Extract**: `skextract` reads the files the game needs from your disks into `data/soccerkid.dat` (about 4 MB).
   The file stays on your machine (`data/` is not tracked by git).
2. **Compile**: the build puts that file inside the program, so a single binary is the whole game.

The result is `build/macos/Soccer Kid.app` on macOS (with an icon cut from the title picture of your own data),
`build/linux/soccerkid` on Linux and `build/windows/soccerkid.exe` on Windows. Only the macOS build has been tested.

The same by hand:

```sh
cmake --preset macos                      # or linux, windows
cmake --build --preset macos
build/macos/skextract /path/to/your/disks
cmake --build --preset macos              # again, now with the data inside
cmake --build --preset macos-app          # macOS only: Soccer Kid.app
```

Without `data/soccerkid.dat` the program is built empty and asks for `--roms /path/to/your/disks` instead.

## Play

Arrows or WASD to move, up to jump, Space or X to kick, F10 or P to pause, Escape to leave a game. Gamepads work too;
F12 opens a screen to change the keys and buttons. Saves and the controls are kept in your user data folder.

More options and details: [docs/reference.md](docs/reference.md).
