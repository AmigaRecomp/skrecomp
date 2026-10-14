#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "core/bytes.h"
#include "app/frontend.h"
#include "game/input.h"

static sk_frontend fe;
static uint8_t title[SK_TITLE_SIZE];
static uint8_t lang[40768];

static sk_fe_result press(uint8_t in) { frontend_step(&fe, 0, 0); return frontend_step(&fe, in, 0); }

void test_language_screen_moves_without_wrapping_and_fire_chooses(void) {
    memset(&fe, 0, sizeof fe);
    sk_err e = {0};
    CHECK(frontend_language_from(&fe, NULL, lang, sizeof lang, &e));
    CHECK_EQ(fe.mode, SK_FE_LANGUAGE); CHECK_EQ(fe.language, 0);
    press(SK_IN_LEFT); CHECK_EQ(fe.language, 0);                 /* clamped at UK */
    for (int i = 0; i < 6; i++) press(SK_IN_RIGHT);
    CHECK_EQ(fe.language, 4);                                    /* clamped at Spain */
    frontend_step(&fe, SK_IN_RIGHT, 0); CHECK_EQ(fe.language, 4);
    CHECK_EQ(press(SK_IN_FIRE), SK_FE_LANGUAGE_CHOSEN);
    frontend_free(&fe);
}

void test_title_menu_wraps_and_starts_on_fire(void) {
    memset(&fe, 0, sizeof fe);
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, NULL, title, sizeof title, &e));
    CHECK_EQ(fe.item, 0);
    press(SK_IN_UP); CHECK_EQ(fe.item, 2);                       /* wraps 0 -> 2 */
    press(SK_IN_DOWN); CHECK_EQ(fe.item, 0);
    CHECK_EQ(frontend_step(&fe, SK_IN_FIRE, 0), SK_FE_START);    /* item 0: fire held, not an edge (0x11CCDE) */
    frontend_free(&fe);
}

void test_title_menu_times_out_after_750_frames(void) {
    memset(&fe, 0, sizeof fe);
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, NULL, title, sizeof title, &e));
    int n = 1;
    while (frontend_step(&fe, 0, 0) == SK_FE_STAY && n < 2000) n++;
    CHECK_EQ(n, 750);
    CHECK(frontend_menu_from(&fe, NULL, title, sizeof title, &e));
    for (int i = 0; i < 700; i++) frontend_step(&fe, 0, 0);
    press(SK_IN_DOWN);                                           /* any move restarts the count */
    CHECK_EQ(fe.attract_counter, 1);
    CHECK_EQ(frontend_step(&fe, 0, SK_FE_KEY_QUIT), SK_FE_QUIT);
    frontend_free(&fe);
}

void test_title_menu_shows_the_kit_on_the_kid(void) {            /* apply_kit_preview 0x11D3DC */
    memset(title, 0, sizeof title);
    for (int k = 0; k < 16; k++) { put_be16(title + 0x14400 + 3 * 64 + 4 * k, (uint16_t)k); put_be16(title + 0x14400 + 5 * 64 + 4 * k, (uint16_t)(0x100 + k)); }
    memset(&fe, 0, sizeof fe);
    fe.shirt = 3; fe.shorts = 5;
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, NULL, title, sizeof title, &e));
    CHECK_EQ(fe.pal[0x70], 0x000000u);   CHECK_EQ(fe.pal[0x7C], 0x0000C0u);   /* shirt entries 0..12: hi nibbles 0x00C -> blue C0 */
    CHECK_EQ(fe.pal[0x6C], 0x0000C0u);   CHECK_EQ(fe.pal[0x6F], 0x0000F0u);   /* entries 12..15 */
    CHECK_EQ(fe.pal[0x8B], 0x100000u);   CHECK_EQ(fe.pal[0x1E], 0x100040u);   /* shorts 0 -> 0x8B, 4 -> 0x87 and 0x1E */
    CHECK_EQ(fe.pal[0x1F], 0x1000B0u);   CHECK_EQ(fe.pal[0x80], 0x1000B0u);
    CHECK_EQ(fe.pal[255], 0x4488CCu);                            /* COLOR255 = 0x48C, the panels' box */
    frontend_free(&fe);
}

void test_title_sparkle_plays_eleven_steps_after_fifty_frames(void) {   /* title_sparkle_tick 0x11CEFC */
    memset(title, 0, sizeof title);
    memset(&fe, 0, sizeof fe);
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, NULL, title, sizeof title, &e));
    for (int i = 0; i < 49; i++) frontend_step(&fe, 0, 0);
    CHECK(!fe.sparkle_on);
    frontend_step(&fe, 0, 0);
    CHECK(fe.sparkle_on); CHECK_EQ(fe.sparkle_wait, 150);       /* the next waits: 150, 20, 150, 100 */
    for (int i = 0; i < 11; i++) frontend_step(&fe, 0, 0);
    CHECK(!fe.sparkle_on); CHECK_EQ(fe.sparkle_step, 0);
    frontend_free(&fe);
}

void test_title_menu_needs_fire_released_once(void) {          /* ruling: the original's disk loading gave time to let go */
    memset(&fe, 0, sizeof fe);
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, NULL, title, sizeof title, &e));
    CHECK_EQ(frontend_step(&fe, SK_IN_FIRE, 0), SK_FE_STAY);     /* still held from the last screen */
    CHECK_EQ(frontend_step(&fe, SK_IN_FIRE, 0), SK_FE_STAY);
    frontend_step(&fe, 0, 0);
    CHECK_EQ(frontend_step(&fe, SK_IN_FIRE, 0), SK_FE_START);
    frontend_free(&fe);
}

