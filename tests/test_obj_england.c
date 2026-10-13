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
#include "game/obj_england.h"
#include "game/obj_helpers.h"
#include "game/resources.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static sk_resources g_res;
static uint8_t g_sheet[64 * 64 / 8 * 5 * 16];
static sk_section g_sec;

static bool make_game(bool with_res) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv); memset(&g_res, 0, sizeof g_res);
    memset(&g_sec, 0, sizeof g_sec); g_sec.data = g_sheet; g_sec.len = sizeof g_sheet;
    for (int i = 0; i < SK_SLOTS; i++) g_res.slot[i] = &g_sec;
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    if (!game_setup_level(&g_g, &g_lv, 0)) return false;
    g_g.res = with_res ? &g_res : NULL;
    objects_reset(&g_g);
    g_g.camera_x = 0; g_g.camera_y = 0;
    g_g.player.x = 1000; g_g.player.y = 12 * 16 - 41;          /* the kid far away unless a test moves it */
    g_g.player.state = 0;
    return true;
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static sk_slot *spawn(int x, int y, int type, int param) { g_g.objs.marker_param = param; return spawn_object(&g_g, x, y, type); }
static void tick(int n) { for (int i = 0; i < n; i++) { g_g.frame_ticks++; update_objects(&g_g); } }
static int count_type(int type) { int c = 0; for (int i = 0; i < 16; i++) c += g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == type * 4; return c; }
static void kicked_ball_at(int x, int y) {
    g_g.ball = (sk_ball){0};
    g_g.ball.x = x << 16; g_g.ball.y = y << 16; g_g.ball.vx = 0x40000;
    g_g.player.ball_free = 1; g_g.player.ball_hold = 0;
}

void test_type_without_its_slot_spawns_nothing(void) {         /* Review Focus 3 */
    CHECK(make_game(false));
    int count = g_g.object_count, draws = g_g.objs.draw_count;
    spawn(100, 100, 12, 0x55);
    CHECK_EQ(count_type(12), 0);
    CHECK_EQ(g_g.object_count, count); CHECK_EQ(g_g.objs.draw_count, draws);
    drop_game();
}

void test_bird_init_and_flight(void) {                         /* types 12 and 28 */
    CHECK(make_game(true));
    sk_slot *s = spawn(200, 60, 12, 0x55);
    CHECK(s != NULL);
    CHECK_EQ(s->t0, 10); CHECK_EQ(s->t1, 0x40); CHECK_EQ(s->hits, 1); CHECK_EQ(s->vx, 10); CHECK_EQ(s->aux, 25);
    CHECK(s->flags & 8); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 30); CHECK_EQ(s->frame, 0); CHECK_EQ(s->draw->prio, 1);
    tick(3);
    CHECK_EQ(s->draw->x, 200 - 15); CHECK_EQ(s->aux, 28);
    tick(0x40 - 28 + 1);                                         /* past t1: gliding, slowing down */
    CHECK_EQ(s->frame, 8); CHECK(s->vx < 10);
    sk_slot *b = spawn(300, 60, 28, 0x55);
    CHECK(b != NULL && b->draw->frames == g_sheet && b->t0 == 10);
    drop_game();
}

void test_dropping_falls_and_splashes(void) {                  /* types 8 and 124 */
    CHECK(make_game(true));
    sk_slot *s = spawn(100, 100, 8, 0);
    CHECK_EQ(s->vx, 2); CHECK_EQ(s->draw->w, 16);
    tick(0x35);
    CHECK_EQ(aux_c(s), 1); CHECK(s->flags & 4); CHECK_EQ(s->vx, 0);
    for (int i = 0; i < 200 && !(s->flags & 2); i++) tick(1);
    CHECK(s->flags & 2); CHECK_EQ(s->vy, -8); CHECK_EQ(s->frame, 2); CHECK_EQ(g_g.sfx_last, 15);
    CHECK_EQ(count_type(124), 1);
    drop_game();
}

void test_bulldog_turns_at_the_pit(void) {                     /* type 16 */
    CHECK(make_game(true));
    sk_slot *s = spawn(22 * 16, 11 * 16, 16, 1);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->vx, 4); CHECK_EQ(s->draw->y, 11 * 16 - 16); CHECK_EQ(s->draw->w, 48);
    s->flags |= 1;                                               /* facing right, towards the pit at column 26 */
    bool turned = false;
    for (int i = 0; i < 40 && !turned; i++) { tick(1); turned = !(s->flags & 1); }
    CHECK(turned);
    CHECK(s->draw->x + 48 <= 26 * 16 + 2);
    drop_game();
}

void test_crate_breaks_into_debris_and_a_bonus(void) {         /* types 24, 116 and 112 */
    CHECK(make_game(true));
    sk_slot *s = spawn(100, 11 * 16, 24, 4);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 20); CHECK_EQ(s->draw->y, 12 * 16 - 20);
    kicked_ball_at(98, s->draw->y + 4);
    tick(1);
    CHECK_EQ(s->hits, 1);
    tick(1); CHECK_EQ(s->frame, 1);                              /* the damaged frame */
    for (int i = 0; i < 5; i++) tick(1);                         /* the hit cooldown */
    kicked_ball_at(98, s->draw->y + 4);
    tick(1);
    CHECK_EQ(count_type(24), 0); CHECK_EQ(count_type(116), 4); CHECK_EQ(count_type(112), 1); CHECK_EQ(g_g.sfx_last, 8);
    drop_game();
}

