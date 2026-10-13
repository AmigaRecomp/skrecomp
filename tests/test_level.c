#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "bk_test_pack.h"
#include "core/bytekiller.h"
#include "core/bytes.h"
#include "core/disks.h"
#include "formats/level.h"

void test_bk_test_pack_round_trips(void) {
    uint8_t data[700];
    for (size_t i = 0; i < sizeof data; i++) data[i] = (uint8_t)(i * 13 + 7);
    uint8_t *packed = NULL, *out = NULL;
    size_t pn = 0, on = 0;
    sk_err e = {0};
    bk_test_pack(data, sizeof data, &packed, &pn);
    CHECK(bk_is_packed(packed, pn));
    CHECK(bk_unpack(packed, pn, &out, &on, &e));
    CHECK_EQ(on, sizeof data);
    CHECK(out && on == sizeof data && memcmp(out, data, sizeof data) == 0);
    free(packed);
    free(out);
}

void test_level_parses_tiny_fixture(void) {
    uint8_t *d = NULL;
    size_t n = 0;
    sk_level lv = {0};
    sk_err e = {0};
    CHECK(load_fixture("level_tiny.bin", &d, &n));
    CHECK(level_parse(d, n, &lv, &e));
    CHECK_EQ(lv.map.width, 2);
    CHECK_EQ(lv.map.height, 1);
    CHECK_EQ(level_cell(&lv, 1, 0) & 0x1FF, 1);
    CHECK_EQ(level_cell(&lv, 1, 0) >> 9, 5);
    CHECK_EQ(lv.backdrop.width, 194);
    CHECK_EQ(lv.backdrop.height, 19);
    CHECK_EQ(lv.backdrop.values[0], 3);
    CHECK_EQ(lv.backdrop.grid[1], 2);
    CHECK_EQ(lv.tiles_a.count, 2);
    CHECK_EQ(lv.tiles_b.count, 1);
    CHECK_EQ(lv.tiles_a.colours[1], 0x0F00);
    CHECK_EQ(lv.tiles_a.colours[0], 0);
    CHECK_EQ(lv.tiles_b.colours[1], 0x00F0);
    CHECK(lv.tiles_a.tails != NULL && lv.tiles_b.tails == NULL);
    CHECK_EQ(lv.sky_len, 2);
    CHECK_EQ(lv.sky[0], 0x0BF);
    CHECK_EQ(lv.copp2_len, 1);
    CHECK_EQ(lv.pals_len, 3);
    const sk_tileset *s = NULL;
    CHECK(level_tile(&lv, 1, &s) == lv.tiles_a.tiles + 128 && s == &lv.tiles_a);
    CHECK(level_tile(&lv, 2, &s) == NULL);
    CHECK_EQ(lv.chunk_sizes[0], 12 + SK_MAP_GAP);              /* header, the 0x4000-byte gap, two words */
    level_free(&lv);
    CHECK(lv.map.cells == NULL && lv.tiles_a.tiles == NULL);
    free(d);
}

/* Seven packed chunks: chunk 0 as given, the others a tag plus 12 zero bytes. Caller frees *out. */
static void seven_chunks(const uint8_t *first, size_t first_len, uint8_t **out, size_t *len) {
    static const char *tags[7] = { "MAPS", "MAPS", "GRAF", "GRAF", "COPP", "COPP", "PALS" };
    uint8_t *chunks[7];
    size_t lens[7], total = 0;
    for (int i = 0; i < 7; i++) {
        uint8_t body[16] = {0};
        memcpy(body, tags[i], 4);
        if (i == 0) bk_test_pack(first, first_len, &chunks[i], &lens[i]);
        else bk_test_pack(body, sizeof body, &chunks[i], &lens[i]);
        total += lens[i];
    }
    uint8_t *file = malloc(total);
    size_t o = 0;
    for (int i = 0; i < 7; i++) { memcpy(file + o, chunks[i], lens[i]); o += lens[i]; free(chunks[i]); }
    *out = file;
    *len = total;
}

void test_level_rejects_short_map(void) {               /* Review Focus 3 */
    uint8_t chunk[12] = { 'M', 'A', 'P', 'S', 0x02, 0x58, 0x00, 0x01, 0, 0, 0, 0 };   /* claims 600x1 */
    uint8_t *file = NULL;
    size_t n = 0;
    seven_chunks(chunk, sizeof chunk, &file, &n);
    sk_level lv = {0};
    sk_err e = {0};
    CHECK(!level_parse(file, n, &lv, &e));
    CHECK_STR(e.msg, "does not fit");
    CHECK(lv.map.cells == NULL);
    free(file);
}

void test_level_rejects_wrong_tags_and_truncation(void) {
    uint8_t *d = NULL;
    size_t n = 0;
    sk_level lv = {0};
    sk_err e = {0};
    CHECK(load_fixture("level_tiny.bin", &d, &n));
    CHECK(!level_parse(d, n - 3, &lv, &e));               /* last chunk cut short */
    CHECK_STR(e.msg, "chunk 6");
    /* seven chunks with chunk 2 tagged GRAX */
    uint8_t *chunks[7];
    size_t lens[7];
    static const char *tags[7] = { "MAPS", "MAPS", "GRAX", "GRAF", "COPP", "COPP", "PALS" };
    size_t total = 0;
    for (int i = 0; i < 7; i++) {
        uint8_t body[16] = {0};
        memcpy(body, tags[i], 4);
        bk_test_pack(body, sizeof body, &chunks[i], &lens[i]);
        total += lens[i];
    }
    uint8_t *file = malloc(total);
    size_t o = 0;
    for (int i = 0; i < 7; i++) { memcpy(file + o, chunks[i], lens[i]); o += lens[i]; free(chunks[i]); }
    CHECK(!level_parse(file, total, &lv, &e));
    CHECK_STR(e.msg, "unexpected chunk tags");
    free(file);
    free(d);
}

void test_level_real_level0(void) {                    /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0};
    sk_err e = {0};
    CHECK(disks_open(&d, roms, &e));
    uint8_t *f = NULL;
    size_t n = 0;
    CHECK(disks_read_file_no(&d, 1, 0, false, &f, &n, &e));
    sk_level lv = {0};
    CHECK(level_parse(f, n, &lv, &e));
    CHECK_EQ(lv.map.width, 546);
    CHECK_EQ(lv.map.height, 30);
    CHECK_EQ(level_cell(&lv, 16, 9) & 0x1FF, 0x1BC);           /* live_map (16, 9) = chunk word 8192 + 9 * 546 + 16 */
    int ground = 0, sky = 0;
    for (int x = 0; x < 546; x++) { ground += (level_cell(&lv, x, 29) & 0x1FF) != 0; sky += (level_cell(&lv, x, 0) & 0x1FF) != 0; }
    CHECK(ground > 500); CHECK(sky < 40);                     /* the bottom row is kerb and asphalt, the top row sky */
    CHECK_EQ(lv.backdrop.width, 194);
    CHECK_EQ(lv.backdrop.height, 19);
    CHECK_EQ(lv.tiles_a.count, 512);
    CHECK_EQ(lv.tiles_b.count, 256);
    CHECK_EQ(lv.sky_len, 510);
    CHECK_EQ(lv.copp2_len, 5840);
    CHECK_EQ(lv.pals_len, 162);
    CHECK_EQ(lv.tiles_a.colours[1], 0x0EEE);
    CHECK(memcmp(lv.tiles_a.tails + 4 * 32, "\xff\xff\xff\xff", 4) == 0);
    level_free(&lv);
    free(f);
    disks_close(&d);
}
