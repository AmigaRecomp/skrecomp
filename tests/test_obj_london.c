#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "core/tables.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/obj_helpers.h"
#include "game/obj_london.h"
#include "game/platforms.h"
#include "game/player.h"
#include "game/resources.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static sk_resources g_res; static sk_section g_sec, g_psec; static uint8_t g_sheet[64 * 64 * 2], g_plat[32 / 8 * 10 * 5 * 4];
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv); memset(&g_res, 0, sizeof g_res);
    g_sec.data = g_sheet; g_sec.len = sizeof g_sheet;
    memset(g_plat, 0xFF, sizeof g_plat);
    g_psec.data = g_plat; g_psec.len = sizeof g_plat;
    for (int i = 0; i < SK_SLOTS; i++) g_res.slot[i] = &g_sec;
    g_res.param[0].sec = &g_psec;
    static const uint8_t bytes[6] = { 16, 5, 0, 0, 0x01, 0 };     /* half 16 x 5, no offset, frame_sel 1, stand_dy 0 */
    memcpy(g_res.param[0].bytes, bytes, 6);
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

void test_platform_type_5_moves_and_reverses(void) {
    CHECK(make_game());
    sk_slot *s = spawn(100, 120, 5, 0x12);                          /* speed (2 << 4) = 32: 1 px a frame; angle 0x10 << 2 = 64: right */
    CHECK(s != NULL);
    sk_platform *pf = slot_platform(&g_g, s);
    CHECK(pf != NULL);
    CHECK_EQ(pf->px >> 16, 108); CHECK_EQ(pf->py >> 16, 120); CHECK_EQ(pf->half_w, 16); CHECK_EQ(pf->pspeed, 32); CHECK_EQ(pf->pangle, 64);
    CHECK_EQ(pf->pvx, 0x10000); CHECK_EQ(pf->pvy, 0);
    tick(10);
    CHECK_EQ(pf->px >> 16, 118); CHECK_EQ(s->draw->x, 118 - 16);
    g_g.attrs.probe_kind = 2; g_g.attrs.probe_slot = s;            /* attribute 23 turns it */
    attr_table[23](&g_g, 0, 0);
    CHECK_EQ(s->t0, 1);
    tick(20);
    CHECK(pf->pvx < 0);                                             /* slowed, turned and speeding up leftward */
    tick(10);
    CHECK_EQ(s->t0, 0); CHECK_EQ(pf->pspeed, 32); CHECK_EQ(pf->pangle, 192);
    drop_game();
}

void test_triggered_platform_type_10_starts_when_stood_on(void) {
    CHECK(make_game());
    sk_slot *s = spawn(100, 120, 10, 0x12);
    sk_platform *pf = slot_platform(&g_g, s);
    CHECK(pf != NULL);
    tick(5);
    CHECK_EQ(pf->px >> 16, 108); CHECK_EQ(aux_c(s), 0);
    g_g.player.x = 108; g_g.player.y = 120 - 41; g_g.player.jumping = 0; g_g.player.falling = 0;
    tick(1);
    CHECK_EQ(aux_c(s), 0xFF);
    tick(5);
    CHECK(pf->px >> 16 > 108);
    drop_game();
}

void test_sin_1024_quadrants(void) {
    CHECK_EQ(sin_1024(0), 0); CHECK_EQ(sin_1024(256), 256); CHECK_EQ(sin_1024(512), 0); CHECK_EQ(sin_1024(768), -256);
    CHECK_EQ(cos_1024(0), 256); CHECK_EQ(sin_1024(128), 181); CHECK_EQ(sin_1024(257), sin_1024(255));
}

void test_sinking_platform_type_26(void) {
    CHECK(make_game());
    g_res.param[2].sec = &g_psec;                                    /* position 2 is hard-wired (move.w #$80,d0) */
    static const uint8_t bytes[6] = { 16, 12, 0, 8, 0, 0 };          /* COUNTRY2's entry */
    memcpy(g_res.param[2].bytes, bytes, 6);
    sk_slot *s = spawn(200, 100, 26, 0);
    sk_platform *pf = slot_platform(&g_g, s);
    CHECK(pf != NULL); CHECK_EQ(pf->py >> 16, 108); CHECK_EQ(s->y0, 108); CHECK_EQ(s->t0, 2); CHECK_EQ(pf->half_h, 12);
    g_g.player.jumping = 0; g_g.player.falling = 0; g_g.player.x = 208;
    s->t1 = 3;                                                       /* stood on */
    for (int k = 0; k < 20; k++) { g_g.player.y = (int16_t)((pf->py >> 16) + 1 - 41); tick(1); }   /* he stays on it */
    CHECK_EQ(pf->py >> 16, 108 + 20); CHECK_EQ(s->frame, 1); CHECK_EQ(s->draw->y, 108 + 20 - 4);
    g_g.player.x = 1000;
    tick(15);
    CHECK(pf->py >> 16 >= 106 && pf->py >> 16 <= 108);              /* back at rest, 2 a frame */
    for (int k = 0; k < 40; k++) { tick(1); CHECK(pf->py >> 16 >= 106 && pf->py >> 16 <= 108); }   /* hovering within 2 px */
    drop_game();
}

void test_stomper_type_61_bounces_the_kid(void) {
    CHECK(make_game());
    sk_slot *s = spawn(200, 11 * 16, 61, 0);
    CHECK_EQ(s->hits, 2); CHECK_EQ(s->vx, 2); CHECK_EQ(s->draw->w, 64); CHECK_EQ(s->draw->y, 11 * 16 - 48);
    g_g.player.x = (int16_t)(s->draw->x + 32 - 85); g_g.player.y = 12 * 16 - 41; g_g.player.invulnerable = 0;   /* in range, out of reach */
    tick(1);
    CHECK_EQ(aux_c(s), 1); CHECK_EQ(s->vx, 0);
    int i = 0;
    while (g_g.player.state != 72 && i++ < 60) tick(1);
    CHECK_EQ(g_g.player.state, 72); CHECK_EQ(g_g.flash_timer, 16);
    drop_game();
}

void test_fade_colour_steps_toward_white(void) {
    CHECK_EQ(fade_colour(0x000, 0xFFF, 0), 0x000);
    CHECK_EQ(fade_colour(0x000, 0xFFF, 8), 0x777);                  /* 8 steps of 15/16 */
    CHECK_EQ(fade_colour(0x123, 0xFFF, 15), 0xEEE);
}
