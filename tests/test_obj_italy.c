#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "app/scene.h"
#include "fixture.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "core/tables.h"
#include "game/attrs.h"
#include "game/world.h"
#include "gfx/framebuffer.h"
#include "gfx/display_model.h"
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

void test_pendulum_swings_and_reverses(void) {                 /* type 36 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 6 * 16, 36, 0);
    CHECK(g_g.objs.chain_a); CHECK(s->draw == NULL);
    CHECK_EQ(s->t1, 28); CHECK_EQ(s->y, 6 * 16 + 8);
    tick(1);
    CHECK_EQ(s->t0, 4); CHECK_EQ(s->t1, 27);
    const sk_draw *ball = &g_g.objs.chain[7];
    CHECK(ball->y > s->y + 40);                                      /* hangs below its pivot */
    CHECK(ball->x > s->x - 8);                                       /* swung to the right */
    CHECK_EQ(g_g.objs.chain[0].x, s->x); CHECK_EQ(g_g.objs.chain[0].y, s->y);
    int i = 0;
    while ((s->flags & 0xFF) == 0 && i++ < 100) tick(1);
    CHECK_EQ(s->flags & 0xFF, 0xFF); CHECK_EQ(aux_c(s), 0xFF);      /* it turns at the top of the swing */
    int top = s->t0;
    CHECK(top > 60 && top <= 70);
    tick(1);
    CHECK_EQ(s->t0, top);                                           /* speed 0: it hangs one frame */
    tick(1);
    CHECK(s->t0 < top);
    i = 0;
    while (s->t0 > 0 && i++ < 100) tick(1);
    CHECK_EQ(s->t0, 0); CHECK_EQ(aux_c(s), 0);                      /* back at the bottom: slowing again */
    g_g.camera_x = 2000;
    tick(1);
    CHECK(s->x < 0); CHECK(!g_g.objs.chain_a);
    drop_game();
}

void test_jumper_lands_waits_and_jumps_again(void) {          /* type 37 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 11 * 16, 37, 0);
    CHECK_EQ(s->vy, -20); CHECK_EQ(s->t0, -20); CHECK_EQ(s->draw->w, 16); CHECK_EQ(s->draw->h, 21);
    int y0 = s->draw->y;
    tick(4);
    CHECK(s->draw->y < y0);                                          /* thrown up */
    int i = 0;
    while (aux_c(s) == 0 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->x0, 0);                       /* landed */
    tick(30);
    CHECK_EQ(s->x0, 30);
    tick(25);
    CHECK(s->draw->dflags & 4);                                      /* hidden after 50 */
    tick(20);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->draw->y, s->y); CHECK_EQ(s->vy, -20);   /* back at its start */
    CHECK_EQ(s->draw->dflags & 6, 0);
    drop_game();
}

static uint8_t g_plat[32 * 10 / 8 * 5];
static sk_section g_psec;
static void give_platform_param(int pos) {
    memset(g_plat, 0xFF, sizeof g_plat);
    g_psec.data = g_plat; g_psec.len = sizeof g_plat;
    g_res.param[pos].sec = &g_psec;
    static const uint8_t bytes[6] = { 16, 5, 0, 0, 0x01, 0 };     /* half 16 x 5, frame_sel 1, stand_dy 0 */
    memcpy(g_res.param[pos].bytes, bytes, 6);
}

