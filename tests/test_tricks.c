#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/game.h"
#include "game/player.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e) || !world_build(&g_g.world, &g_lv)) return false;
    init_player(&g_g);
    return true;
}
static void drop_game(void) { world_free(&g_g.world); level_free(&g_lv); free(g_d); }
static void frames(int n, uint8_t input) {
    for (int i = 0; i < n; i++) { g_g.frame_ticks++; g_g.input_state = input; g_g.mask.built = false; player_input_state(&g_g); }
}
static void ball_at(int x, int y) { g_g.ball.x = x << 16; g_g.ball.y = y << 16; g_g.ball.vx = g_g.ball.vy = 0; }
static void trick_frame(int trick, int facing) {
    sk_player *p = &g_g.player;
    p->state = 16; p->trick = (uint8_t)trick; p->trick_phase = 0; p->trick_flag = -1; p->facing = (int16_t)facing;
    p->input = SK_IN_FIRE; p->jump_input = SK_IN_FIRE; p->anim = 0;
    g_g.ball.vx = g_g.ball.vy = 0;
    player_state_tricks(&g_g);
}

void test_ground_kick_sends_the_ball(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.x = 200;
    ball_at(207, g_g.player.y + 24);
    g_g.player.ball_hold = (int8_t)0x80;                         /* at the feet */
    frames(1, SK_IN_FIRE | SK_IN_RIGHT);
    CHECK_EQ(g_g.player.state, 8);
    frames(1, SK_IN_FIRE | SK_IN_RIGHT);
    CHECK_EQ(g_g.ball.vx, 0x60000); CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFE0000);
    CHECK_EQ(g_g.sfx_last, 3); CHECK_EQ(g_g.player.ball_hold, 8); CHECK_EQ(g_g.kick_cooldown, 200);
    drop_game();
}

void test_kick_ignores_an_inactive_ball(void) {               /* Review Focus 4 */
    CHECK(make_game());
    frames(20, 0);
    g_g.player.ball_hold = (int8_t)0x80;
    ball_at(g_g.player.x + 7, g_g.player.y + 24);
    g_g.ball.inactive = 1;
    int sfx = g_g.sfx_count;
    CHECK_EQ(kick_ball(&g_g), 0);
    CHECK_EQ(g_g.ball.vx, 0); CHECK_EQ(g_g.ball.vy, 0); CHECK_EQ(g_g.sfx_count, sfx);
    drop_game();
}

void test_tricks_set_the_table_velocities(void) {
    CHECK(make_game());
    frames(20, 0);
    trick_frame(1, 1);  CHECK_EQ(g_g.ball.vx, (int32_t)0xFFFF4000); CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFA0000); CHECK_EQ(g_g.player.facing, -1);
    trick_frame(2, -1); CHECK_EQ(g_g.ball.vx, 0xC000); CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFA0000); CHECK_EQ(g_g.player.facing, 1);
    trick_frame(3, 1);  CHECK_EQ(g_g.ball.vx, (int32_t)0xFFFFB000); CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFA0000);
    trick_frame(3, -1); CHECK_EQ(g_g.ball.vx, 0x5000);
    trick_frame(4, 1);  CHECK_EQ(g_g.ball.vx, (int32_t)0xFFFF9000); CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFA0000); CHECK_EQ(g_g.kill_kind, 3);
    trick_frame(4, -1); CHECK_EQ(g_g.ball.vx, 0x7000);
    trick_frame(5, 1);  CHECK_EQ(g_g.ball.vx, 0x3000); CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFE8000); CHECK_EQ(g_g.player.trick, 3);
    trick_frame(6, -1); CHECK_EQ(g_g.ball.vx, (int32_t)0xFFFFA000); CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFB0000); CHECK_EQ(g_g.player.anim, 0x37);
    trick_frame(7, 1);  CHECK_EQ(g_g.ball.vx, 0x6000); CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFB0000); CHECK_EQ(g_g.player.anim, 0x35);
    drop_game();
}

void test_volley_right_and_left(void) {
    CHECK(make_game());
    frames(20, 0);
    for (int side = 0; side < 2; side++) {
        g_g.player.state = side ? 48 : 44; g_g.player.timer = 0; g_g.player.anim = 0; g_g.player.facing = side ? -1 : 1;
        ball_at(g_g.player.x, g_g.player.y - 10);
        for (int i = 0; i < 12 && g_g.ball.vx == 0; i++) side ? player_state_volley_left(&g_g) : player_state_volley_right(&g_g);
        CHECK_EQ(g_g.ball.vx, side ? (int32_t)0xFFF80000 : 0x80000);
        CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFC0000);
        CHECK_EQ(g_g.kill_kind, 2); CHECK_EQ(g_g.sfx_last, 3);
    }
    drop_game();
}

void test_spring_off_the_ball(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.state = 36; g_g.player.timer = 0;
    ball_at(g_g.player.x - 8, g_g.player.y + 39);
    int i = 0;
    while (g_g.player.timer != 12 && i++ < 100) player_state_ball_spring(&g_g);
    CHECK_EQ(g_g.player.timer, 12);
    CHECK_EQ(g_g.player.fixed_vy, (int16_t)0xF800);              /* -8.0 in 8.8 */
    CHECK_EQ(g_g.ball.vy, (int32_t)0xFFFC0000);
    CHECK_EQ(g_g.sfx_last, 18); CHECK(g_g.player.jumping);
    int y0 = g_g.player.y;
    player_state_ball_spring(&g_g);
    CHECK_EQ(g_g.player.y, y0 - 8);                               /* rising */
    drop_game();
}

void test_kick_charge_table(void) {
    CHECK(make_game());
    frames(20, 0);
    sk_player *p = &g_g.player;
    p->x = 200; p->facing = 1; p->fire_held = 11; g_g.throw_ready = 0;
    kick_charge(&g_g);                                            /* entry 11: (0, -16) */
    CHECK_EQ(g_g.throw_x, 200); CHECK_EQ(g_g.throw_y, (p->y - 16 + 7) & ~15); CHECK_EQ(g_g.throw_ready, 1);
    p->facing = -1; p->fire_held = 8; g_g.throw_ready = 0;
    kick_charge(&g_g);                                            /* entry 0: (-16, 32) */
    CHECK_EQ(g_g.throw_x, 184); CHECK_EQ(g_g.throw_y, (p->y + 32 + 7) & ~15);
    p->fire_held = 7; g_g.throw_ready = 0; g_g.throw_x = -1;
    kick_charge(&g_g);                                            /* under 8 frames: nothing */
    CHECK_EQ(g_g.throw_x, -1); CHECK_EQ(g_g.throw_ready, 0);
    drop_game();
}
