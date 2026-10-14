#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "core/disks.h"
#include "formats/mapsprites.h"

void test_masked8_draws_the_index_where_the_mask_is_set(void) {
    uint8_t img[9] = { 0xC0, 0x80, 0, 0, 0x40, 0, 0, 0, 0x80 };   /* 8 x 1: mask 11000000, plane 0 / 3 / 7 bits */
    uint8_t px[4 * 2];
    memset(px, 5, sizeof px);
    masked8_draw(img, sizeof img, 0, 8, 1, px, 4, 2, 2, 1);       /* at (2, 1): pixels 0, 1 land on x 2, 3 */
    CHECK_EQ(px[4 + 2], 0x81);                                    /* plane 0 + plane 7 */
    CHECK_EQ(px[4 + 3], 0x08);                                    /* plane 3, inside the mask */
    CHECK_EQ(px[4 + 1], 5); CHECK_EQ(px[0 + 2], 5);              /* untouched outside */
    memset(px, 5, sizeof px);
    masked8_draw(img, sizeof img, 0, 8, 1, px, 4, 2, -1, 0);     /* clipped on the left: pixel 1 at x 0 */
    CHECK_EQ(px[0], 0x08); CHECK_EQ(px[1], 5);
    memset(px, 5, sizeof px);
    masked8_draw(img, sizeof img, 9, 8, 1, px, 4, 2, 0, 0);      /* past the chunk: zeros, an empty mask */
    for (size_t i = 0; i < sizeof px; i++) CHECK_EQ(px[i], 5);
}

void test_mapsprites_parse_rejects_a_short_pack(void) {
    uint8_t hdr[10] = { 0, 2, 0, 0, 0, 0, 0, 0, 0, 0 };
    sk_mapsprites m; sk_err e = {0};
    CHECK(!mapsprites_parse(hdr, sizeof hdr, &m, &e));          /* chunk 0 needs 2 bytes after the header */
    CHECK_STR(e.msg, "file253");
}

void test_mapsprites_real_pack_and_pictures(void) {              /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_err e = {0};
    CHECK(disks_open(&d, roms, &e));
    uint8_t *f = NULL; size_t n = 0;
    CHECK(disks_read_file_no(&d, 1, 253, true, &f, &n, &e));
    CHECK_EQ(n, 24872);
    sk_mapsprites m;
    CHECK(mapsprites_parse(f, n, &m, &e));
    size_t want[5] = { 7920, 4, 2052, 486, 14400 };
    for (int k = 0; k < 5; k++) CHECK_EQ(m.len[k], want[k]);
    free(f);
    CHECK(disks_read_file_no(&d, 1, 252, true, &f, &n, &e)); CHECK_EQ(n, 65024); free(f);
    CHECK(disks_read_file_no(&d, 3, 250, true, &f, &n, &e)); CHECK_EQ(n, 82944); free(f);
    CHECK(disks_read_file_no(&d, 3, 251, true, &f, &n, &e)); CHECK_EQ(n, 82944); free(f);
    disks_close(&d);
}
