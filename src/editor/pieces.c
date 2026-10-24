#include <stdlib.h>
#include <string.h>
#include "editor/doc.h"
#include "editor/pieces.h"
#include "game/world.h"

enum { NT = 512, NONE = -2, MANY = -3 };
enum { MAX_W = 24, MAX_H = 16 };    /* a grown group larger than about a screen is ground, not a thing: kept ungrown */
static const int DX[4] = { 1, 0, -1, 0 }, DY[4] = { 0, 1, 0, -1 };   /* right, down, left, up: opposite = d ^ 2 */

typedef struct {
    const sk_tilemap *m;
    int n, count[NT], side[NT][4];  /* each tile's uses, and its neighbour per side: one tile, NONE or MANY */
    bool never[NT], bound[NT][4];
    bool *core, *in;                /* cells joined by the pairing rule; cells in a piece after growing */
    int *run, *comp, *stack;        /* a run is a 4-connected patch of one tile */
} finder;

static int tile_at(const sk_tilemap *m, int x, int y) {
    return x < 0 || y < 0 || x >= m->width || y >= m->height ? -1 : m->cells[y * m->width + x] & SK_INDEX_MASK;
}

static int cell_tile(const finder *f, int c) { return f->m->cells[c] & SK_INDEX_MASK; }

static int neighbour(const finder *f, int c, int d) {        /* the cell beside c on side d, -1 off the map */
    int x = c % f->m->width + DX[d], y = c / f->m->width + DY[d];
    return x < 0 || y < 0 || x >= f->m->width || y >= f->m->height ? -1 : y * f->m->width + x;
}

static int by_size(const sk_piece *p, const sk_piece *q) { return p->cells != q->cells ? q->cells - p->cells : q->uses - p->uses; }

/* Adds the piece unless an identical one is there already (then counts the use); false only out of memory. */
static bool add(sk_pieces *out, int *cap, int w, int h, int16_t *tiles, int cells) {
    for (int i = 0; i < out->n; i++)
        if (out->v[i].w == w && out->v[i].h == h && !memcmp(out->v[i].tiles, tiles, (size_t)w * h * sizeof *tiles)) {
            out->v[i].uses++; free(tiles); return true;
        }
    if (out->n == *cap) {
        int c = *cap ? *cap * 2 : 32;
        sk_piece *v = realloc(out->v, (size_t)c * sizeof *v);
        if (!v) { free(tiles); return false; }
        out->v = v; *cap = c;
    }
    out->v[out->n++] = (sk_piece){ w, h, cells, 1, tiles };
    return true;
}

/* Pairing: one tile's neighbour on a side is always the other, and the other's on the opposite side always the one. */
static void find_pairs(finder *f) {
    const sk_tilemap *m = f->m;
    for (int t = 0; t < NT; t++) for (int d = 0; d < 4; d++) f->side[t][d] = NONE;
    for (int y = 0; y < m->height; y++)
        for (int x = 0; x < m->width; x++) {
            int t = tile_at(m, x, y);
            f->count[t]++;
            for (int d = 0; d < 4; d++) {
                int b = tile_at(m, x + DX[d], y + DY[d]);
                f->side[t][d] = f->side[t][d] == NONE || f->side[t][d] == b ? b : MANY;
            }
        }
    f->never[SK_START_TILE] = true;                          /* the start and the three commonest tiles: sky, wall, ground */
    for (int k = 0; k < 3; k++) {
        int best = -1;
        for (int t = 0; t < NT; t++) if (!f->never[t] && f->count[t] && (best < 0 || f->count[t] > f->count[best])) best = t;
        if (best >= 0) f->never[best] = true;
    }
    for (int t = 0; t < NT; t++)
        for (int d = 0; d < 4; d++) {
            int b = f->side[t][d];
            f->bound[t][d] = f->count[t] >= 2 && b >= 0 && b != t && !f->never[t] && !f->never[b] && f->side[b][d ^ 2] == t;
        }
    for (int c = 0; c < f->n; c++) {
        int t = cell_tile(f, c);
        f->core[c] = f->in[c] = f->bound[t][0] || f->bound[t][1] || f->bound[t][2] || f->bound[t][3];
    }
}

/* Growing: a tile used at least twice joins where its patches touch a piece, if at least half of them do; a patch
   touches through a piece tile at most three times as common, so ground that is everywhere stays out. */
static bool grow(finder *f) {
    int runs = 0;
    for (int c = 0; c < f->n; c++) f->run[c] = -1;
    for (int c = 0; c < f->n; c++) {
        if (f->run[c] >= 0) continue;
        int top = 0, t = cell_tile(f, c);
        f->stack[top++] = c; f->run[c] = runs;
        while (top) {
            int a = f->stack[--top];
            for (int d = 0; d < 4; d++) {
                int b = neighbour(f, a, d);
                if (b >= 0 && f->run[b] < 0 && cell_tile(f, b) == t) { f->run[b] = runs; f->stack[top++] = b; }
            }
        }
        runs++;
    }
    bool *touch = malloc((size_t)runs);
    int *total = calloc(NT, sizeof *total), *good = malloc(NT * sizeof *good);
    if (!touch || !total || !good) { free(touch); free(total); free(good); return false; }
    memset(touch, 0, (size_t)runs);
    for (int c = 0; c < f->n; c++) if (!touch[f->run[c]]) { touch[f->run[c]] = true; total[cell_tile(f, c)]++; }
    for (bool changed = true; changed; ) {
        changed = false;
        memset(touch, 0, (size_t)runs); memset(good, 0, NT * sizeof *good);
        for (int c = 0; c < f->n; c++) {
            int t = cell_tile(f, c);
            bool on = f->in[c];
            for (int d = 0; d < 4 && !on; d++) {
                int b = neighbour(f, c, d);
                on = b >= 0 && f->in[b] && f->count[t] <= 3 * f->count[cell_tile(f, b)];
            }
            if (on && !touch[f->run[c]]) { touch[f->run[c]] = true; good[t]++; }
        }
        for (int c = 0; c < f->n; c++) {
            int t = cell_tile(f, c);
            if (!f->in[c] && touch[f->run[c]] && !f->never[t] && f->count[t] >= 2 && 2 * good[t] >= total[t]) { f->in[c] = true; changed = true; }
        }
    }
    free(touch); free(total); free(good);
    return true;
}

