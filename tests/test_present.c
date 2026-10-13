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
    uint16_t c17w[4] = { 0x0AF, 0x0BF, 0x0DF, 0x0EF }, c18w[3] = { 0x0CF, 0x0DF, 0x0FF };
    sk_sky sky = { c17w, 4, 1, c18w, 3, 2 };                    /* COLOR17 from word 1 on, COLOR18 from word 2 on */
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
    CHECK_EQ(out[1], rgb12_to_xrgb(0x0BF));        /* window line 0 -> COLOR17 word 1 */
    CHECK_EQ(out[2], rgb12_to_xrgb(0x0FF));        /* COLOR18 word 2 (the table's last word) */
    CHECK_EQ(out[3], 0xFF0000);
    CHECK_EQ(out[4], 0x0000FF);
    CHECK_EQ(out[16 + 0], rgb12_to_xrgb(0x0DF));   /* window line 1 -> COLOR17 word 2 */
    CHECK_EQ(out[32 + 0], rgb12_to_xrgb(0x0EF));   /* line 2 -> word 3; beyond the table the last word holds */
    sk_sky none = { NULL, 0, 0, NULL, 0, 0 };
    present_rows(&fb, &p, &none, 8, 1, 16, 1, out);
    CHECK_EQ(out[1], 0x00FF00);                    /* no sky table: colour 17 stays set B colour 1 */
    fb_free(&fb);
}

void test_present_screen_places_the_play_window(void) {
    sk_tileset a = {0}, b = {0};
    a.colours[1] = 0xF00;
    sk_palette p;
    palette_from_tilesets(&p, &a, &b);
    sk_sky none = { NULL, 0, 0, NULL, 0, 0 };
    sk_fb fb;
    CHECK(fb_init(&fb, SK_FB_W, SK_FB_H));
    fb_clear(&fb, 0x01);                           /* everything playfield-1 colour 1 */
    uint32_t *out = calloc(SK_SCREEN_W * SK_SCREEN_H, sizeof *out);
    memset(out, 0x55, SK_SCREEN_W * SK_SCREEN_H * sizeof *out);
    present_screen(&fb, &p, &none, NULL, 0, out);
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

void test_present_sprite_priority(void) {
    sk_fb fb; CHECK(fb_init(&fb, SK_FB_W, SK_FB_H));
    fb_clear(&fb, 0);
    fb.px[SK_VIEW_X + 10] = 0x03;                                /* playfield 1 pixel at window (10, 0) */
    fb.px[SK_VIEW_X + 11] = 0x20;                                /* playfield 2 only at (11, 0) */
    sk_palette pal = {0};
    for (int i = 0; i < 256; i++) pal.xrgb[i] = 0x100u * (uint32_t)i;
    static sk_sprite_layer L;
    memset(&L, 0, sizeof L);
    L.spr[0].visible = true; L.spr[0].x = SK_VIEW_X + 10; L.spr[0].y = 0;
    L.spr[0].px[0] = 5; L.spr[0].px[1] = 5;                     /* covers window x 10 and 11 */
    static uint32_t out[SK_SCREEN_W * SK_SCREEN_H];
    sk_sky none = { NULL, 0, 0, NULL, 0, 0 };
    present_screen(&fb, &pal, &none, &L, 0, out);
    CHECK_EQ(out[SK_PLAY_X + 10], pal.xrgb[3]);                  /* behind playfield 1 */
    CHECK_EQ(out[SK_PLAY_X + 11], pal.xrgb[48 + 5]);             /* over playfield 2 */
    L.spr[0].in_front = true;
    present_screen(&fb, &pal, &none, &L, 0, out);
    CHECK_EQ(out[SK_PLAY_X + 10], pal.xrgb[48 + 5]);
    L.spr[0].x = SK_VIEW_X - 63; L.spr[0].y = SK_VIEW_H - 1;     /* clipped at the edges: no crash */
    present_screen(&fb, &pal, &none, &L, 0, out);
    CHECK_EQ(out[SK_PLAY_X + 0 + (SK_PLAY_Y + SK_VIEW_H - 1) * SK_SCREEN_W], pal.xrgb[SK_PF2_COLOUR_BASE]);   /* only transparent pixels reach the window */
    fb_free(&fb);
}

void test_present_curtain_blacks_rows(void) {
    sk_fb fb; CHECK(fb_init(&fb, SK_FB_W, SK_FB_H));
    fb_clear(&fb, 0x01);                                        /* playfield 1 colour 1 everywhere */
    sk_palette pal = {0};
    pal.xrgb[1] = 0xABCDEF;
    static uint32_t out[SK_SCREEN_W * SK_SCREEN_H];
    sk_sky none = { NULL, 0, 0, NULL, 0, 0 };
    present_screen(&fb, &pal, &none, NULL, 10, out);
    CHECK_EQ(out[(SK_PLAY_Y + 9) * SK_SCREEN_W + SK_PLAY_X + 5], 0);                   /* hidden top row */
    CHECK_EQ(out[(SK_PLAY_Y + 10) * SK_SCREEN_W + SK_PLAY_X + 5], 0xABCDEF);          /* first visible row */
    CHECK_EQ(out[(SK_PLAY_Y + SK_VIEW_H - 10) * SK_SCREEN_W + SK_PLAY_X + 5], 0);      /* hidden bottom row */
    CHECK_EQ(out[(SK_PLAY_Y + SK_VIEW_H - 11) * SK_SCREEN_W + SK_PLAY_X + 5], 0xABCDEF);
    fb_free(&fb);
}