#include "core/hunk.h"
static sk_image fimg;
static void make_fimg(void) {                                    /* hunks 0 and 1, zeroed, with a font glyph and a kit row */
    image_free(&fimg); memset(&fimg, 0, sizeof fimg);
    fimg.count = 2;
    fimg.seg[0] = (sk_segment){ SK_HUNK_BASE(0), 0x20000, calloc(1, 0x20000), false };
    fimg.seg[1] = (sk_segment){ SK_HUNK_BASE(1), 0x10000, calloc(1, 0x10000), false };
    uint8_t *d = fimg.seg[1].mem;
    d[0x4148 + ('A' - 0x20) * 16] = 0x80;                        /* 'A' row 0: body at pixel 0 */
    d[0x4148 + ('A' - 0x20) * 16 + 1] = 0x40;                    /* outline at pixel 1 */
    put_be16(d + 0x934 + 2 * 16 + 12, 7); put_be16(d + 0x934 + 2 * 16 + 14, 11);   /* hi-score entry 2: kit 7, 11 */
}
static void run(int n) { for (int i = 0; i < n; i++) frontend_step(&fe, 0, 0); }

void test_kit_design_grows_its_panel_and_changes_the_kit(void) {   /* kit_design 0x11CF92 */
    memset(title, 0, sizeof title);
    for (int i = 0; i < 320 * 256; i += 7) title[i % 81920] = 0x55;
    make_fimg();
    memset(&fe, 0, sizeof fe);
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, &fimg, title, sizeof title, &e));
    uint8_t before = fe.base[152 * 320 + 220];
    press(SK_IN_DOWN);
    press(SK_IN_FIRE);                                           /* KIT DESIGN */
    CHECK_EQ(fe.mode, SK_FE_KIT);
    run(16);                                                     /* the grow: 16 frames */
    CHECK_EQ(fe.base[118 * 320 + 156], 255); CHECK_EQ(fe.base[185 * 320 + 283], 255);   /* the final 128 x 68 box */
    press(SK_IN_UP); CHECK_EQ(fe.shirt, 13);                     /* wraps 0 -> 13 */
    press(SK_IN_RIGHT); CHECK_EQ(fe.shorts, 1);
    frontend_step(&fe, 0, SK_FE_KEY_F1 << 2);                    /* F3: hi-score entry 2's kit */
    CHECK_EQ(fe.shirt, 7); CHECK_EQ(fe.shorts, 11);
    press(SK_IN_FIRE);
    CHECK_EQ(fe.mode, SK_FE_MENU);
    CHECK_EQ(fe.base[152 * 320 + 220], before);                  /* the saved rectangle is back */
    frontend_free(&fe);
    image_free(&fimg); memset(&fimg, 0, sizeof fimg);
}

void test_options_toggle_music_and_ask_for_the_tutorial(void) {  /* options_menu 0x11D504 */
    memset(title, 0, sizeof title);
    memset(&fe, 0, sizeof fe);
    bool music_off = false, muted = false;
    fe.music_off = &music_off; fe.sfx_muted = &muted;
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, NULL, title, sizeof title, &e));
    press(SK_IN_UP);                                             /* OPTIONS */
    press(SK_IN_FIRE);
    CHECK_EQ(fe.mode, SK_FE_OPTIONS); CHECK_EQ(fe.option, 4);    /* starts on EXIT */
    run(16);
    press(SK_IN_DOWN); CHECK_EQ(fe.option, 0);                   /* wraps over five */
    press(SK_IN_FIRE); CHECK(music_off); CHECK(fe.music_changed);
    press(SK_IN_DOWN); press(SK_IN_FIRE); CHECK(muted);
    press(SK_IN_DOWN); press(SK_IN_FIRE);                        /* TUTORIAL: leaves the panel */
    CHECK_EQ(fe.mode, SK_FE_MENU); CHECK(fe.tutorial);
    CHECK_EQ(frontend_step(&fe, 0, 0), SK_FE_TUTORIAL);
    frontend_free(&fe);
}

void test_panel_text_draws_body_and_outline_colours(void) {    /* draw_text_8x8 0x11DC86 */
    make_fimg();
    memset(title, 0, sizeof title);
    memset(&fe, 0, sizeof fe);
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, &fimg, title, sizeof title, &e));
    for (int i = 0; i < 16; i++) fe.base[10 * 320 + 40 + i] = 255;
    frontend_text(&fe, "AA", 40, 10);
    CHECK_EQ(fe.base[10 * 320 + 40], 254); CHECK_EQ(fe.base[10 * 320 + 41], 253); CHECK_EQ(fe.base[10 * 320 + 42], 255);
    CHECK_EQ(fe.base[10 * 320 + 48], 254);
    frontend_free(&fe);
    image_free(&fimg); memset(&fimg, 0, sizeof fimg);
}

void test_tab_shows_the_version_box_and_holds_the_menu(void) {   /* show_version 0x11CD64 */
    memset(title, 0, sizeof title);
    memset(&fe, 0, sizeof fe);
    sk_err e = {0};
    CHECK(frontend_menu_from(&fe, NULL, title, sizeof title, &e));
    frontend_step(&fe, 0, SK_FE_KEY_TAB);
    frontend_step(&fe, SK_IN_DOWN, SK_FE_KEY_TAB);
    CHECK_EQ(fe.item, 0);                                        /* the menu waits while TAB is held */
    static uint32_t out[320 * 256];
    frontend_present(&fe, out);
    CHECK_EQ(fe.px[250 * 320 + 300], 255);                       /* the box x 232..319, y 248..255 */
    frontend_step(&fe, 0, 0);
    frontend_present(&fe, out);
    CHECK_EQ(fe.px[250 * 320 + 300], 0);
    frontend_free(&fe);
}
