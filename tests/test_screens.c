#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "core/disks.h"
#include "formats/screens.h"

void test_screen_parse_synthetic_with_palette_tail(void) {
    /* 16x1, 2 planes row-interleaved, then 4 colours as (hi, lo) 12-bit word pairs */
    uint8_t data[4 + 16] = {
        0x80, 0x00, 0xC0, 0x00,
        0x00, 0x00, 0x00, 0x00,   /* colour 0: (0x000, 0x000) */
        0x00, 0xF0, 0x00, 0xF0,   /* colour 1: (0x0F0, 0x0F0) -> 00FF00 */
        0x00, 0x0F, 0x00, 0x0F,   /* colour 2: (0x00F, 0x00F) -> 0000FF */
        0x0F, 0xFF, 0x00, 0x00,   /* colour 3: (0xFFF, 0x000) -> F0F0F0 */
    };
    sk_screen s;
    sk_err e = {0};
    CHECK(screen_parse(data, sizeof data, 16, 1, 2, true, 0, true, &s, &e));
    CHECK_EQ(screen_pixel(&s, 0, 0), 3);
    CHECK_EQ(screen_pixel(&s, 1, 0), 2);
    CHECK_EQ(screen_pixel(&s, 2, 0), 0);
    CHECK(s.has_palette);
    CHECK_EQ(s.palette[1], 0x00FF00);
    CHECK_EQ(s.palette[2], 0x0000FF);
    CHECK_EQ(s.palette[3], 0xF0F0F0);
    CHECK(!screen_parse(data, sizeof data - 1, 16, 1, 2, true, 0, true, &s, &e));
    CHECK_STR(e.msg, "needs");
    CHECK(!screen_parse(data, 3, 16, 1, 2, true, 0, false, &s, &e));
    CHECK_STR(e.msg, "needs");
    /* plane-sequential: plane 0 rows then plane 1 rows */
    uint8_t seq[4] = { 0x80, 0x00, 0xC0, 0x00 };
    CHECK(screen_parse(seq, sizeof seq, 16, 1, 2, false, 0, false, &s, &e));
    CHECK_EQ(screen_pixel(&s, 0, 0), 3);
    CHECK_EQ(screen_pixel(&s, 1, 0), 2);
    CHECK(!s.has_palette);
}

void test_screen_real_title_palette(void) {              /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0};
    sk_err e = {0};
    CHECK(disks_open(&d, roms, &e));
    uint8_t *f = NULL;
    size_t n = 0;
    CHECK(disks_read_file_no(&d, 1, 78, true, &f, &n, &e));
    sk_screen s;
    CHECK(screen_parse_title(f, n, &s, &e));
    CHECK_EQ(s.w, 320);
    CHECK_EQ(s.h, 256);
    CHECK_EQ(s.palette[0], 0);
    CHECK_EQ(s.palette[1], 0x8B6FA3);
    free(f);
    CHECK(disks_read(&d, 0, "language.pak", &f, &n, &e));
    uint8_t *plain = NULL;
    size_t pn = 0;
    extern bool bk_unpack(const uint8_t *, size_t, uint8_t **, size_t *, sk_err *);
    CHECK(bk_unpack(f, n, &plain, &pn, &e));
    CHECK(screen_parse_language(plain, pn, &s, &e));
    CHECK_EQ(s.w, 320);
    CHECK_EQ(s.h, 200);
    CHECK(!s.has_palette);
    free(plain);
    free(f);
    disks_close(&d);
}
