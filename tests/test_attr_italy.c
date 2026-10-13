#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/player.h"
#include "game/resources.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static sk_resources g_res; static sk_section g_sec; static uint8_t g_sheet[64 * 24 / 8 * 5 * 8];
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv); memset(&g_res, 0, sizeof g_res);
    g_sec.data = g_sheet; g_sec.len = sizeof g_sheet;
    for (int i = 0; i < SK_SLOTS; i++) g_res.slot[i] = &g_sec;
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    if (!game_setup_level(&g_g, &g_lv, 0)) return false;
    g_g.res = &g_res;
    objects_reset(&g_g);
    g_g.player.state = 0; g_g.player.x = 1000; g_g.player.y = 12 * 16 - 41;
    g_g.ball = (sk_ball){0}; g_g.ball.x = 1000 << 16;
    return true;
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static void run(int attr, int kind, int cx, int cy) { g_g.attrs.probe_kind = kind; attr_table[attr](&g_g, cx, cy); }

void test_hazard_63_takes_a_rising_ball_and_hurts_from_below(void) {
    CHECK(make_game());
    g_g.ball.x = (5 * 16 + 4) << 16; g_g.ball.y = (10 * 16 + 4) << 16; g_g.ball.vy = 2 << 16;
    run(63, 1, 5, 10);
    CHECK_EQ(g_g.ball.vanishing, 0);                                 /* falling: passes */
    g_g.ball.vy = -(2 << 16); g_g.ball.vx = 3 << 16; g_g.player.state = 16;
    run(63, 1, 5, 10);
    CHECK_EQ(g_g.ball.vanishing, 0xFF); CHECK_EQ(g_g.ball.vx, 0); CHECK_EQ(g_g.sfx_last, 14); CHECK_EQ(g_g.player.state, 0);
    g_g.player.x = 5 * 16 + 4; g_g.player.y = 10 * 16 + 4; g_g.player.vy = 2; g_g.player.health = 2; g_g.player.invulnerable = 0;
    run(63, 0, 5, 10);
    CHECK(g_g.player.health < 2 || g_g.player.state == 52);
    drop_game();
}

void test_hazard_64_takes_a_falling_ball_and_hurts_from_above(void) {
    CHECK(make_game());
    g_g.ball.x = (5 * 16 + 4) << 16; g_g.ball.y = (10 * 16 + 4) << 16; g_g.ball.vy = -(2 << 16);
    run(64, 1, 5, 10);
    CHECK_EQ(g_g.ball.vanishing, 0);                                 /* rising: passes */
    g_g.ball.vy = 2 << 16;
    run(64, 1, 5, 10);
    CHECK_EQ(g_g.ball.vanishing, 0xFF);
    g_g.player.x = 5 * 16 + 4; g_g.player.y = 10 * 16 - 30; g_g.player.vy = 0; g_g.player.health = 2; g_g.player.invulnerable = 0;
    run(64, 0, 5, 10);
    CHECK(g_g.player.health < 2 || g_g.player.state == 52);
    drop_game();
}

void test_ball_targets_69_71_74_count_apart(void) {
    CHECK(make_game());
    static const int attrs[3] = { 69, 71, 74 }, kinds[3] = { 4, 5, 4 }, index[3] = { 2, 4, 7 };
    for (int a = 0; a < 3; a++) {
        for (int i = 0; i < 8; i++) {
            g_g.ball.x = (5 * 16 + 4) << 16; g_g.ball.y = (10 * 16 + 4) << 16; g_g.ball.vx = 3 << 16; g_g.ball.vy = 0;
            g_g.player.ball_free = 1;
            run(attrs[a], 1, 5, 10);
        }
        CHECK_EQ(g_g.ball_target[index[a]], 0xFF);                   /* 0x300395 + index */
        CHECK_EQ(g_g.objs.marker_param, kinds[a]);
    }
    CHECK_EQ(g_g.ball_target[3], 0); CHECK_EQ(g_g.ball_target[5], 0); CHECK_EQ(g_g.ball_target[6], 0);
    drop_game();
}
