/* The two scroll modes (scroll_mode 0x30035C; objects.md "JAPAN TRAIN scrolling"): BATTLE SHIP (2) rocks and its
   backdrop drifts; TRAIN (1) streams its backdrop past at 0..10 px a frame with tunnel phases that fade the palette. */
#ifndef SK_GAME_SCROLL_MODES_H
#define SK_GAME_SCROLL_MODES_H

#include "game/game.h"

void scroll_modes_setup(sk_game *g, int data_index);   /* 0x104190..0x104234 */
void scroll_modes_frame(sk_game *g);                   /* sub_1049C8 (mode 1) / sub_104BF0 (mode 2) */
void backdrop_stream_step(sk_game *g);                 /* sub_1068EE: view.bg_x and the column ring */
int  backdrop_stream_column(const sk_game *g, int buffer_col);   /* the backdrop map column a buffer column shows */
uint16_t train_scale_colour(uint16_t c, int level);    /* sub_104DF6: step level of 16 (component * (level + 1) / 16) */

#endif
