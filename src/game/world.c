#include <stdlib.h>
#include <string.h>
#include "game/world.h"

static int clamp0(int v) { return v > 0 ? v : 0; }

bool world_build(sk_world *w, const sk_level *lv) {
    memset(w, 0, sizeof *w);
    w->lv = lv;
    w->w = lv->map.width;
    w->h = lv->map.height;
    size_t n = (size_t)w->w * (size_t)w->h;
    w->live = malloc(n * sizeof *w->live);
    w->tile = malloc(n * sizeof *w->tile);
    w->state = calloc(n ? n : 1, 1);
    if (!w->live || !w->tile || !w->state) { world_free(w); return false; }
    w->start_x = 16;                                        /* find_start_position: 16, 16 - 24 when no tile 0x1FF */
    w->start_y = 16 - 24;
    bool found = false;
    for (int cy = 0; cy < w->h; cy++)
        for (int cx = 0; cx < w->w; cx++) {
            uint16_t word = level_cell(lv, cx, cy);
            size_t i = (size_t)cy * (size_t)w->w + (size_t)cx;
            w->live[i] = word;
            uint16_t t = word & SK_INDEX_MASK;
            unsigned flags = word & 0xFE00u;
            if (flags && flags < 0x4000u) t |= SK_MARKER_BIT;       /* flags 1..31: spawn markers */
            w->tile[i] = t;
            if (flags >= 0x4200u && world_attr(w, cx, cy) == 0x25) w->collectables_left++;
            if (!found && (word & SK_INDEX_MASK) == 0x1FF) {
                found = true;
                w->start_x = cx * 16;
                w->start_y = cy * 16 - 24;
            }
        }
    w->fg_max_x = clamp0(w->w * 16 - 352);
    w->fg_max_y = clamp0(w->h * 16 - 200);
    w->bg_max_x = clamp0(lv->backdrop.width * 16 - 320);
    w->bg_max_y = clamp0(lv->backdrop.height * 16 - 200);
    return true;
}

void world_free(sk_world *w) {
    free(w->live);
    free(w->tile);
    free(w->state);
    memset(w, 0, sizeof *w);
}

int world_attr(const sk_world *w, int cx, int cy) {
    int f = world_flag(w, cx, cy);
    if (f < 33) return -1;                                  /* flag 32 is neither marker nor attribute */
    size_t i = 5 + (size_t)(f - 33);
    return i < w->lv->pals_len ? w->lv->pals[i] : -1;
}

int world_marker_type(const sk_world *w, int cx, int cy, int *param) {
    int f = world_flag(w, cx, cy);
    if (f < 1 || f > 31) return -1;
    if (133 + (size_t)f >= w->lv->pals_len) return -1;
    *param = w->lv->pals[133 + f];
    return w->lv->pals[100 + f];
}

const uint8_t *world_tail(const sk_world *w, int cx, int cy) {
    int t = world_tile_index(w, cx, cy);
    if (!w->lv->tiles_a.tails || t >= w->lv->tiles_a.count) return NULL;
    return w->lv->tiles_a.tails + (size_t)t * SK_TILE_TAIL;
}
