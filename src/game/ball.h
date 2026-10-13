/* The ball (0x302B74, 34 bytes; re/docs/notes/player.md "Ball record") and its per-frame routines. Positions and
   velocities are 16.16 fixed point: the high word of x/y is the pixel position of the 16 x 16 ball's top-left. */
#ifndef SK_GAME_BALL_H
#define SK_GAME_BALL_H

#include <stdbool.h>
#include <stdint.h>
#include "gfx/sprites.h"

typedef struct sk_game sk_game;

typedef struct sk_ball {
    int32_t x, y, vx, vy;           /* +0x0 +0x4 +0x8 +0xC */
    int16_t prev_x, prev_y;         /* +0x10 +0x12 */
    int16_t speed, rest_timer;      /* +0x14 +0x16 */
    uint8_t contact_frames;         /* +0x18 */
    uint8_t at_rest;                /* +0x19 */
    uint8_t flag_1a;                /* +0x1A */
    uint8_t kick_timer;             /* +0x1B */
    uint8_t lifted;                 /* +0x1C */
    uint8_t vanishing;              /* +0x1D */
    uint8_t inactive;               /* +0x1E */
    uint8_t touched;                /* +0x1F */
    uint8_t kick_anim;              /* +0x20 */
} sk_ball;

#define SK_BALL_MASK_ROWS 96        /* ball_mask (0x552E70): 64 px wide, up to 6 tiles tall when above the map */

typedef struct sk_ball_world {      /* the globals the ball routines share */
    uint8_t mask[SK_BALL_MASK_ROWS * 8];
    int mask_x, mask_y;             /* ball_mask_origin_x/y (0x300216/0x300218) */
    bool mask_dirty;                /* 0x300360 */
    bool mask_valid;
    int32_t gravity;                /* ball_gravity (0x300008) = 0x4000 */
    int min_bounce;                 /* 0x3001D2 = 0x0A00 */
    int drag_div;                   /* 0x3001D4 = 0x20 */
    int roll_phase;                 /* ball_roll_phase (0x300222) */
    int carry_dx;                   /* ball_carry_dx (0x300280) */
    int contact_dx, contact_dy, contact_dir, in_dir;   /* 0x30021C 0x30021E 0x300220 ball_in_dir */
} sk_ball_world;

void ball_world_init(sk_ball_world *bw);       /* the start-up values (0x1003E2..) */
void update_ball(sk_game *g);                  /* 0x1098CA */
void ball_move(sk_game *g);                    /* 0x10A28E */
void ball_bounce(sk_game *g);                  /* 0x109C90 */
void rebuild_ball_mask(sk_game *g);            /* 0x109ACE */
void ball_tile_attrs(sk_game *g);              /* 0x11A620 */
bool ball_near_player(const sk_game *g);       /* 0x109978 */
void ball_sprites(sk_game *g, sk_sprite *spr); /* 0x10A2EA */
int  sin_256(int a);                           /* 0x105E6E (cos_256 of a + 0x40) */
int  cos_256(int a);                           /* 0x105E72: -64..64 from the 65-byte table 0x105E94 */
int  atan2_256(int dx, int dy);                /* 0x105ED6: direction 0..255 */

#endif
