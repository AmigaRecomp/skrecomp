#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/player.h"
#include "game/scroll_modes.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(int index) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    if (!game_setup_level(&g_g, &g_lv, 0)) return false;
    scroll_modes_setup(&g_g, index);
    return true;
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static void drive(int n) { for (int i = 0; i < n; i++) { g_g.frame_ticks++; scroll_modes_frame(&g_g); backdrop_stream_step(&g_g); } }

void test_train_setup_starts_the_ramp(void) {
    CHECK(make_game(20));
    CHECK_EQ(g_g.scroll_mode, 1); CHECK_EQ(g_g.train_flags, 5); CHECK_EQ(g_g.train_timer, 500); CHECK_EQ(g_g.bg_speed, 0);
    CHECK_EQ(g_g.stream.base, g_g.world.bg_max_x - 0x20); CHECK_EQ(g_g.stream.width, 0x130); CHECK_EQ(g_g.stream.cursor, 0x130);
    CHECK_EQ(g_g.view.bg_x, g_g.stream.base + 32); CHECK_EQ(g_g.train_level, 15);
    drop_game();
    CHECK(make_game(0));
    CHECK_EQ(g_g.scroll_mode, 0);
    drop_game();
}

void test_train_speeds_up_to_ten_and_rattles(void) {
    CHECK(make_game(20));
    g_g.frame_ticks = 0;
    drive(32 * 10);
    CHECK_EQ(g_g.bg_speed, 10); CHECK_EQ(g_g.train_flags & 4, 0);
    g_g.shake_amplitude = 0;
    drive(32);
    CHECK_EQ(g_g.shake_amplitude, 1);                                /* full speed: it rattles every 32 frames */
    drop_game();
}

void test_train_tunnel_fades_down_and_back(void) {
    CHECK(make_game(20));
    g_g.train_timer = 0;
    drive(1);
    CHECK(g_g.train_flags & 0x20); CHECK_EQ(g_g.train_fade, (int8_t)0xE6); CHECK_EQ(g_g.stream.width, 0xA0);
    CHECK_EQ(g_g.stream.base, g_g.world.bg_max_x - 0x160);
    static const int dark[4] = { 100, 150, 200, 250 }; (void)dark;
    drive(15);
    CHECK_EQ(g_g.train_level, 15);                                   /* d still >= 11: no step yet */
    drive(1);
    CHECK_EQ(g_g.train_level, 14);
    drive(9);
    CHECK_EQ(g_g.train_level, 5);
    drive(1);
    CHECK_EQ(g_g.train_level, 4); CHECK_EQ(g_g.train_fade, 0);       /* the tunnel: 5/16 bright */
    g_g.train_timer = 0;
    drive(1);
    CHECK_EQ(g_g.train_flags & 0x20, 0); CHECK_EQ(g_g.train_fade, 0x14); CHECK_EQ(g_g.stream.base, 0); CHECK_EQ(g_g.stream.width, 0x7E0);
    drive(20);
    CHECK_EQ(g_g.train_level, 15); CHECK_EQ(g_g.train_fade, 0);
    drop_game();
}

void test_backdrop_streams_new_columns_from_the_window(void) {
    CHECK(make_game(20));
    g_g.bg_speed = 10; g_g.train_flags = 0;
    int x0 = g_g.view.bg_x, cur = g_g.stream.cursor;
    drive(1);
    CHECK_EQ(g_g.view.bg_x, x0 - 10);
    int i = 0;
    while (g_g.stream.cursor == cur && i++ < 4) drive(1);
    CHECK_EQ(g_g.stream.cursor, cur - 16);
    int k = g_g.view.bg_x >> 4;
    CHECK_EQ(g_g.stream.col[k & 31], (g_g.stream.cursor + g_g.stream.base) >> 4);   /* the column that came in */
    g_g.view.bg_x = 20; g_g.stream.last = 20;
    drive(1);
    CHECK_EQ(g_g.view.bg_x, 20 - 10 + 0x5800);                       /* wraps far right, the ring unchanged */
    drop_game();
}

void test_battle_ship_rocks_and_drifts(void) {
    CHECK(make_game(15));
    CHECK_EQ(g_g.scroll_mode, 2); CHECK_EQ(g_g.bg_speed, 1); CHECK_EQ(g_g.stream.base, g_g.world.bg_max_x - 0x40);
    g_g.frame_ticks = 13;
    drive(1);                                                        /* frame 14: entry 7 */
    CHECK_EQ(g_g.shake_amplitude, 3);
    CHECK_EQ(g_g.stream.base, 0x20); CHECK_EQ(g_g.stream.width, g_g.world.bg_max_x - 0x70);
    g_g.frame_ticks = 55;
    drive(1);
    CHECK_EQ(g_g.shake_amplitude, -1);
    drop_game();
}

void test_train_exit_walks_left_once_stopped(void) {
    CHECK(make_game(20));
    sk_player *p = &g_g.player;
    p->state = 60; g_g.bg_speed = 5; p->x = (int16_t)(g_g.camera_x + 100);
    dispatch_player_state(&g_g);
    CHECK(g_g.train_flags & 0x10); CHECK_EQ(g_g.train_timer, 150); CHECK_EQ(p->timer, 9); CHECK_EQ(p->input, SK_IN_LEFT);
    for (int i = 0; i < 9; i++) dispatch_player_state(&g_g);
    CHECK_EQ(p->input, 0); CHECK_EQ(g_g.level_outcome & 0x80, 0);    /* the train still moving: wait */
    g_g.bg_speed = 0;
    dispatch_player_state(&g_g);
    CHECK_EQ(p->input, SK_IN_LEFT); CHECK_EQ(g_g.level_outcome & 0x80, 0);
    p->x = (int16_t)(g_g.camera_x + 32);
    dispatch_player_state(&g_g);
    CHECK_EQ(g_g.level_outcome, 0x81);
    drop_game();
}