void test_bonus_gives_its_kind(void) {                         /* type 112 kind 5: a life */
    CHECK(make_game(true));
    sk_slot *s = spawn(100, 100, 112, 4);
    CHECK_EQ(s->t0, 5); CHECK_EQ(s->t1, 100); CHECK_EQ(s->vy, -2); CHECK_EQ(s->draw->w, 32);
    int lives = g_g.player.lives;
    uint32_t score = g_g.player.score;
    g_g.player.x = (int16_t)(s->draw->x + 8); g_g.player.y = s->draw->y;
    tick(1);
    CHECK_EQ(count_type(112), 0); CHECK_EQ(g_g.player.lives, lives + 1); CHECK(g_g.player.score != score);
    CHECK_EQ(count_type(123), 1); CHECK_EQ(g_g.score_value, 0x2000);
    drop_game();
}

void test_workman_winds_up_and_punches(void) {                 /* types 45 and 99 */
    CHECK(make_game(true));
    sk_slot *s = spawn(200, 11 * 16, 45, 0);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->vx, 2); CHECK_EQ(s->draw->y, 11 * 16 - 48); CHECK_EQ(s->draw->w, 48);
    g_g.player.x = (int16_t)(s->draw->x + 24 - 60); g_g.player.y = 12 * 16 - 41;   /* in front: it faces left */
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0); CHECK_EQ(s->draw->w, 64);
    tick(8);
    CHECK_EQ(aux_c(s), 2); CHECK_EQ(count_type(99), 1); CHECK_EQ(s->t1, 100);
    tick(8);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->vx, 2);
    drop_game();
}

void test_rat_waits_then_runs(void) {                          /* type 58 */
    CHECK(make_game(true));
    sk_slot *s = spawn(200, 11 * 16, 58, 0x6D);
    CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->vx, 4); CHECK_EQ(s->t0, 32); CHECK_EQ(s->t1, 0x60); CHECK_EQ(s->hits, 1);
    tick(31);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->frame, 7); CHECK_EQ(s->draw->x, 200);
    tick(1);
    CHECK_EQ(aux_c(s), 1);
    tick(2);
    CHECK_EQ(s->draw->x, 204);
    drop_game();
}

void test_walker_pauses_away_from_home(void) {                 /* type 60 */
    CHECK(make_game(true));
    sk_slot *s = spawn(210, 11 * 16, 60, 0);          /* a clear run left of the slope */
    CHECK_EQ(s->vx, 4); CHECK_EQ(s->hits, 1); CHECK_EQ(s->draw->y, 11 * 16 - 33); CHECK_EQ(s->draw->h, 49);
    int i = 0;
    while (aux_c(s) == 0 && i++ < 100) tick(1);
    CHECK_EQ(aux_c(s), 1);
    CHECK(210 - s->draw->x >= 150);
    tick(16);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->flags & 1, 1);           /* turned round */
    drop_game();
}

void test_cyclist_crashes_when_hit_twice(void) {               /* types 63, 116 and 113 */
    CHECK(make_game(true));
    sk_slot *s = spawn(200, 11 * 16, 63, 0);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->vx, 4); CHECK_EQ(s->draw->w, 64); CHECK_EQ(s->draw->h, 56);
    for (int k = 0; k < 2; k++) { kicked_ball_at(s->draw->x - 4, s->draw->y + 30); tick(k ? 1 : 6); }
    CHECK(s->flags & 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(count_type(116), 1); CHECK_EQ(count_type(113), 1);
    drop_game();
}

void test_score_popup_picks_its_number(void) {                 /* type 113 */
    CHECK(make_game(true));
    g_g.score_value = 0x300;
    sk_slot *s = spawn(100, 100, 113, 0);
    CHECK_EQ(s->frame, 4); CHECK_EQ(s->vy, -8); CHECK_EQ(s->t0, 50);
    tick(51);
    CHECK_EQ(count_type(113), 0);
    drop_game();
}

void test_kick_off_banner_holds_the_kid(void) {                /* type 105 */
    CHECK(make_game(true));
    spawn_jingle(&g_g, 0);
    CHECK_EQ(count_type(105), 1); CHECK_EQ(g_g.player.state, 68); CHECK_EQ(g_g.player.invulnerable, 150);
    g_g.timer_flags = 1;
    int i = 0;
    while (count_type(105) && i++ < 400) tick(1);
    CHECK_EQ(count_type(105), 0); CHECK_EQ(g_g.player.state, 0); CHECK(g_g.timer_flags & 0x80);
    drop_game();
}

void test_every_home_marker_spawns_a_real_type(void) {         /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0}; sk_err e = {0};
    uint8_t *f = NULL; size_t n = 0;
    CHECK(disks_open(&d, roms, &e) && disks_read(&d, 0, "sk.pak", &f, &n, &e));
    CHECK(hunk_load(f, n, &img, &e)); free(f);
    CHECK(tables_load(&t, &img, &e));
    obj_england_install();
    for (int idx = 0; idx < 2; idx++) {
        CHECK(disks_read_file_no(&d, t.record[idx].disk + 1, idx, false, &f, &n, &e));
        sk_level lv = {0}; sk_world w = {0};
        CHECK(level_parse(f, n, &lv, &e)); free(f);
        CHECK(world_build(&w, &lv));
        int found = 0;
        for (int cy = 0; cy < w.h; cy++)
            for (int cx = 0; cx < w.w; cx++) {
                int param = 0, type = world_marker_type(&w, cx, cy, &param);
                if (type <= 0) continue;
                found++;
                bool real = type < 128 && object_init_table[type] && object_init_table[type] != obj_stub_init;
                if (!real) printf("level %d: type %d at (%d, %d) is a stub\n", idx, type, cx, cy);
                CHECK(real);
            }
        printf("level %d: %d markers\n", idx, found);
        CHECK(found > 10);
        world_free(&w); level_free(&lv);
    }
    tables_free(&t); image_free(&img); disks_close(&d);
}
