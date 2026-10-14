/* Level flow: setup_level (0x104176) + the gameplay part of load_level, one iteration of game_frame_loop (0x1042DA),
   run_curtain (0x10471A), the VBL's level timer, the time-out check (sub_11EF7C) (play_level's sequence walk is between.c). */
#ifndef SK_GAME_LEVEL_FLOW_H
#define SK_GAME_LEVEL_FLOW_H

#include <stdbool.h>
#include <stdint.h>
#include "core/tables.h"
#include "formats/level.h"
#include "gfx/sprites.h"

typedef struct sk_game sk_game;

typedef enum { SK_OUTCOME_RUNNING, SK_OUTCOME_NEXT_LEVEL, SK_OUTCOME_GAME_OVER } sk_outcome;

void game_new_game(sk_game *g);                  /* lives 3, score 0, level_number 0 */
bool game_setup_level(sk_game *g, const sk_level *lv, int data_index);   /* builds the world; false = out of memory */
void game_close(sk_game *g);                     /* frees the world */
void tile_anim_step(sk_game *g);          /* sub_1094EA: the 8 animation groups (frame step 19) */
uint16_t fade_colour(uint16_t from, uint16_t to, int step);   /* build_fade_tables (0x104D74 / sub_104E40): step 0..15 */
void game_frame_step(sk_game *g, uint8_t input, sk_sprite *kid);   /* the VBL work + the frame loop's steps; sets level_end */
void run_curtain(sk_game *g);
int  curtain_rows(const sk_game *g);             /* play-area rows hidden at the top (and the bottom) */
void timer_tick(sk_game *g);                     /* the VBL's level_timer decrement */
void timeout_check(sk_game *g);                  /* frame-loop step 8 */
sk_outcome game_outcome(const sk_game *g);

#endif
