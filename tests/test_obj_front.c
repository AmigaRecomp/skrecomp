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

void test_demo_banner_102_sits_at_the_bottom_of_the_view(void) {
    CHECK(make_game());
    sk_slot *s = spawn(0, 0, 102, 0);
    CHECK(s && s->draw);
    tick(3);
    CHECK_EQ(s->draw->x, 0x70); CHECK_EQ(s->draw->y, 0xB4);
    CHECK_EQ(s->draw->w, 64); CHECK_EQ(s->draw->h, 13);
    CHECK((s->draw->dflags & 9) == 9);                           /* screen position */
    CHECK_EQ(count_type(102), 1);
    drop_game();
}

static int tile(int x, int y) { return g_g.world.tile[(y >> 4) * g_g.world.w + (x >> 4)] & 0x1FF; }

void test_tutorial_86_shows_the_joystick_moves_then_closes(void) {
    CHECK(make_game());
    g_g.curtain_flags = 0;                                       /* the curtain is open */
    spawn(0, 0, 86, 0);
    CHECK_EQ(tile(0x90, 0xB0), 12);                              /* the fire button, unlit */
    tick(199);
    CHECK_EQ(tile(176, 176), 0);
    tick(1);                                                     /* entry 0: step 3 (RIGHT): tiles 23, 24 */
    CHECK_EQ(tile(176, 176), 23); CHECK_EQ(tile(176, 192), 24);
    tick(70);                                                    /* entry 1: step 8, everything cleared */
    CHECK_EQ(tile(176, 176), 0);
    int n = 0;
    while (!(g_g.curtain_flags & 0x10) && n < 4000) { tick(1); n++; }
    CHECK_EQ(200 + 70 + n, 3160);                                /* step 9 closes the curtain */
    drop_game();
}

void test_fire_ends_a_demo_at_once(void) {                      /* frame step 32 with game_flags bit 2 */
    CHECK(make_game());
    static sk_sprite kid;
    g_g.game_flags = 0xFE;
    game_frame_step(&g_g, 0, &kid);
    CHECK(!g_g.level_end);
    game_frame_step(&g_g, SK_IN_FIRE, &kid);
    CHECK(g_g.level_end); CHECK_EQ(game_outcome(&g_g), SK_OUTCOME_LEFT);
    drop_game();
}
