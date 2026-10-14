#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "check.h"
#include "core/tables.h"
#include "core/hunk.h"
#include "core/disks.h"
#include "fixture.h"
#include "game/attrs.h"
#include "game/world.h"
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

static int count_type(int type) { int c = 0; for (int i = 0; i < 16; i++) c += g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == type * 4; return c; }

void test_hedgehog_3_walks_and_curls_at_an_edge(void) {             /* type 3 */
    CHECK(make_game());
    sk_slot *s = spawn(360, 12 * 16, 3, 0);                          /* between the wall (x 320) and the pit (x 416) */
    CHECK(s != NULL); if (!s) { drop_game(); return; }
    CHECK_EQ(count_type(3), 1); CHECK_EQ(s->y, 12 * 16 - 11); CHECK_EQ(s->vx, 2); CHECK_EQ(s->hits, 1); CHECK(s->flags & 8);
    CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 27);
    int x = s->draw->x;
    tick(1);
    CHECK_EQ(s->draw->x, x - 1);                                     /* walking left at 1 px a frame */
    s->vx = 0; s->t0 = 25; set_aux_c(s, 1);                          /* curled (a steep slope ahead turns it, ledge_distance >= 3) */
    int y = s->draw->y;
    tick(24);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0);
    tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->vx, 2); CHECK_EQ(s->draw->y, y - 1);   /* uncurls and walks on */
    drop_game();
}

void test_roller_47_bounces_and_is_knocked_by_the_ball(void) {      /* type 47 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 12 * 16, 47, 0);
    CHECK_EQ(s->y, 12 * 16 - 12); CHECK_EQ(s->vx, 4); CHECK_EQ(s->vy, -6); CHECK(s->flags & 4); CHECK_EQ(s->draw->w, 32);
    int i = 0;
    while (s->vy != -6 || i < 3) { tick(1); if (++i > 60) break; }
    CHECK(i <= 60);                                                  /* it lands and bounces again */
    g_g.ball.x = (s->draw->x + 24) << 16; g_g.ball.y = (s->draw->y + 16) << 16; g_g.player.ball_free = 1;   /* a loose ball */
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->flags & 1, 0); CHECK_EQ(s->vx, 0x20);       /* the ball on its right: knocked left */
    g_g.ball.x = 3000 << 16;
    tick(8);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->vx, 4);
    drop_game();
}

void test_gunman_69_shoots_a_kid_in_front(void) {                    /* type 69 */
    CHECK(make_game());
    sk_slot *s = spawn(250, 11 * 16, 69, 0);                         /* the marker row: one above the floor */
    CHECK_EQ(s->y, 11 * 16 - 48); CHECK_EQ(s->hits, 3); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 64);
    tick(1);                                                         /* (the fixture's edge may turn it) */
    CHECK_EQ(aux_c(s), 0);
    g_g.player.x = (int16_t)(s->draw->x + 24 + ((s->flags & 1) ? 50 : -50));   /* 50 px in front */
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->draw->w, 64); CHECK_EQ(s->vx, 0);
    int i = 0;
    while (count_type(99) == 0 && i++ < 40) tick(1);
    CHECK_EQ(count_type(99), 1);
    i = 0;
    while (aux_c(s) != 0 && i++ < 40) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->t0, 50); CHECK_EQ(s->vx, 2);
    drop_game();
}

void test_rock_73_shakes_drops_and_breaks(void) {                    /* type 73 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 9 * 16, 73, 0);
    CHECK_EQ(s->y, 9 * 16 - 10); CHECK_EQ(s->t1, 100); CHECK_EQ(s->draw->w, 16);
    tick(1);
    CHECK_EQ(s->draw->x, s->x + 1);                                  /* the shake: 0, +1, -1, 0 */
    tick(99);
    CHECK_EQ(aux_c(s), 1); CHECK(s->flags & 4);
    int i = 0;
    while (aux_c(s) == 1 && i++ < 60) tick(1);
    CHECK_EQ(aux_c(s), 2); CHECK_EQ(count_type(116), 1); CHECK_EQ(s->vx, 4); CHECK(s->flags & 1);
    drop_game();
}

void test_skater_7_pushes_on_the_flat(void) {                       /* type 7 */
    CHECK(make_game());
    sk_slot *s = spawn(100, 11 * 16, 7, 4);                          /* param 4: three hits, facing bit clear */
    CHECK_EQ(s->y, 11 * 16 - 28); CHECK_EQ(s->hits, 3); CHECK_EQ(s->vx, 0x5A); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 44);
    tick(1);
    CHECK_EQ(s->t1, 0x14); CHECK_EQ(s->t0, 1); CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->frame, 2);   /* crouched, the offsets taken */
    int x = s->draw->x;
    tick(1);
    CHECK_EQ(s->draw->x, x + 4);                                     /* (0x5A * 3 / 2) >> 5 */
    tick(20);
    CHECK_EQ(aux_c(s), 16); CHECK_EQ(s->vx, 0x5A);                   /* pushing: the speed held at its cap */
    drop_game();
}
