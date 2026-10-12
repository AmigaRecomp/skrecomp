#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "formats/level.h"
#include "gfx/display_model.h"
#include "gfx/framebuffer.h"
#include "gfx/palette.h"
#include "gfx/present.h"

void test_rgb12_to_xrgb(void) {
    CHECK_EQ(rgb12_to_xrgb(0x0BF), 0x00BBFF);
    CHECK_EQ(rgb12_to_xrgb(0xFFF), 0xFFFFFF);
    CHECK_EQ(rgb12_to_xrgb(0x000), 0x000000);
    CHECK_EQ(rgb12_to_xrgb(0x0F0), 0x00FF00);
}

void test_palette_from_tilesets_follows_the_register_layout(void) {
    sk_tileset a = {0}, b = {0};
    a.colours[1] = 0xF00;
    a.colours[15] = 0x123;
    b.colours[1] = 0x0F0;
    b.colours[15] = 0x456;
    sk_palette p;
    palette_from_tilesets(&p, &a, &b);
    CHECK_EQ(p.xrgb[0], 0);
    CHECK_EQ(p.xrgb[1], 0xFF0000);                 /* playfield 1 colours 1..15 = set A */
    CHECK_EQ(p.xrgb[15], rgb12_to_xrgb(0x123));
    CHECK_EQ(p.xrgb[16], 0);                       /* playfield 2 value 0 shows colour 0 */
    CHECK_EQ(p.xrgb[17], 0x00FF00);                /* playfield 2 colours 17..31 = set B */
    CHECK_EQ(p.xrgb[31], rgb12_to_xrgb(0x456));
    CHECK_EQ(p.xrgb[32], 0);
}

void test_present_rows_resolves_dual_playfield_and_sky(void) {
    sk_tileset a = {0}, b = {0};
    a.colours[1] = 0xF00;
    b.colours[1] = 0x0F0;
    b.colours[3] = 0x00F;
    sk_palette p;
    palette_from_tilesets(&p, &a, &b);
    uint16_t words[4] = { 0x0BF, 0x0CF, 0x0DF, 0x0EF };        /* two lines of (COLOR17, COLOR18) */
    sk_line_colours sky = { words, 4 };
    sk_fb fb;
    CHECK(fb_init(&fb, 32, 8));
    fb_clear(&fb, 0x00);
    fb.px[1 * 32 + 9] = 0x10;                      /* pf2 = 1 -> colour 17 -> sky word of its line */
    fb.px[1 * 32 + 10] = 0x20;                     /* pf2 = 2 -> colour 18 */
    fb.px[1 * 32 + 11] = 0x11;                     /* pf1 = 1 wins over pf2 */
    fb.px[1 * 32 + 12] = 0x30;                     /* pf2 = 3 -> set B colour 3 */
    fb.px[2 * 32 + 8] = 0x10;
    fb.px[3 * 32 + 8] = 0x10;
    uint32_t out[16 * 3];
    present_rows(&fb, &p, &sky, 8, 1, 16, 3, out);
    CHECK_EQ(out[0], 0);                           /* value 0: colour 0 */
    CHECK_EQ(out[1], rgb12_to_xrgb(0x0BF));        /* window line 0 -> words 0, 1 */
    CHECK_EQ(out[2], rgb12_to_xrgb(0x0CF));
    CHECK_EQ(out[3], 0xFF0000);
    CHECK_EQ(out[4], 0x0000FF);
    CHECK_EQ(out[16 + 0], rgb12_to_xrgb(0x0DF));   /* window line 1 -> words 2, 3 */
    CHECK_EQ(out[32 + 0], rgb12_to_xrgb(0x0DF));   /* beyond the table: the last pair holds */
    sk_line_colours none = { NULL, 0 };
    present_rows(&fb, &p, &none, 8, 1, 16, 1, out);
    CHECK_EQ(out[1], 0x00FF00);                    /* no sky table: colour 17 stays set B colour 1 */
    fb_free(&fb);
}

void test_present_screen_places_the_play_window(void) {
    sk_tileset a = {0}, b = {0};
    a.colours[1] = 0xF00;
    sk_palette p;
    palette_from_tilesets(&p, &a, &b);
    sk_line_colours none = { NULL, 0 };
    sk_fb fb;
    CHECK(fb_init(&fb, SK_FB_W, SK_FB_H));
    fb_clear(&fb, 0x01);                           /* everything playfield-1 colour 1 */
    uint32_t *out = calloc(SK_SCREEN_W * SK_SCREEN_H, sizeof *out);
    memset(out, 0x55, SK_SCREEN_W * SK_SCREEN_H * sizeof *out);
    present_screen(&fb, &p, &none, out);
    CHECK_EQ(out[0], 0);                                                   /* left border */
    CHECK_EQ(out[SK_PLAY_X - 1], 0);
    CHECK_EQ(out[SK_PLAY_X], 0xFF0000);                                    /* first play pixel */
    CHECK_EQ(out[SK_PLAY_X + SK_VIEW_W - 1], 0xFF0000);
    CHECK_EQ(out[SK_PLAY_X + SK_VIEW_W], 0);                               /* right border */
    CHECK_EQ(out[(SK_VIEW_H - 1) * SK_SCREEN_W + SK_PLAY_X], 0xFF0000);    /* last play line */
    CHECK_EQ(out[SK_VIEW_H * SK_SCREEN_W + SK_PLAY_X], 0);                 /* below the play area */
    CHECK_EQ(out[(SK_SCREEN_H - 1) * SK_SCREEN_W + SK_SCREEN_W - 1], 0);
    free(out);
    fb_free(&fb);
}
