#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/ball.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/player.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    return game_setup_level(&g_g, &g_lv, 0);
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static void place_ball(int x, int y, int32_t vx, int32_t vy) {
    g_g.ball = (sk_ball){0};
    g_g.ball.x = x << 16; g_g.ball.y = y << 16; g_g.ball.vx = vx; g_g.ball.vy = vy; g_g.ball.rest_timer = 50;
    g_g.player.x = 1000; g_g.player.ball_hold = 0;            /* the kid far away */
}

void test_sin_and_atan_tables(void) {
    CHECK_EQ(cos_256(0), 0); CHECK_EQ(cos_256(0x40), 0x40); CHECK_EQ(cos_256(0xC0), -0x40);
    CHECK_EQ(cos_256(0x80), 0);                                  /* the table's quirk: a half turn reads 0 */
    CHECK_EQ(sin_256(0), 0x40); CHECK_EQ(sin_256(0x80), -0x40);
    CHECK_EQ(atan2_256(10, 0), 0); CHECK_EQ(atan2_256(10, 10), 0x20); CHECK_EQ(atan2_256(0, 10), 0x40);
    CHECK_EQ(atan2_256(-10, 0), 0x80); CHECK_EQ(atan2_256(0, -10), 0xC0);
}

void test_ball_falls_and_comes_to_rest(void) {
    CHECK(make_game());
    place_ball(8 * 16, 2 * 16, 0, 0);
    int32_t vy1;
    update_ball(&g_g); vy1 = g_g.ball.vy;
    CHECK(vy1 > 0 && vy1 <= 0x4000);                            /* gravity 0x4000 less drag vy/32 */
    int bounces = 0, prev_vy = 0, top_after_first = 9999;
    for (int i = 0; i < 1500 && !g_g.ball.at_rest; i++) {
        update_ball(&g_g);
        if (prev_vy > 0 && g_g.ball.vy <= 0) bounces++;
        if (bounces == 1 && (g_g.ball.y >> 16) < top_after_first) top_after_first = g_g.ball.y >> 16;
        prev_vy = g_g.ball.vy;
        CHECK((g_g.ball.y >> 16) + 15 < 12 * 16 + 4);           /* never sinks into the floor (row 12) */
    }
    CHECK(bounces >= 1);
    CHECK(top_after_first > 2 * 16);                             /* it bounces lower than it was dropped from */
    CHECK_EQ(g_g.ball.at_rest, 1);
    CHECK((g_g.ball.y >> 16) + 16 >= 12 * 16 - 2);               /* resting on the floor */
    drop_game();
}

void test_ball_bounces_off_the_wall_with_reversed_vx(void) {
    CHECK(make_game());
    place_ball(20 * 16 - 40, 12 * 16 - 16, 0x40000, 0);          /* rolling right at 4 px/frame into column 20 */
    int32_t vx_after = 0;
    for (int i = 0; i < 40; i++) { update_ball(&g_g); if (g_g.ball.vx < 0) { vx_after = g_g.ball.vx; break; } }
    CHECK(vx_after < 0);
    CHECK(-vx_after < 0x40000);                                  /* friction took some speed */
    CHECK((g_g.ball.x >> 16) + 14 < 20 * 16);                     /* the disc's solid pixels (columns 1..14) stay out of the wall */
    drop_game();
}

void test_ball_falls_out_and_goes_inactive(void) {             /* Review Focus 1 */
    CHECK(make_game());
    place_ball(27 * 16, 10 * 16, 0, 0);                           /* over the pit */
    for (int i = 0; i < 400 && !g_g.ball.inactive; i++) update_ball(&g_g);
    CHECK_EQ(g_g.ball.inactive, 1);
    CHECK_EQ(g_g.ball.y >> 16, g_g.world.fg_max_y + 184);
    int32_t y = g_g.ball.y;
    update_ball(&g_g);
    CHECK_EQ(g_g.ball.y, y);                                     /* inactive: not updated */
    place_ball(-3, 5 * 16, -0x20000, 0);                          /* left of column 0: x clamps to 0, vx turns */
    ball_move(&g_g);
    CHECK_EQ(g_g.ball.x, 0); CHECK(g_g.ball.vx > 0);
    drop_game();
}

void test_ball_sprite_frame_follows_the_roll_phase(void) {
    CHECK(make_game());
    place_ball(100, 100, 0, 0);
    g_g.ball.prev_x = 96;                                        /* moved 4 px right */
    g_g.bw.roll_phase = 14;
    static sk_sprite s;
    g_g.view.cam_x = 50; g_g.view.cam_y = 40;
    ball_sprites(&g_g, &s);
    CHECK_EQ(g_g.bw.roll_phase, (14 + 4) & 15);
    CHECK(s.visible); CHECK_EQ(s.x, 100 - 50 + 16); CHECK_EQ(s.y, 100 - 40);
    g_g.ball.inactive = 1; ball_sprites(&g_g, &s); CHECK(!s.visible);
    drop_game();
}