void test_swing_platform_hangs_on_its_chain(void) {           /* type 38 */
    CHECK(make_game());
    give_platform_param(0);
    sk_slot *s = spawn(160, 4 * 16, 38, 0x10);                       /* radius 64 + 16 */
    CHECK_EQ(s->x0, 80); CHECK_EQ(g_g.objs.chain_count, 3); CHECK(g_g.objs.chain_b);
    sk_platform *pf = slot_platform(&g_g, s);
    CHECK(pf != NULL);
    CHECK_EQ(pf->pspeed, 0x1E); CHECK_EQ(pf->pangle, 0);
    CHECK_EQ(pf->px >> 16, s->x); CHECK_EQ(pf->py >> 16, s->y + 80);
    tick(1);
    CHECK_EQ(pf->pangle, 3); CHECK_EQ(pf->pspeed, 0x1D);
    CHECK((pf->px >> 16) > s->x);                                    /* swung right on its radius */
    CHECK_EQ(s->draw->x, (pf->px >> 16) - 16); CHECK_EQ(s->draw->y, pf->py >> 16);
    CHECK_EQ(g_g.objs.chain[0].y, s->y + 15);                        /* the first link 16 px down, cos(12) * 16 / 256 */
    int i = 0;
    while (((uint16_t)s->t0 >> 8) == 0 && i++ < 100) tick(1);
    CHECK_EQ((uint16_t)s->t0 >> 8, 0xFF);                            /* turned at the top */
    CHECK(pf->pangle > 40 && pf->pangle < 100);
    g_g.camera_x = 3000;
    tick(1);
    CHECK(s->x < 0); CHECK(!g_g.objs.chain_b);
    drop_game();
}

void test_strider_walks_turns_and_leaps(void) {               /* type 43 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 11 * 16, 43, 0);
    CHECK_EQ(s->vx, 8); CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 52); CHECK_EQ(s->y, 11 * 16 - 36);
    int i = 0;
    while (aux_c(s) == 0 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK(s->x - s->draw->x >= 160);         /* walked 160 px left */
    i = 0;
    while (aux_c(s) == 1 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->vx, 0);   /* stopped and turned */
    drop_game();
    CHECK(make_game());
    s = spawn(400, 11 * 16, 43, 0);
    g_g.player.x = (int16_t)(s->draw->x + 24 - 96);
    tick(1);
    CHECK_EQ(aux_c(s), 4); CHECK_EQ(s->draw->h, 58); CHECK(s->flags & 4);   /* leaps at the kid */
    i = 0;
    while (aux_c(s) == 4 && i++ < 200) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->draw->h, 52); CHECK(!(s->flags & 4));
    drop_game();
}

void test_thrown_rider_falls_after_50(void) {                 /* type 100 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 5 * 16, 100, 1);
    CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->vx, 6); CHECK_EQ(s->t0, 50); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->frame, 5);
    tick(49);
    CHECK(!(s->flags & 4));
    tick(1);
    CHECK(s->flags & 4);
    tick(200);
    CHECK(s->x >= 0);                                                /* the original never frees it (the offscreen result is dropped) */
    drop_game();
}

void test_bouncer_hops_then_leaves(void) {                    /* type 44 */
    CHECK(make_game());
    g_g.camera_x = 400;                                              /* its marker in view */
    sk_slot *s = spawn(600, 11 * 16, 44, 0);
    CHECK_EQ(s->vx, 6); CHECK(s->flags & 4); CHECK(s->flags & 8); CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->y, 11 * 16 - 16);
    int i = 0, bounces = 0;
    for (; i < 300 && !(s->flags & 2); i++) { tick(1); bounces += s->vy == -4; }
    CHECK(bounces >= 2);                                             /* bounced off the ground */
    CHECK(s->flags & 2);                                             /* 400 px from its marker: out of play */
    CHECK(s->x < 0);                                                 /* and freed, its draw 200 px outside the view */
    drop_game();
}

void test_rock_trembles_then_drops_on_the_kid(void) {         /* type 48 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 3 * 16, 48, 0);
    CHECK_EQ(s->x, 152); CHECK_EQ(s->y, 3 * 16 - 24); CHECK_EQ(s->draw->w, 32);
    g_g.player.x = 400;
    g_g.frame_ticks = 3;                                             /* the next frame is phase 0 */
    tick(1);
    CHECK_EQ(s->draw->x, 153); CHECK_EQ(aux_c(s), 0);              /* trembles: +1, 0, -1, 0 */
    tick(3);
    CHECK_EQ(s->draw->x, 152);
    g_g.player.x = (int16_t)(s->draw->x + 16 + 10);
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK(s->flags & 4); CHECK(s->flags & 2);
    int y = s->draw->y;
    tick(5);
    CHECK(s->draw->y > y);                                           /* falls */
    drop_game();
}

