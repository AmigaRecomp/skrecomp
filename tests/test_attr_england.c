#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/attr_england.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/obj_helpers.h"
#include "game/player.h"
#include "game/resources.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static sk_resources g_res; static sk_section g_sec; static uint8_t g_sheet[64 * 64];
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
    g_g.ball = (sk_ball){0}; g_g.ball.x = 1000 << 16; g_g.ball.y = 10 << 16;
    return true;
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static int count_type(int type) { int c = 0; for (int i = 0; i < 16; i++) c += g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == type * 4; return c; }
static void run(int attr, int kind, int cx, int cy) { g_g.attrs.probe_kind = kind; attr_table[attr](&g_g, cx, cy); }
static void ball_at(int x, int y, int vx, int vy) {
    g_g.ball.x = x << 16; g_g.ball.y = y << 16; g_g.ball.vx = vx << 16; g_g.ball.vy = vy << 16; g_g.player.ball_free = 1;
}
static int queued(int cx, int cy, int *kind) {
    for (int i = 0; i < 32; i++)
        if (g_g.attrs.queue[i].used && g_g.attrs.queue[i].cell == cy * g_g.world.w + cx) { *kind = g_g.attrs.queue[i].kind; return 1; }
    return 0;
}

void test_coin_scores_and_sparkles(void) {                     /* attributes 0 and 19 */
    CHECK(make_game());
    int kind = -1;
    g_g.player.x = 5 * 16 + 4;
    run(0, 0, 5, 11);
    CHECK_EQ(g_g.player.score, 0x20); CHECK_EQ(g_g.sfx_last, 1);
    CHECK(queued(5, 11, &kind)); CHECK_EQ(kind, 0);
    ball_at(8 * 16 + 14, 11 * 16 - 14, 0, 0);                   /* the ball: strictly inside +-15 */
    run(0, 1, 8, 11);
    CHECK_EQ(g_g.player.score, 0x40);
    ball_at(8 * 16 + 15, 11 * 16, 0, 0);                        /* 15 px right of cell 8: outside */
    run(0, 1, 8, 11);
    CHECK_EQ(g_g.player.score, 0x40);
    g_g.player.x = 9 * 16;
    run(19, 0, 9, 11);
    CHECK(queued(9, 11, &kind)); CHECK_EQ(kind, 1);              /* coin_keep: the second sparkle, tile 0x1EF stays */
    drop_game();
}

void test_full_queue_writes_the_fallback_tile(void) {
    CHECK(make_game());
    for (int i = 0; i < 32; i++) queue_tile_change(&g_g, i, 2, 0, 0);
    queue_tile_change(&g_g, 3, 3, 1, 0x1EF);
    CHECK_EQ(g_g.world.live[3 * g_g.world.w + 3], 0x1EF); CHECK_EQ(world_tile_index(&g_g.world, 3, 3), 0x1EF);
    drop_game();
}

void test_card_is_collected(void) {                            /* attributes 37 and 66 */
    CHECK(make_game());
    g_g.world.live[11 * g_g.world.w + 5] = (uint16_t)(0x60 << 9 | 0x33);
    g_g.player.x = 5 * 16 + 10;
    run(37, 0, 5, 11);
    CHECK_EQ(g_g.hud_card_flag, 0xFF); CHECK_EQ(g_g.player.score, 0x1000); CHECK_EQ(g_g.sfx_last, 9);
    CHECK_EQ(count_type(123), 1); CHECK_EQ(g_g.world.live[11 * g_g.world.w + 5], 0);
    g_g.world.live[11 * g_g.world.w + 6] = (uint16_t)(0x60 << 9 | 0x33);
    g_g.player.x = 6 * 16 + 10;
    run(66, 0, 6, 11);
    CHECK_EQ(g_g.world.live[11 * g_g.world.w + 6], 0x33);        /* the attribute is stripped, the tile stays */
    drop_game();
}

void test_kit_shades_follow_the_position(void) {               /* attributes 5 6 7 9 10 */
    CHECK(make_game());
    g_g.player.x = 4 * 16 + 9;                                    /* 9 px in: (9 >> 1) & 12 = 4 -> kit 1 */
    run(5, 0, 4, 11); CHECK_EQ(g_g.kit_current, 1);
    run(6, 0, 4, 11); CHECK_EQ(g_g.kit_current, 2);              /* reversed: 12 - 4 */
    run(9, 0, 4, 11); CHECK_EQ(g_g.kit_current, 5);              /* dark: + 16 */
    run(10, 0, 4, 11); CHECK_EQ(g_g.kit_current, 6);
    g_g.player.y = 11 * 16 + 13; run(7, 0, 4, 11); CHECK_EQ(g_g.kit_current, 3);
    g_g.player.x = 4 * 16 - 1; run(5, 0, 4, 11); CHECK_EQ(g_g.kit_current, 3);   /* left of the cell: no change */
    g_g.ball.prev_x = 4 * 16 + 30; run(5, 1, 4, 11); CHECK_EQ(g_g.kit_extra, 3);  /* the ball uses its previous x */
    drop_game();
}

