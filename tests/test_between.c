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

#include <stdlib.h>
#include "fixture.h"
#include "game/game.h"
#include "game/level_flow.h"

void test_bonus_check_sends_the_world_to_its_bonus_level(void) {
    static sk_game g;
    memset(&g, 0, sizeof g);
    game_new_game(&g);
    g.level_number = 9;                                          /* England's last scoreboard done */
    bonus_check(&g);
    CHECK_EQ(g.level_number, 9);                                 /* nothing pending */
    g.bonus_pending = 0xFF; g.bonus_tokens = 7;
    bonus_check(&g);
    CHECK_EQ(g.level_number, 0x34); CHECK_EQ(g.fulltime_level, 9); CHECK_EQ(g.timer_rate, 0x32);
    CHECK(g.timeout_armed); CHECK_EQ(g.bonus_tokens, 0);
    static const int from[5] = { 0x0A, 0x14, 0x1D, 0x26, 0x30 }, to[5] = { 0x34, 0x35, 0x36, 0x37, 0x38 };
    for (int k = 0; k < 5; k++) { g.level_number = from[k]; bonus_check(&g); CHECK_EQ(g.level_number, to[k]); }
    g.level_number = 0x31; bonus_check(&g); CHECK_EQ(g.level_number, 0x39);
}

void test_bonus_level_setup_keeps_its_clock_and_time_out(void) {
    static sk_game g;
    static sk_level lv;
    uint8_t *d = NULL; size_t n = 0; sk_err e = {0};
    memset(&g, 0, sizeof g); memset(&lv, 0, sizeof lv);
    CHECK(load_fixture("level_play.bin", &d, &n) && level_parse(d, n, &lv, &e));
    game_new_game(&g);
    CHECK_EQ(g.timer_rate, 0x5DC);
    g.timer_rate = 0x32; g.timeout_armed = true;
    CHECK(game_setup_level(&g, &lv, 36));
    CHECK_EQ(g.timer_rate, 0x32); CHECK(g.timeout_armed);
    g.level_timer = 0; g.bonus_pending = 0xFF;
    timeout_check(&g);
    CHECK_EQ(g.bonus_pending, 0); CHECK_EQ(g.timer_rate, 0x5DC); CHECK(!g.timeout_armed); CHECK_EQ(g.player.state, 68);
    game_close(&g);
}