void test_hopper_49_bounces_and_leaves(void) {                /* type 49 */
    CHECK(make_game());
    g_g.camera_x = 400;
    sk_slot *s = spawn(600, 11 * 16, 49, 0);
    CHECK_EQ(s->vx, 4); CHECK_EQ(s->vy, -4); CHECK(s->flags & 12);
    int i = 0, bounces = 0;
    for (; i < 400 && !(s->flags & 2); i++) { tick(1); bounces += s->vy == -4; }
    CHECK(bounces >= 2); CHECK(s->flags & 2);
    drop_game();
}

static int count_type(int type) { int c = 0; for (int i = 0; i < 16; i++) c += g_g.objs.slot[i].x >= 0 && (g_g.objs.slot[i].type4 & 0x1FF) == type * 4; return c; }

void test_thrower_64_patrols_and_throws_at_the_kid(void) {    /* type 64 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 64, 0);
    CHECK_EQ(s->vx, 2); CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->draw->h, 64); CHECK_EQ(s->y, 11 * 16 - 48);
    g_g.player.x = (int16_t)(s->draw->x - 40);                       /* in front of it (it faces left) */
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0);                       /* stops to throw */
    int i = 0;
    while (count_type(99) == 0 && i++ < 100) tick(1);
    CHECK_EQ(count_type(99), 1);                                     /* the throw on the 4th attack frame */
    i = 0;
    while (aux_c(s) == 1 && i++ < 100) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->vx, 2); CHECK_EQ(s->t0, 50); /* walks on, 50 frames before the next */
    drop_game();
}

void test_thrower_68_throws_and_waits_25(void) {              /* type 68 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 68, 0);
    CHECK_EQ(s->vx, 2); CHECK_EQ(s->hits, 2); CHECK_EQ(s->draw->w, 48); CHECK_EQ(s->y, 11 * 16 - 48);
    g_g.player.x = (int16_t)(s->draw->x + 24 - 60);
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0);
    int i = 0;
    while (count_type(99) == 0 && i++ < 100) tick(1);
    CHECK_EQ(count_type(99), 1);
    i = 0;
    while (aux_c(s) == 1 && i++ < 100) tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->t0, 25); CHECK_EQ(s->frame, 0);
    drop_game();
}

void test_flyer_70_bobs_and_chases(void) {                    /* type 70 */
    CHECK(make_game());
    g_g.player.x = 1000;
    sk_slot *s = spawn(200, 8 * 16, 70, 0);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->flags & 1, 1); CHECK_EQ(s->draw->h, 64); CHECK_EQ(s->y, 8 * 16 - 48);   /* faces the kid */
    tick(1);
    CHECK_EQ(aux_c(s), 0); CHECK_EQ(s->vx, 0); CHECK_EQ(s->t0, 1);   /* waits, bobbing */
    tick(31);
    CHECK_EQ(s->t0, 32);
    tick(1);
    CHECK_EQ(s->t0, 0);                                              /* the 0x1000 end: the 32-step bob wraps */
    g_g.player.x = (int16_t)(s->draw->x + 24 + 60);
    tick(1);
    CHECK_EQ(aux_c(s) & 3, 3); CHECK_EQ(s->vx, 4);                   /* the kid near: it chases */
    int x = s->draw->x;
    tick(4);
    CHECK(s->draw->x > x);
    drop_game();
}

