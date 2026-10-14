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
