/* The player record (0x302B16, 94 bytes; re/docs/notes/player.md "Player record") and the per-frame routines. */
#ifndef SK_GAME_PLAYER_H
#define SK_GAME_PLAYER_H

#include <stdbool.h>
#include <stdint.h>
#include "gfx/sprites.h"

typedef struct sk_game sk_game;

typedef struct sk_player {
    int16_t top_speed;      /* +0x00 */
    uint32_t score;         /* +0x02 BCD */
    int8_t lives;           /* +0x06 */
    uint8_t jumping;        /* +0x08 */
    uint8_t falling;        /* +0x09 */
    int16_t jump_hold;      /* +0x0C */
    int16_t vy;             /* +0x0E half pixels per frame, positive = up */
    int16_t x, y;           /* +0x10, +0x12 */
    int16_t prev_x, prev_y; /* +0x14, +0x16 */
    int16_t anim;           /* +0x18 */
    int16_t speed;          /* +0x1A quarter pixels per frame */
    int16_t facing;         /* +0x1C 1 right, -1 left */
    int16_t state;          /* +0x1E state * 4 */
    int16_t walk_acc;       /* +0x20: walk-cycle accumulator (+= speed, wraps at 64) */
    int16_t walk_step;      /* +0x22: walk-cycle frame 0..5 (2 while standing) */
    int16_t input;          /* +0x24 */
    int16_t prev_input;     /* +0x26 */
    int16_t idle_frames;    /* +0x2A */
    int16_t timer;          /* +0x2C */
    int16_t trick_phase;    /* +0x2E */
    int16_t fixed_vy;       /* +0x30 */
    int16_t carry_dx;       /* +0x32 */
    int16_t knock_dir;      /* +0x34 */
    int16_t gap_below;      /* +0x36 */
    int16_t push_up;        /* +0x38 */
    int16_t jump_y;         /* +0x3A */
    int16_t invulnerable;   /* +0x3E */
    uint8_t health;         /* +0x40 */
    uint8_t ball_free;      /* +0x41 */
    uint8_t skidding;       /* +0x42 */
    uint8_t controllable;   /* +0x43 */
    uint8_t jump_input;     /* +0x44 */
    uint8_t run_chain;      /* +0x45: a state handler asks for the movement chain to run after it */
    int8_t on_rope;         /* +0x46 */
    uint8_t pushing;        /* +0x47 (the edge teeter flag; see the ledger) */
    int8_t ball_hold;       /* +0x48 */
    uint8_t no_push_up;     /* +0x49 */
    uint8_t dying;          /* +0x4A */
    uint8_t trick;          /* +0x4B */
    uint8_t trick_flag;     /* +0x4C */
    uint8_t probe_flags;    /* +0x4D */
    uint8_t tricking;       /* +0x4E */
    uint8_t fire_lock;      /* +0x4F */
    uint8_t kick_phase;     /* +0x50 */
    uint8_t anim_hold;      /* +0x51 */
    uint8_t crouching;      /* +0x53 */
    uint8_t on_platform;    /* +0x54 */
    uint8_t landing;        /* +0x55 */
    uint8_t sliding;        /* +0x56 */
    uint8_t fall_flags;     /* +0x57 */
    uint8_t fire_held;      /* +0x58 */
    uint8_t down_held;      /* +0x59 */
    uint8_t exit_walk;      /* +0x5A */
    uint8_t powerup;        /* +0x5B */
    uint8_t death_phase;    /* +0x5C */
    uint8_t crushed;        /* +0x5D */
} sk_player;

void init_player(sk_game *g);               /* 0x10BC28 */
void player_input_state(sk_game *g);        /* 0x10A576: the per-frame routine (frame-loop step 26) */
void rebuild_player_mask(sk_game *g);       /* rebuild_collision_mask with the player's position */
void slope_probe_step(sk_game *g);          /* slope_probe -> top_speed */
void floor_probe_step(sk_game *g);          /* floor_probe -> gap_below / push_up */
void crouch_slide(sk_game *g);
void horizontal_move(sk_game *g);
void step_down(sk_game *g);
void push_up_and_walls(sk_game *g);
void slide_turn(sk_game *g);
void jump_control(sk_game *g);
void ceiling_check(sk_game *g);
int  ceiling_probe_at(sk_game *g, int x, int y);   /* the probe plus its side effects (bump, slide end); -1 = bump */
void edge_probe_step(sk_game *g);
int  hurt_player(sk_game *g);               /* -1 hit, -2 killed, -3 ignored */
void bounce_player(sk_game *g);
void kill_player(sk_game *g);               /* sub_10A780: instant death (deadly tiles) */
void freeze_death_target(sk_game *g);       /* sub_106C22: camera_target -> death_target_x/y */
void player_physics(sk_game *g);            /* the non-ball part of 0x10A80A */
void player_logic(sk_game *g);              /* 0x10B586: chooses player.anim */
void update_player(sk_game *g, sk_sprite *kid);   /* 0x10A4DC: player_logic + the kid's sprite placement */

/* Hooks defined by later modules (no-ops in stubs.c until then). */
void dispatch_player_state(sk_game *g);     /* player_states.c */
void player_tile_attrs(sk_game *g);         /* attrs.c */
void select_layers(sk_game *g);             /* attrs.c */
void camera_target(sk_game *g, int *tx, int *ty);   /* camera.c */

#endif