void test_wings_88_follow_then_fly_off(void) {                /* type 88 */
    CHECK(make_game());
    g_g.player.x = 1000;
    sk_slot *f = spawn(200, 8 * 16, 70, 0);
    sk_slot *w = spawn(204, 80, 88, 1);
    CHECK_EQ(w->flags & 1, 1); CHECK_EQ(w->draw->w, 48); CHECK_EQ(w->draw->h, 39);
    tick(1);
    CHECK_EQ(w->draw->x, f->draw->x + 4); CHECK_EQ(w->draw->y, f->draw->y);
    w->t0 = 1;                                                       /* type 70 killed */
    tick(1);
    CHECK_EQ(aux_c(w), 0xFF); CHECK(w->flags & 4); CHECK(w->flags & 2);
    int y = w->draw->y;
    tick(3);
    CHECK(w->draw->y < y);                                           /* flies up */
    drop_game();
}

void test_frog_31_hops_its_pattern_and_spits(void) {          /* type 31, child 111 */
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 31, 0);
    CHECK_EQ(s->vy, -10); CHECK_EQ(s->t0, 10); CHECK_EQ(s->t1, 1); CHECK_EQ(s->aux, 8); CHECK_EQ(s->hits, 2);
    CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 21);
    int i = 0;
    while (s->aux == 8 && i++ < 100) tick(1);
    CHECK_EQ(s->aux, 16);                                            /* one hop, then the third entry: (4, -18, 1, 0) */
    CHECK_EQ(s->vx, 4); CHECK_EQ(s->vy, -18); CHECK_EQ(s->t0, 18); CHECK_EQ(s->t1, 1); CHECK_EQ(s->flags & 1, 0);
    i = 0;
    while (count_type(111) == 0 && i++ < 100) tick(1);
    CHECK_EQ(count_type(111), 1);                                    /* spits at the top of a high hop */
    for (i = 0; i < 16; i++) if ((g_g.objs.slot[i].type4 & 0x1FF) == 111 * 4 && g_g.objs.slot[i].x >= 0) {
        CHECK_EQ(g_g.objs.slot[i].vx, 8); CHECK_EQ(g_g.objs.slot[i].vy, -2); CHECK(g_g.objs.slot[i].flags & 4);
    }
    i = 0;
    while (s->aux != 0 && i++ < 600) tick(1);
    CHECK_EQ(s->aux, 0);                                             /* the -1 end wraps to entry 0 */
    drop_game();
}

void test_pendulum_knocks_the_kid_along_its_swing(void) {     /* type 36: player.knock_dir (0x302B4A) */
    CHECK(make_game());
    sk_slot *s = spawn(160, 2 * 16, 36, 0);
    tick(1);
    const sk_draw *ball = &g_g.objs.chain[7];
    g_g.player.x = (int16_t)(ball->x + 8 + 8); g_g.player.y = (int16_t)(ball->y + 7 - 10);
    g_g.player.health = 3; g_g.player.invulnerable = 0; g_g.player.knock_dir = 5;
    tick(1);
    CHECK_EQ(s->flags & 0xFF, 0);
    CHECK_EQ(g_g.player.knock_dir, -1);                              /* not.w of ext.w of the swing direction 0 */
    drop_game();
}

void test_backdrop_relative_records_draw_at_the_backdrop_scroll(void) {   /* draw_object: bit 4 skips view +0x1C */
    CHECK(make_game());
    static sk_fb fb; CHECK(fb_init(&fb, SK_FB_W, SK_FB_H));
    sk_draw *d = &g_g.objs.chain[0];
    static uint8_t img[16 * 16 / 8 * 5]; memset(img, 0xFF, sizeof img);
    set_draw_size(d, 16, 16, img); d->used = true; d->dflags = 0x10;
    d->x = 200 + 40; d->y = 30 + 20;
    g_g.objs.chain_a = true;
    g_g.view.bg_x = 200; g_g.view.bg_y = 30;
    fb_clear(&fb, 0);
    draw_all_objects(&g_g, &fb, 1000, 500);
    CHECK(fb.px[(20 + 4) * fb.w + 40 + 4] != 0);                     /* backdrop coordinates, whatever the camera */
    fb_free(&fb);
    drop_game();
}

