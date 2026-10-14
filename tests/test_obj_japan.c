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

static uint8_t g_plat[32 * 10 / 8 * 5];
static sk_section g_psec;

void test_wheel_platform_6_turns_full_circles(void) {               /* type 6 */
    CHECK(make_game());
    memset(g_plat, 0xFF, sizeof g_plat);
    g_psec.data = g_plat; g_psec.len = sizeof g_plat;
    g_res.param[0].sec = &g_psec;
    static const uint8_t bytes[6] = { 16, 5, 0, 0, 0x01, 0 };
    memcpy(g_res.param[0].bytes, bytes, 6);
    sk_slot *s = spawn(160, 4 * 16, 6, 0x16);                        /* radius 64 + 16, speed (0x16 & 0xE) / 2 + 1 = 4 */
    CHECK_EQ(s->x0, 80); CHECK(g_g.objs.chain_b); CHECK_EQ(g_g.objs.chain_count, 3);
    sk_platform *pf = slot_platform(&g_g, s);
    CHECK(pf != NULL); CHECK_EQ(pf->pspeed, 4); CHECK_EQ(pf->pangle, 0);
    tick(1);
    CHECK_EQ(pf->pangle, 4);
    tick(63);
    CHECK_EQ(pf->pangle, 0);                                         /* 64 frames a turn, never reversing */
    CHECK_EQ(pf->py >> 16, s->y + 80);                               /* back at the bottom */
    drop_game();
}

void test_crab_18_wakes_attacks_and_explodes(void) {                /* type 18 */
    CHECK(make_game());
    g_g.player.x = 1000; g_g.player.y = 11 * 16;
    sk_slot *s = spawn(216, 12 * 16, 18, 0);
    CHECK_EQ(s->x, 200); CHECK_EQ(s->y, 12 * 16 - 16); CHECK_EQ(s->hits, 3); CHECK_EQ(s->draw->w, 32);
    tick(3);
    CHECK_EQ(aux_c(s), 0);                                           /* asleep: the kid far */
    g_g.player.x = (int16_t)(s->draw->x + 16 + 100); g_g.player.y = (int16_t)(s->draw->y - 8);
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->flags & 1, 1);               /* waking, facing the kid */
    int i = 0;
    while (aux_c(s) != 2 && i++ < 40) tick(1);
    CHECK_EQ(aux_c(s), 2); CHECK_EQ(s->vx, 4);
    s->flags |= 2; s->t0 = 0; s->timer = 8;                          /* killed: the death run */
    i = 0;
    while (s->x >= 0 && i++ < 80) tick(1);
    CHECK(s->x < 0); CHECK_EQ(count_type(116), 5); CHECK_EQ(g_g.flash_timer, 16); CHECK_EQ(g_g.sfx_last, 0x13);
    drop_game();
}

void test_random_matches_the_shift_register(void) {                 /* sub_102B42, seeded 0xDC359A7B at start-up (0x1003FC) */
    static sk_game g;
    memset(&g, 0, sizeof g);
    game_new_game(&g);
    CHECK_EQ(g.rng, 0xDC359A7Bu);
    uint32_t r = game_random(&g);
    CHECK_EQ(r, 0xB86B34F6u);                                        /* doubled, the carry set: no eor */
    r = game_random(&g);
    CHECK_EQ(r, 0x70D669ECu);                                        /* the top bit set again: no eor */
    for (int i = 0; i < 3; i++) r = game_random(&g);
    CHECK_EQ(r, 0xF0AFE264u);
}

void test_popper_33_appears_throws_and_sinks(void) {                /* type 33, child 111 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 8 * 16, 33, 0);
    CHECK_EQ(s->t0, 4); CHECK(s->draw->dflags & 4); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 28);
    tick(4);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->draw->dflags & 4, 0);         /* up, at one of its eight spots */
    int i = 0;
    while (count_type(111) < 2 && i++ < 200) tick(1);
    CHECK_EQ(count_type(111), 2);
    i = 0;
    while (aux_c(s) != 0 && i++ < 300) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->t0, 40); CHECK(s->draw->dflags & 4);   /* sunk: 40 frames hidden */
    drop_game();
}

void test_walker_80_walks_and_turns(void) {                         /* type 80 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 12 * 16 - 16, 80, 0);
    CHECK_EQ(s->vx, 4); CHECK_EQ(s->hits, 2); CHECK(s->flags & 8); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 16);
    int x = s->draw->x;
    tick(4);
    CHECK_EQ(s->draw->x, x - 8);                                     /* facing left, 2 px a frame */
    drop_game();
}

