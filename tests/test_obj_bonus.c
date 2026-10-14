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

static int count_type(int type) { int c = 0; for (int i = 0; i < 16; i++) c += g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == type * 4; return c; }

void test_alien_82_without_the_cup_sends_the_kid_home(void) {   /* type 82, harmless */
    CHECK(make_game());
    g_g.cup_pieces = 0x1B; g_g.level_number = 48;
    sk_slot *s = spawn(400, 11 * 16, 82, 0);
    CHECK(s != NULL);
    CHECK_EQ(count_type(116), 1);                                /* the second layer */
    CHECK_EQ(g_g.push_x, 400 - 0xD4);
    g_g.player.x = 300;
    tick(50);
    CHECK(!g_g.message_box_request);
    g_g.player.x = 400 - 0x40 + 1;
    tick(1);
    CHECK(g_g.message_box_request); CHECK_EQ(g_g.message_box_kind, 0);
    tick(28);
    CHECK_EQ(g_g.level_outcome, 0);
    tick(1);
    CHECK_EQ(g_g.level_outcome, 0x81); CHECK_EQ(g_g.level_number, 49); CHECK(g_g.curtain_flags & 0x10);
    drop_game();
}

void test_alien_82_with_the_cup_fights_and_dies(void) {          /* type 82, the boss */
    CHECK(make_game());
    g_g.cup_pieces = 0x1F; g_g.level_number = 48;
    g_g.player.x = 100;
    sk_slot *s = spawn(400, 11 * 16, 82, 0);
    CHECK(s != NULL);
    CHECK_EQ(count_type(84), 2); CHECK_EQ(count_type(116), 1);
    CHECK_EQ(s->hits, 35); CHECK(g_g.objs.chain_c); CHECK_EQ(g_g.objs.chain_count, 5);
    CHECK_EQ(s->x, 400 - 0x20); CHECK_EQ(s->y, 11 * 16 - 0x31);
    tick(99);
    CHECK(s->draw->dflags & 4);                                  /* hidden for the intro */
    tick(51);
    CHECK(!(s->draw->dflags & 4));                               /* blinked in */
    int dx = g_g.objs.chain[5].x - s->draw->x;
    CHECK(dx > 0x20);                                            /* arm B hangs from the right shoulder */
    s->hits = 0;                                                 /* the 35th hit */
    set_aux_c(s, aux_c(s) | 4);
    uint32_t score = g_g.player.score;
    int n = 0;
    while (g_g.level_outcome != 0x81 && n < 3000) { tick(1); n++; }
    CHECK_EQ(g_g.level_outcome, 0x81); CHECK_EQ(g_g.level_number, 49);
    CHECK_EQ(g_g.player.score - score, 0x100000u);
    drop_game();
}

void test_alien_82_starts_over_after_the_kid_dies(void) {
    CHECK(make_game());
    g_g.cup_pieces = 0x1F; g_g.player.x = 100;
    spawn(400, 11 * 16, 82, 0);
    tick(10);
    g_g.player.dying = 1;
    tick(1);
    CHECK_EQ(count_type(82), 1);
    g_g.player.dying = 0;
    tick(1);
    CHECK_EQ(count_type(82) + count_type(84) + count_type(116), 0);
    CHECK(!g_g.objs.chain_c);
    drop_game();
}

void test_alien_shot_83_flies_along_the_arm_and_clamps_its_frame(void) {
    CHECK(make_game());
    sk_slot *s = spawn(200, 100, 83, 0xC0);                      /* level, to the left */
    CHECK(s != NULL);
    CHECK_EQ(s->frame, 2);
    int x = s->draw->x;
    tick(1);
    CHECK(s->draw->x < x);
    s = spawn(200, 100, 83, 0x90);                               /* nearly straight up: frame -1 clamped */
    CHECK(s != NULL);
    CHECK_EQ(s->frame, 0);
    drop_game();
}

void test_tokens_left_85_shows_in_the_bonus_levels(void) {        /* setup_level 0x104270 spawns type 85 when the clock is armed */
    CHECK(make_game());
    CHECK_EQ(count_type(85), 0);
    g_g.timeout_armed = true;
    CHECK(game_setup_level(&g_g, &g_lv, 36));
    objects_reset(&g_g);                                         /* make_game's own reset is after the setup: spawn again as the setup does */
    g_g.timeout_armed = true;
    CHECK(game_setup_level(&g_g, &g_lv, 36));
    CHECK_EQ(count_type(85), 1);
    sk_slot *s = NULL;
    for (int i = 0; i < 16; i++) if (g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == 85 * 4) s = &g_g.objs.slot[i];
    CHECK(s && s->draw);
    tick(1);
    CHECK(!(s->draw->dflags & 4)); CHECK(s->draw->dflags & 8);   /* screen-relative, shown */
    CHECK_EQ(s->draw->x, 16); CHECK_EQ(s->draw->y, 16);
    CHECK_EQ(g_g.token_digits[0], 0x5D + 5); CHECK_EQ(g_g.token_digits[1], 0x5D + 0);   /* "50" */
    g_g.bonus_tokens = 43; tick(1);
    CHECK_EQ(g_g.token_digits[0], 0x5D + 0); CHECK_EQ(g_g.token_digits[1], 0x5D + 7);
    drop_game();
}

void test_alien_children_come_from_the_spawns_not_a_stale_slot(void) {
    CHECK(make_game());
    g_g.cup_pieces = 0x1F; g_g.player.x = 100;
    for (int i = 0; i < 14; i++) spawn(50 + i, 50, 81, 0);       /* two slots left: the boss and one piece */
    sk_slot *s = spawn(400, 11 * 16, 82, 0);
    CHECK(s != NULL);
    CHECK(g_g.alien.piece_b == NULL);                            /* its spawn failed */
    CHECK(g_g.alien.piece_a != s && g_g.alien.piece_b != s);
    tick(400);                                                   /* through the rise: no pose on the boss itself */
    CHECK(s->t1 >= 0 && s->t1 < 39 * 4);
    drop_game();
}

void test_alien_lets_go_of_a_head_freed_under_it(void) {
    CHECK(make_game());
    g_g.cup_pieces = 0x1F; g_g.player.x = 100;
    sk_slot *s = spawn(400, 11 * 16, 82, 0);
    sk_slot *head = g_g.alien.head;
    CHECK(head != NULL);
    free_object(&g_g, head);
    sk_slot *other = spawn(200, 100, 81, 1);                     /* takes the head's slot */
    CHECK(other == head);
    uint8_t flags = other->draw->dflags;
    tick(160);
    CHECK_EQ(other->draw->dflags & ~2, flags & ~2);              /* not the boss's flags */
    CHECK(s->x >= 0);
    drop_game();
}
