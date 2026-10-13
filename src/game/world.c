#include <stdlib.h>
#include <string.h>
#include "game/world.h"
#include "game/game.h"

static int clamp0(int v) { return v > 0 ? v : 0; }

/* A byte of the GRAF A chunk's tile units (160 bytes each: 128 of image, 32 of solidity) as one linear run. */
static int unit_byte(const sk_tileset *ts, size_t o) {
    size_t unit = o / SK_TILE_UNIT_A, in = o % SK_TILE_UNIT_A;
    if (unit >= (size_t)ts->count) return -1;
    if (in < SK_TILE_BYTES) return ts->tiles[unit * SK_TILE_BYTES + in];
    return ts->tails ? ts->tails[unit * SK_TILE_TAIL + in - SK_TILE_BYTES] : 0;
}

/* sub_10380C: from unit offset 96, in steps of 128 bytes, the first run of 32 bytes that are all 0xFF; the step count
   is the wall tile (the original's stride differs from the 160-byte units; reproduced as is). 0 when none. */
static int wall_tile(const sk_level *lv) {
    for (int k = 0; ; k++) {
        size_t o = 96 + (size_t)k * 128;
        int all = 1;
        for (int i = 0; i < 32; i++) {
            int b = unit_byte(&lv->tiles_a, o + (size_t)i);
            if (b < 0) return 0;
            if (b != 0xFF) { all = 0; break; }
        }
        if (all) return k;
    }
}

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
    if (lv->map.states) memcpy(w->state, lv->map.states, n);       /* the level's animated cells */
    w->bg_w = lv->backdrop.grid ? lv->backdrop.width : 0;
    w->bg_h = lv->backdrop.grid ? lv->backdrop.height : 0;
    if (w->bg_w && w->bg_h) {
        w->bg = malloc((size_t)w->bg_w * (size_t)w->bg_h);
        if (!w->bg) { world_free(w); return false; }
        memcpy(w->bg, lv->backdrop.grid, (size_t)w->bg_w * (size_t)w->bg_h);
    }
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
    int wall = wall_tile(lv);                                   /* relocate_map: wall in the first and last column of every row */
    for (int cy = 0; cy < w->h && w->w > 0; cy++) {
        size_t a = (size_t)cy * (size_t)w->w, b = a + (size_t)w->w - 1;
        w->live[a] = w->live[b] = (uint16_t)wall;
        w->tile[a] = w->tile[b] = (uint16_t)wall;
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
    free(w->bg);
    memset(w, 0, sizeof *w);
}

int world_attr(const sk_world *w, int cx, int cy) {
    int f = world_flag(w, cx, cy);
    if (f < 33) return -1;                                  /* flag 32 is neither marker nor attribute */
    size_t i = SK_PALS_ATTR + (size_t)(f - 33);
    return i < w->lv->pals_len ? w->lv->pals[i] : -1;
}

int world_marker_type(const sk_world *w, int cx, int cy, int *param) {
    int f = world_flag(w, cx, cy);
    if (f < 1 || f > 31) return -1;
    if (SK_PALS_PARAM + (size_t)f >= w->lv->pals_len) return -1;
    *param = w->lv->pals[SK_PALS_PARAM + f];
    return w->lv->pals[SK_PALS_TYPE + f];
}

const uint8_t *world_tail(const sk_world *w, int cx, int cy) {
    int t = world_tile_index(w, cx, cy);
    if (!w->lv->tiles_a.tails || t >= w->lv->tiles_a.count) return NULL;
    return w->lv->tiles_a.tails + (size_t)t * SK_TILE_TAIL;
}

void paint_bg_cells(sk_game *g, int x, int y, int tile, int n) {         /* sub_11A49A */
    sk_world *w = &g->world;
    int cx = (x & 0xFFF0) >> 4, cy = (y & 0xFFF0) >> 4;
    if (!w->bg || cy < 0 || cy >= w->bg_h) return;                       /* port guard */
    for (int k = 0; k < n && cx + k < w->bg_w; k++) w->bg[cy * w->bg_w + cx + k] = (uint8_t)tile;
}

static void copy_backdrop(sk_game *g, bool save) {
    sk_world *w = &g->world;
    int col = (w->bg_max_x & 0xFFF0) >> 4;
    if (!w->bg) return;
    for (int r = 0; r < 14 && r < w->bg_h; r++)
        for (int c = 0; c < 20 && col + c < w->bg_w; c++) {
            uint8_t *far = &w->bg[r * w->bg_w + col + c], *near = &w->bg[r * w->bg_w + c];
            if (save) *near = *far; else *far = *near;
        }
}
void save_backdrop(sk_game *g) { copy_backdrop(g, true); }               /* sub_1154C2 */
void restore_backdrop(sk_game *g) { copy_backdrop(g, false); }           /* sub_1154FA */
