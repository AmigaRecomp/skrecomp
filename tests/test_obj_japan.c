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

void test_insect_25_homes_on_the_kid(void) {                        /* type 25 */
    CHECK(make_game());
    g_g.player.x = 400; g_g.player.y = 150;
    sk_slot *s = spawn(200, 4 * 16, 25, 0);
    CHECK_EQ(s->t0, 8); CHECK_EQ(s->vx, 8); CHECK_EQ(s->vy, 8); CHECK_EQ(s->t1, 8); CHECK_EQ(s->hits, 1); CHECK(s->flags & 4);
    CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 27);
    tick(1);
    CHECK(aux_c(s) & 1);                                             /* facing left, the kid right: it starts to turn */
    int i = 0;
    while (!(s->flags & 1) && i++ < 60) tick(1);
    CHECK_EQ(s->flags & 1, 1); CHECK(aux_c(s) & 2);                  /* slowed to 0 and turned */
    i = 0;
    while ((aux_c(s) & 3) && i++ < 60) tick(1);
    CHECK_EQ(s->vx, 8);                                              /* back up to speed */
    drop_game();
}

void test_thrower_67_wakes_and_throws(void) {                       /* type 67, child 99 */
    CHECK(make_game());
    g_g.player.x = 1000;
    sk_slot *s = spawn(100, 11 * 16, 67, 0);                         /* clear of the fixture's wall at column 20 */
    CHECK_EQ(s->hits, 3); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->y, 11 * 16 - 32); CHECK_EQ(s->flags & 1, 1);
    tick(5);
    CHECK_EQ(aux_c(s), 0);                                           /* the kid far: idle */
    g_g.player.x = (int16_t)(s->draw->x + 100);
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 2);                       /* within 120: it patrols */
    g_g.player.x = (int16_t)(s->draw->x + 60);
    int i = 0;
    while (aux_c(s) != 2 && i++ < 60) tick(1);
    CHECK_EQ(aux_c(s), 2);                                           /* the kid in front within 80: it throws */
    i = 0;
    while (count_type(99) == 0 && i++ < 60) tick(1);
    CHECK_EQ(count_type(99), 1);
    i = 0;
    while (aux_c(s) == 2 && i++ < 60) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->t0, 40);
    drop_game();
}
