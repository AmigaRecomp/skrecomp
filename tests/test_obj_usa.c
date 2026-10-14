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
    CHECK_EQ(s->draw->x, s->x);                                      /* the shake: t0 1 of 1, 0, -1, 0 (0x119972) */
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

void test_bouncer_29_bounces_and_is_turned_by_the_ball(void) {     /* type 29 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 29, 0);
    CHECK_EQ(s->y, 11 * 16 - 5); CHECK_EQ(s->vx, 4); CHECK(s->flags & 4); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 21);
    int i = 0;
    while (s->vy != -18 && i++ < 60) tick(1);
    CHECK_EQ(s->vy, -18);                                            /* landed: bounces */
    g_g.ball.x = (s->draw->x + 20) << 16; g_g.ball.y = (s->draw->y + 10) << 16; g_g.ball.vx = -(3 << 16); g_g.player.ball_free = 1;
    tick(1);
    CHECK_EQ(s->timer, 6); CHECK_EQ(s->flags & 1, 0);                /* the ball bounced back right: it faces left */
    drop_game();
}

void test_egg_30_hatches_a_walker(void) {                            /* type 30 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 30, 0);
    CHECK_EQ(s->y, 11 * 16 - 6); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 26); CHECK_EQ(s->hits, 1);
    tick(99);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(count_type(116), 0);
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(count_type(116), 2); CHECK_EQ(s->vy, -10);   /* the shell splits, it hops out */
    int i = 0;
    while (aux_c(s) == 1 && i++ < 60) tick(1);
    CHECK_EQ(aux_c(s), 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 13); CHECK_EQ(s->flags & 4, 0);
    drop_game();
}

void test_flyer_14_patrols_drops_and_falls_apart(void) {            /* types 14 and 119 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 8 * 16, 14, 0x40 | 0x08 | 2);           /* turn after 64, top speed 2, two hits */
    CHECK_EQ(s->y, 8 * 16 - 0x26); CHECK_EQ(s->t0, 2); CHECK_EQ(s->t1, 0x40); CHECK_EQ(s->hits, 2); CHECK_EQ(s->vx, 2);
    CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 53);
    tick(24);
    CHECK_EQ(count_type(111), 2);                                    /* its pair of drops */
    tick(64 - 24 + 1);
    CHECK_EQ(s->vx, 1);                                              /* past the patrol: slowing to turn */
    tick(1);
    CHECK(s->flags & 1); CHECK_EQ(aux_c(s), 0);                      /* turned */
    s->hits = 1;                                                     /* the last hit */
    g_g.ball.x = (s->draw->x + 30) << 16; g_g.ball.y = (s->draw->y + 20) << 16; g_g.ball.vx = 6 << 16; g_g.player.ball_free = 1;
    tick(1);
    CHECK(s->flags & 2); CHECK_EQ(count_type(119), 1);
    drop_game();
}

void test_pot_13_drops_on_a_kid_below(void) {                        /* type 13 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 6 * 16, 13, 0);
    CHECK_EQ(s->y, 6 * 16 + 8); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 27);
    g_g.player.x = (int16_t)(s->draw->x + 10);                       /* under it, the pot on frame 0 */
    tick(1);
    CHECK_EQ(s->aux, 2); CHECK_EQ(s->vy, 10); CHECK(s->flags & 2); CHECK(s->flags & 4);
    drop_game();
}

void test_rat_15_rests_then_runs(void) {                             /* type 15 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 15, 0x08 | 0x20 | 2);          /* rest 32, run 32, speed 2, facing left */
    CHECK_EQ(s->y, 11 * 16 - 8); CHECK_EQ(s->vx, 2); CHECK_EQ(s->t0, 0x20); CHECK_EQ(s->t1, 0x20); CHECK_EQ(s->hits, 1);
    CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 24);
    int x = s->draw->x;
    tick(32);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->draw->x, x);                  /* rested in place */
    tick(32);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->draw->x, x - 32); CHECK(s->flags & 1);   /* ran 1 px a frame, turned */
    drop_game();
}

