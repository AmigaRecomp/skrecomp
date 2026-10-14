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



void test_cup_piece_81_waits_for_fifty_tokens_then_ends_the_bonus_level(void) {   /* type 81 */
    CHECK(make_game());
    g_g.fulltime_level = 9; g_g.bonus_pending = 0xFF; g_g.timer_rate = 0x32; g_g.timeout_armed = true; g_g.timer_flags |= 0x80;
    sk_slot *s = spawn(400, 11 * 16, 81, 2);                    /* Russia's piece */
    CHECK(s != NULL);
    tick(100);
    CHECK(s->draw->dflags & 4);                                  /* hidden until the 50th token */
    g_g.bonus_tokens = 0x32;
    tick(25);
    CHECK(!(s->draw->dflags & 4));                               /* blinked in: 25 toggles */
    uint32_t score = g_g.player.score;
    g_g.player.x = (int16_t)(s->draw->x + 8); g_g.player.y = s->draw->y;
    tick(1);
    CHECK_EQ(g_g.cup_pieces, 1 << 2); CHECK_EQ(g_g.piece_collected, 0xFF); CHECK(!(g_g.timer_flags & 0x80));
    CHECK_EQ(g_g.player.score - score, 0x1000u);
    tick(198);
    CHECK_EQ(g_g.level_outcome, 0);
    tick(1);
    CHECK_EQ(g_g.level_outcome, 0x81); CHECK_EQ(g_g.level_number, 9); CHECK_EQ(g_g.bonus_pending, 0);
    CHECK_EQ(g_g.timer_rate, 0x5DC); CHECK(!g_g.timeout_armed); CHECK(g_g.curtain_flags & 0x10);
    drop_game();
}

void test_bonus_token_80_counts_as_a_coin(void) {               /* attribute 80 */
    CHECK(make_game());
    CHECK(attr_table[80] != NULL);
    g_g.attrs.probe_kind = 0;
    g_g.player.x = 20 * 16 + 4;
    uint32_t score = g_g.player.score;
    attr_table[80](&g_g, 20, 5);
    CHECK_EQ(g_g.bonus_tokens, 1); CHECK_EQ(g_g.player.score - score, 0x20u);
    g_g.player.x = 0;
    attr_table[80](&g_g, 21, 5);                                 /* out of reach: nothing */
    CHECK_EQ(g_g.bonus_tokens, 1);
    drop_game();
}
