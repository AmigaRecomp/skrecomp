#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "core/tables.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/obj_helpers.h"
#include "game/resources.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static sk_resources g_res; static sk_section g_sec; static uint8_t g_sheet[64 * 64 / 8 * 5 * 30];
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
    g_g.camera_x = 0; g_g.camera_y = 0;
    g_g.player.x = 1000; g_g.player.y = 12 * 16 - 41; g_g.player.state = 0;
    return true;
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }

static sk_slot *spawn(int x, int y, int type, int param) { g_g.objs.marker_param = param; return spawn_object(&g_g, x, y, type); }
static void tick(int n) { for (int i = 0; i < n; i++) { g_g.frame_ticks++; update_objects(&g_g); } }

void test_pendulum_swings_and_reverses(void) {                 /* type 36 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 6 * 16, 36, 0);
    CHECK(g_g.objs.chain_a); CHECK(s->draw == NULL);
    CHECK_EQ(s->t1, 28); CHECK_EQ(s->y, 6 * 16 + 8);
    tick(1);
    CHECK_EQ(s->t0, 4); CHECK_EQ(s->t1, 27);
    const sk_draw *ball = &g_g.objs.chain[7];
    CHECK(ball->y > s->y + 40);                                      /* hangs below its pivot */
    CHECK(ball->x > s->x - 8);                                       /* swung to the right */
    CHECK_EQ(g_g.objs.chain[0].x, s->x); CHECK_EQ(g_g.objs.chain[0].y, s->y);
    int i = 0;
    while ((s->flags & 0xFF) == 0 && i++ < 100) tick(1);
    CHECK_EQ(s->flags & 0xFF, 0xFF); CHECK_EQ(aux_c(s), 0xFF);      /* it turns at the top of the swing */
    int top = s->t0;
    CHECK(top > 60 && top <= 70);
    tick(1);
    CHECK_EQ(s->t0, top);                                           /* speed 0: it hangs one frame */
    tick(1);
    CHECK(s->t0 < top);
    i = 0;
    while (s->t0 > 0 && i++ < 100) tick(1);
    CHECK_EQ(s->t0, 0); CHECK_EQ(aux_c(s), 0);                      /* back at the bottom: slowing again */
    g_g.camera_x = 2000;
    tick(1);
    CHECK(s->x < 0); CHECK(!g_g.objs.chain_a);
    drop_game();
}

void test_jumper_lands_waits_and_jumps_again(void) {          /* type 37 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 11 * 16, 37, 0);
    CHECK_EQ(s->vy, -20); CHECK_EQ(s->t0, -20); CHECK_EQ(s->draw->w, 16); CHECK_EQ(s->draw->h, 21);
    int y0 = s->draw->y;
    tick(4);
    CHECK(s->draw->y < y0);                                          /* thrown up */
    int i = 0;
    while (aux_c(s) == 0 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->x0, 0);                       /* landed */
    tick(30);
    CHECK_EQ(s->x0, 30);
    tick(25);
    CHECK(s->draw->dflags & 4);                                      /* hidden after 50 */
    tick(20);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->draw->y, s->y); CHECK_EQ(s->vy, -20);   /* back at its start */
    CHECK_EQ(s->draw->dflags & 6, 0);
    drop_game();
}
