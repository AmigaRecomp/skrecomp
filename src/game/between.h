/* Between the levels (re/docs/notes/between-levels.md): play_level's sequence walk (0x104060), the world map's flight
   (plane_flight 0x11E778) and the scoreboard (scoreboard_setup 0x103DAC, scoreboard_update 0x1038E8). */
#ifndef SK_GAME_BETWEEN_H
#define SK_GAME_BETWEEN_H

#include <stdbool.h>
#include <stdint.h>
#include "core/tables.h"

typedef enum { SK_RUN_LEVEL, SK_RUN_MAP, SK_RUN_ENDING, SK_RUN_GAME_END } sk_run_kind;
typedef struct { sk_run_kind kind; int data_index; int map_index; } sk_run;

/* What play_level runs at *level_number: a world map (30..35) or the end screens (29) take their slot (+= 1 inside),
   a tile level is left for its exit walk to move on, 42 ends the game. */
sk_run play_level_entry(const sk_tables *t, int *level_number);

typedef struct sk_game sk_game;
void bonus_check(sk_game *g);                    /* 0x11EF1C: with bonus_pending, the world's bonus level next */

/* plane_flight (0x11E778): the aircraft marker, 16.16, one pixel a frame along atan2_256's heading, always east. */
typedef struct { int32_t x, y, sx, sy; int tx; int big_off, small_off; } sk_flight;
void flight_init(sk_flight *f, int from_x, int from_y, int to_x, int to_y);
bool flight_step(sk_flight *f);                  /* one frame's move; true when it reached the target (left drawn there) */
int  flight_draw_x(const sk_flight *f);          /* the marker's x in the picture, 0..319 */

#endif
