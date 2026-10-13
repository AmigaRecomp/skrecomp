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
#define SK_SPAWNED_BIT 0x4000       /* bit 30 of the cell long: its object is alive (init_object_slot's bset #6 through +0x18) */
#define SK_MARKER_BIT 0x8000        /* bit 31 of the original cell long: a spawn marker (flags 1..31) */

typedef struct sk_game sk_game;

typedef struct sk_world {
    const sk_level *lv;
    int w, h;                       /* cells */
    uint16_t *live;                 /* live_map: the map words (high byte = flag*2 | index bit 8) */
    uint16_t *tile;                 /* fg_cell_table: tile index 0..511 | SK_MARKER_BIT */
    uint8_t *state;                 /* cell_state */
    int start_x, start_y;
    int fg_max_x, fg_max_y, bg_max_x, bg_max_y;
    int collectables_left;          /* cells whose attribute is 0x25 */
    uint8_t *bg; int bg_w, bg_h;    /* the backdrop's cells (bg_cell_table / 0x30EBA4), writable: the boss arena repaints it */
} sk_world;

bool world_build(sk_world *w, const sk_level *lv);     /* false = out of memory */
void world_free(sk_world *w);
void paint_bg_cells(sk_game *g, int x, int y, int tile, int n);   /* 0x11A49A: n backdrop cells in a row at pixel x, y */
void save_backdrop(sk_game *g);                                   /* 0x1154C2: the last screen's 20 x 14 cells over columns 0..19 */
void restore_backdrop(sk_game *g);                                /* 0x1154FA: and back */

static inline int world_flag(const sk_world *w, int cx, int cy) { return w->live[cy * w->w + cx] >> SK_FLAG_SHIFT; }
static inline int world_tile_index(const sk_world *w, int cx, int cy) { return w->tile[cy * w->w + cx] & SK_INDEX_MASK; }
static inline bool world_inside(const sk_world *w, int cx, int cy) { return cx >= 0 && cy >= 0 && cx < w->w && cy < w->h; }

int world_attr(const sk_world *w, int cx, int cy);                      /* attribute 0..94 or -1 */
int world_marker_type(const sk_world *w, int cx, int cy, int *param);   /* type byte or -1 (flags 1..31 only) */
const uint8_t *world_tail(const sk_world *w, int cx, int cy);           /* the cell's 32-byte solidity bitmask, or NULL */

#endif
