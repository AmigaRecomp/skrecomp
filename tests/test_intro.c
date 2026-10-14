#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "app/intro.h"
#include "gfx/display_model.h"

void test_intro_fade_steps_floor_toward_the_target(void) {     /* lerp_colour 0x10122E */
    CHECK_EQ(intro_lerp(0x000, 0xFFF, 0), 0x000);
    CHECK_EQ(intro_lerp(0x000, 0xFFF, 16), 0xFFF);
    CHECK_EQ(intro_lerp(0x000, 0xF00, 8), 0x700);                /* 15 * 8 / 16 = 7.5: floor */
    CHECK_EQ(intro_lerp(0x003, 0x000, 8), 0x001);                /* -3 * 8 / 16 = -1.5: floor -2 */
    CHECK_EQ(intro_lerp(0x0F0, 0x0F0, 5), 0x0F0);
}

static sk_intro it;
void test_intro_types_text_one_and_ends_on_a_key(void) {       /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_err e = {0};
    static sk_sound snd;
    CHECK(disks_open(&d, roms, &e));
    sound_init(&snd, NULL, 48000);
    CHECK(intro_open(&it, &d, &snd, &e));
    if (!SK_OK(&e)) printf("intro: %s\n", e.msg);
    intro_step(&it, false);
    CHECK(snd.music_use4); CHECK(snd.pt.master_on);              /* music_start at position 1 on the first tick */
    for (int i = 1; i < 9 * 3; i++) intro_step(&it, false);      /* text 1, one character every 9 ticks: " TH" */
    static uint32_t out[SK_SCREEN_W * SK_SCREEN_H];
    intro_present(&it, out);
    int lit = 0;
    for (int y = 0; y < 16; y++) for (int x = 16; x < 48; x++) lit += out[y * SK_SCREEN_W + x] != 0;
    CHECK(lit > 20);                                             /* 'T' and 'H' in white */
    intro_step(&it, true);
    CHECK(it.done);                                              /* typewriter_wait polls the key at once */
    intro_free(&it); sound_free(&snd); disks_close(&d);
}

void test_intro_plays_through_in_about_111_seconds(void) {     /* ROM-backed: the whole script */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_err e = {0};
    static sk_sound snd;
    CHECK(disks_open(&d, roms, &e));
    sound_init(&snd, NULL, 48000);
    CHECK(intro_open(&it, &d, &snd, &e));
    int n = 0, coloured = 0;
    static uint32_t out[SK_SCREEN_W * SK_SCREEN_H];
    while (!it.done && n < 8000) {
        intro_step(&it, false); sound_frame(&snd, NULL, 0); n++;
        if (n == 1500) { intro_present(&it, out); for (int i = 0; i < 320 * 200; i++) coloured += out[i] != 0; }
    }
    CHECK(it.done);
    CHECK(n >= 5515 && n <= 5560);                               /* about 5546 ticks (intro.md, a READING sum) */
    CHECK(coloured > 20000);                                     /* the space ship, not a black screen */
    intro_free(&it); sound_free(&snd); disks_close(&d);
}

void test_intro_keeps_a_tap_until_its_next_poll_and_fades_text_one(void) {   /* frame_interrupt's quit_flag, text_on_black */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_err e = {0};
    static sk_sound snd;
    CHECK(disks_open(&d, roms, &e));
    sound_init(&snd, NULL, 48000);
    CHECK(intro_open(&it, &d, &snd, &e));
    while (it.op < 6 && !it.done) intro_step(&it, false);        /* text 1 typed: the fade to black */
    intro_step(&it, false);
    CHECK(it.copper[31] != 0);                                   /* fades from 0xEEE over 17 ticks, not cut */
    while (it.op < 8 && !it.done) intro_step(&it, false);        /* animation 1/0 */
    for (int i = 0; i < 30; i++) intro_step(&it, false);         /* among its delta frames: no poll here */
    intro_step(&it, true);                                       /* a tap */
    CHECK(!it.done);
    int n = 0;
    while (!it.done && n < 400) { intro_step(&it, false); n++; }
    CHECK(it.done);                                              /* at the next key frame */
    intro_free(&it); sound_free(&snd); disks_close(&d);
}
