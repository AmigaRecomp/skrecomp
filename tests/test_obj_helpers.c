#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/obj_helpers.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static const uint8_t frames[0x258 * 4];
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    return game_setup_level(&g_g, &g_lv, 0);
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static sk_slot *object_at(int x, int y, int w, int h) {
    sk_slot *s = find_free_slot(&g_g);
    init_object_slot(&g_g, s, x, y, 8 * 4);
    set_draw_size(s->draw, w, h, frames);
    s->draw->x = (int16_t)x; s->draw->y = (int16_t)y;
    return s;
}
static void kicked_ball_at(int x, int y) {
    g_g.ball = (sk_ball){0};
    g_g.ball.x = x << 16; g_g.ball.y = y << 16; g_g.ball.vx = 0x40000;
    g_g.player.ball_free = 1; g_g.player.ball_hold = 0; g_g.player.state = 0;
}

void test_ball_hits_object_counts_hits_and_kills(void) {
    CHECK(make_game());
    g_g.camera_x = 0; g_g.camera_y = 0;
    sk_slot *s = object_at(100, 100, 32, 30);
    s->hits = 2;
    g_g.kill_kind = 1;
    kicked_ball_at(92, 104);                                             /* left of the box centre: a side hit */
    CHECK_EQ(ball_hits_object(&g_g, s, 0, 0, 16, 16, 5, 0x100), -2);    /* hurt */
    CHECK_EQ(g_g.sfx_last, 0x0B); CHECK_EQ(s->timer, 4); CHECK(s->draw->dflags & 2);
    CHECK_EQ(g_g.ball.vx >> 16, -4);                                     /* bounced back */
    CHECK_EQ(g_g.player.ball_free, 0);
    uint32_t score0 = g_g.player.score;
    kicked_ball_at(100, 100);
    for (int i = 0; i < 4; i++) CHECK_EQ(ball_hits_object(&g_g, s, 0, 0, 16, 16, 5, 0x100), -1);   /* the cooldown */
    CHECK_EQ(s->hits, 1);
    int slots0 = 0; for (int i = 0; i < 16; i++) slots0 += g_g.objs.slot[i].x >= 0;
    CHECK_EQ(ball_hits_object(&g_g, s, 0, 0, 16, 16, 5, 0x100), 0);      /* killed */
    CHECK_EQ(s->vy, -10); CHECK_EQ(s->vx, 0); CHECK_EQ(s->flags & 6, 6); CHECK_EQ(s->frame, 5);
    CHECK(g_g.player.score != score0); CHECK_EQ(g_g.kills[1], 1); CHECK_EQ(g_g.score_value, 0x100);
    int slots1 = 0; bool popup = false;
    for (int i = 0; i < 16; i++) if (g_g.objs.slot[i].x >= 0) { slots1++; popup |= g_g.objs.slot[i].type4 == 113 * 4; }
    CHECK_EQ(slots1, slots0 + 1); CHECK(popup);
    drop_game();
}

void test_ball_overlap_ignores_a_resting_ball(void) {
    CHECK(make_game());
    kicked_ball_at(100, 100);
    g_g.player.ball_free = 0;                                            /* not kicked */
    CHECK_EQ(ball_overlap(&g_g, 100, 100, 16, 16), 0);
    kicked_ball_at(100, 100);
    g_g.player.ball_hold = (int8_t)0x80;                                 /* at the feet */
    CHECK_EQ(ball_overlap(&g_g, 100, 100, 16, 16), 0);
    kicked_ball_at(100, 80);                                             /* from above: a vertical bounce */
    g_g.ball.vx = 0; g_g.ball.vy = 0x30000;
    CHECK_EQ(ball_overlap(&g_g, 92, 92, 16, 16), 2);
    CHECK_EQ(g_g.ball.vy >> 16, -3);
    drop_game();
}

void test_child_spawn_with_full_slots_is_skipped(void) {               /* Review Focus 2 */
    CHECK(make_game());
    for (int i = 0; i < 16; i++) if (g_g.objs.slot[i].x < 0) init_object_slot_invisible(&g_g, &g_g.objs.slot[i], 50 + i, 60, 4);
    sk_slot before[16];
    memcpy(before, g_g.objs.slot, sizeof before);
    CHECK(spawn_child(&g_g, &g_g.objs.slot[0], 100, 100, 111, 4, frames, frames, 16, 8, 4, -16) == NULL);
    CHECK(memcmp(before, g_g.objs.slot, sizeof before) == 0);
    drop_game();
}

void test_spawn_child_mirrors_for_a_right_facing_parent(void) {
    CHECK(make_game());
    sk_slot *p = object_at(100, 100, 16, 16);
    p->flags |= 1;                                                       /* facing right */
    CHECK(spawn_child(&g_g, p, 100, 100, 111, 4, frames, frames, 16, 8, 4, -16) != NULL);
    CHECK_EQ(g_g.objs.child_vx, 4); CHECK_EQ(g_g.objs.child_w, 16); CHECK_EQ(g_g.objs.child_vy, -16);   /* type 111 took bit 15 (bclr) */
    CHECK_EQ(g_g.objs.last_slot->flags & 1, 1); CHECK_EQ(g_g.objs.last_slot->vx, 4);                    /* as its facing */
    CHECK_EQ(g_g.objs.last_slot->x, 104);                                /* x - 4 + 8 */
    drop_game();
}

void test_ground_ahead_at_the_pit_edge(void) {
    CHECK(make_game());
    sk_slot *s = object_at(25 * 16, 12 * 16 - 16, 16, 16);              /* standing on column 25, the pit at 26 */
    CHECK_EQ(ground_ahead(&g_g, s, 0), 0);                               /* facing left: ground */
    CHECK_EQ(ground_ahead(&g_g, s, 1), -1);                              /* facing right: the corner is over the pit */
    s->draw->y = 12 * 16 - 13;                                           /* sunk 3 px into the floor */
    CHECK_EQ(probe_rows_below(&g_g, s, 4, 0), -3);
    s->draw->y = 12 * 16 - 16;
    CHECK_EQ(probe_rows_below(&g_g, s, 4, 0), 0);
    drop_game();
}

void test_frames_32x30_sizes(void) {
    CHECK(make_game());
    sk_slot *s = object_at(10, 20, 16, 16);
    s->x = 33; s->y = 44;
    frames_32x30(&g_g, s, frames, 7);
    CHECK_EQ(s->draw->blit_size, 0x783); CHECK_EQ(s->draw->plane_bytes, 0x78); CHECK_EQ(s->draw->frame_bytes, 0x258);
    CHECK_EQ(s->draw->w, 32); CHECK_EQ(s->draw->h, 30); CHECK_EQ(s->draw->prio, 0); CHECK_EQ(s->draw->dflags, 7);   /* the word store: byte 0x1A 0, flags 7 */
    CHECK_EQ(s->draw->x, 33); CHECK_EQ(s->draw->y, 44); CHECK(s->draw->frame == frames);
    frames_16x16(&g_g, s, frames, 0);
    CHECK_EQ(s->draw->blit_size, 0x402); CHECK_EQ(s->draw->frame_bytes, 0xA0);
    drop_game();
}
