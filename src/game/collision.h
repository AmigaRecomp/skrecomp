/* The player's collision mask (collision_mask 0x552B70: 96 rows x 64 pixels, one bit per pixel) and the
   probes that read it. rebuild_collision_mask (0x10B2C2) blits the 16x16 solidity bitmasks (the 32-byte
   tile tails) of the 4 x 6 tiles around the kid; see re/docs/notes/player.md "Probes and collision". */
#ifndef SK_GAME_COLLISION_H
#define SK_GAME_COLLISION_H

#include <stdbool.h>
#include <stdint.h>
#include "game/world.h"

#define SK_MASK_W 64
#define SK_MASK_H 96

typedef struct sk_mask {
    uint8_t bits[SK_MASK_H * 8];        /* 8 bytes per row, bit 7 of byte 0 = leftmost pixel */
    int origin_x, origin_y;             /* mask_origin_x/y (world pixels of the top-left bit) */
    bool dirty;                         /* mask_dirty: a tile changed */
    bool built;                         /* 0x3001EC: already rebuilt this frame */
    void (*merge)(struct sk_mask *, void *ctx);   /* merge_platform_masks hook (5b) */
    void *merge_ctx;
} sk_mask;

/* rebuild_collision_mask: (x, y) = the player's position. Skipped when built this frame, or when the
   origin is unchanged and the mask is not dirty. Rows above the map are solid; cells outside are empty. */
void mask_rebuild(sk_mask *m, const sk_world *w, int x, int y);
/* A world pixel's solidity; false outside the 64 x 96 window. */
bool mask_solid(const sk_mask *m, int x, int y);

/* The probes take the player's world position (x, y): the kid's feet are at y + 40. */
int  probe_ground(const sk_mask *m, int x, int y);                            /* ground_probe: solid rows from y+41, 0..16 */
void probe_floor(const sk_mask *m, int x, int y, int *gap_below, int *push_up);   /* floor_probe */
int  probe_slope(const sk_mask *m, int x, int y, int facing);                 /* slope_probe: 16, 18 or 23 */
/* wall_probe: probe_x = x - 8 (dir < 0) or x + 8 (dir >= 0). Returns -1 when a wall was found; then
   *player_x was pushed away from it (up to 4 px) and *speed cleared when the kid faces the wall. */
int  probe_wall(const sk_mask *m, int probe_x, int y, int dir, int facing, bool sliding, int16_t *player_x, int16_t *speed);
#define SK_CEIL_BUMP 1              /* ten solid rows above the head: the caller bumps */
#define SK_CEIL_SLIDE_BLOCKED 2     /* while sliding: something above the kid */
int  probe_ceiling(const sk_mask *m, int x, int y, int facing, bool sliding);   /* ceiling_probe */
bool probe_edge(const sk_mask *m, int x, int y, int facing);                  /* edge_probe: ground within 15 rows ahead of the feet */
int  probe_surroundings(const sk_mask *m, int x, int y, int facing, bool sliding);   /* bit 6 wall either side, bit 7 ceiling */

#endif
