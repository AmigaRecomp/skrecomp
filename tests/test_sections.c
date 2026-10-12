#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/bytes.h"
#include "core/disks.h"
#include "formats/bobs.h"
#include "formats/sections.h"

void test_sections_parses_packed_and_raw(void) {
    uint8_t *d = NULL;
    size_t n = 0;
    sk_sections s = {0};
    sk_err e = {0};
    CHECK(load_fixture("sections_tiny.bin", &d, &n));
    CHECK(sections_parse(d, n, &s, &e));
    CHECK_EQ(s.count, 2);
    CHECK(s.sec[0].packed);
    CHECK_EQ(s.sec[0].len, 480);
    CHECK(!s.sec[1].packed);
    CHECK_EQ(s.sec[1].len, 4);
    CHECK(s.sec[1].data && s.sec[1].len == 4 && memcmp(s.sec[1].data, "\x00\x16\x00\x00", 4) == 0);
    uint8_t *frames = NULL;
    size_t fn = 0;
    CHECK(load_fixture("frames_tiny.plain", &frames, &fn));
    CHECK(frames && s.sec[0].data && fn == 480 && memcmp(s.sec[0].data, frames, 480) == 0);
    sections_free(&s);
    CHECK(s.sec == NULL && s.count == 0);
    free(frames);
    free(d);
}

void test_sections_rejects_bad_layouts(void) {
    sk_sections s = {0};
    sk_err e = {0};
    uint8_t odd[8] = { 0x00, 0x03, 0, 0, 0, 0, 0, 0 };                /* odd header length */
    CHECK(!sections_parse(odd, sizeof odd, &s, &e));
    CHECK_STR(e.msg, "bad section header length");
    uint8_t decreasing[16] = { 0x00, 0x04, 0x00, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };  /* offset 2 < header 4 */
    CHECK(!sections_parse(decreasing, sizeof decreasing, &s, &e));
    CHECK_STR(e.msg, "not increasing");
    uint8_t mismatch[10] = { 0x00, 0x02, 0x00, 0x00, 0x00, 0x99, 1, 2, 3, 4 };   /* length field 0x99 != 8 */
    CHECK(!sections_parse(mismatch, sizeof mismatch, &s, &e));
    CHECK_STR(e.msg, "does not match");
    uint8_t tiny[3] = { 0, 2, 0 };
    CHECK(!sections_parse(tiny, sizeof tiny, &s, &e));
    CHECK_STR(e.msg, "too short");
    CHECK(s.sec == NULL);
}

void test_bobs_split_and_pixels(void) {
    uint8_t *d = NULL;
    size_t n = 0;
    sk_sections s = {0};
    sk_frames f = {0};
    sk_err e = {0};
    CHECK(load_fixture("sections_tiny.bin", &d, &n));
    CHECK(sections_parse(d, n, &s, &e));
    CHECK(bobs_split(&s.sec[0], 16, 16, &f, &e));
    CHECK_EQ(f.count, 3);
    CHECK_EQ(f.width, 16);
    CHECK_EQ(f.height, 16);
    CHECK_EQ(f.frame[2].row_bytes, 2);
    CHECK_EQ(bobs_pixel(&f.frame[0], 0, 0), 1);          /* make_frame(16, 16, colour 1): solid, fully opaque */
    CHECK_EQ(bobs_pixel(&f.frame[2], 15, 15), 1);
    CHECK(f.frame[1].planes[0] == s.sec[0].data + 160);
    bobs_free(&f);
    CHECK(f.frame == NULL && f.count == 0);
    /* a hand-made 16x4 frame: colour 3 everywhere, mask covers the left 8 pixels of row 0 only */
    uint8_t raw[5 * 4 * 2];
    memset(raw, 0, sizeof raw);
    memset(raw + 0 * 8, 0xFF, 8);                         /* plane 0 */
    memset(raw + 1 * 8, 0xFF, 8);                         /* plane 1 */
    raw[4 * 8 + 0] = 0xFF;                                /* mask row 0 = 0xFF00 */
    sk_section sec = { true, raw, sizeof raw };
    CHECK(bobs_split(&sec, 16, 4, &f, &e));
    CHECK_EQ(f.count, 1);
    CHECK_EQ(bobs_pixel(&f.frame[0], 0, 0), 3);
    CHECK_EQ(bobs_pixel(&f.frame[0], 8, 0), -1);
    CHECK_EQ(bobs_pixel(&f.frame[0], 0, 1), -1);
    bobs_free(&f);
    sections_free(&s);
    free(d);
}

void test_bobs_rejects_partial_frame(void) {            /* Review Focus 1 of the sprites plan, kept */
    uint8_t buf[479] = {0};
    sk_section sec = { true, buf, sizeof buf };
    sk_frames f = {0};
    sk_err e = {0};
    CHECK(!bobs_split(&sec, 16, 16, &f, &e));
    CHECK_STR(e.msg, "whole number of frames");
    CHECK(f.frame == NULL);
    CHECK(!bobs_split(&sec, 12, 16, &f, &e));
    CHECK_STR(e.msg, "multiple of 16");
}

void test_bobs_real_kid(void) {                          /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0};
    sk_err e = {0};
    CHECK(disks_open(&d, roms, &e));
    uint8_t *file = NULL;
    size_t n = 0;
    CHECK(disks_read_file_no(&d, 3, 36, false, &file, &n, &e));   /* disk 3: the kid's sheet */
    sk_sections s = {0};
    CHECK(sections_parse(file, n, &s, &e));
    CHECK(s.count >= 1 && s.sec[0].packed);
    CHECK_EQ(s.sec[0].len, 46080);
    sk_frames f = {0};
    CHECK(bobs_split(&s.sec[0], 48, 64, &f, &e));
    CHECK_EQ(f.count, 24);
    int visible = 0, masked = 0;
    for (int y = 0; y < 64; y++)
        for (int x = 0; x < 48; x++) {
            int v = bobs_pixel(&f.frame[0], x, y);
            if (v < 0) masked++; else if (v > 0) visible++;
        }
    CHECK(visible > 100 && masked > 100);
    bobs_free(&f);
    sections_free(&s);
    free(file);
    disks_close(&d);
}