void test_press_19_rises_slams_and_fills_its_column(void) {         /* type 19 */
    CHECK(make_game());
    sk_slot *s = spawn(168, 11 * 16, 19, 3);                         /* travels 48 px */
    CHECK_EQ(s->x, 160); CHECK_EQ(s->t0, 48); CHECK_EQ(s->vy, -2); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 16);
    int i = 0;
    while (s->vy != 16 && i++ < 100) tick(1);
    CHECK_EQ(s->vy, 16); CHECK((int16_t)(s->draw->y - s->y) <= -41);   /* reached -49, then the first 8 px of the slam */
    int cell = ((s->y - 16) >> 4) * g_g.world.w + ((s->draw->x + 8) >> 4);
    i = 0;
    while (s->vy != -2 && i++ < 40) tick(1);
    CHECK_EQ(s->vy, -2); CHECK_EQ(g_g.sfx_last, 0x16);               /* landed: rising again */
    CHECK_EQ(g_g.world.tile[cell] & SK_INDEX_MASK, 0x17F);           /* its shaft filled on the way down */
    drop_game();
}

void test_bubble_21_blinks_then_rises(void) {                       /* type 21 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 10 * 16, 21, 0);
    CHECK_EQ(s->timer, 100); CHECK_EQ(s->t0, 4); CHECK_EQ(s->draw->w, 16); CHECK_EQ(s->draw->h, 15);
    tick(1);
    CHECK(s->draw->dflags & 2); CHECK_EQ(s->timer, 99);              /* blinking */
    tick(99);
    CHECK_EQ(s->timer, 0); CHECK_EQ(s->vy, -4); CHECK(s->flags & 4); CHECK_EQ(s->draw->dflags & 2, 0);
    int y = s->draw->y;
    tick(4);
    CHECK_EQ(s->draw->y, y - 8);                                     /* rising 2 px a frame */
    drop_game();
}

void test_tube_22_carries_the_kid_up(void) {                        /* type 22 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 22, 3);                         /* 2 + 1 rows of animated tube */
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->t0, 16); CHECK_EQ(s->t1, 2); CHECK(s->draw == NULL);
    sk_player *p = &g_g.player;
    p->x = (int16_t)(s->x0 + 16 + 4 - 2); p->y = s->y0;              /* at its mouth */
    tick(1);
    CHECK_EQ(aux_c(s), 2); CHECK_EQ(p->state, 0x40); CHECK_EQ(s->vy, -16); CHECK_EQ(g_g.cam_ease, 8);
    tick(1);
    CHECK_EQ(p->y, s->y0); CHECK_EQ(p->x, s->x0 + 16); CHECK_EQ(s->y0, s->y - 16);  /* carried up 8 px a frame, from the frame it caught him */
    tick(5);
    CHECK_EQ(p->exit_walk, 0xFF);                                    /* 40 px up: out of sight */
    drop_game();
}

void test_kicker_32_shoots_a_fire_trail(void) {                     /* types 32 and 110 */
    CHECK(make_game());
    sk_slot *s = spawn(100, 11 * 16, 32, 0);
    CHECK_EQ(s->vx, 2); CHECK_EQ(s->hits, 3); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 43); CHECK_EQ(s->y, 11 * 16 - 27);
    g_g.player.y = (int16_t)(s->draw->y + 2);
    g_g.player.x = (int16_t)(s->draw->x + 16 - 50);                  /* level with it, 50 px in front (it faces left) */
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0);
    int i = 0;
    while (count_type(110) == 0 && i++ < 60) tick(1);
    CHECK_EQ(count_type(110), 1); CHECK_EQ(s->t0, 100);
    g_g.player.x = 1000;
    i = 0;
    while (count_type(114) < 2 && i++ < 200) tick(1);
    CHECK(count_type(114) >= 2);                                     /* the burst, then fires along the floor */
    drop_game();
}

