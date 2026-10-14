# Soccer Kid port

A native rewrite of Soccer Kid (Amiga AGA, Krisalis 1993) in C with SDL3. The game's own data
files are read at run time from the original disk images; nothing from the disks is stored
in this repository. The reverse-engineering notes and tools that this code is written from
live in the sibling repository `../re`.

## Build

Requirements: a C11 compiler, CMake 3.24 or newer, SDL3 and zlib (on macOS:
`brew install sdl3` and the system zlib).

```sh
cmake -B build -G Ninja
cmake --build build                          # builds skextract, skembed and soccerkid
build/skextract ../SourceROMS                # your five disks (.adf or .zip) -> data/soccerkid.dat
cmake --build build                          # again: soccerkid now carries the game inside
build/soccerkid                              # the whole game, no other files needed
```

`skextract` reads the files the game needs from your own disk images into `data/soccerkid.dat` (about 4 MB); the
next build embeds that file in the `soccerkid` program, so the one binary is the complete game. The repository never
holds any of it (`data/` is ignored). Without the file the program is built empty and asks for `--roms <dir>` (the disk
images) or `--data FILE` (a `soccerkid.dat`) instead.

## Run

With the data built in, `soccerkid` needs no arguments. Otherwise point it at a directory holding the five disk
images, either as `.adf` files or as `.zip` archives containing one `.adf` each (`--roms`); the images are
recognised by their volume names `SKDisk0` to `SKDisk4`, not by file name.

```sh
build/soccerkid --roms ../SourceROMS                                  # the game: the intro, the language screen, the title
build/soccerkid --roms ../SourceROMS --no-intro                       # start at the language screen
build/soccerkid --roms ../SourceROMS --new-game                       # straight into a new game at the first world map
build/soccerkid --roms ../SourceROMS --level 0                        # play level 0 in a window
build/soccerkid --roms ../SourceROMS --level 0 --frames 300 --script R150,RU20,R130 --screenshot out.png
                                                                      # play 300 frames headless from a script, then a PNG
build/soccerkid --roms ../SourceROMS --level 0 --viewer               # the level viewer (scrolls, no gameplay)
build/soccerkid --roms ../SourceROMS --level 17 --viewer --section file036:0   # show a graphics set's frames
build/soccerkid --help                                                # every option
```

Without `--level` the program starts as the original does: the intro (the alien Scab's story, about two minutes;
any key, the mouse or fire skips it at its next text or animation), then the language screen (the language
of the in-game messages), then the title with START GAME, KIT DESIGN (the kid's shirt and shorts) and OPTIONS (music,
sound, the tutorial, RESUME GAME). Left alone, the title shows the hi-score table and the demo level; a good score asks
for a name; before each new world the game offers to save (the file is kept in your user data folder, or `--save FILE`).
A game then runs as the original's level sequence: the loading strip, the world map with the aircraft
flying to the next country (and the cup pieces collected so far), each pair of levels, the scoreboard that counts the
cards, balls, time and trick kicks into a bonus (all 11 cards of a pair open the world's bonus level: a minute to take
50 tokens, then the cup piece), the ALIEN (it sends the kid home without the five cup pieces and fights with them), and
the end screen, then back to the title. `--cup 31` starts with all five pieces. `--level N` plays one level by its data index (0 = England 1 ... 28,
12 = the scoreboard) and moves on along the sequence from there; `--section
fileNNN:S[:WxH]` shows the frames of a graphics-set section from the level's disk (the frame
size is estimated from the data unless given); `--cam X,Y` sets the camera; `--no-backdrop`
and `--no-frames` reduce the picture to one layer.

Playing: arrows or WASD move, up jumps, down crouches (down while running slides), Space,
Left Ctrl or X is fire, `F` toggles fullscreen, `M` mutes the sound, F10 or `P` pauses (with the level's help box).
Escape in a game closes the curtain and goes back to the title (as the original); on the title, the intro or the attract
screens it quits the program.

Gamepads (any controller SDL knows: Xbox, PlayStation, Switch Pro, ...) work out of the box: d-pad or left stick to move,
A to jump, B or X for fire, Start to pause, Back to leave. F12 opens the remap screen: press a key or pad button for each
action in turn (Backspace or the pad's Back keeps one, F5 restores the defaults, Escape cancels; F, M, TAB, F1..F12 stay fixed, and an input moved to a new action leaves its old one). The bindings are kept in `controls.cfg`
in your user data folder (or `--controls FILE`), a text file you can also edit. On the title, TAB shows the version
and F1..F10 in KIT DESIGN copy a hi-score's kit; typing names a save. The game runs at the original's
50 frames per second with the original's movement, camera, collision and level flow
(`../re/docs/notes/gameplay.md`, `player.md`); the kid and the ball are drawn as the hardware
sprite pairs the game uses, in the level's kit colours, and the HUD shows health, the timer, the
score, the card pieces and the lives.

Sound: each level plays its ProTracker module through the game's own replay routine (transcribed: its
quirks included), with the KICK OFF, life-lost and game-over jingles and the bosses' music changes; the
effects play on the fourth channel by the original's priority rule (`../re/docs/notes/audio.md`). With no
audio device the game runs silent.

The ball (all five worlds are complete, every level of the game: England's streets, country and London with LONDON2's boss;
Italy's Ruins, Venice and the Riviera with RIVIERA2's boss; Russia's forest, the rocking battle ship with its rope and RED SQUARE
with its boss; Japan's TRAIN whose scenery streams past through tunnels and the FACTORY with FACTORY2's boss; the USA's Wild West,
the beach and the city with the last boss, the five bonus levels and the ALIEN):
- **Kick:** fire with left or right kicks the ball at your feet; kicked into an enemy it knocks
  it out for points. Jumping onto the ball with down held stands you on it.
- **Tricks:** with the ball at your feet, hold fire still for a moment: the kid flicks it up and
  juggles it; a direction while juggling picks the next trick (overhead, volleys, the spring off
  the ball with up). Holding fire without the ball calls it back to you.
- **Signs and bushes:** kick the ball hard at a sign for its message (fire closes it), at a bush
  for butterflies, at a target for a bonus. Crates break into a bonus when kicked twice.

Each level opens with KICK OFF, which holds the kid for about three seconds. A level's exit tile
walks the kid off the right edge into the next level; game over restarts the level.

`--script` drives the kid without a keyboard: tokens `<keys><frames>` joined by commas, keys
`U D L R F` or `.` for none (`R150,RU20,R130` = run right, jump, run on). With `--frames N` and
`--screenshot` no window opens.

Keys in the viewer (`--viewer`): arrows scroll, PageUp/PageDown change level, `[` and `]`
change the graphics-set section shown. The window shows the 320x256 PAL frame with the game's
288x200 play area; the HUD panel is not drawn yet.

## Test

```sh
ctest --test-dir build --output-on-failure                 # synthetic fixtures only
SOCCERKID_ROMS=/abs/path/SourceROMS ctest --test-dir build  # plus the tests that read the real disks
```

`tests/fixtures/` holds synthetic test data (no game bytes), generated by
`uv run python tools/make_port_fixtures.py` from the `re` toolkit.
