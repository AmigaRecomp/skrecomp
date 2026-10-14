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
#include "game/platforms.h"

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


void test_kit_shade_11_12_by_height(void) {
    CHECK(make_game());
    g_g.player.y = 10 * 16 + 5;                                      /* d 4 */
    run(11, 0, 5, 10);
    CHECK_EQ(g_g.kit_current, (4 + 16) >> 2);
    run(12, 0, 5, 10);
    CHECK_EQ(g_g.kit_current, (28 - 4) >> 2);
    g_g.player.y = 10 * 16 - 1;
    g_g.kit_current = 0;
    run(11, 0, 5, 10);
    CHECK_EQ(g_g.kit_current, 0);                                    /* above the cell: nothing */
    drop_game();
}

void test_freeze_18_holds_the_kick_animation(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16; g_g.player.y = 10 * 16 - 20;
    run(18, 0, 5, 10);
    CHECK_EQ(g_g.player.anim_hold, 0xFF);
    drop_game();
}

void test_gap_wind_45_carries_and_belts_46_47_push(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16; g_g.player.y = 10 * 16 - 20;
    run(45, 0, 5, 10);
    CHECK_EQ(g_g.player.state, 0x38); CHECK_EQ(g_g.player.carry_dx, 10);
    g_g.player.state = 0; g_g.player.y = 10 * 16 - 20;
    run(46, 0, 5, 10);
    CHECK_EQ(g_g.player.carry_dx, -2);                               /* standing on it */
    g_g.ball.x = (5 * 16 + 4) << 16; g_g.ball.y = (10 * 16 - 8) << 16;
    run(47, 1, 5, 10);
    CHECK_EQ(g_g.bw.carry_dx, 2);
    drop_game();
}

void test_rail_arrows_48_to_51_steer_an_object(void) {
    CHECK(make_game());
    g_g.objs.marker_param = 0;
    sk_slot *s = spawn_object(&g_g, 200, 10 * 16, 21);
    CHECK(s != NULL); if (!s) { drop_game(); return; }
    s->t0 = 4;
    g_g.attrs.probe_slot = s; g_g.attrs.probe_x = 5 * 16 + 8; g_g.attrs.probe_y = 10 * 16 + 8;
    run(48, 2, 5, 10);
    CHECK_EQ(s->vy, -4); CHECK_EQ(s->vx, 0); CHECK(s->flags & 4); CHECK_EQ(g_g.attrs.probe_kind, 0xFF);
    run(50, 2, 5, 10);
    CHECK_EQ(s->vy, 4);
    run(49, 2, 5, 10);
    CHECK_EQ(s->vx, 4); CHECK_EQ(s->vy, 0); CHECK(s->flags & 1); CHECK_EQ(s->flags & 4, 0);
    run(51, 2, 5, 10);
    CHECK_EQ(s->flags & 1, 0);
    g_g.attrs.probe_y = 10 * 16 + 2;                                 /* off the arrow's middle (49 and 51 read y) */
    s->vx = 7;
    run(49, 2, 5, 10);
    CHECK_EQ(s->vx, 7);
    drop_game();
}

void test_rail_turn_21_turns_a_platform_a_quarter(void) {
    CHECK(make_game());
    g_g.objs.marker_param = 0;
    sk_slot *s = spawn_object(&g_g, 200, 10 * 16, 5);
    sk_platform *pf = s ? slot_platform(&g_g, s) : NULL;
    CHECK(pf != NULL); if (!pf) { drop_game(); return; }
    pf->pangle = 0x10; pf->pspeed = 4; pf->pmode = 0; s->t0 = 0;
    g_g.attrs.probe_slot = s;
    run(21, 2, 5, 10);
    CHECK_EQ(pf->pangle, 0xD0); CHECK_EQ(aux_d(s), 2);
    int32_t vx, vy; platform_velocity(0xD0, 4, &vx, &vy);
    CHECK_EQ(pf->pvx, vx); CHECK_EQ(pf->pvy, vy);
    pf->pangle = 0x10;
    run(21, 2, 5, 10);
    CHECK_EQ(pf->pangle, 0x10);                                      /* once per arrow */
    drop_game();
}

void test_hide_56_to_59_by_half_cell(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 12;
    run(56, 0, 5, 10); CHECK(g_g.player_hidden);
    g_g.player.x = 5 * 16 + 3;
    run(56, 0, 5, 10); CHECK(!g_g.player_hidden);
    run(57, 0, 5, 10); CHECK(g_g.player_hidden);
    g_g.player.y = 10 * 16 + 12;
    run(58, 0, 5, 10); CHECK(g_g.player_hidden);
    run(59, 0, 5, 10); CHECK(!g_g.player_hidden);
    drop_game();
}

void test_train_trigger_78_starts_one_train(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 3;
    run(78, 0, 5, 10);
    CHECK_EQ(g_g.post_flag, 0xFF); CHECK_EQ(count_type(23), 1);
    run(78, 0, 5, 10);
    CHECK_EQ(count_type(23), 1);                                     /* already running */
    g_g.post_flag = 0; g_g.player.x = 5 * 16 + 12;
    run(78, 0, 5, 10);
    CHECK_EQ(g_g.post_flag, 0); CHECK_EQ(count_type(23), 1);         /* the far half: nothing */
    run(79, 0, 5, 10);
    CHECK_EQ(g_g.post_flag, 0xFF); CHECK_EQ(count_type(23), 2);
    drop_game();
}