void test_jumper_17_jumps_onto_a_wall(void) {                        /* type 17 */
    CHECK(make_game());
    sk_slot *s = spawn(280, 11 * 16, 17, 0x18 | 2 | 1);             /* rest 96, speed 2, facing right toward the wall at x 320 */
    CHECK_EQ(s->x, 272); CHECK_EQ(s->y, 11 * 16 - 8); CHECK(s->flags & 1);
    tick(96);
    CHECK_EQ(aux_c(s), 1);
    int i = 0;
    while (aux_c(s) == 1 && i++ < 60) tick(1);
    CHECK_EQ(aux_c(s), 2); CHECK(s->flags & 4);                      /* the wall ahead: up it goes */
    drop_game();
}

static sk_slot *find_type(int type) { for (int k = 0; k < 16; k++) if (g_g.objs.slot[k].x >= 0 && (g_g.objs.slot[k].type4 & 0x1FF) == type * 4) return &g_g.objs.slot[k]; return NULL; }

void test_final_boss_9_brings_its_parts(void) {                     /* types 9, 120, 121 */
    CHECK(make_game());
    g_g.camera_x = 100;
    sk_slot *s = spawn(300, 11 * 16, 9, 0);
    CHECK_EQ(s->x, 284); CHECK_EQ(s->y, 11 * 16 - 48); CHECK_EQ(s->hits, 30); CHECK_EQ(s->draw->w, 64);
    CHECK_EQ(g_g.push_x, 284 - 160);
    sk_slot *carried = find_type(121), *arm = find_type(120);
    CHECK(carried != NULL); CHECK(arm != NULL); CHECK_EQ(count_type(118), 1);
    if (!carried || !arm) { drop_game(); return; }
    CHECK_EQ(carried->flags_b, 2); CHECK_EQ(carried->draw->x, 284 + 40);
    tick(1);
    CHECK_EQ(s->frame, 13); CHECK(arm->draw->dflags & 4);            /* its intro frames hide the arm (table entry 13: -1) */
    tick(150);
    CHECK_EQ(s->t1, 4);                                              /* phase 1: hits count from here */
    s->hits = 11;
    g_g.ball.x = (s->draw->x + 30) << 16; g_g.ball.y = (s->draw->y + 30) << 16; g_g.ball.vx = 6 << 16; g_g.player.ball_free = 1;
    tick(1);
    CHECK_EQ(s->hits, 10); CHECK(aux_c(s) & 2); CHECK(arm->flags & 2);   /* ten left: the arm falls away */
    drop_game();
}

void test_final_boss_9_last_phase_ends_the_game(void) {             /* phases 19, 20, then 21: motion 0x34 */
    CHECK(make_game());
    sk_slot *s = spawn(300, 11 * 16, 9, 0);
    s->hits = 0xFF;                                                  /* beaten */
    int i = 0;
    while (!(g_g.level_outcome & 0x80) && i++ < 1000) tick(1);     /* the intro, then the fall (100 + 200 frames) */
    CHECK_EQ(g_g.level_outcome, 0x81); CHECK(g_g.curtain_flags & 0x10);
    drop_game();
}

static bool usa_open(sk_disks *d, sk_image *img, sk_tables *t) {
    sk_err e = {0}; uint8_t *f = NULL; size_t n = 0;
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!disks_open(d, roms, &e) || !disks_read(d, 0, "sk.pak", &f, &n, &e)) return false;
    bool ok = hunk_load(f, n, img, &e) && tables_load(t, img, &e);
    free(f);
    return ok;
}

void test_usa_markers_spawn_real_types(void) {                    /* ROM-backed, indices 23..28 */
    if (!getenv("SOCCERKID_ROMS")) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0}; sk_err e = {0};
    CHECK(usa_open(&d, &img, &t));
    static sk_game g; memset(&g, 0, sizeof g); objects_reset(&g); attrs_reset(&g);   /* install the handler tables */
    for (int idx = 23; idx <= 28; idx++) {
        uint8_t *f = NULL; size_t n = 0;
        CHECK(disks_read_file_no(&d, t.record[idx].disk + 1, idx, false, &f, &n, &e));
        sk_level lv = {0}; sk_world w = {0};
        CHECK(level_parse(f, n, &lv, &e)); free(f);
        CHECK(world_build(&w, &lv));
        for (int cy = 0; cy < w.h; cy++)
            for (int cx = 0; cx < w.w; cx++) {
                int param = 0, type = world_marker_type(&w, cx, cy, &param);
                if (type <= 0) continue;
                bool real = type < 128 && object_init_table[type] && object_init_table[type] != obj_stub_init;
                if (!real) printf("level %d: type %d at (%d, %d) is a stub\n", idx, type, cx, cy);
                CHECK(real);
            }
        for (int i = 0; i < w.w * w.h; i++) {                        /* every attribute the cells carry has a handler */
            int a = world_attr(&w, i % w.w, i / w.w);
            if (a < 0) continue;
            if (!attr_table[a]) printf("level %d: attribute %d has no handler\n", idx, a);
            CHECK(attr_table[a] != NULL);
        }
        world_free(&w); level_free(&lv);
    }
    tables_free(&t); image_free(&img); disks_close(&d);
}

