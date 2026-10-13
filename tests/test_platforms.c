#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/ball.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/platforms.h"
#include "game/player.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static const uint8_t solid[32 * 10] = { [0 ... 319] = 0xFF };    /* 32 px wide plane 0, 10 rows */
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    if (!game_setup_level(&g_g, &g_lv, 0)) return false;
    g_g.player.state = 0;
    return true;
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static sk_platform *platform_at(int cx, int y) {               /* a 32 px wide platform, its surface at y */
    int i = -1;
    sk_platform *pf = alloc_platform(&g_g, &i);
    pf->px = (int32_t)((uint32_t)(uint16_t)(cx + 16) << 16); pf->py = (int32_t)((uint32_t)(uint16_t)y << 16); pf->half_w = 16; pf->half_h = 5; pf->pframe = solid;
    pf->stand_dy = 0; pf->frame_sel = 0;
    return pf;
}
static void frames(int n, uint8_t input) {
    static sk_sprite kid;
    for (int i = 0; i < n; i++) game_frame_step(&g_g, input, &kid);
}

void test_kid_stands_on_a_platform(void) {
    CHECK(make_game());
    frames(200, 0);                                               /* KICK OFF over */
    sk_platform *pf = platform_at(27 * 16, 160);                  /* over the pit (columns 26..29) */
    g_g.player.x = 27 * 16 + 16; g_g.player.y = 160 - 41;
    for (int i = 0; i < 30; i++) { platform_carry(&g_g, pf, 0, 0); frames(1, 0); }
    CHECK_EQ(g_g.player.y, 160 - 41);                              /* not fallen into the pit */
    CHECK(pf->player_on);
    drop_game();
}

void test_platform_carries_the_kid(void) {
    CHECK(make_game());
    frames(200, 0);
    sk_platform *pf = platform_at(27 * 16, 160);
    g_g.player.x = 27 * 16 + 16; g_g.player.y = 160 - 41;
    g_g.player.jumping = 0; g_g.player.falling = 0;
    int x0 = g_g.player.x;
    CHECK_EQ(platform_carry(&g_g, pf, 2, -1), -1);
    CHECK_EQ(g_g.player.x, x0 + 2); CHECK_EQ(g_g.player.on_platform, 1);
    drop_game();
}

void test_ball_rides_a_platform(void) {                         /* Review Focus 4 */
    CHECK(make_game());
    frames(200, 0);
    sk_platform *pf = platform_at(27 * 16, 160);
    g_g.player.x = 500; g_g.player.ball_hold = 0;
    g_g.ball.x = (27 * 16 + 10) << 16; g_g.ball.y = (160 - 16) << 16; g_g.ball.inactive = 0;
    platform_carry(&g_g, pf, 0, 2);
    CHECK_EQ(g_g.ball.y >> 16, 160 - 16 + 2); CHECK(pf->ball_on); CHECK_EQ(g_g.ball.touched, 0xFF);
    drop_game();
}

void test_platform_carry_against_a_ceiling(void) {              /* Review Focus 1: carried up into a ceiling and above the map */
    CHECK(make_game());
    static sk_sprite kid;
    for (int i = 0; i < 200; i++) game_frame_step(&g_g, 0, &kid);   /* KICK OFF over */
    sk_platform *pf = platform_at(5 * 16, 7 * 16 + 30);              /* under the ceiling cell (6, 7) */
    g_g.player.x = 6 * 16 + 4; g_g.player.y = (int16_t)(7 * 16 + 30 - 41);
    for (int i = 0; i < 200; i++) {                                   /* lift him 1 px a frame, through the ceiling and off the top */
        g_g.player.jumping = 0; g_g.player.falling = 0;
        platform_carry(&g_g, pf, 0, -1);
        pf->py -= 0x10000;
        game_frame_step(&g_g, 0, &kid);
    }
    CHECK(pf->py >> 16 < 0);                                         /* the platform is above the map: no crash, no wild write */
    CHECK(g_g.player.y < 7 * 16);
    drop_game();
}

void test_platform_records_full(void) {                         /* Review Focus 2 */
    CHECK(make_game());
    int idx = -1;
    for (int i = 0; i < 16; i++) CHECK(alloc_platform(&g_g, &idx) != NULL);
    CHECK_EQ(g_g.platform_count, 16);
    CHECK(alloc_platform(&g_g, &idx) == NULL); CHECK_EQ(idx, -1);
    drop_game();
}

void test_ball_on_clears_when_the_ball_leaves(void) {             /* clr.w $1c(a4) clears player_on and ball_on (0x118028) */
    CHECK(make_game());
    sk_platform *pf = platform_at(27 * 16, 160);
    g_g.player.x = 500; g_g.player.ball_hold = 0;
    g_g.ball.x = (27 * 16 + 10) << 16; g_g.ball.y = (160 - 16) << 16;
    platform_carry(&g_g, pf, 0, 0);
    CHECK(pf->ball_on);
    g_g.ball.x = 600 << 16;
    platform_carry(&g_g, pf, 0, 0);
    CHECK_EQ(pf->ball_on, 0);
    drop_game();
}

void test_a_rising_ball_passes_through_a_platform(void) {       /* tst.w vy; bmi: no merge (0x109BD4 / 0x109C68) */
    CHECK(make_game());
    sk_platform *pf = platform_at(10 * 16, 100);
    pf->ball_on = 1;
    g_g.ball.x = (10 * 16 + 8) << 16; g_g.ball.y = 96 << 16; g_g.ball.vy = (int32_t)0xFFFE0000;
    g_g.bw.mask_valid = false;
    rebuild_ball_mask(&g_g);
    int mx = 10 * 16 + 8 - g_g.bw.mask_x, my = 100 - g_g.bw.mask_y;
    CHECK_EQ(g_g.bw.mask[my * 8 + (mx >> 3)], 0);                    /* rising: not merged */
    g_g.ball.vy = 0x10000; g_g.bw.mask_dirty = true;
    rebuild_ball_mask(&g_g);
    CHECK(g_g.bw.mask[my * 8 + (mx >> 3)] != 0);                     /* falling: merged */
    drop_game();
}
