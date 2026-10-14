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