void test_backdrop_cells_paint_save_and_restore(void) {        /* 0x11A49A, 0x1154C2, 0x1154FA */
    CHECK(make_game());
    sk_world *w = &g_g.world;
    CHECK(w->bg != NULL); CHECK(w->bg_w >= 20); CHECK(w->bg_h >= 14);
    paint_bg_cells(&g_g, 3 * 16 + 5, 2 * 16 + 9, 8, 4);
    for (int k = 0; k < 4; k++) CHECK_EQ(w->bg[2 * w->bg_w + 3 + k], 8);
    int col = (w->bg_max_x & 0xFFF0) >> 4;
    w->bg[1 * w->bg_w + col + 2] = 77;
    save_backdrop(&g_g);
    CHECK_EQ(w->bg[1 * w->bg_w + 2], 77);                            /* the last screen copied over columns 0..19 */
    w->bg[1 * w->bg_w + 2] = 55;
    restore_backdrop(&g_g);
    CHECK_EQ(w->bg[1 * w->bg_w + col + 2], 55);
    drop_game();
}

void test_fill_cells_writes_a_column(void) {                   /* 0x11A41E */
    CHECK(make_game());
    sk_world *w = &g_g.world;
    fill_cells(&g_g, 6 * 16 + 3, 4 * 16, 0x1BB, 0, 2);
    for (int k = 0; k < 3; k++) {
        CHECK_EQ(w->tile[(4 + k) * w->w + 6] & SK_INDEX_MASK, 0x1BB);
        CHECK_EQ(w->state[(4 + k) * w->w + 6], 0x20);
    }
    CHECK((w->tile[7 * w->w + 6] & SK_INDEX_MASK) != 0x1BB);
    drop_game();
}

void test_halfstep_velocity_adds_the_whole_step_on_odd_frames(void) {   /* 0x10E0B6 */
    CHECK(make_game());
    sk_slot *s = spawn(160, 6 * 16, 44, 0);
    s->flags = 1 | 4; s->vx = 5; s->vy = -3;
    int x = s->draw->x, y = s->draw->y;
    g_g.frame_ticks = 2;
    apply_velocity_halfstep(&g_g, s);
    CHECK_EQ(s->draw->x, x + 2 + 1); CHECK_EQ(s->draw->y, y - (1 + 1));   /* even: the odd bit */
    x = s->draw->x; y = s->draw->y;
    g_g.frame_ticks = 3;
    apply_velocity_halfstep(&g_g, s);
    CHECK_EQ(s->draw->x, x + 2 + 5); CHECK_EQ(s->draw->y, y - (1 - 3));   /* odd: the whole word, as the original */
    drop_game();
}

void test_the_backdrop_draws_from_the_writable_cells(void) {
    CHECK(make_game());
    static sk_fb a, b; CHECK(fb_init(&a, SK_FB_W, SK_FB_H)); CHECK(fb_init(&b, SK_FB_W, SK_FB_H));
    sk_world *w = &g_g.world;
    fb_clear(&a, 0); scene_draw_backdrop_cells(&g_lv, w->bg, &a, 0, 0);
    int differ = 0;
    uint8_t keep = w->bg[1 * w->bg_w + 2];
    w->bg[1 * w->bg_w + 2] = (uint8_t)(keep + 1);
    fb_clear(&b, 0); scene_draw_backdrop_cells(&g_lv, w->bg, &b, 0, 0);
    for (int i = 0; i < SK_FB_W * SK_FB_H; i++) differ += a.px[i] != b.px[i];
    CHECK(differ > 0);
    fb_free(&a); fb_free(&b);
    drop_game();
}
