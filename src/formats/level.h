/* A level file: a chain of seven ByteKiller containers tagged MAPS MAPS GRAF GRAF COPP COPP PALS.
   Port of re/skre/formats/{containers,level,maps,graf,copp}.py; formats in re/docs/notes/levels.md. */
#ifndef SK_FORMATS_LEVEL_H
#define SK_FORMATS_LEVEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

#define SK_TILE_BYTES 128           /* 16 rows x (4 planes x 2 bytes), row-interleaved */
#define SK_TILE_UNIT_A 160          /* set A: the tile image followed by a 32-byte table */
#define SK_TILE_TAIL 32
#define SK_INDEX_MASK 0x1FF         /* foreground cell: bits 0..8 tile index, bits 9..15 flags */
#define SK_FLAG_SHIFT 9

typedef struct { uint16_t width, height; uint16_t *cells; } sk_tilemap;
typedef struct { uint16_t values[5]; uint16_t width, height; uint8_t *grid; } sk_backdrop;
typedef struct {
    uint16_t colours[16];           /* 12-bit words; colours[0] = 0 */
    int count;
    uint8_t *tiles;                 /* count * SK_TILE_BYTES */
    uint8_t *tails;                 /* set A: count * SK_TILE_TAIL; set B: NULL */
} sk_tileset;

typedef struct {
    sk_tilemap map;
    sk_backdrop backdrop;
    sk_tileset tiles_a, tiles_b;
    uint16_t *sky;  int sky_len;    /* COPP chunk 4: (COLOR17, COLOR18) pairs per display line */
    uint16_t *copp2; int copp2_len; /* COPP chunk 5 */
    uint8_t *pals; size_t pals_len;
    size_t chunk_sizes[7];          /* unpacked sizes */
} sk_level;

bool level_parse(const uint8_t *data, size_t len, sk_level *lv, sk_err *e);
void level_free(sk_level *lv);

static inline uint16_t level_cell(const sk_level *lv, int x, int y) {
    return lv->map.cells[y * lv->map.width + x];
}

/* The tile image for a foreground index (set A only), or NULL when out of range; *set gets the set. */
const uint8_t *level_tile(const sk_level *lv, int index, const sk_tileset **set);

/* Pixel value 0..15 of a 128-byte tile image. */
static inline int tile_pixel(const uint8_t *tile, int x, int y) {
    const uint8_t *row = tile + y * 8;
    int shift = 7 - (x & 7), byte = x >> 3, v = 0;
    for (int p = 0; p < 4; p++)
        if (row[p * 2 + byte] >> shift & 1) v |= 1 << p;
    return v;
}

#endif
