#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/player.h"
#include "game/obj_helpers.h"
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

void test_tilt_plate_24_tilts_under_the_kid(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 20; g_g.player.y = 10 * 16 - 35;         /* right part of the plate */
    run(24, 0, 5, 10);
    CHECK_EQ(g_g.world.tile[10 * g_g.world.w + 5] & SK_INDEX_MASK, 0x1E7); CHECK_EQ(g_g.flush_count, 1);
    g_g.player.x = 5 * 16 + 4;
    run(24, 0, 5, 10);
    CHECK_EQ(g_g.world.tile[10 * g_g.world.w + 5] & SK_INDEX_MASK, 0x1E9);
    g_g.player.x = 5 * 16 - 3;
    run(24, 0, 5, 10);
    CHECK_EQ(g_g.world.tile[10 * g_g.world.w + 5] & SK_INDEX_MASK, 0x1E8);
    drop_game();
}

void test_spring_40_launches_a_falling_kid(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 8; g_g.player.y = 10 * 16 - 36; g_g.player.prev_y = 10 * 16 - 40;   /* falling onto it */
    run(40, 0, 5, 10);
    CHECK_EQ(g_g.player.vy, 26); CHECK_EQ(g_g.player.jump_hold, 16); CHECK_EQ(g_g.player.jumping, 0xFF);
    CHECK_EQ(g_g.sfx_last, 0x12); CHECK_EQ(g_g.player.jump_y, 10 * 16 - 36);
    drop_game();
}

void test_side_drain_41_needs_a_side_entry(void) {
    CHECK(make_game());
    g_g.ball.x = (5 * 16 + 4) * 65536; g_g.ball.y = (10 * 16) * 65536; g_g.ball.prev_x = 5 * 16 + 2;   /* from above */
    run(41, 1, 5, 10);
    CHECK_EQ(g_g.ball.vanishing, 0);
    g_g.ball.prev_x = 5 * 16 + 20;                                   /* from the side */
    run(41, 1, 5, 10);
    CHECK_EQ(g_g.ball.vanishing, 0xFF);
    drop_game();
}

void test_object_marks_52_77_and_trap_60_61(void) {
    CHECK(make_game());
    sk_slot s = {0};
    g_g.attrs.probe_slot = &s;
    run(52, 2, 5, 10);
    CHECK(s.flags & 0x10); CHECK_EQ(g_g.attrs.probe_kind, 0xFF);
    run(77, 2, 5, 10);
    CHECK_EQ(aux_c(&s), 5);
    g_g.player.x = 5 * 16 + 4; g_g.player.y = 10 * 16 - 45; g_g.player.vy = -2;
    run(60, 0, 5, 10);
    CHECK_EQ(count_type(104), 1);                                    /* the fall trigger */
    g_g.player.x = 8 * 16 + 4; g_g.player.y = 10 * 16; g_g.player.dying = 0;
    run(61, 0, 8, 10);
    CHECK(g_g.player.dying);                                         /* a deadly drain */
    drop_game();
}