void test_factory_boss_34_stomps_and_drops_rocks(void) {           /* types 34 and 106 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 34, 0);
    CHECK_EQ(s->x, 184); CHECK_EQ(s->y, 11 * 16 - 48); CHECK_EQ(s->hits, 25); CHECK_EQ(s->draw->w, 64);
    CHECK_EQ(count_type(106), 4); CHECK_EQ(g_g.push_x, 184 - 128);
    sk_slot *rock = NULL;
    for (int i = 0; i < 16; i++) if ((g_g.objs.slot[i].type4 & 0x1FF) == 106 * 4 && g_g.objs.slot[i].x == 184 + 0x60) rock = &g_g.objs.slot[i];
    CHECK(rock != NULL); if (!rock) { drop_game(); return; }
    CHECK_EQ(rock->t0, 0x14); CHECK_EQ(aux_c(rock), 3);
    g_g.fboss_drop = 1;                                              /* a stomp: one rock falls at once */
    tick(1);
    CHECK_EQ(g_g.fboss_drop, 0);
    int dropped = 0;
    for (int i = 0; i < 16; i++) if ((g_g.objs.slot[i].type4 & 0x1FF) == 106 * 4 && aux_c(&g_g.objs.slot[i]) == 2) dropped++;
    CHECK_EQ(dropped, 1);
    int i = 0;
    while (count_type(107) == 0 && i++ < 400) tick(1);
    CHECK_EQ(count_type(107), 2);                                    /* the pair it throws */
    drop_game();
}

void test_train_23_warns_then_passes(void) {                  /* types 23, 108 and 109 */
    CHECK(make_game());
    g_g.camera_x = 100; g_g.camera_y = g_g.world.fg_max_y;          /* the train's view: at the bottom limit */
    sk_slot *s = spawn(200, 10 * 16, 23, 0);
    CHECK_EQ(s->t0, 75); CHECK(s->draw->dflags & 4); CHECK_EQ(s->draw->w, 16); CHECK_EQ(s->draw->h, 176);
    tick(35);
    CHECK_EQ(count_type(108), 1);                                    /* the warning blinks */
    tick(40);
    CHECK_EQ(s->t0, 0); CHECK_EQ(s->vx, 24); CHECK_EQ(s->draw->dflags & 4, 0);
    CHECK_EQ(s->draw->x, g_g.camera_x - 16); CHECK_EQ(s->draw->y, g_g.world.fg_max_y + 30);
    CHECK_EQ(count_type(109), 1);
    int x = s->draw->x;
    tick(1);
    CHECK_EQ(s->draw->x, x + 12);                                    /* the post sweeps by */
    drop_game();
}

static bool japan_open(sk_disks *d, sk_image *img, sk_tables *t) {
    sk_err e = {0}; uint8_t *f = NULL; size_t n = 0;
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!disks_open(d, roms, &e) || !disks_read(d, 0, "sk.pak", &f, &n, &e)) return false;
    bool ok = hunk_load(f, n, img, &e) && tables_load(t, img, &e);
    free(f);
    return ok;
}

void test_japan_markers_spawn_real_types(void) {                    /* ROM-backed, indices 18..22 */
    if (!getenv("SOCCERKID_ROMS")) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0}; sk_err e = {0};
    CHECK(japan_open(&d, &img, &t));
    static sk_game g; memset(&g, 0, sizeof g); objects_reset(&g); attrs_reset(&g);   /* install the handler tables */
    for (int idx = 18; idx <= 22; idx++) {
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

void test_japan_slots_cover_the_frames(void) {                      /* ROM-backed */
    if (!getenv("SOCCERKID_ROMS")) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0}; sk_err e = {0};
    CHECK(japan_open(&d, &img, &t));
    static const struct { int type, slot, frames, frame_bytes; } need[] = {   /* the highest frame each type indexes, + 1 (w / 8 * h * 5) */
        { 19, 21, 1, 320 }, { 21, 22, 4, 150 }, { 32, 33, 18, 860 }, { 32, 34, 19, 160 }, { 32, 35, 4, 440 },
        { 34, 40, 31, 2560 }, { 34, 41, 2, 340 }, { 34, 42, 8, 200 }, { 20, 36, 1, 1760 }, { 20, 37, 1, 640 }, { 20, 38, 6, 640 },
    };   /* type 20 stands for TRAIN's attribute-78/79 train (type 23), which no marker places */
    int seen = 0;
    for (int idx = 18; idx <= 22; idx++) {
        uint8_t *f = NULL; size_t n = 0;
        CHECK(disks_read_file_no(&d, t.record[idx].disk + 1, idx, false, &f, &n, &e));
        sk_level lv = {0}; sk_world w = {0};
        CHECK(level_parse(f, n, &lv, &e)); free(f);
        CHECK(world_build(&w, &lv));
        bool present[128] = {0};
        for (int cy = 0; cy < w.h; cy++)
            for (int cx = 0; cx < w.w; cx++) { int param = 0, type = world_marker_type(&w, cx, cy, &param); if (type > 0 && type < 128) present[type] = true; }
        present[20] = idx == 20;
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
    CHECK(seen >= 8);
    tables_free(&t); image_free(&img); disks_close(&d);
}
