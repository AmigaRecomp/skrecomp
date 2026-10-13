#include <stdlib.h>
#include <string.h>
#include "formats/level.h"
#include "core/bytekiller.h"
#include "core/bytes.h"

#define CHUNKS 7
static const char *TAGS[CHUNKS] = { "MAPS", "MAPS", "GRAF", "GRAF", "COPP", "COPP", "PALS" };

static bool parse_map(const uint8_t *c, size_t n, sk_tilemap *m, sk_err *e) {
    if (n < 8) return sk_fail(e, "map chunk too short");
    unsigned w = be16(c + 4), h = be16(c + 6);
    if (w == 0 || h == 0 || 8 + SK_MAP_GAP + (size_t)w * h * 2 > n) return sk_fail(e, "map %ux%u does not fit in %zu bytes", w, h, n);
    m->cells = malloc((size_t)w * h * sizeof *m->cells);
    if (!m->cells) return sk_fail(e, "out of memory");
    for (size_t i = 0; i < (size_t)w * h; i++) m->cells[i] = be16(c + 8 + SK_MAP_GAP + i * 2);
    m->states = calloc((size_t)w * h, 1);                       /* cell_state (0x302B9E) is the gap itself: the cells' animation states */
    if (m->states) memcpy(m->states, c + 8, (size_t)w * h < SK_MAP_GAP ? (size_t)w * h : SK_MAP_GAP);
    m->width = (uint16_t)w;
    m->height = (uint16_t)h;
    return true;
}

static bool parse_backdrop(const uint8_t *c, size_t n, sk_backdrop *b, sk_err *e) {
    if (n < 14) return sk_fail(e, "second map chunk too short");
    for (int i = 0; i < 5; i++) b->values[i] = be16(c + 4 + i * 2);
    unsigned w = b->values[1], h = b->values[2];
    size_t cells = (size_t)w * h;
    if (cells == 0 || cells > n - 14) return sk_fail(e, "backdrop grid %ux%u does not fit in %zu cell bytes", w, h, n - 14);
    b->grid = malloc(cells);
    if (!b->grid) return sk_fail(e, "out of memory");
    memcpy(b->grid, c + 14, cells);
    b->width = (uint16_t)w;
    b->height = (uint16_t)h;
    return true;
}

static bool parse_graf(const uint8_t *c, size_t n, int unit, sk_tileset *ts, sk_err *e) {
    if (n < 34 || (n - 34) % (size_t)unit) return sk_fail(e, "GRAF length %zu is not 34 + %d * tiles", n, unit);
    ts->colours[0] = 0;
    for (int i = 1; i < 16; i++) ts->colours[i] = be16(c + 4 + (i - 1) * 2);
    ts->count = (int)((n - 34) / (size_t)unit);
    ts->tiles = malloc((size_t)ts->count * SK_TILE_BYTES + 1);
    ts->tails = unit > SK_TILE_BYTES ? malloc((size_t)ts->count * (size_t)(unit - SK_TILE_BYTES) + 1) : NULL;
    if (!ts->tiles || (unit > SK_TILE_BYTES && !ts->tails)) return sk_fail(e, "out of memory");
    for (int i = 0; i < ts->count; i++) {
        const uint8_t *u = c + 34 + (size_t)i * (size_t)unit;
        memcpy(ts->tiles + (size_t)i * SK_TILE_BYTES, u, SK_TILE_BYTES);
        if (ts->tails) memcpy(ts->tails + (size_t)i * (size_t)(unit - SK_TILE_BYTES), u + SK_TILE_BYTES, (size_t)(unit - SK_TILE_BYTES));
    }
    return true;
}

static bool parse_copp(const uint8_t *c, size_t n, uint16_t **words, int *count, sk_err *e) {
    size_t o = 4, end = n;
    for (size_t i = 4; i + 4 <= n; i += 2)
        if (memcmp(c + i, "COPP", 4) == 0) { end = i; break; }
    int k = (int)((end - o) / 2);
    *words = malloc((size_t)(k > 0 ? k : 1) * sizeof **words);
    if (!*words) return sk_fail(e, "out of memory");
    for (int i = 0; i < k; i++) (*words)[i] = be16(c + o + (size_t)i * 2);
    *count = k;
    return true;
}

bool level_parse(const uint8_t *data, size_t len, sk_level *lv, sk_err *e) {
    memset(lv, 0, sizeof *lv);
    uint8_t *chunk[CHUNKS] = {0};
    size_t clen[CHUNKS] = {0};
    int found = 0;
    size_t o = 0;
    bool ok = true;
    while (o < len && ok) {
        if (o + 4 > len) { ok = sk_fail(e, "chunk %d: truncated length field at offset %zu", found, o); break; }
        uint32_t n = be32(data + o);
        if (o + 4 + n > len) { ok = sk_fail(e, "chunk %d at offset %zu: %u bytes declared, %zu available", found, o, n, len - o - 4); break; }
        if (found >= CHUNKS) { ok = sk_fail(e, "expected %d chunks, found more", CHUNKS); break; }
        sk_err inner = {0};
        if (!bk_decompress(data + o + 4, n, &chunk[found], &clen[found], &inner)) {
            ok = sk_fail(e, "chunk %d at offset %zu: %s", found, o, inner.msg);
            break;
        }
        o += 4 + n;
        found++;
    }
    if (ok && found != CHUNKS) ok = sk_fail(e, "expected %d chunks, found %d chunks", CHUNKS, found);
    if (ok) {
        for (int i = 0; i < CHUNKS; i++)
            if (clen[i] < 4 || memcmp(chunk[i], TAGS[i], 4) != 0) { ok = sk_fail(e, "unexpected chunk tags (chunk %d)", i); break; }
    }
    if (ok) {
        for (int i = 0; i < CHUNKS; i++) lv->chunk_sizes[i] = clen[i];
        ok = parse_map(chunk[0], clen[0], &lv->map, e)
          && parse_backdrop(chunk[1], clen[1], &lv->backdrop, e)
          && parse_graf(chunk[2], clen[2], SK_TILE_UNIT_A, &lv->tiles_a, e)
          && parse_graf(chunk[3], clen[3], SK_TILE_BYTES, &lv->tiles_b, e)
          && parse_copp(chunk[4], clen[4], &lv->sky, &lv->sky_len, e)
          && parse_copp(chunk[5], clen[5], &lv->copp2, &lv->copp2_len, e);
        if (ok) {
            lv->pals_len = clen[6] - 4;
            lv->pals = malloc(lv->pals_len + 1);
            if (!lv->pals) ok = sk_fail(e, "out of memory");
            else memcpy(lv->pals, chunk[6] + 4, lv->pals_len);
        }
    }
    for (int i = 0; i < CHUNKS; i++) free(chunk[i]);
    if (!ok) level_free(lv);
    return ok;
}

void level_free(sk_level *lv) {
    free(lv->map.cells);
    free(lv->map.states);
    free(lv->backdrop.grid);
    free(lv->tiles_a.tiles); free(lv->tiles_a.tails);
    free(lv->tiles_b.tiles); free(lv->tiles_b.tails);
    free(lv->sky);
    free(lv->copp2);
    free(lv->pals);
    memset(lv, 0, sizeof *lv);
}

const uint8_t *level_tile(const sk_level *lv, int index, const sk_tileset **set) {
    if (index < 0 || index >= lv->tiles_a.count) return NULL;
    if (set) *set = &lv->tiles_a;
    return lv->tiles_a.tiles + (size_t)index * SK_TILE_BYTES;
}
