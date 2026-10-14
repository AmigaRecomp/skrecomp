/* The attract mode's game side (re/docs/notes/frontend.md 10.1, 10.2): the hi-score table written as tiles of the attract level
   (file077), the name entry and its letter strip in the HUD. The table itself lives in the image (default_hiscores 0x200934). */
#ifndef SK_GAME_ATTRACT_H
#define SK_GAME_ATTRACT_H

#include <stdint.h>

typedef struct sk_game sk_game;

typedef struct {
    int rank;                           /* 0x3002AA */
    int ac, ae, dir;                    /* 0x3002AC / 0x3002AE / 0x3002B0 */
    int cursor;                         /* 0x3002B1 */
    uint8_t blink, rep;                 /* 0x3002B2 / 0x3002B3 */
} sk_name_entry;

void hiscore_tiles(sk_game *g, int c0, int only_rank);         /* draw_hiscore_rows (only_rank < 0) / draw_hiscore_row */
void name_entry_start(sk_name_entry *ne, int rank);            /* attract_hiscores' set-up */
int  name_entry_step(sk_game *g, sk_name_entry *ne, int input); /* hiscore_name_input: -1 at END */
void name_entry_hud(sk_game *g, const sk_name_entry *ne);      /* hud_clear_bitmap + build/draw_letter_strip + the marker */

#endif
