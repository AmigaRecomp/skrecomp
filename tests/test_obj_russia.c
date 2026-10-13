#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "check.h"
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

void test_mine_35_explodes_once_set_off(void) {                     /* type 35 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 11 * 16, 35, 0);
    CHECK_EQ(s->y, 11 * 16 - 10); CHECK_EQ(s->draw->w, 32);
    tick(20);
    CHECK_EQ(count_type(111), 0);                                    /* dormant */
    set_aux_c(s, 1);                                                 /* set off */
    int i = 0;
    while (count_type(111) == 0 && i++ < 20) tick(1);
    CHECK_EQ(count_type(111), 4);                                    /* four pieces fly */
    CHECK_EQ(g_g.shake_amplitude, 4); CHECK_EQ(g_g.flash_timer, 16); CHECK_EQ(g_g.sfx_last, 0x13);
    int x = s->draw->x, y = s->draw->y;
    i = 0;
    while (s->x >= 0 && i++ < 40) tick(1);
    CHECK(s->x < 0);                                                 /* gone, its marker held (free_object_keep) */
    CHECK_EQ(g_g.world.tile[((y + 10) >> 4) * g_g.world.w + (x >> 4)] & SK_INDEX_MASK, 0x150);
    CHECK_EQ(g_g.world.tile[((y + 10) >> 4) * g_g.world.w + ((x + 16) >> 4)] & SK_INDEX_MASK, 0x151);
    drop_game();
}

void test_sled_65_speeds_downhill_and_is_kicked_off(void) {         /* type 65 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 65, 1);
    CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->t0, 50); CHECK_EQ(s->t1, 16); CHECK_EQ(s->hits, 1); CHECK_EQ(s->draw->h, 40);
    CHECK_EQ(s->frame, 6);
    tick(49);
    CHECK_EQ(s->vx, 0);                                              /* waits 50 frames */
    tick(2);
    CHECK_EQ(s->vx, 4);                                              /* flat: t1 16 >> 2 */
    int x = s->draw->x;
    tick(10);
    CHECK(s->draw->x > x);                                           /* facing right: rides right */
    CHECK_EQ(s->t1, 16);                                             /* flat ground: no speed gained */
    drop_game();
}

void test_icicle_66_drops_waits_and_resets(void) {                  /* type 66 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 2 * 16, 66, 0);
    CHECK_EQ(s->x, 152); CHECK_EQ(s->y, 2 * 16 + 8); CHECK_EQ(s->draw->h, 26);
    g_g.player.x = 600;
    tick(5);
    CHECK_EQ(aux_c(s), 0);
    g_g.player.x = (int16_t)(s->draw->x + 16 + 5);
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK(s->flags & 4);                      /* falls */
    int i = 0;
    while (aux_c(s) < 3 && i++ < 300) tick(1);
    CHECK_EQ(aux_c(s), 3); CHECK(s->draw->dflags & 4); CHECK_EQ(s->t0, 49);   /* landed, broke, hidden (falls into the wait) */
    g_g.player.x = 600;                                              /* gone from under it */
    tick(49);
    CHECK_EQ(aux_c(s), 4); CHECK_EQ(s->draw->y, s->y); CHECK_EQ(s->t0, 24);   /* back at its marker, blinking */
    tick(24);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->draw->dflags & 4, 0);
    drop_game();
}

void test_burrower_72_rises_hops_out_and_walks(void) {              /* type 72 */
    CHECK(make_game());
    sk_slot *s = spawn(176, 14 * 16 + 8, 72, 0);                   /* 50 px under the ground line: it rises 48 */
    CHECK_EQ(s->y, 14 * 16 - 8); CHECK_EQ(s->t0, 50); CHECK_EQ(s->hits, 2);
    tick(50);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->draw->h, 45); CHECK_EQ(s->t0, 23);   /* rising (the first climb step runs in the same frame) */
    int y = s->draw->y;
    tick(4);
    CHECK_EQ(s->draw->y, y - 8);
    int i = 0;
    while (aux_c(s) == 1 && i++ < 60) tick(1);
    CHECK_EQ(aux_c(s), 2); CHECK(s->flags & 4); CHECK_EQ(s->vx, 4);   /* ground on a side: it hops out */
    i = 0;
    while (aux_c(s) == 2 && i++ < 60) tick(1);
    CHECK_EQ(aux_c(s), 3); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 16);   /* walking */
    drop_game();
}

