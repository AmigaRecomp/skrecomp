#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "formats/bobs.h"
#include "formats/level.h"
#include "gfx/display_model.h"
#include "gfx/framebuffer.h"

/* Tile 0: every pixel colour 5 (planes 0 and 2). Tile 1: colour 9 (planes 0 and 3) in the top-left pixel only. */
static uint8_t tile_bytes[2 * SK_TILE_BYTES];
static sk_tileset two_tiles(void) {
    memset(tile_bytes, 0, sizeof tile_bytes);
    for (int y = 0; y < 16; y++) {
        uint8_t *row = tile_bytes + y * 8;
        row[0] = row[1] = 0xFF;             /* plane 0 */
        row[4] = row[5] = 0xFF;             /* plane 2 */
    }
    uint8_t *t1 = tile_bytes + SK_TILE_BYTES;
    t1[0] = 0x80;                           /* plane 0, row 0, pixel 0 */
    t1[6] = 0x80;                           /* plane 3, row 0, pixel 0 */
    sk_tileset ts = {0};
    ts.count = 2;
    ts.tiles = tile_bytes;
    return ts;
}

void test_tile_blit_writes_nibble_and_keeps_other_group(void) {
    sk_tileset ts = two_tiles();
    sk_fb fb;
    CHECK(fb_init(&fb, 64, 32));
    fb_clear(&fb, 0xA0);                    /* group 1 = 0xA everywhere */
    fb_blit_tile(&fb, &ts, 0, 16, 0, 0);
    CHECK_EQ(fb.px[0 * 64 + 16], 0xA5);
    CHECK_EQ(fb.px[15 * 64 + 31], 0xA5);
    CHECK_EQ(fb.px[0], 0xA0);
    CHECK_EQ(fb.px[16 * 64 + 16], 0xA0);
    fb_blit_tile(&fb, &ts, 1, 0, 16, 1);
    CHECK_EQ(fb.px[16 * 64 + 0], 0x90);     /* group 1 nibble replaced by 9 */
    CHECK_EQ(fb.px[16 * 64 + 1], 0x00);     /* tile value 0 clears the nibble */
    fb_blit_tile(&fb, &ts, 7, 0, 0, 0);     /* out-of-range index: nothing drawn */
    CHECK_EQ(fb.px[0], 0xA0);
    fb_free(&fb);
    CHECK(fb.px == NULL);
}

void test_tile_blit_clips(void) {
    sk_tileset ts = two_tiles();
    sk_fb fb;
    CHECK(fb_init(&fb, 64, 32));
    fb_clear(&fb, 0xA0);
    fb_blit_tile(&fb, &ts, 0, -8, 24, 0);
    CHECK_EQ(fb.px[24 * 64 + 0], 0xA5);
    CHECK_EQ(fb.px[24 * 64 + 7], 0xA5);
    CHECK_EQ(fb.px[24 * 64 + 8], 0xA0);
    CHECK_EQ(fb.px[31 * 64 + 7], 0xA5);
    CHECK_EQ(fb.px[23 * 64 + 7], 0xA0);
    fb_blit_tile(&fb, &ts, 0, 60, -8, 0);
    CHECK_EQ(fb.px[0 * 64 + 60], 0xA5);
    CHECK_EQ(fb.px[7 * 64 + 63], 0xA5);
    CHECK_EQ(fb.px[8 * 64 + 63], 0xA0);
    fb_blit_tile(&fb, &ts, 0, 64, 0, 0);    /* entirely outside: nothing changes */
    fb_blit_tile(&fb, &ts, 0, 0, 32, 0);
    CHECK_EQ(fb.px[0 * 64 + 59], 0xA0);
    CHECK_EQ(fb.px[31 * 64 + 8], 0xA0);
    fb_free(&fb);
}

void test_bob_blit_respects_mask_and_clips(void) {   /* Review Focus 4 */
    /* frame 16x4: colour 3 everywhere, mask row 0 = 0xFF00 (left 8 pixels), other rows masked out */
    uint8_t raw[5 * 4 * 2];
    memset(raw, 0, sizeof raw);
    memset(raw + 0 * 8, 0xFF, 8);
    memset(raw + 1 * 8, 0xFF, 8);
    raw[4 * 8 + 0] = 0xFF;
    sk_section sec = { true, raw, sizeof raw };
    sk_frames f = {0};
    sk_err e = {0};
    CHECK(bobs_split(&sec, 16, 4, &f, &e));
    uint8_t buf[64 * 32 + 16];
    memset(buf, 0xA0, sizeof buf);
    memset(buf + 64 * 32, 0xEE, 16);        /* canary after the buffer */
    sk_fb fb = { 64, 32, buf };
    fb_blit_bob(&fb, &f.frame[0], -4, 30, 0);
    CHECK_EQ(buf[30 * 64 + 0], 0xA3);       /* frame x 4..7 land on buffer x 0..3 */
    CHECK_EQ(buf[30 * 64 + 3], 0xA3);
    CHECK_EQ(buf[30 * 64 + 4], 0xA0);       /* frame x 8: mask clear */
    CHECK_EQ(buf[31 * 64 + 0], 0xA0);       /* frame row 1: mask clear */
    for (int i = 0; i < 16; i++) CHECK_EQ(buf[64 * 32 + i], 0xEE);   /* rows 32, 33 never written */
    fb_blit_bob(&fb, &f.frame[0], 56, 0, 0);
    CHECK_EQ(buf[0 * 64 + 63], 0xA3);
    fb_blit_bob(&fb, &f.frame[0], 0, -1, 0);   /* mask row 0 above the top: nothing */
    CHECK_EQ(buf[0], 0xA0);
    bobs_free(&f);
}

void test_bob_blit_second_group(void) {
    uint8_t raw[5 * 2 * 2];
    memset(raw, 0, sizeof raw);
    memset(raw + 0 * 4, 0xFF, 4);           /* plane 0 */
    memset(raw + 1 * 4, 0xFF, 4);           /* plane 1: colour 3 */
    memset(raw + 4 * 4, 0xFF, 4);           /* mask: all */
    sk_section sec = { true, raw, sizeof raw };
    sk_frames f = {0};
    sk_err e = {0};
    CHECK(bobs_split(&sec, 16, 2, &f, &e));
    sk_fb fb;
    CHECK(fb_init(&fb, 32, 4));
    fb_clear(&fb, 0x05);
    fb_blit_bob(&fb, &f.frame[0], 0, 0, 1);
    CHECK_EQ(fb.px[0], 0x35);
    CHECK_EQ(fb.px[1 * 32 + 15], 0x35);
    CHECK_EQ(fb.px[16], 0x05);
    CHECK_EQ(fb.px[2 * 32], 0x05);
    fb_free(&fb);
    bobs_free(&f);
}

void test_display_model_constants(void) {
    CHECK_EQ(SK_FB_W, 352);
    CHECK_EQ(SK_VIEW_W, 288);
    CHECK_EQ(SK_VIEW_H, 200);
    CHECK_EQ(SK_VIEW_X + SK_VIEW_W, 320);
    CHECK_EQ(SK_PLAY_X + SK_VIEW_W, 304);
    CHECK_EQ(SK_HUD_Y + SK_HUD_H, 226);
    CHECK_EQ(SK_DUAL_PLAYFIELD, 1);
    CHECK_EQ(SK_GROUP_TILES_A, 0);
    CHECK_EQ(SK_GROUP_TILES_B, 1);
}
