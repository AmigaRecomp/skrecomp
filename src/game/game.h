/* sk_game: the original's gameplay memory as one struct (addresses in re/docs/notes/gameplay.md Memory map).
   Modules add their own sub-structs; everything is plain data so a test can build one from a fixture level. */
#ifndef SK_GAME_GAME_H
#define SK_GAME_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "core/hunk.h"
#include "core/tables.h"
#include "game/collision.h"
#include "game/input.h"
#include "game/attrs.h"
#include "game/objects.h"
#include "game/player.h"
#include "game/world.h"

typedef struct sk_game sk_game;

typedef struct sk_ball {                /* placeholder for 5b: the fields the player code touches */
    int32_t x, y, vx, vy;               /* 16.16 */
    uint8_t at_rest, inactive, touched;
} sk_ball;

typedef struct sk_view {
    int cam_x, cam_y;                   /* view block +0x14/+0x16: the camera the strips were built for */
    int bg_x, bg_y;
    bool full_redraw;                   /* view +0x22: rebuild everything (level start) */
} sk_view;

struct sk_game {
    const sk_image *sk;                 /* sk.pak image for the data tables; NULL in fixture tests */
    const sk_tables *tables;
    const struct sk_resources *res;     /* slot_table for this level; NULL in fixture tests */
    sk_world world;
    sk_mask mask;
    sk_view view;
    sk_player player;
    sk_ball ball;
    sk_objects objs;
    sk_attrs attrs;
    int object_count;                   /* 0x300228 */
    bool untouchable;                   /* bytes 0x30033A / 0x300382: touch_player ignores the kid */
    uint8_t input_state;                /* the five SK_IN_ bits (0x300387) */
    int frame_ticks, frame_counter;     /* 0x30020E, 0x300064 */
    int camera_x, camera_y, camera_y_raw;   /* 0x3001A2, 0x3001A4, 0x3001BA */
    int shake_amplitude, push_x, look_x;    /* 0x30026A, 0x300268, 0x300242 */
    int cam_ease;                       /* 0x300284: vertical follow step (READING: 4) */
    int scan_col_left, scan_col_cx, scan_col_cy0;   /* 0x300020/0x300022: pending edge columns */
    int scan_row_left, scan_row_cy, scan_row_cx0;   /* 0x300026/0x300028: pending edge rows */
    bool scrolled_right, scrolled_down;             /* 0x30034E / 0x30034F */
    int death_target_x, death_target_y; /* 0x30024A/0x30024C: the camera target frozen by freeze_death_target (sub_106C22) */
    int edge_cache;                     /* 0x3001F4: x the edge probe last ran for */
    int scroll_mode;                    /* 0x30035C; only 0 in 5a */
    uint8_t layer_flags;                /* 0x300363 */
    uint8_t layer_flags_record;         /* 0x300380: level record byte 3 */
    uint16_t bplcon2_value;             /* 0x30028A */
    uint8_t layer_zone;                 /* 0x30038E */
    int kit;                            /* 0x300264: level record byte 4 (the respawn kit) */
    int kit_current;                    /* 0x300262: the kit the sprite colours show */
    uint32_t level_timer;               /* 0x3002BA, BCD */
    int timer_tick, timer_rate;         /* 0x3002C2, 0x3002C4 */
    uint8_t timer_flags;                /* 0x300354: bit 7 running, bit 0 changed */
    bool timeout_armed;                 /* 0x30038F */
    int powerup_timer;                  /* 0x3002F6 */
    uint8_t level_outcome;              /* 0x30034B */
    uint8_t game_flags;                 /* 0x30034C */
    uint8_t curtain_flags;              /* 0x300212 */
    int curtain_step;                   /* 0x30020A */
    int curtain_top, curtain_bottom;    /* play-area rows hidden by the DIW words */
    bool level_end;                     /* 0x300214 */
    int level_number, level_data_index; /* 0x30022E, 0x300230 */
    uint8_t hud_redraw;                 /* bit 0 health, 1 lives, 2 score, 3 cards (0x300350..3) */
    int sfx_last, sfx_count;            /* play_sfx hook */
    bool jingle_requested; int jingle;  /* spawn_jingle hook */
    bool music_requested; int music_request;   /* music_request hook (tune number) */
    uint8_t death_flags;                /* 0x300349: bit 0 = the death's life already taken */
    bool music_stopped;
    void (*step_probe)(sk_game *, int step);
};

void play_sfx(sk_game *g, int n);

#endif
