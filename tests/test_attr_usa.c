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


void test_ladder_3_catches_a_standing_kid(void) {
    CHECK(make_game());
    sk_player *p = &g_g.player;
    p->x = 5 * 16 + 4; p->y = 10 * 16 - 38; p->state = 0;           /* y + 40 - cell = 2: low on it */
    run(3, 0, 5, 10);
    CHECK_EQ(p->state, 0x1C); CHECK_EQ(p->no_push_up, 0xFF);
    p->state = 0; p->y = 10 * 16 - 30;                               /* 10: higher */
    run(3, 0, 5, 10);
    CHECK_EQ(p->state, 0x18);
    p->state = 8;                                                    /* busy: nothing */
    run(3, 0, 5, 10);
    CHECK_EQ(p->state, 8);
    drop_game();
}

void test_spikes_4_hurt_the_kid(void) {
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 4; g_g.player.y = 10 * 16 - 20; g_g.player.health = 3; g_g.player.invulnerable = 0;
    run(4, 0, 5, 10);
    CHECK_EQ(g_g.player.health, 2);
    drop_game();
}

void test_hoop_36_scores_baskets_and_an_extra_life(void) {
    CHECK(make_game());
    sk_player *p = &g_g.player;
    p->ball_free = 1; p->lives = 2;
    for (int k = 0; k < 8; k++) {
        g_g.ball.x = (5 * 16 + 2) << 16; g_g.ball.y = (10 * 16 - 2) << 16; g_g.ball.prev_y = 10 * 16 - 8; g_g.ball.vy = 1 << 16;
        run(36, 1, 5, 10);
    }
    CHECK_EQ(p->lives, 3); CHECK_EQ(g_g.sfx_last, 9); CHECK(g_g.hoop_count < 0);   /* the eighth: a life, then no more */
    run(36, 1, 5, 10);
    CHECK_EQ(p->lives, 3);
    g_g.hoop_count = 0;
    g_g.ball.vy = -(1 << 16);                                        /* going up through it: no basket */
    run(36, 1, 5, 10);
    CHECK_EQ(g_g.hoop_count, 0);
    drop_game();
}
