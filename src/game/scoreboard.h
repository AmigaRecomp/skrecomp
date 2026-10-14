/* The scoreboard level (data index 12, re/docs/notes/between-levels.md section 6): scoreboard_setup (0x103DAC),
   scoreboard_update (0x1038E8, frame step 7), scoreboard_player_input (0x10C116), scoreboard_free (0x103F9A) and the level-end
   statistics that feed it (0x104558). */
#ifndef SK_GAME_SCOREBOARD_H
#define SK_GAME_SCOREBOARD_H

#include <stdint.h>

typedef struct sk_game sk_game;

void scoreboard_setup(sk_game *g);              /* at setup_level; nothing unless data index 12 */
void scoreboard_free(sk_game *g);               /* reset_level_state: the cards and the board's counters */
void scoreboard_update(sk_game *g);
int  scoreboard_player_input(sk_game *g, int input);   /* the kid's input in the scoreboard level */
void level_end_stats(sk_game *g);               /* the frame loop's exit: TIME and BALLS */
void scoreboard_text(sk_game *g, const char *s, int x, int y);   /* 0x103CFC: up to 15 dot-matrix glyphs */

#endif