void test_walkers_74_78_patrol(void) {                              /* types 74 and 78 */
    CHECK(make_game());
    g_g.camera_x = 0;
    sk_slot *a = spawn(160, 12 * 16, 74, 0);
    CHECK_EQ(a->vx, 2); CHECK_EQ(a->hits, 3); CHECK_EQ(a->draw->w, 64); CHECK_EQ(a->y, 12 * 16 - 48);
    sk_slot *b = spawn(200, 12 * 16, 78, 0);
    CHECK_EQ(b->vx, 4); CHECK_EQ(b->hits, 2); CHECK_EQ(b->draw->w, 64); CHECK_EQ(b->draw->h, 50); CHECK_EQ(b->y, 12 * 16 - 34);
    int i = 0;
    while (!(b->flags & 1) && i++ < 200) tick(1);
    CHECK(b->flags & 1); CHECK(b->x - b->draw->x >= 180 || i < 200);   /* turned at 180 px (or an edge) */
    drop_game();
}

void test_sprout_11_grows_and_rests(void) {                         /* type 11: the flags word as its pause counter */
    CHECK(make_game());
    sk_slot *s = spawn(160, 12 * 16, 11, 0x12);                      /* t0 2, t1 0x10 * 2 */
    CHECK_EQ(s->t0, 2); CHECK_EQ(s->t1, 0x20); CHECK_EQ(s->aux, 2); CHECK_EQ(s->x, 153); CHECK_EQ(s->y, 12 * 16 - 22);
    int i = 0;
    while (!(s->draw->dflags & 4) && i++ < 60) tick(1);
    CHECK(s->draw->dflags & 4);                                      /* grown: back to frame 0, hidden */
    CHECK_EQ(s->flags, 0); CHECK_EQ(s->flags_b, 9);                 /* 10, less this frame's */
    CHECK_EQ(s->aux, 1);
    tick(9);
    CHECK_EQ(s->draw->dflags & 4, 0);
    i = 0;
    while (!(s->draw->dflags & 4) && i++ < 60) tick(1);
    CHECK_EQ(s->aux, 2); CHECK_EQ(s->flags_b, 0x1F);                /* every second cycle: the longer pause, t1 */
    drop_game();
}

void test_shell_20_fires_every_100_frames(void) {                   /* type 20 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 11 * 16, 20, 1);                         /* faces left */
    CHECK_EQ(s->flags & 1, 0); CHECK_EQ(s->x, 167); CHECK_EQ(s->vx, 8); CHECK(s->draw->dflags & 4);
    tick(1);
    CHECK_EQ(count_type(115), 1); CHECK_EQ(aux_c(s), 1);            /* the muzzle flash */
    tick(15);
    CHECK_EQ(aux_c(s), 2); CHECK_EQ(s->draw->dflags & 4, 0);         /* flying */
    int x = s->draw->x;
    tick(4);
    CHECK(s->draw->x < x);
    int i = 0;
    while (aux_c(s) != 3 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 3);                                           /* off the view (or into a wall) */
    i = 0;
    while (aux_c(s) != 1 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->draw->x, s->x);               /* reloaded at 100: fires again */
    drop_game();
}