void test_drain_hurts_the_kid_and_swallows_the_ball(void) {    /* attributes 1 and 42 */
    CHECK(make_game());
    g_g.player.x = 5 * 16 + 4; g_g.player.y = 11 * 16 - 20; g_g.player.health = 2; g_g.player.invulnerable = 0;
    run(1, 0, 5, 11);
    CHECK(g_g.player.health < 2 || g_g.player.state == 52);
    ball_at(8 * 16 + 4, 11 * 16, 3, 1);
    g_g.player.state = 16;
    run(1, 1, 8, 11);
    CHECK_EQ(g_g.ball.vanishing, 0xFF); CHECK_EQ(g_g.ball.vx, 0); CHECK_EQ(g_g.sfx_last, 14); CHECK_EQ(g_g.player.state, 0);
    g_g.ball.vanishing = 0; ball_at(9 * 16, 11 * 16, 2, 2); g_g.ball.prev_y = 11 * 16 - 4;
    run(42, 1, 9, 11);
    CHECK_EQ(g_g.ball.vanishing, 0xFF);
    drop_game();
}

void test_bump_block_falls_and_is_restored(void) {             /* attribute 25 and type 122 */
    CHECK(make_game());
    int cell = 9 * g_g.world.w + 6;
    g_g.world.live[cell] = (uint16_t)(0x70 << 9 | 1); g_g.world.tile[cell] = 1;
    g_g.player.x = 6 * 16 + 8; g_g.player.y = 9 * 16 - 40; g_g.player.vy = 0;   /* standing on it */
    run(25, 0, 6, 9);
    CHECK_EQ(g_g.world.live[cell], 0); CHECK_EQ(count_type(122), 1); CHECK(g_g.mask.dirty);
    CHECK_EQ(g_g.bump_restores, 1);
    for (int i = 0; i < 30; i++) { g_g.frame_ticks++; update_objects(&g_g); }
    CHECK_EQ(count_type(122), 0);
    init_player(&g_g);                                            /* a respawn puts the block back */
    CHECK_EQ(g_g.world.live[cell], (uint16_t)(0x70 << 9 | 1)); CHECK_EQ(g_g.bump_restores, 0);
    drop_game();
}

void test_message_sign_raises_the_box_once(void) {             /* attributes 27 and 28, message_box_tick */
    CHECK(make_game());
    ball_at(5 * 16 + 4, 10 * 16, 3, 1);                          /* speed 4 */
    run(27, 0, 5, 10);
    CHECK_EQ(g_g.message_box_request, 0);                         /* the kid: nothing */
    run(27, 1, 5, 10);
    CHECK_EQ(g_g.message_box_request, 1); CHECK_EQ(count_type(125), 1); CHECK_EQ(count_type(126), 1);
    CHECK_EQ(g_g.world.live[10 * g_g.world.w + 5], 0); CHECK_EQ(g_g.sfx_last, 8);
    message_box_tick(&g_g);
    CHECK_EQ(g_g.message_box_timer, 250); CHECK_EQ(g_g.message_wait, 1); CHECK_EQ(g_g.sfx_last, 10); CHECK_EQ(g_g.message_box_request, 0);
    ball_at(6 * 16 + 4, 10 * 16, 3, 1);
    run(28, 1, 6, 10);
    CHECK_EQ(g_g.message_box_request, 0);                         /* the timer blocks a second request */
    drop_game();
}

void test_butterflies_rise_from_a_hit_bush(void) {             /* attribute 62 */
    CHECK(make_game());
    for (int i = 0; i < 8; i++) { ball_at((5 + i) * 16 + 4, 10 * 16 + 4, 2, 1); run(62, 1, 5 + i, 10); }
    CHECK_EQ(count_type(103), 5);                                 /* flutter_max */
    CHECK_EQ(g_g.flutter_count, 5);
    drop_game();
}

void test_ball_target_counts_hits(void) {                      /* attribute 67 and types 115, 112, 123 */
    CHECK(make_game());
    for (int i = 0; i < 4; i++) { ball_at(5 * 16 + 4, 10 * 16 + 4, 3, 0); run(67, 1, 5, 10); }
    CHECK_EQ(g_g.ball_target[0], 4); CHECK_EQ(count_type(123), 1); CHECK_EQ(g_g.score_value, 0x4000);
    for (int i = 0; i < 4; i++) { ball_at(5 * 16 + 4, 10 * 16 + 4, 3, 0); run(67, 1, 5, 10); }
    CHECK_EQ(g_g.ball_target[0], 0xFF); CHECK_EQ(count_type(112), 1);
    ball_at(5 * 16 + 4, 10 * 16 + 4, 3, 0); run(67, 1, 5, 10);
    CHECK_EQ(g_g.ball_target[0], 0xFF);                           /* done */
    drop_game();
}
