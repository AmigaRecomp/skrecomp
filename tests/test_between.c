#include <string.h>
#include "check.h"
#include "game/between.h"

void test_play_level_entry_runs_maps_and_endings_on_their_own_slot(void) {
    sk_tables t = {0};
    uint8_t seq[] = { 30, 0, 1, 12, 31, 6 };
    memcpy(t.sequence, seq, sizeof seq);
    int ln = 0;
    sk_run r = play_level_entry(&t, &ln);                        /* world_map: level_number += 1 inside */
    CHECK_EQ(r.kind, SK_RUN_MAP); CHECK_EQ(r.map_index, 0); CHECK_EQ(ln, 1);
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_LEVEL); CHECK_EQ(r.data_index, 0); CHECK_EQ(ln, 1);   /* a level moves on by its exit walk */
    ln = 3;
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_LEVEL); CHECK_EQ(r.data_index, 12);  /* the scoreboard is a tile level */
    ln = 4;
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_MAP); CHECK_EQ(r.map_index, 1); CHECK_EQ(ln, 5);
}

void test_play_level_entry_walks_the_games_tail(void) {
    sk_tables t = {0};
    uint8_t seq[] = { 28, 12, 41, 29, 35, 42, 36 };
    memcpy(t.sequence, seq, sizeof seq);
    int ln = 2;
    sk_run r = play_level_entry(&t, &ln);                        /* ALIEN is 8b: its slot is passed over (ruling) */
    CHECK_EQ(r.kind, SK_RUN_ENDING); CHECK_EQ(ln, 4);            /* end_screens: level_number += 1 */
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_MAP); CHECK_EQ(r.map_index, 5); CHECK_EQ(ln, 5);
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_GAME_END); CHECK_EQ(ln, 5);          /* 42: play_level returns */
    ln = SK_LEVEL_SEQUENCE_LEN;                                  /* past the table: the end, no read */
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_GAME_END);
}

static int fly(sk_flight *f, int *frames) {
    *frames = 0;
    while (!flight_step(f) && *frames < 1000) ++*frames;
    return ++*frames;
}

void test_flight_heads_down_to_italy_with_the_empty_shadow(void) {
    sk_flight f;
    flight_init(&f, 149, 44, 164, 70);                           /* map 1: England -> Italy, atan2 +43 */
    CHECK_EQ(f.big_off, 0); CHECK_EQ(f.small_off, 0x1D4);
    int n; fly(&f, &n);
    CHECK_EQ(f.x >> 16, 164);
    CHECK(((f.y >> 16) - 70) * ((f.y >> 16) - 70) <= 1);       /* 1 px a frame along the heading lands on the target */
    CHECK(n >= 29 && n <= 31);                                   /* sqrt(15^2 + 26^2) = 30 */
}

void test_flight_heads_up_to_russia_and_level_elsewhere(void) {
    sk_flight f;
    flight_init(&f, 164, 70, 179, 35);                           /* atan2 -48: up */
    CHECK_EQ(f.big_off, 0x558); CHECK_EQ(f.small_off, 0xEA);
    flight_init(&f, 179, 35, 274, 65);                           /* +12: level */
    CHECK_EQ(f.big_off, 0x2AC); CHECK_EQ(f.small_off, 0);
}

void test_flight_wraps_east_from_japan_to_the_usa(void) {
    sk_flight f;
    flight_init(&f, 274, 65, 63, 61);                            /* dx = 63 - 274 + 320 = 109, always east */
    CHECK(f.sx > 0);
    int n; fly(&f, &n);
    CHECK_EQ((f.x >> 16) - 320, 63);                             /* the second test of the loop: x - 320 == target */
    CHECK(n >= 108 && n <= 111);
    CHECK_EQ(flight_draw_x(&f), 63);                             /* drawn wrapped into the picture */
}
