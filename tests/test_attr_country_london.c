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
static int count_type(int type) { int c = 0; for (int i = 0; i < 16; i++) c += g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == type * 4; return c; }

void test_deadly_water_43_kills(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 4; g_g.player.y = 11 * 16 - 30; g_g.player.health = 2;
    run(43, 1, 5, 11);
    CHECK_EQ(g_g.player.health, 2);                                 /* the ball probe: nothing */
    run(43, 0, 5, 11);
    CHECK_EQ(g_g.player.health, 0); CHECK_EQ(g_g.player.state, 20);
    drop_game();
}

void test_splash_55_once_until_a_respawn(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 4; g_g.player.y = 11 * 16 - 20;
    run(55, 0, 5, 11);
    CHECK_EQ(count_type(115), 1); CHECK_EQ(g_g.sfx_last, 21); CHECK_EQ(g_g.splash_done, 0xFF);
    run(55, 0, 5, 11);
    CHECK_EQ(count_type(115), 1);
    drop_game();
}

void test_oneway_38_pushes_back(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 12; g_g.player.prev_x = 5 * 16 + 14; g_g.player.speed = 8;   /* moving left into it */
    run(38, 0, 5, 11);
    CHECK_EQ(g_g.player.x, 5 * 16 + 14); CHECK_EQ(g_g.player.speed, 0);
    g_g.ball.x = (5 * 16 + 4) << 16; g_g.ball.prev_x = 5 * 16 + 6; g_g.ball.vx = (int32_t)0xFFFE0000;
    run(38, 1, 5, 11);
    CHECK_EQ(g_g.ball.x >> 16, 5 * 16 + 6); CHECK_EQ(g_g.ball.vx, 0x20000);
    drop_game();
}

void test_bare_sign_26_breaks_without_a_message(void) {
    CHECK(make_game());
    g_g.ball.x = (5 * 16 + 4) << 16; g_g.ball.y = (10 * 16) << 16; g_g.ball.vx = 3 << 16; g_g.ball.vy = 1 << 16;
    run(26, 1, 5, 10);
    CHECK_EQ(count_type(125), 1); CHECK_EQ(g_g.message_box_request, 0);
    drop_game();
}

void test_ball_targets_70_72_73(void) {
    CHECK(make_game());
    static const int attrs[3] = { 70, 72, 73 }, kinds[3] = { 0, 6, 6 };
    for (int a = 0; a < 3; a++) {
        for (int i = 0; i < 8; i++) {
            g_g.ball.x = (5 * 16 + 4) << 16; g_g.ball.y = (10 * 16 + 4) << 16; g_g.ball.vx = 3 << 16; g_g.ball.vy = 0;
            g_g.player.ball_free = 1;
            run(attrs[a], 1, 5, 10);
        }
        static const int index[3] = { 3, 5, 6 };                     /* 0x300398, 0x30039A, 0x30039B */
        CHECK_EQ(g_g.ball_target[index[a]], 0xFF);
        CHECK_EQ(g_g.objs.marker_param, kinds[a]);
    }
    drop_game();
}
