#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "app/pictures.h"
#include "core/bytes.h"

static uint8_t pic[64000 + 1024];
static uint8_t pack[10 + 7920 + 4 + 2052 + 486 + 14400];
static sk_pictures pc;

static void make_map_data(void) {
    memset(pic, 0, sizeof pic);
    put_be16(pic + 64000 + 4 * 7, 0x0F00);                       /* colour 7: red high nibbles */
    memset(pack, 0, sizeof pack);
    uint16_t sizes[5] = { 7920, 4, 2052, 486, 14400 };
    for (int k = 0; k < 5; k++) put_be16(pack + 2 * k, sizes[k]);
    uint8_t *trophy = pack + 10 + 7920 + 4 + 2052 + 486;         /* chunk 4, piece 2 (Russia): top-left pixel colour 7 */
    trophy[0x1680] = 0x80; trophy[0x1680 + 8] = 0x80; trophy[0x1680 + 16] = 0x80; trophy[0x1680 + 24] = 0x80;
}

static int run_until_done(void) {
    int n = 0;
    while (!pictures_step(&pc, 0) && n < 5000) n++;
    return n + 1;
}

void test_world_map_zero_holds_201_frames(void) {
    make_map_data();
    sk_err e = {0};
    CHECK(pictures_map_from(&pc, pic, sizeof pic, pack, sizeof pack, 0, 0, &e));
    CHECK_EQ(run_until_done(), 201);                             /* vbl_counter > 0xC8, no input */
}

void test_world_map_draws_the_collected_pieces_only(void) {
    make_map_data();
    sk_err e = {0};
    CHECK(pictures_map_from(&pc, pic, sizeof pic, pack, sizeof pack, 0, 1u << 2, &e));
    CHECK_EQ(pc.px[143 * 320 + 9], 7);                           /* Russia's piece at (9, 143) */
    CHECK(pictures_map_from(&pc, pic, sizeof pic, pack, sizeof pack, 0, 0x1B, &e));
    CHECK_EQ(pc.px[143 * 320 + 9], 0);                           /* bit 2 clear: the silhouette stays */
    CHECK_EQ(pc.pal[7], 0xF00000u);
}

void test_world_map_one_waits_flies_and_holds(void) {
    make_map_data();
    sk_err e = {0};
    CHECK(pictures_map_from(&pc, pic, sizeof pic, pack, sizeof pack, 1, 0, &e));
    int n = run_until_done();                                    /* 51 standing, ~30 flying, 101 after */
    CHECK(n >= 51 + 29 + 101 && n <= 51 + 31 + 101);
}

void test_loading_strip_shows_25_frames_in_lines_58_to_141(void) {
    uint8_t strip[26880 + 1024];
    memset(strip, 0xFF, 26880);                                  /* every pixel colour 255 */
    memset(strip + 26880, 0, 1024);
    put_be16(strip + 26880 + 4 * 255, 0x0FFF);
    sk_err e = {0};
    CHECK(pictures_loading_from(&pc, strip, sizeof strip, &e));
    CHECK_EQ(pc.px[57 * 320], 0); CHECK_EQ(pc.px[58 * 320], 255); CHECK_EQ(pc.px[141 * 320 + 319], 255); CHECK_EQ(pc.px[142 * 320], 0);
    CHECK_EQ(run_until_done(), 25);
}

void test_ending_waits_for_fire_then_fades_64_frames(void) {
    static uint8_t end[81920 + 1024];
    memset(end, 0, sizeof end);
    sk_err e = {0};
    CHECK(pictures_ending_from(&pc, end, sizeof end, true, &e));
    for (int i = 0; i < 500; i++) CHECK(!pictures_step(&pc, 0));  /* no timeout */
    CHECK(!pictures_step(&pc, SK_IN_FIRE));
    CHECK(pc.fading);
    int n = 0;
    while (!pictures_step(&pc, 0) && n < 200) n++;
    CHECK_EQ(n + 1, 64);                                         /* fade_music_out */
}

void test_ending_goes_black_while_the_music_fades(void) {        /* copper_blank, COLOR00 = 0 (0x11F104) */
    static uint8_t end[81920 + 1024];
    memset(end, 0xFF, 81920); memset(end + 81920, 0, 1024); put_be16(end + 81920 + 4 * 255, 0x0FFF);
    sk_err e = {0};
    CHECK(pictures_ending_from(&pc, end, sizeof end, true, &e));
    static uint32_t out[320 * 256];
    pictures_present(&pc, out); CHECK(out[0] != 0);
    pictures_step(&pc, SK_IN_FIRE);
    pictures_present(&pc, out); CHECK_EQ(out[0], 0); CHECK_EQ(out[320 * 256 - 1], 0);
}
