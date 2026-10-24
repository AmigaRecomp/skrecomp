# Soccer Kid port: reference

The full list of options, keys and build details. The short version is in the [README](../README.md).

This is my native rewrite of Soccer Kid (Amiga AGA, Krisalis 1993) in C with SDL3. The game's own data files come from the original disk images; nothing from the disks is stored in this repository. I keep my reverse-engineering notes and tools, which this code is written from, separately.

## Build

Requirements: a C11 compiler, CMake 3.24 or newer, Ninja, SDL3 and zlib.

| system | install | build |
|---|---|---|
| macOS | `brew install sdl3 ninja cmake` (zlib is in the system) | `scripts/build.sh <your disks>` |
| Linux | SDL3 and zlib development packages, e.g. `libsdl3-dev zlib1g-dev ninja-build cmake` | `scripts/build.sh <your disks>` |
| Windows | MSYS2, in its UCRT64 shell: `pacman -S mingw-w64-ucrt-x86_64-{gcc,SDL3,zlib,cmake,ninja}` | `scripts/build.sh <your disks>` |

`scripts/build.sh` picks the CMake preset for the system (`macos`, `linux` or `windows` in `CMakePresets.json`, output in `build/<preset>/`). Given a directory of your disk images it runs both steps below. On macOS it also packs `build/macos/Soccer Kid.app` (`cmake --build --preset macos-app`): the game with SDL3 inside the app, signed ad hoc, with an icon cut from the title picture of your own data (the kid, by `skicon`). On Windows `SDL3.dll` is copied next to `soccerkid.exe`. I've only tested the macOS build; the Linux and Windows presets are written for those systems' usual toolchains.

By hand:

```sh
cmake -B build -G Ninja
cmake --build build                          # builds skextract, skembed and soccerkid
build/skextract <your disks>                 # your five disks (.adf or .zip) -> data/soccerkid.dat
cmake --build build                          # again: soccerkid now carries the game inside
build/soccerkid                              # the whole game, no other files needed
```

`skextract` reads the files the game needs from your own disk images into `data/soccerkid.dat` (about 4 MB), and the next build embeds that file in the `soccerkid` program, so the one binary is the complete game. The repository never holds any of it (`data/` is ignored). Without the file the program is built empty and asks for `--roms <dir>` (the disk images) or `--data FILE` (a `soccerkid.dat`) instead.

## Run

With the data built in, `soccerkid` needs no arguments. Otherwise point it at a directory holding the five disk images (`--roms`), either as `.adf` files or as `.zip` archives containing one `.adf` each. The images are recognised by their volume names `SKDisk0` to `SKDisk4`, not by file name.

```sh
build/soccerkid                                       # the game: the intro, the language screen, the title
build/soccerkid --no-intro                            # start at the language screen
build/soccerkid --new-game                            # straight into a new game at the first world map
build/soccerkid --level 0                             # play level 0 in a window
build/soccerkid --level 0 --frames 300 --script R150,RU20,R130 --screenshot out.png
                                                      # play 300 frames headless from a script, then a PNG
build/soccerkid --level 0 --viewer                    # the level viewer (scrolls, no gameplay)
build/soccerkid --level 17 --viewer --section file036:0   # show a graphics set's frames
build/soccerkid --help                                # every option
```

Without `--level` the program starts as the original does. First the intro (the alien Scab's story, about two minutes; any key, the mouse or fire skips it at its next text or animation), then the language screen (the language of the in-game messages), then the title with START GAME, KIT DESIGN (the kid's shirt and shorts) and OPTIONS (music, sound, the tutorial, RESUME GAME). Left alone, the title shows the hi-score table and the demo level. A good score asks for a name, and before each new world the game offers to save (the file is kept in your user data folder, or `--save FILE`).

A game runs as the original's level sequence: the loading strip, the world map with the aircraft flying to the next country (and the cup pieces collected so far), each pair of levels, then the scoreboard that counts the cards, balls, time and trick kicks into a bonus. All 11 cards of a pair open the world's bonus level: a minute to take 50 tokens, then the cup piece. At the end comes the ALIEN (it sends the kid home without the five cup pieces and fights with them), then the end screen and back to the title.

More options:

