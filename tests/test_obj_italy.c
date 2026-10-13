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
#include "game/platforms.h"
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

static uint8_t g_plat[32 * 10 / 8 * 5];
static sk_section g_psec;
static void give_platform_param(int pos) {
    memset(g_plat, 0xFF, sizeof g_plat);
    g_psec.data = g_plat; g_psec.len = sizeof g_plat;
    g_res.param[pos].sec = &g_psec;
    static const uint8_t bytes[6] = { 16, 5, 0, 0, 0x01, 0 };     /* half 16 x 5, frame_sel 1, stand_dy 0 */
    memcpy(g_res.param[pos].bytes, bytes, 6);
}

void test_swing_platform_hangs_on_its_chain(void) {           /* type 38 */
    CHECK(make_game());
    give_platform_param(0);
    sk_slot *s = spawn(160, 4 * 16, 38, 0x10);                       /* radius 64 + 16 */
    CHECK_EQ(s->x0, 80); CHECK_EQ(g_g.objs.chain_count, 3); CHECK(g_g.objs.chain_b);
    sk_platform *pf = slot_platform(&g_g, s);
    CHECK(pf != NULL);
    CHECK_EQ(pf->pspeed, 0x1E); CHECK_EQ(pf->pangle, 0);
    CHECK_EQ(pf->px >> 16, s->x); CHECK_EQ(pf->py >> 16, s->y + 80);
    tick(1);
    CHECK_EQ(pf->pangle, 3); CHECK_EQ(pf->pspeed, 0x1D);
    CHECK((pf->px >> 16) > s->x);                                    /* swung right on its radius */
    CHECK_EQ(s->draw->x, (pf->px >> 16) - 16); CHECK_EQ(s->draw->y, pf->py >> 16);
    CHECK_EQ(g_g.objs.chain[0].y, s->y + 15);                        /* the first link 16 px down, cos(12) * 16 / 256 */
    int i = 0;
    while (((uint16_t)s->t0 >> 8) == 0 && i++ < 100) tick(1);
    CHECK_EQ((uint16_t)s->t0 >> 8, 0xFF);                            /* turned at the top */
    CHECK(pf->pangle > 40 && pf->pangle < 100);
    g_g.camera_x = 3000;
    tick(1);
    CHECK(s->x < 0); CHECK(!g_g.objs.chain_b);
    drop_game();
}

void test_strider_walks_turns_and_leaps(void) {               /* type 43 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 11 * 16, 43, 0);
    CHECK_EQ(s->vx, 8); CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 52); CHECK_EQ(s->y, 11 * 16 - 36);
    int i = 0;
    while (aux_c(s) == 0 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK(s->x - s->draw->x >= 160);         /* walked 160 px left */
    i = 0;
    while (aux_c(s) == 1 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->vx, 0);   /* stopped and turned */
    drop_game();
    CHECK(make_game());
    s = spawn(400, 11 * 16, 43, 0);
    g_g.player.x = (int16_t)(s->draw->x + 24 - 96);
    tick(1);
    CHECK_EQ(aux_c(s), 4); CHECK_EQ(s->draw->h, 58); CHECK(s->flags & 4);   /* leaps at the kid */
    i = 0;
    while (aux_c(s) == 4 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->draw->h, 52); CHECK(!(s->flags & 4));
    drop_game();
}

void test_thrown_rider_falls_after_50(void) {                 /* type 100 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 5 * 16, 100, 1);
    CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->vx, 6); CHECK_EQ(s->t0, 50); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->frame, 5);
    tick(49);
    CHECK(!(s->flags & 4));
    tick(1);
    CHECK(s->flags & 4);
    tick(200);
    CHECK(s->x >= 0);                                                /* the original never frees it (the offscreen result is dropped) */
    drop_game();
}

void test_bouncer_hops_then_leaves(void) {                    /* type 44 */
    CHECK(make_game());
    g_g.camera_x = 400;                                              /* its marker in view */
    sk_slot *s = spawn(600, 11 * 16, 44, 0);
    CHECK_EQ(s->vx, 6); CHECK(s->flags & 4); CHECK(s->flags & 8); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->y, 11 * 16 - 16);
    int i = 0, bounces = 0;
    for (; i < 300 && !(s->flags & 2); i++) { tick(1); bounces += s->vy == -4; }
    CHECK(bounces >= 2);                                             /* bounced off the ground */
    CHECK(s->flags & 2);                                             /* 400 px from its marker: out of play */
    CHECK(s->x < 0);                                                 /* and freed, its draw 200 px outside the view */
    drop_game();
}

void test_rock_trembles_then_drops_on_the_kid(void) {         /* type 48 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 3 * 16, 48, 0);
    CHECK_EQ(s->x, 152); CHECK_EQ(s->y, 3 * 16 - 24); CHECK_EQ(s->draw->w, 32);
    g_g.player.x = 400;
    g_g.frame_ticks = 3;                                             /* the next frame is phase 0 */
    tick(1);
    CHECK_EQ(s->draw->x, 153); CHECK_EQ(aux_c(s), 0);              /* trembles: +1, 0, -1, 0 */
    tick(3);
    CHECK_EQ(s->draw->x, 152);
    g_g.player.x = (int16_t)(s->draw->x + 16 + 10);
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK(s->flags & 4); CHECK(s->flags & 2);
    int y = s->draw->y;
    tick(5);
    CHECK(s->draw->y > y);                                           /* falls */
    drop_game();
}

void test_hopper_49_bounces_and_leaves(void) {                /* type 49 */
    CHECK(make_game());
    g_g.camera_x = 400;
    sk_slot *s = spawn(600, 11 * 16, 49, 0);
    CHECK_EQ(s->vx, 4); CHECK_EQ(s->vy, -4); CHECK(s->flags & 12);
    int i = 0, bounces = 0;
    for (; i < 400 && !(s->flags & 2); i++) { tick(1); bounces += s->vy == -4; }
    CHECK(bounces >= 2); CHECK(s->flags & 2);
    drop_game();
}

static int count_type(int type) { int c = 0; for (int i = 0; i < 16; i++) c += g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == type * 4; return c; }

void test_thrower_64_patrols_and_throws_at_the_kid(void) {    /* type 64 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 64, 0);
    CHECK_EQ(s->vx, 2); CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 64); CHECK_EQ(s->y, 11 * 16 - 48);
    g_g.player.x = (int16_t)(s->draw->x - 40);                       /* in front of it (it faces left) */
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0);                       /* stops to throw */
    int i = 0;
    while (count_type(99) == 0 && i++ < 100) tick(1);
    CHECK_EQ(count_type(99), 1);                                     /* the throw on the 4th attack frame */
    i = 0;
    while (aux_c(s) == 1 && i++ < 100) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->vx, 2); CHECK_EQ(s->t0, 50); /* walks on, 50 frames before the next */
    drop_game();
}

void test_thrower_68_throws_and_waits_25(void) {              /* type 68 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 68, 0);
    CHECK_EQ(s->vx, 2); CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->y, 11 * 16 - 48);
    g_g.player.x = (int16_t)(s->draw->x + 24 - 60);
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0);
    int i = 0;
    while (count_type(99) == 0 && i++ < 100) tick(1);
    CHECK_EQ(count_type(99), 1);
    i = 0;
    while (aux_c(s) == 1 && i++ < 100) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->t0, 25); CHECK_EQ(s->frame, 0);
    drop_game();
}