void test_usa_slots_cover_the_frames(void) {                      /* ROM-backed */
    if (!getenv("SOCCERKID_ROMS")) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0}; sk_err e = {0};
    CHECK(usa_open(&d, &img, &t));
    static const struct { int type, slot, frames, frame_bytes; } need[] = {   /* the highest frame each type indexes, + 1 (w / 8 * h * 5) */
        { 3, 121, 10, 540 }, { 3, 122, 1, 620 }, { 47, 63, 4, 640 }, { 69, 118, 12, 1920 }, { 69, 119, 7, 2560 }, { 69, 120, 8, 500 },
        { 73, 110, 3, 160 }, { 7, 1, 20, 880 }, { 29, 30, 8, 420 }, { 30, 77, 2, 520 }, { 30, 78, 2, 120 }, { 30, 76, 8, 390 },
        { 14, 6, 2, 1590 }, { 14, 75, 1, 640 }, { 13, 5, 2, 540 }, { 15, 7, 14, 480 }, { 17, 7, 14, 480 }, { 9, 10, 21, 2560 }, { 9, 11, 17, 640 },
    };
    int seen = 0;
    for (int idx = 23; idx <= 28; idx++) {
        uint8_t *f = NULL; size_t n = 0;
        CHECK(disks_read_file_no(&d, t.record[idx].disk + 1, idx, false, &f, &n, &e));
        sk_level lv = {0}; sk_world w = {0};
        CHECK(level_parse(f, n, &lv, &e)); free(f);
        CHECK(world_build(&w, &lv));
        bool present[128] = {0};
        for (int cy = 0; cy < w.h; cy++)
            for (int cx = 0; cx < w.w; cx++) { int param = 0, type = world_marker_type(&w, cx, cy, &param); if (type > 0 && type < 128) present[type] = true; }
        static sk_resources r;
        CHECK(resources_load(&r, &d, &t, idx, &e));
        for (size_t k = 0; k < sizeof need / sizeof *need; k++) {
            if (!present[need[k].type]) continue;
            size_t len = 0;
            CHECK(resource_slot(&r, need[k].slot, &len) != NULL);
            seen++;
            if (len < (size_t)need[k].frames * (size_t)need[k].frame_bytes) printf("level %d type %d slot %d: %zu bytes\n", idx, need[k].type, need[k].slot, len);
            CHECK(len >= (size_t)need[k].frames * (size_t)need[k].frame_bytes);
        }
        resources_free(&r); world_free(&w); level_free(&lv);
    }
    CHECK(seen >= 15);
    tables_free(&t); image_free(&img); disks_close(&d);
}

void test_jumper_17_ignores_a_low_step(void) {                       /* review: sub_10DC0A tests rows 0..15 of one cell */
    CHECK(make_game());
    int t = g_lv.tiles_a.count - 1;                                  /* a step: only its bottom four rows solid */
    uint8_t *m = g_lv.tiles_a.tails + (size_t)t * SK_TILE_TAIL;
    for (int r = 0; r < 16; r++) m[2 * r] = m[2 * r + 1] = r >= 12 ? 0xFF : 0;
    sk_slot *s = spawn(200, 11 * 16, 17, 0x18 | 2 | 1);
    int cx = (s->draw->x + 16 + 30 - 3 + 4) >> 4;                    /* the cell 30 px ahead, above the floor */
    g_g.world.tile[11 * g_g.world.w + cx] = (uint16_t)t;
    CHECK(!solid_column_ahead(&g_g, s, 1, 16 + 30));                 /* a 4 px step is not a wall */
    for (int r = 0; r < 16; r++) m[2 * r] = m[2 * r + 1] = 0xFF;
    CHECK(solid_column_ahead(&g_g, s, 1, 16 + 30));                  /* a full cell is */
    drop_game();
}
