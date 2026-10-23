/* Playing a level: the scene (disks, tables, level, framebuffer) plus the gameplay state, stepped at 50 Hz. */
#ifndef SK_APP_PLAY_H
#define SK_APP_PLAY_H

#include <stdbool.h>
#include <stdint.h>
#include "app/args.h"
#include "app/scene.h"
#include "app/pictures.h"
#include "app/frontend.h"
#include "app/savescreen.h"
#include "app/intro.h"
#include "game/savegame.h"
#include "game/attract.h"
#include "game/between.h"
#include "game/game.h"
#include "game/resources.h"
#include "audio/sound.h"
#include "gfx/sprites.h"
#include "formats/custom.h"

typedef enum { SK_ATTRACT_NONE, SK_ATTRACT_PAN, SK_ATTRACT_NAME, SK_ATTRACT_DEMO, SK_ATTRACT_TUTORIAL } sk_attract;

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
    sk_pictures pic;                /* a picture mode (loading strip, world map, ending) runs instead of the level when on */
    sk_run next;                    /* what follows the loading strip */
    int cup_arg;                    /* --cup, -1 none */
    sk_intro *intro;                /* the intro, while it plays */
    sk_frontend fe;                 /* the frontend's screens, when on */
    bool quit;                      /* the frontend asked to leave the program */
    bool with_frontend;             /* started without --level / --new-game: the game returns to the title */
    sk_attract attract;             /* a frontend level running: the hi-score pan, the name entry, the demo, the tutorial */
    int attract_x, attract_dx, attract_frames;   /* 0x3002A6 / 0x3002A4 / attract_counter */
    int session_round;              /* level_session's d7: menu, pan, menu, pan, demo */
    bool saved_muted;               /* sfx_muted before a demo */
    sk_name_entry name;
    sk_savescreen save;             /* the save prompt or the RESUME list, when on */
    char save_path[512];
    char controls_path[512];        /* controls.cfg (the window's bindings), empty: not saved */
    bool resuming; sk_save_slot resume_slot;   /* RESUME GAME: the slot load_game_slot puts over the new game */
    int typed_key;                  /* last_ascii_key: this step's typed character */
    int typed_queue[8], typed_n;    /* the window's keys waiting for their frame */
    bool esc_key;                   /* Escape: a save screen cancels, a level closes its curtain, the frontend quits */
    bool pause_key;                 /* F10 or P: the pause */
    bool leaving;                   /* the fade before the title after ESC */
    unsigned fe_keys;               /* SK_FE_KEY_* held this frame (the window sets them) */
    sk_custom custom;               /* --custom: a private copy of the level, applied over its setting's map */
    bool has_custom, custom_applied;
    bool custom_done;               /* the custom level's exit was reached: the run ends */
    bool custom_quit_on_game_over, custom_game_over;   /* skld: a game over ends the run (and says so) */
    int custom_cleared;             /* cells whose flags the setting does not have */
} sk_play;

bool play_open(sk_play *p, const sk_args *a, sk_err *e);
/* Starts the level at a sequence position (pos >= 0) or by data index (pos < 0). */
bool play_start_level(sk_play *p, int pos, int data_index, sk_err *e);
void play_step(sk_play *p, uint8_t keyboard_input);     /* one game frame; the script overrides the keyboard */
void play_draw(sk_play *p);                             /* backdrop, foreground, objects into the framebuffer */
void play_present(sk_play *p, uint32_t *out);           /* SK_SCREEN_W x SK_SCREEN_H */
void play_close(sk_play *p);
void play_type(sk_play *p, int key);                    /* a typed key, one taken per frame */
int  play_next_typed(sk_play *p);                       /* 0 none */

/* One script token at *s: letters U D L R F or '.', then a frame count; advances past the comma. */
bool script_parse_step(const char **s, uint8_t *input, int *frames, sk_err *e);

#endif