- `--cup 31` starts with all five cup pieces.
- `--level N` plays one level by its data index (0 = England 1 ... 28, 12 = the scoreboard) and moves on along the sequence from there.
- `--section fileNNN:S[:WxH]` shows the frames of a graphics-set section from the level's disk (the frame size is estimated from the data unless given).
- `--cam X,Y` sets the camera; `--no-backdrop` and `--no-frames` reduce the picture to one layer.
- `--script` drives the kid without a keyboard: tokens `<keys><frames>` joined by commas, keys `U D L R F` or `.` for none (`R150,RU20,R130` = run right, jump, run on). With `--frames N` and `--screenshot` no window opens.

## Playing

Arrows or WASD move, up jumps, down crouches (down while running slides), and Space, Left Ctrl or X is fire. `F` toggles fullscreen, `M` mutes the sound, F10 or `P` pauses (with the level's help box). Escape in a game closes the curtain and goes back to the title, as in the original; on the title, the intro or the attract screens it quits the program.

Gamepads (any controller SDL knows: Xbox, PlayStation, Switch Pro, ...) work out of the box: d-pad or left stick to move, A to jump, B or X for fire, Start to pause, Back to leave. F12 opens the remap screen: press a key or pad button for each action in turn (Backspace or the pad's Back keeps one, F5 restores the defaults, Escape cancels; F, M, TAB and F1..F12 stay fixed, and an input moved to a new action leaves its old one). The bindings are kept in `controls.cfg` in your user data folder (or `--controls FILE`), a text file you can also edit.

On the title, TAB shows the version, F1..F10 in KIT DESIGN copy a hi-score's kit, and typing names a save.

The game runs at the original's 50 frames per second with the original's movement, camera, collision and level flow. The kid and the ball are drawn as the hardware sprite pairs the game uses, in the level's kit colours, and the HUD shows health, the timer, the score, the card pieces and the lives.

The ball:

- **Kick:** fire with left or right kicks the ball at your feet; kicked into an enemy it knocks it out for points. Jumping onto the ball with down held stands you on it.
- **Tricks:** with the ball at your feet, hold fire still for a moment and the kid flicks it up and juggles it; a direction while juggling picks the next trick (overhead, volleys, the spring off the ball with up). Holding fire without the ball calls it back to you.
- **Signs and bushes:** kick the ball hard at a sign for its message (fire closes it), at a bush for butterflies, at a target for a bonus. Crates break into a bonus when kicked twice.

Each level opens with KICK OFF, which holds the kid for about three seconds. A level's exit walks the kid off the right edge into the next level.

All five worlds are complete: England's streets, country and London; Italy's ruins, Venice and the Riviera; Russia's forest, the rocking battleship and Red Square; Japan's train and factory; the USA's Wild West, beach and city; the five bonus levels and the ALIEN.

## Sound

Each level plays its ProTracker module through the game's own replay routine, which I transcribed with its quirks, along with the KICK OFF, life-lost and game-over jingles and the bosses' music changes. The effects play on the fourth channel by the original's priority rule. With no audio device the game runs silent.

## Viewer

Keys in the viewer (`--viewer`): arrows scroll, PageUp/PageDown change level, `[` and `]` lay a graphics set's frames over the level and step through its sections (a debugging aid; `--section` starts with one shown). The window shows the 320x256 PAL frame with the game's 288x200 play area.

## Level designer

`skld` makes new levels on the setting of an original level: its tiles, its objects (each with the parameter the original gives it), its cell behaviours (coins, cards, exits, ladders, checkpoints, ...), backdrop, colours and music. It's built with the game and carries the same data inside (or takes `--roms` / `--data`).

```sh
skld --settings                          # list the settings
skld new SETTING FILE                    # a new level from a copy of the setting's map
skld new SETTING FILE --size WxH         # or an empty one: a floor, a start and an exit
skld FILE                                # open a level
soccerkid --custom FILE                  # play it (--start and --near work too)
```

The settings are the ordinary levels and the five bonus levels. The boss levels, the train, the battleship, the scoreboard and the ALIEN are left out: their code depends on their exact maps or their own scrolling.

A level is at least 22 × 13 cells (the game's scroll buffer and view) and at most 16,384 cells, the size of the game's cell-state buffer (the largest original, 546 × 30, uses 16,380).

The window shows the map as the game draws it, with the side panel on the right:

- **Tiles:** the setting's tiles. Painting a tile keeps the cell's object or behaviour, and gives the cell the animation state the original map uses for that tile. Dragging across the tiles makes a brush of several, and so does Alt + drag on the map (its empty cells are left out). A brush goes down with its top-left corner on the mouse, repeats on its own grid while you drag, and Shift + drag fills a rectangle with it.
- **Objects:** the setting's enemies and objects.
- **Specials:** the start position and the setting's cell behaviours.
- **Pieces:** the things the original map builds from several tiles, like LONDON 1's taxi, post box and lamp post, placed whole with one click. A click puts the piece's bottom-left corner on the cell under the mouse and changes only the piece's own cells, keeping their objects and behaviours.

skld finds the pieces itself from the setting's original map. Two tiles belong together when one's neighbour on a side is always the other, and the other's on the opposite side is always the one, at least twice in the map. The pieces then grow into the tiles around them that belong to them but repeat inside one thing, like a tree's trunk: a tile used at least twice joins where it touches a piece, as long as at least half the places it's used do and it isn't more than three times as common as the piece's tile it touches, so ground that's everywhere stays out. A grown piece bigger than 24 × 16 cells is ground after all and goes back to its parts. The map's three commonest tiles (sky, wall, ground) never join anything, and pieces of fewer than three tiles are left out. Something the original places only once can't be told apart from what happens to sit next to it, so it isn't a piece; take it from a copy of the original map with Alt + drag instead.

A cell holds one object or one behaviour, so placing one replaces the other. The checks at the bottom of the panel list what would stop the level from working (no start, no exit) and what may surprise you (several starts, a different card count from the original's, more objects on one screen than the game's 16 slots); click one to jump to it.

| input | action |
|---|---|
| left button | paint the selection |
| right button | erase (tiles on the Tiles and Pieces tabs, else objects and behaviours) |
| Shift + drag | fill a rectangle (right button: clear it) |
| Alt + click | pick the tile, object or behaviour under the mouse |
| drag on the tiles, Alt + drag on the map | a brush of several tiles |
| Ctrl + drag, Ctrl+C, Ctrl+V, Del | select, copy, paste at the mouse, clear the selection |
| middle drag, arrows, wheel | scroll (Shift: faster or sideways) |
| 1 2 3, Ctrl + wheel | zoom |
| Ctrl+Z, Ctrl+Y | undo, redo (256 steps) |
| Ctrl + arrow | add a column or row on that side (with Shift: remove one) |
| Tab | next panel tab |
| G, O, B, V | the grid, the object and behaviour labels, the solid parts of the tiles, the game's view around the mouse |
| F5, Shift+F5 | play from the start, or from the cell under the mouse; Escape comes back |
| Ctrl+S | save |
| F1 | the keys |

Saving writes a temporary file and renames it over the level, so a failed save never damages the old file. Quitting with unsaved changes asks first.

A `.skl` file is `SKLEVEL1`, the setting's data index, the width and height, an empty marker table (room for later), the cell words and their initial animation states. When a level is played, the setting's original level is loaded and its map replaced, so everything else (objects, behaviours, music, scrolling) runs exactly as in the original game: a copy of LONDON 1 plays frame for frame like LONDON 1. Under `--custom`, reaching the exit ends the run and losing every life restarts the level.

## Why no widescreen

The game is shown as the original showed it: a 288x200 play area in a 320x256 PAL frame, scaled up with borders. I've left widescreen out on purpose, because the game's rules are built around that width, not just its drawing:

- **Objects appear at the edges.** Enemies and pickups come to life as their columns scroll into view and are removed a set distance off-screen. A wider view would show them popping in and vanishing at the sides, and widening those margins changes how many enemies are active at once and when they start moving.
- **Boss arenas.** The bosses lock the camera so the arena exactly fills the original view. Any wider and you'd see past the walls into empty map.
- **Backdrops and scrolling.** Each backdrop is only as wide as the original view needs, and the train and battleship levels stream their scenery through a buffer sized for it.
- **Full-screen pictures.** The title, world maps, endings and intro are 320 pixels wide, so they would need side borders anyway.

It could be done as an option that leaves the original mode untouched (wider margins, and the original width in boss rooms and the streaming levels), but it would change how the game plays. For now I'd rather keep the port faithful.

## Test

```sh
ctest --test-dir build --output-on-failure                     # synthetic fixtures only
SOCCERKID_ROMS=/path/to/your/disks ctest --test-dir build      # plus the tests that read the real disks
```

`tests/fixtures/` holds synthetic test data, with no game bytes in it.
