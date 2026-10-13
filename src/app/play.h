/* Playing a level: the scene (disks, tables, level, framebuffer) plus the gameplay state, stepped at 50 Hz. */
#ifndef SK_APP_PLAY_H
#define SK_APP_PLAY_H

#include <stdbool.h>
#include <stdint.h>
#include "app/args.h"
#include "app/scene.h"
#include "game/game.h"
#include "game/resources.h"
#include "audio/sound.h"
#include "gfx/sprites.h"

typedef struct sk_play {
    sk_scene scene;
    sk_game game;
    sk_sprite_layer sprites;
    sk_resources res;
    int level_pos;                  /* position in level_sequence, -1 when the level was given by data index */
    bool has_start; int start_level, start_x, start_y;   /* --start / --near: the kid's start on that level */
    const char *script;             /* --script, NULL = keyboard */
    const char *script_cursor;
    uint8_t script_input;
    int script_left;
    bool script_done;
    int game_over_frames;
    bool load_failed;               /* a level failed to load: the run stops (load_err says why) */
    sk_sound snd;                   /* the sound driver (one VBL of it per step) */
    struct sk_audio_out *audio;     /* the device, NULL headless */
    int16_t audio_buf[2 * 960];     /* the step's 1/50 s at 48 kHz */
    int fade_wait;                  /* frames of fade_music_out left before the next level loads (0x1040F2) */
    sk_err load_err;
} sk_play;

bool play_open(sk_play *p, const sk_args *a, sk_err *e);
/* Starts the level at a sequence position (pos >= 0) or by data index (pos < 0). */
bool play_start_level(sk_play *p, int pos, int data_index, sk_err *e);
void play_step(sk_play *p, uint8_t keyboard_input);     /* one game frame; the script overrides the keyboard */
void play_draw(sk_play *p);                             /* backdrop, foreground, objects into the framebuffer */
void play_present(sk_play *p, uint32_t *out);           /* SK_SCREEN_W x SK_SCREEN_H */
void play_close(sk_play *p);

/* One script token at *s: letters U D L R F or '.', then a frame count; advances past the comma. */
bool script_parse_step(const char **s, uint8_t *input, int *frames, sk_err *e);

#endif
