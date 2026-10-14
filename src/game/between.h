/* Between the levels (re/docs/notes/between-levels.md): play_level's sequence walk (0x104060), the world map's flight
   (plane_flight 0x11E778) and the scoreboard (scoreboard_setup 0x103DAC, scoreboard_update 0x1038E8). */
#ifndef SK_GAME_BETWEEN_H
#define SK_GAME_BETWEEN_H

#include "core/tables.h"

typedef enum { SK_RUN_LEVEL, SK_RUN_MAP, SK_RUN_ENDING, SK_RUN_GAME_END } sk_run_kind;
typedef struct { sk_run_kind kind; int data_index; int map_index; } sk_run;

/* What play_level runs at *level_number: a world map (30..35) or the end screens (29) take their slot (+= 1 inside),
   a tile level is left for its exit walk to move on, 42 ends the game. */
sk_run play_level_entry(const sk_tables *t, int *level_number);

#endif
