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

void test_goose_turns_at_its_range(void) {                    /* type 54 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 11 * 16, 54, 1);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->vx, 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->y, 11 * 16 - 12);
    int i = 0;
    while (aux_c(s) == 0 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0); CHECK(160 - s->draw->x >= 0x78);
    tick(6);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->vx, 2);
    drop_game();
}

void test_hopper_hops_and_turns(void) {                       /* type 53 */
    CHECK(make_game());
    sk_slot *s = spawn(100, 11 * 16, 53, 0);                       /* 2 hops before turning */
    CHECK_EQ(s->x0, 2); CHECK_EQ(s->t0, 100);
    tick(100);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vy, -12 + 0); CHECK_EQ(s->vx, 6);
    int i = 0;
    while (aux_c(s) != 2 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 2);
    drop_game();
}

void test_farmer_walks_and_takes_two_hits(void) {             /* type 57 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 57, 0);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 64); CHECK_EQ(s->draw->y, 11 * 16 - 48);
    tick(10);
    CHECK_EQ(s->draw->x, 200 - 10);
    drop_game();
}

void test_flytrap_swallows_the_ball(void) {                   /* types 55 and 56 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 55, 0);
    CHECK_EQ(count_type(56), 1);
    CHECK(s->draw->dflags & 4);                                    /* hidden until it grows */
    tick(14);                                                      /* 7 frames x 2 ticks: full height */
    g_g.ball.x = 200 << 16; g_g.ball.y = (11 * 16 + 10) << 16; g_g.ball.vanishing = 0; g_g.ball.inactive = 0;
    tick(1);
    CHECK_EQ(g_g.ball.vanishing, 0xFF); CHECK_EQ(g_g.sfx_last, 14);
    drop_game();
}

void test_mole_throws_a_clod(void) {                          /* types 62 and 89 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 62, 0);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 64);
    s->t0 = -4;                                                     /* the first target: 4 px left */
    int k = 0;
    while (aux_c(s) != 1 && k++ < 20) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->draw->x, 196);
    int i = 0;
    while (count_type(89) == 0 && i++ < 400) tick(1);
    CHECK_EQ(count_type(89), 1);
    drop_game();
}

void test_leaper_jumps_from_below(void) {                     /* type 27 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 100, 27, 1);
    CHECK_EQ(s->draw->y, 100 + 160); CHECK_EQ(s->vy, -36); CHECK(s->flags & 4); CHECK_EQ(s->draw->dflags, 0);
    tick(10);
    CHECK(s->draw->y < 100 + 160);
    int i = 0;
    while (aux_c(s) == 0 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->t0, 40);
    drop_game();
}

void test_flies_use_a_fixed_record(void) {                    /* type 59 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 100, 59, 1);
    CHECK(s->draw_index >= SK_FIXED_INDEX_BASE);
    CHECK_EQ(s->draw->x, 212); CHECK_EQ(s->draw->y, 107);
    tick(1);
    CHECK_EQ(s->draw->dflags & 4, 0);
    drop_game();
}

void test_country_london_markers_spawn_real_types(void) {     /* ROM-backed, indices 2..5 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0}; sk_err e = {0};
    uint8_t *f = NULL; size_t n = 0;
    CHECK(disks_open(&d, roms, &e) && disks_read(&d, 0, "sk.pak", &f, &n, &e));
    CHECK(hunk_load(f, n, &img, &e)); free(f);
    CHECK(tables_load(&t, &img, &e));
    static sk_game g;
    memset(&g, 0, sizeof g);
    objects_reset(&g);
    for (int idx = 2; idx <= 5; idx++) {
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
        world_free(&w); level_free(&lv);
    }
    tables_free(&t); image_free(&img); disks_close(&d);
}
