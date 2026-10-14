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
#include "game/ball.h"
#include "game/hud.h"
#include "game/platforms.h"
#include "game/objects.h"
#include "game/player.h"
#include "game/world.h"

typedef struct sk_game sk_game;


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
    sk_ball_world bw;
    sk_objects objs;
    sk_attrs attrs;
    int object_count;                   /* 0x300228 */
    bool untouchable;                   /* byte 0x30033A: touch_player ignores the kid */
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
    bool music_requested; int music_module, music_request;   /* the last music request: SK_MUSIC_LEVEL / SK_MUSIC_JINGLE, the tune */
    int music_slot_last, jingle_loads, jingle_frees;   /* for the tests */
    struct sk_sound *snd;
    struct { bool used; int16_t x, y, h; const uint8_t *mask; size_t mask_len; } occl[4];   /* the sprite occluders 0x3007BA (types 40, 41) */
    int occl_count;                     /* 0x30038A */
    uint32_t rng;                       /* 0x30000C: sub_102B42's shift register, seeded once at start-up */
    int8_t train_fade;                  /* 0x30035D: JAPAN TRAIN's palette fade in progress (sub_1049C8) */
    int train_timer;                    /* 0x30026C: frames to TRAIN's next phase (bright / tunnel) */
    int train_level;                    /* the brightness step apply_fade_step last copied: 4..15 (15 = the level's own colours) */
    int16_t hoop_count;                 /* 0x30028C: CITY 2's baskets (attribute 36); negative after the eighth */
    int bg_speed;                       /* 0x3002DE: the self-scrolling backdrop's speed, px per frame (scroll modes 1, 2) */
    struct { int last, cursor, width, base; uint16_t col[32]; } stream;   /* 0x3002CE / D2 / D4 / D6 and the strip cache (0x3007E2) as a column ring */
    uint8_t post_flag;                  /* 0x300379: bit 7 the side of attribute 78 / 79 the kid is on; nonzero, the oncoming train (type 23) runs */
    uint8_t fboss_wobble, fboss_drop;   /* 0x300383 / 0x300384: FACTORY 2's boss shakes the rocks loose (type 106) */
    uint8_t train_flags;                /* 0x30035B */               /* the sound driver; NULL plays nothing (tests) */
    uint8_t kill_kind;                  /* 0x30037F: 1 pick-up kick, 2 volley, 3 overhead (kill statistics) */
    uint8_t kills[4];                   /* 0x300373 / 0x300375 / 0x300374 by kill_kind 1..3 */
    uint32_t score_value;               /* 0x300050: the score the type-113 popup shows */
    bool player_hidden;                 /* 0x300382: set by the attribute handler at 0x11BE3E (not in England 1/2); ball_overlap and touch_player skip */
    uint8_t ball_kicked_flag;           /* 0x30037E: only ever cleared in the original (the ball_kill_check paths are dead) */
    uint32_t timer_start;               /* 0x3002BE: the level record's timer, bonus kind 2 adds 0x15 */
    uint8_t extra_heart;                /* 0x300366 */
    int flutter_count;                  /* 0x3002F8: live butterflies */
    int fulltime_level;                 /* 0x30029A: FULL TIME's next level (scoreboard) */
    uint8_t cup_pieces;                 /* 0x300394: bit k = world k's cup piece (type 81); 0x1F = all five */
    uint8_t piece_collected;            /* 0x300298: a piece since the last map (blocks a bonus level) */
    uint8_t bonus_pending;              /* 0x300299: scoreboard_setup found 11 cards */
    int bonus_tokens;                   /* 0x300296: attribute 80's tokens taken (0x32 = all) */
    uint8_t token_digits[2];            /* 0x11A3DE: type 85's two characters (0x5D + digit) */
    uint8_t token_bits[0x104];
    uint8_t demo_banner[0x208];
    bool demo_fire_released;
    bool paused; int pause_frames;      /* F10: sub_1045E4's loop (0x3002E8 counts the box's 100 frames) */
    uint8_t pause_timer_flags; int pause_box_kind;            /* port: fire seen up since the demo began (the original's load gave time to let go) */         /* dat_300058: type 102's 64 x 13 bob, four planes and the mask */          /* dat_300070: type 85's 32 x 13 bob, four planes and the mask */
    struct {                            /* the ALIEN's parts (type 82): 0x300090 / 0x30007C / 0x300084 and their draw records */
        sk_slot *head, *piece_a, *piece_b;
        sk_draw *head_draw, *pa_draw, *pb_draw;
        int8_t hand_a, hand_b;          /* 0x300095 / 0x300094: hand frame overrides, -1 = by the arm's angle */
        int16_t arm_drop;               /* 0x300096 */
    } alien;
    uint8_t stat_balls, stat_time;      /* 0x300371, 0x300372 (BCD): the scoreboard's BALLS and TIME (CARDS is cards_taken) */
    uint8_t balls_allotted;             /* 0x300376: the level record's +6 */
    struct {                            /* the scoreboard (between-levels.md section 6) */
        bool active;                    /* board_font is allocated (scoreboard_setup ran) */
        uint8_t state;                  /* 0x300362: 0 walk in, 0xFE fetch the ball, 0xFF juggle */
        int8_t phase;                   /* 0x30036B: 0..5 rows, -1.. the bonus digits, 0x7F the end */
        int8_t wait, delay;             /* 0x30036C, 0x30036D */
        uint8_t draw;                   /* 0x30036E: the label is drawn twice (two buffers) */
        uint8_t juggle_wait;            /* player +0x52 */
        uint8_t juggle;                 /* player +0x5C: the juggle script's index */
        uint32_t bonus;                 /* 0x3000F2 (BCD) */
        uint16_t counter;               /* 0x3002DC */
        int ticker_pos, ticker_len;     /* 0x3002D8, 0x3002DA */
        char ticker[160];               /* 0x3020BE */
        char value[8];                  /* 0x203E3A: "VVo" + the points */
        int border_x[2], border_y;      /* where place_tile_column put the frame this frame (world pixels) */
        uint8_t text[256][40];          /* the dots scoreboard_text wrote into playfield 1's plane 0, by window pixel */
        uint8_t text_set[256][40];      /* which pixels it wrote (a glyph cell replaces 16 pixels of 8 even lines) */
    } board;
    uint8_t banner[0x514];              /* 0x300054: the jingle banner, 160 x 13, four planes and the mask */
    uint8_t hud_card_flag;              /* 0x3001E8: a card piece was taken; the HUD counts it */
    int message_box_timer, message_box_kind, message_box_request;   /* 0x3001E0 / E2 / E4 */
    uint8_t message_shown[8];           /* the flags in the table at 0x200DA4: each message once a level */
    int message_box_shown_kind;         /* the box on screen (drawn by the HUD code) */
    uint8_t message_wait;               /* 0x3001DE: frame step 31 holds the game until fire */
    int flutter_max, flutter_last_cell; /* 0x300288 (5), 0x3002FA */
    uint8_t ball_target[8];             /* 0x300395 + n: hits per target attribute (67 68 69 70 71 72 73 74) */
