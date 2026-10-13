/* The live level state the gameplay reads: live_map (0x306B9E), fg_cell_table (0x316C4A), cell_state
   (0x302B9E), the start position and the scroll limits. Built by build_fg_cell_table (0x10969A),
   find_start_position (0x10385C) and the scroll-limit routines; see re/docs/notes/gameplay.md Memory map. */
#ifndef SK_GAME_WORLD_H
#define SK_GAME_WORLD_H

#include <stdbool.h>
#include <stdint.h>
#include "formats/level.h"

/* PALS payload offsets (the chunk after its tag; the game's level_pals points at the tag, so it reads +5, +100, +133) */
#define SK_PALS_ATTR 1              /* + (flag - 33): the attribute of flags 33..127 */
#define SK_PALS_TYPE 96             /* + flag: the object type of marker flags 1..31 */
#define SK_PALS_PARAM 129           /* + flag: its spawn parameter */
#define SK_MARKER_BIT 0x8000        /* bit 31 of the original cell long: a spawn marker (flags 1..31) */

typedef struct sk_world {
    const sk_level *lv;
    int w, h;                       /* cells */
    uint16_t *live;                 /* live_map: the map words (high byte = flag*2 | index bit 8) */
    uint16_t *tile;                 /* fg_cell_table: tile index 0..511 | SK_MARKER_BIT */
    uint8_t *state;                 /* cell_state */
    int start_x, start_y;
    int fg_max_x, fg_max_y, bg_max_x, bg_max_y;
    int collectables_left;          /* cells whose attribute is 0x25 */
} sk_world;

bool world_build(sk_world *w, const sk_level *lv);     /* false = out of memory */
void world_free(sk_world *w);

static inline int world_flag(const sk_world *w, int cx, int cy) { return w->live[cy * w->w + cx] >> SK_FLAG_SHIFT; }
static inline int world_tile_index(const sk_world *w, int cx, int cy) { return w->tile[cy * w->w + cx] & SK_INDEX_MASK; }
static inline bool world_inside(const sk_world *w, int cx, int cy) { return cx >= 0 && cy >= 0 && cx < w->w && cy < w->h; }

int world_attr(const sk_world *w, int cx, int cy);                      /* attribute 0..94 or -1 */
int world_marker_type(const sk_world *w, int cx, int cy, int *param);   /* type byte or -1 (flags 1..31 only) */
const uint8_t *world_tail(const sk_world *w, int cx, int cy);           /* the cell's 32-byte solidity bitmask, or NULL */

#endif
