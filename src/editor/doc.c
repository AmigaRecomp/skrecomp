#include <stdlib.h>
#include <string.h>
#include "editor/doc.h"
#include "game/world.h"

#define TILE_MASK ((1 << SK_FLAG_SHIFT) - 1)

static void tile_states(sk_doc *d, const sk_level *lv) {
    int (*count)[256] = calloc(512, sizeof *count);
    memset(d->tile_state, 0, sizeof d->tile_state);
    if (!count || !lv->map.states) { free(count); return; }
    for (int i = 0; i < lv->map.width * lv->map.height; i++) count[lv->map.cells[i] & SK_INDEX_MASK][lv->map.states[i]]++;
    for (int t = 0; t < 512; t++) {
        int best = 0;
        for (int s = 1; s < 256; s++) if (count[t][s] > count[t][best]) best = s;
        d->tile_state[t] = (uint8_t)best;
    }
    free(count);
}

int doc_flag_for_attr(const sk_level *lv, int attr) {
    bool used[128] = { false };
    for (int i = 0; i < lv->map.width * lv->map.height; i++) used[lv->map.cells[i] >> SK_FLAG_SHIFT] = true;
    for (int f = 33; f < 128; f++)
        if (used[f] && SK_PALS_ATTR + (size_t)(f - 33) < lv->pals_len && lv->pals[SK_PALS_ATTR + f - 33] == attr) return f;
    return -1;
}

int doc_floor_tile(const sk_level *lv) {
    for (int i = 0; i < lv->map.width * lv->map.height; i++) {
        if ((lv->map.cells[i] & SK_INDEX_MASK) != SK_START_TILE) continue;
        int x = i % lv->map.width;
        for (int y = i / lv->map.width + 1; y < lv->map.height; y++) {
            int t = level_cell(lv, x, y) & SK_INDEX_MASK;
            if (!lv->tiles_a.tails || t >= lv->tiles_a.count) continue;
            const uint8_t *tail = lv->tiles_a.tails + (size_t)t * SK_TILE_TAIL;
            for (int k = 0; k < SK_TILE_TAIL; k++) if (tail[k]) return t;
        }
        return -1;
    }
    return -1;
}

static void reset(sk_doc *d) { memset(d, 0, sizeof *d); }

bool doc_new_copy(sk_doc *d, const sk_level *lv, int setting_index, sk_err *e) {
    reset(d);
    if (!custom_alloc(&d->lvl, setting_index, lv->map.width, lv->map.height, e)) return false;
    memcpy(d->lvl.cells, lv->map.cells, (size_t)d->lvl.w * d->lvl.h * 2);
    if (lv->map.states) memcpy(d->lvl.states, lv->map.states, (size_t)d->lvl.w * d->lvl.h);
    tile_states(d, lv);
    return true;
}

bool doc_new_empty(sk_doc *d, const sk_level *lv, int setting_index, int w, int h, sk_err *e) {
    reset(d);
    if (!custom_check_size(w, h, e) || !custom_alloc(&d->lvl, setting_index, w, h, e)) return false;
    tile_states(d, lv);
    int floor = doc_floor_tile(lv), exit_flag = doc_flag_for_attr(lv, 13);
    if (floor >= 0)
        for (int y = h - 2; y < h; y++)
            for (int x = 0; x < w; x++) { d->lvl.cells[y * w + x] = (uint16_t)floor; d->lvl.states[y * w + x] = d->tile_state[floor]; }
    d->lvl.cells[(h - 3) * w + 1] = SK_START_TILE;
    if (exit_flag > 0)
        for (int y = h - 6; y < h - 2; y++) d->lvl.cells[y * w + w - 2] = (uint16_t)(exit_flag << SK_FLAG_SHIFT);
    d->dirty = true;
    return true;
}

void doc_open(sk_doc *d, const sk_level *lv, sk_custom *loaded) {
    reset(d);
    d->lvl = *loaded;
    memset(loaded, 0, sizeof *loaded);
    tile_states(d, lv);
}

void doc_free(sk_doc *d) {
    custom_free(&d->lvl);
    for (int i = 0; i < d->past_n; i++) custom_free(&d->past[i]);
    for (int i = 0; i < d->future_n; i++) custom_free(&d->future[i]);
    reset(d);
}

static void push(sk_custom *stack, int *n, const sk_custom *c) {
    if (*n == SK_UNDO_MAX) {                                 /* the oldest step goes */
        custom_free(&stack[0]);
        memmove(stack, stack + 1, (SK_UNDO_MAX - 1) * sizeof *stack);
        (*n)--;
    }
    if (custom_copy(&stack[*n], c)) (*n)++;
}

static void begin_op(sk_doc *d) {                            /* a snapshot before the change; a new change drops the redo tail */
    d->dirty = true;
    if (d->in_stroke) return;
    push(d->past, &d->past_n, &d->lvl);
    for (int i = 0; i < d->future_n; i++) custom_free(&d->future[i]);
    d->future_n = 0;
}

void doc_begin_stroke(sk_doc *d) { begin_op(d); d->in_stroke = true; }
void doc_end_stroke(sk_doc *d) { d->in_stroke = false; }

static bool inside(const sk_doc *d, int x, int y) { return x >= 0 && y >= 0 && x < d->lvl.w && y < d->lvl.h; }

static void set_tile(sk_doc *d, int i, int tile) {
    d->lvl.cells[i] = (uint16_t)((d->lvl.cells[i] & ~SK_INDEX_MASK) | (tile & SK_INDEX_MASK));
    d->lvl.states[i] = tile == SK_START_TILE ? 0 : d->tile_state[tile & SK_INDEX_MASK];
}

