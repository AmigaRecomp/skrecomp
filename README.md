# Soccer Kid

A native port of **Soccer Kid** (Krisalis Software, 1993), the Amiga AGA version, for macOS, Linux and Windows.

It isn't an emulator. I disassembled the original 68000 program and rewrote its game logic in C, routine by routine: the levels, objects, bosses, the ball, the scoreboard, the world map, the title, the intro and the music replay. The pictures, sounds, music, levels and texts are read from your own copy of the game, so **no game data is in this repository**.

I wrote this port with the assistance of **Claude Opus** (Anthropic). The reverse engineering, the C code and its tests came out of a long working session between me and the model.

## What you need

- The five disks of Soccer Kid for the Amiga 1200 (AGA), as `.adf` images or `.zip` files holding one `.adf` each. They're recognised by their volume names (`SKDisk0` .. `SKDisk4`), not by file name.
- CMake 3.24+, Ninja, a C compiler, SDL3 and zlib:
  - macOS: `brew install sdl3 ninja cmake`
  - Linux: your distribution's SDL3 and zlib development packages, `cmake`, `ninja`
  - Windows: MSYS2, then in its UCRT64 shell `pacman -S mingw-w64-ucrt-x86_64-{gcc,SDL3,zlib,cmake,ninja}`

## Build

```sh
scripts/build.sh /path/to/your/disks
```

This does two steps:

1. **Extract**: `skextract` reads the files the game needs from your disks into `data/soccerkid.dat` (about 4 MB). The file stays on your machine (`data/` isn't tracked by git).
2. **Compile**: the build puts that file inside the program, so a single binary is the whole game.

You get `build/macos/Soccer Kid.app` on macOS (with an icon cut from the title picture of your own data), `build/linux/soccerkid` on Linux and `build/windows/soccerkid.exe` on Windows. I've only tested the macOS build so far.

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

Arrows or WASD to move, up to jump, Space or X to kick, F10 or P to pause, Escape to leave a game. Gamepads work too, and F12 opens a screen to change the keys and buttons. Saves and the controls are kept in your user data folder.

## Make your own levels

`skld` is a level designer that's built along with the game. You pick one of the original levels as the setting and build a new level from its tiles, enemies and objects:

```sh
build/macos/skld --settings                          # the settings: home1, london1, venice2, bonus3, ...
build/macos/skld new london1 mylevel.skl             # start from a copy of LONDON 1's map
build/macos/skld new venice1 mylevel.skl --size 200x30   # or from an empty one
build/macos/skld mylevel.skl                         # open it again
build/macos/soccerkid --custom mylevel.skl           # play it
```

Paint with the mouse, place whole things like a car or a post box from the Pieces tab, F1 lists the keys, F5 plays the level right there. A `.skl` file only holds your map, never any of the game's data, so levels can be shared freely; whoever plays one needs their own copy of the game.

More options and details: [docs/reference.md](docs/reference.md).