#define SK_BUMP_RESTORES 24
    struct { int cell; uint16_t word; } bump_list[SK_BUMP_RESTORES];   /* 0x300DAE: replayed by init_player */
    int bump_restores;
    int flush_list[16], flush_count;    /* 0x302856 / 0x30022A (attribute 24) */
    int bump_tile;                      /* the tile copied into bump_bob (0x427CFC) */
    uint8_t bump_bob[0xA0];             /* 0x427CFE: 16 x 16, four planes and a full mask */
    sk_hud hud;                         /* the panel (hud_bitmap_ptr) */
    int cards, cards_taken;             /* 0x3001E6, 0x300370 */
    uint8_t all_cards;                  /* 0x3001E9 */
    int card_anim[11];                  /* 0x301AE8 */
    uint8_t msg_bits[256 * 72 / 8 * 5]; /* 0x300030: the message box bob */
    int msg_w, msg_h;
    uint8_t msg_visible;
    sk_sprite *ball_sprite;             /* where ball_sprites (step 23) writes; NULL = not drawn */
    sk_platform platforms[SK_PLATFORMS]; int platform_count;   /* 0x3028D6 / 0x30022C */
    uint8_t flash_timer;                /* 0x300364: the screen flash, 16 frames */
    uint8_t splash_done;                /* 0x300365: attribute 55 fired (cleared on a respawn) */
    uint16_t tile_anim[256];            /* 0x301AFE: the frame offset (0x80 a tile) for each cell_state value */
    int anim_phase, anim_wait[8];       /* 0x3001F6, 0x3001FA.. */
    int flash_table;                    /* 0x30004C: 0 = the fade at 0x3012BA, 1 = 0x30169A */
    int8_t kick_result;                 /* 0x300338 */
    int32_t trick8_bias;                /* 0x30029C */
    int throw_x, throw_y;               /* 0x3002E4/E6: where the recalled ball appears */
    uint8_t throw_ready;                /* 0x300381 */
    uint8_t trick_block;                /* 0x30037A: blocks the tricks entry (set by a non-England object) */
    int kick_cooldown;                  /* 0x3001EA = 200 at a kick */
    int recalls;                        /* 0x300378 */
    int kit_current, kit_extra;         /* 0x300262 / 0x300266: the kid's and the ball's kit shades */
    uint8_t death_flags;                /* 0x300349: bit 0 = the death's life already taken */
    bool music_stopped;
    void (*step_probe)(sk_game *, int step);
};

uint32_t game_random(sk_game *g);           /* sub_102B42 */
void play_sfx(sk_game *g, int n);                   /* play_sfx 0x102140 */
void game_music(sk_game *g, int module, int tune);  /* music_request 0x100F80 */
void game_music_slot(sk_game *g, int slot);         /* sub_103770 */
void game_jingle_load(sk_game *g);                  /* sub_1037C4 */
void game_jingle_free(sk_game *g);                  /* 0x111C90 */

#endif