void test_conveyor_42_pushes_and_feeds_its_belt(void) {             /* types 42 and 98 */
    CHECK(make_game());
    sk_slot *s = spawn(100, 11 * 16, 42, 0);                        /* its belt runs in the row above the ground */
    CHECK(s->draw == NULL); CHECK_EQ(s->x, 116); CHECK_EQ(s->t1, 144); CHECK_EQ(s->x0, 2);
    g_g.player.x = 150; g_g.player.y = 12 * 16 - 41;                 /* on it: y + 40 - (y0 - 16) = 31 */
    tick(1);
    CHECK_EQ(g_g.player.carry_dx, 2);
    g_g.player.carry_dx = 0; g_g.player.x = 400;
    tick(1);
    CHECK_EQ(g_g.player.carry_dx, 0);
    tick(16);
    CHECK_EQ(count_type(98), 1);                                     /* the 18th frame: a belt piece, speed 4 */
    sk_slot *b = NULL;
    for (int i = 0; i < 16; i++) if (g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == 98 * 4) b = &g_g.objs.slot[i];
    CHECK(b != NULL); CHECK_EQ(b->vx, 4); CHECK_EQ(b->flags & 1, 1);
    tick(18);
    CHECK_EQ(s->x0, 3);                                              /* the second: speed 3 (6), 16 px higher */
    drop_game();
}

void test_gunner_46_fires_both_ways(void) {                         /* types 46 and 97 */
    CHECK(make_game());
    sk_slot *s = spawn(300, 12 * 16, 46, 0);
    CHECK_EQ(s->hits, 4); CHECK_EQ(s->draw->w, 64); CHECK_EQ(s->draw->h, 61); CHECK_EQ(s->y, 12 * 16 - 45);
    g_g.player.x = (int16_t)(s->draw->x + 32 - 95);                  /* 95 px in front (it faces left) */
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0);
    g_g.player.x = 900;                                              /* out of the bullets' way */
    int i = 0;
    while (count_type(97) < 2 && i++ < 100) tick(1);
    CHECK_EQ(count_type(97), 2);                                     /* frames 13 and 18: one each side */
    i = 0;
    while (aux_c(s) == 1 && i++ < 100) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->vx, 2); CHECK_EQ(s->t0, 50);
    drop_game();
}

void test_bullet_97_bursts_on_the_kid(void) {                       /* type 97 */
    CHECK(make_game());
    sk_slot *b = spawn(200, 10 * 16, 97, 1);
    CHECK_EQ(b->flags & 1, 1); CHECK_EQ(b->vx, 8); CHECK_EQ(b->y, 10 * 16 + 16); CHECK_EQ(b->draw->w, 32);
    g_g.player.x = (int16_t)(b->draw->x + 30); g_g.player.y = b->draw->y; g_g.player.health = 3; g_g.player.invulnerable = 0;
    tick(1);
    CHECK_EQ(aux_c(b), 1); CHECK_EQ(b->draw->h, 28);                 /* hit: the burst */
    int i = 0;
    while (b->x >= 0 && i++ < 40) tick(1);
    CHECK(b->x < 0);
    drop_game();
}

void test_wall_cannon_52_fires_every_100(void) {                    /* types 52, 91, 90 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 10 * 16, 52, 1);
    CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->t0, 100); CHECK(s->draw_index >= SK_FIXED_INDEX_BASE);
    CHECK_EQ(s->draw->w, 16); CHECK_EQ(s->draw->x, 159); CHECK_EQ(s->frame, 8);
    tick(99);
    CHECK_EQ(count_type(91), 0);
    int i = 0;
    while (count_type(91) == 0 && i++ < 20) tick(1);
    CHECK_EQ(count_type(91), 1); CHECK_EQ(count_type(90), 1);        /* the shot and the flash, on the 5th frame */
    sk_slot *b = NULL;
    for (int k = 0; k < 16; k++) if (g_g.objs.slot[k].x >= 0 && (g_g.objs.slot[k].type4 & 0x1FF) == 91 * 4) b = &g_g.objs.slot[k];
    CHECK(b != NULL); CHECK_EQ(b->vx, 10); CHECK_EQ(b->flags & 1, 1);
    sk_draw *bd = slot_draw_record(&g_g, b);                         /* a fixed record: looked up, not s->draw */
    CHECK(bd != NULL);
    int x = bd->x;
    tick(2);
    CHECK_EQ(bd->x, x + 10);
    drop_game();
}