static void set_flag(sk_doc *d, int i, int flag) {
    d->lvl.cells[i] = (uint16_t)((d->lvl.cells[i] & SK_INDEX_MASK) | (flag & 0x7F) << SK_FLAG_SHIFT);
}

void doc_paint(sk_doc *d, int x, int y, int tile) {
    if (!inside(d, x, y)) return;
    begin_op(d);
    set_tile(d, y * d->lvl.w + x, tile);
}

void doc_place_flag(sk_doc *d, int x, int y, int flag) {
    if (!inside(d, x, y)) return;
    begin_op(d);
    set_flag(d, y * d->lvl.w + x, flag);
}

void doc_place_start(sk_doc *d, int x, int y) { doc_paint(d, x, y, SK_START_TILE); }

void doc_move_start(sk_doc *d, int x, int y) {
    if (!inside(d, x, y)) return;
    begin_op(d);
    for (int i = 0; i < d->lvl.w * d->lvl.h; i++)
        if ((d->lvl.cells[i] & SK_INDEX_MASK) == SK_START_TILE) { d->lvl.cells[i] &= (uint16_t)~SK_INDEX_MASK; d->lvl.states[i] = 0; }
    set_tile(d, y * d->lvl.w + x, SK_START_TILE);
}

void doc_erase(sk_doc *d, int x, int y, int layer) {
    if (!inside(d, x, y)) return;
    begin_op(d);
    int i = y * d->lvl.w + x;
    if (layer & SK_LAYER_TILE) { d->lvl.cells[i] &= (uint16_t)~SK_INDEX_MASK; d->lvl.states[i] = 0; }
    if (layer & SK_LAYER_FLAG) d->lvl.cells[i] &= SK_INDEX_MASK;
}

static void order(int *a, int *b) { if (*a > *b) { int t = *a; *a = *b; *b = t; } }

void doc_fill(sk_doc *d, int x0, int y0, int x1, int y1, int layer, int tile, int flag) {
    order(&x0, &x1); order(&y0, &y1);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= d->lvl.w) x1 = d->lvl.w - 1;
    if (y1 >= d->lvl.h) y1 = d->lvl.h - 1;
    if (x0 > x1 || y0 > y1) return;
    begin_op(d);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            int i = y * d->lvl.w + x;
            if (layer & SK_LAYER_TILE) set_tile(d, i, tile);
            if (layer & SK_LAYER_FLAG) set_flag(d, i, flag);
        }
}

bool doc_copy(const sk_doc *d, int x0, int y0, int x1, int y1, sk_clip *out) {
    order(&x0, &x1); order(&y0, &y1);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= d->lvl.w) x1 = d->lvl.w - 1;
    if (y1 >= d->lvl.h) y1 = d->lvl.h - 1;
    memset(out, 0, sizeof *out);
    if (x0 > x1 || y0 > y1) return false;
    out->w = x1 - x0 + 1; out->h = y1 - y0 + 1;
    out->cells = malloc((size_t)out->w * out->h * 2);
    out->states = malloc((size_t)out->w * out->h);
    if (!out->cells || !out->states) { clip_free(out); return false; }
    for (int y = 0; y < out->h; y++)
        for (int x = 0; x < out->w; x++) {
            out->cells[y * out->w + x] = d->lvl.cells[(y0 + y) * d->lvl.w + x0 + x];
            out->states[y * out->w + x] = d->lvl.states[(y0 + y) * d->lvl.w + x0 + x];
        }
    return true;
}

void doc_paste(sk_doc *d, const sk_clip *c, int x, int y) {
    if (!c->cells || x >= d->lvl.w || y >= d->lvl.h || x + c->w <= 0 || y + c->h <= 0) return;
    begin_op(d);
    for (int cy = 0; cy < c->h; cy++)
        for (int cx = 0; cx < c->w; cx++)
            if (inside(d, x + cx, y + cy)) {
                d->lvl.cells[(y + cy) * d->lvl.w + x + cx] = c->cells[cy * c->w + cx];
                d->lvl.states[(y + cy) * d->lvl.w + x + cx] = c->states[cy * c->w + cx];
            }
}

void clip_free(sk_clip *c) { free(c->cells); free(c->states); memset(c, 0, sizeof *c); }

bool doc_resize(sk_doc *d, int left, int top, int right, int bottom, sk_err *e) {
    d->in_stroke = false;                                    /* its own step, never part of a stroke */
    int w = d->lvl.w + left + right, h = d->lvl.h + top + bottom;
    if (!custom_check_size(w, h, e)) return false;
    sk_custom n;
    if (!custom_alloc(&n, d->lvl.setting, w, h, e)) return false;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int sx = x - left, sy = y - top;
            if (sx < 0 || sy < 0 || sx >= d->lvl.w || sy >= d->lvl.h) continue;
            n.cells[y * w + x] = d->lvl.cells[sy * d->lvl.w + sx];
            n.states[y * w + x] = d->lvl.states[sy * d->lvl.w + sx];
        }
    begin_op(d);
    custom_free(&d->lvl);
    d->lvl = n;
    return true;
}

static bool swap_from(sk_doc *d, sk_custom *from, int *from_n, sk_custom *to, int *to_n) {
    d->in_stroke = false;                                    /* an undo inside a stroke ends it */
    if (*from_n == 0) return false;
    push(to, to_n, &d->lvl);
    custom_free(&d->lvl);
    d->lvl = from[--*from_n];
    memset(&from[*from_n], 0, sizeof from[*from_n]);
    d->dirty = true;
    return true;
}

bool doc_undo(sk_doc *d) { return swap_from(d, d->past, &d->past_n, d->future, &d->future_n); }
bool doc_redo(sk_doc *d) { return swap_from(d, d->future, &d->future_n, d->past, &d->past_n); }
