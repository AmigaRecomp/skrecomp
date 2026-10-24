#include <stdlib.h>
#include <string.h>
#include "editor/doc.h"
#include "editor/pieces.h"
#include "game/world.h"

enum { NT = 512, NONE = -2, MANY = -3 };
static const int DX[4] = { 1, 0, -1, 0 }, DY[4] = { 0, 1, 0, -1 };   /* right, down, left, up: opposite = d ^ 2 */

static int tile_at(const sk_tilemap *m, int x, int y) {
    return x < 0 || y < 0 || x >= m->width || y >= m->height ? -1 : m->cells[y * m->width + x] & SK_INDEX_MASK;
}

static int by_size(const void *a, const void *b) {
    const sk_piece *p = a, *q = b;
    return p->cells != q->cells ? q->cells - p->cells : q->uses - p->uses;
}

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

bool edit_find_pieces(const sk_tilemap *m, int min_cells, sk_pieces *out) {
    memset(out, 0, sizeof *out);
    int n = m->width * m->height, cap = 0;
    int (*side)[4] = malloc(NT * sizeof *side), *count = calloc(NT, sizeof *count), *stack = malloc((size_t)n * sizeof *stack);
    bool (*bound)[4] = calloc(NT, sizeof *bound), *seen = calloc((size_t)n, 1), ok = side && count && stack && bound && seen;
    if (!ok) goto done;
    for (int t = 0; t < NT; t++) for (int d = 0; d < 4; d++) side[t][d] = NONE;
    for (int y = 0; y < m->height; y++)                                    /* each tile's neighbour per side: one or many */
        for (int x = 0; x < m->width; x++) {
            int t = tile_at(m, x, y);
            count[t]++;
            for (int d = 0; d < 4; d++) {
                int b = tile_at(m, x + DX[d], y + DY[d]);
                side[t][d] = side[t][d] == NONE || side[t][d] == b ? b : MANY;
            }
        }
    bool never[NT] = { false };                                            /* the three commonest tiles and the start */
    never[SK_START_TILE] = true;
    for (int k = 0; k < 3; k++) {
        int best = -1;
        for (int t = 0; t < NT; t++) if (!never[t] && count[t] && (best < 0 || count[t] > count[best])) best = t;
        if (best >= 0) never[best] = true;
    }
    for (int t = 0; t < NT; t++)
        for (int d = 0; d < 4; d++) {
            int b = side[t][d];
            bound[t][d] = count[t] >= 2 && b >= 0 && b != t && !never[t] && !never[b] && side[b][d ^ 2] == t;
        }
    for (int i = 0; i < n && ok; i++) {                                    /* each group of joined cells, once */
        int t = m->cells[i] & SK_INDEX_MASK;
        if (seen[i] || !(bound[t][0] || bound[t][1] || bound[t][2] || bound[t][3])) continue;
        int top = 0, cells = 0, x0 = m->width, y0 = m->height, x1 = 0, y1 = 0;
        stack[top++] = i; seen[i] = true;
        while (top > cells) {               /* stack[0, cells) collects the group, stack[cells, top) waits */
            int c = stack[--top];
            stack[top++] = stack[cells]; stack[cells++] = c;
            int x = c % m->width, y = c / m->width, ct = m->cells[c] & SK_INDEX_MASK;
            if (x < x0) x0 = x;
            if (y < y0) y0 = y;
            if (x > x1) x1 = x;
            if (y > y1) y1 = y;
            for (int d = 0; d < 4; d++) {
                int nx = x + DX[d], ny = y + DY[d];
                if (!bound[ct][d] || nx < 0 || ny < 0 || nx >= m->width || ny >= m->height) continue;
                int nc = ny * m->width + nx;
                if (!seen[nc]) { seen[nc] = true; stack[top++] = nc; }
            }
        }
        if (cells < min_cells) continue;
        int w = x1 - x0 + 1, h = y1 - y0 + 1;
        int16_t *tiles = malloc((size_t)w * h * sizeof *tiles);
        if (!tiles) { ok = false; break; }
        for (int j = 0; j < w * h; j++) tiles[j] = -1;
        for (int j = 0; j < cells; j++) {
            int c = stack[j];
            tiles[(c / m->width - y0) * w + c % m->width - x0] = (int16_t)(m->cells[c] & SK_INDEX_MASK);
        }
        ok = add(out, &cap, w, h, tiles, cells);
    }
    for (int i = 1; ok && i < out->n; i++)                                 /* largest first, then the most used; stable */
        for (int j = i; j > 0 && by_size(&out->v[j - 1], &out->v[j]) > 0; j--) {
            sk_piece tmp = out->v[j]; out->v[j] = out->v[j - 1]; out->v[j - 1] = tmp;
        }
done:
    free(side); free(count); free(stack); free(bound); free(seen);
    if (!ok) edit_pieces_free(out);
    return ok;
}

void edit_pieces_free(sk_pieces *p) {
    for (int i = 0; i < p->n; i++) free(p->v[i].tiles);
    free(p->v);
    memset(p, 0, sizeof *p);
}