static bool linked(const finder *f, int c, int n, int d, bool core_only) {
    int a = cell_tile(f, c), b = cell_tile(f, n);
    if (core_only) return f->core[n] && f->bound[a][d];
    if (!f->in[n]) return false;
    if (f->core[c] && f->core[n]) return f->side[a][d] == b || f->side[b][d ^ 2] == a;   /* one way is enough here */
    return true;
}

/* The group of cells linked to c among those with comp == from, renumbered to `to`: their count and box. */
static int flood(finder *f, int c, int from, int to, bool core_only, int box[4]) {
    int top = 0, cells = 0;
    f->stack[top++] = c; f->comp[c] = to;
    box[0] = box[2] = c % f->m->width; box[1] = box[3] = c / f->m->width;
    while (top > cells) {                  /* stack[0, cells) collects the group, stack[cells, top) waits */
        int a = f->stack[--top];
        f->stack[top++] = f->stack[cells]; f->stack[cells++] = a;
        int x = a % f->m->width, y = a / f->m->width;
        if (x < box[0]) box[0] = x;
        if (y < box[1]) box[1] = y;
        if (x > box[2]) box[2] = x;
        if (y > box[3]) box[3] = y;
        for (int d = 0; d < 4; d++) {
            int b = neighbour(f, a, d);
            if (b >= 0 && f->comp[b] == from && linked(f, a, b, d, core_only)) { f->comp[b] = to; f->stack[top++] = b; }
        }
    }
    return cells;
}

static bool emit(finder *f, sk_pieces *out, int *cap, int cells, const int box[4], int min_cells) {
    if (cells < min_cells) return true;
    int w = box[2] - box[0] + 1, h = box[3] - box[1] + 1;
    int16_t *tiles = malloc((size_t)w * h * sizeof *tiles);
    if (!tiles) return false;
    for (int j = 0; j < w * h; j++) tiles[j] = -1;
    for (int j = 0; j < cells; j++) {
        int c = f->stack[j];
        tiles[(c / f->m->width - box[1]) * w + c % f->m->width - box[0]] = (int16_t)cell_tile(f, c);
    }
    return add(out, cap, w, h, tiles, cells);
}

bool edit_find_pieces(const sk_tilemap *m, int min_cells, sk_pieces *out) {
    memset(out, 0, sizeof *out);
    finder *f = calloc(1, sizeof *f);
    int n = m->width * m->height, cap = 0, next = 1;
    bool ok = f != NULL;
    if (ok) {
        f->m = m; f->n = n;
        f->core = calloc((size_t)n, 1); f->in = calloc((size_t)n, 1);
        f->run = malloc((size_t)n * sizeof *f->run); f->comp = calloc((size_t)n, sizeof *f->comp);
        f->stack = malloc((size_t)n * sizeof *f->stack);
        ok = f->core && f->in && f->run && f->comp && f->stack;
    }
    if (ok) { find_pairs(f); ok = grow(f); }
    for (int c = 0; ok && c < n; c++) {                       /* comp: 0 = not yet grouped, -1 = in no piece */
        if (!f->in[c]) { f->comp[c] = -1; continue; }
        if (f->comp[c]) continue;
        int box[4], id = next++, cells = flood(f, c, 0, id, false, box);
        if (box[2] - box[0] < MAX_W && box[3] - box[1] < MAX_H) { ok = emit(f, out, &cap, cells, box, min_cells); continue; }
        int group = next++;                                   /* too big: its pairing-rule parts, ungrown */
        for (int j = 0; j < cells; j++) f->comp[f->stack[j]] = f->core[f->stack[j]] ? group : -1;
        for (int j = 0; ok && j < cells; j++) {
            int s = 0;
            for (int k = 0; k < n && !s; k++) if (f->comp[k] == group) s = k + 1;   /* the next part's first cell */
            if (!s) break;
            int part[4], pc = flood(f, s - 1, group, next++, true, part);
            ok = emit(f, out, &cap, pc, part, min_cells);
        }
    }
    for (int i = 1; ok && i < out->n; i++)                   /* largest first, then the most used; stable */
        for (int j = i; j > 0 && by_size(&out->v[j - 1], &out->v[j]) > 0; j--) {
            sk_piece tmp = out->v[j]; out->v[j] = out->v[j - 1]; out->v[j - 1] = tmp;
        }
    if (f) { free(f->core); free(f->in); free(f->run); free(f->comp); free(f->stack); }
    free(f);
    if (!ok) edit_pieces_free(out);
    return ok;
}

void edit_pieces_free(sk_pieces *p) {
    for (int i = 0; i < p->n; i++) free(p->v[i].tiles);
    free(p->v);
    memset(p, 0, sizeof *p);
}
