#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/game.h"
#include "game/objects.h"
#include "game/player.h"
#include "gfx/display_model.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e) || !world_build(&g_g.world, &g_lv)) return false;
    init_player(&g_g);
    objects_reset(&g_g);
    return true;
}
static void drop_game(void) { world_free(&g_g.world); level_free(&g_lv); free(g_d); }
static void frames(int n, uint8_t input) {
    for (int i = 0; i < n; i++) { g_g.frame_ticks++; g_g.input_state = input; g_g.mask.built = false; player_input_state(&g_g); }
}
static int g_inits, g_updates;
static void counting_init(sk_game *g, sk_slot *s, int x, int y) { init_object_slot(g, s, x, y, s->type4); g_inits++; }
static void counting_update(sk_game *g, sk_slot *s, int index) { (void)index; g_updates++; despawn_if_far(g, s); }

void test_spawn_marker_allocates_a_slot_with_type_and_param(void) {
    CHECK(make_game());
    object_init_table[7] = counting_init; object_update_table[7] = counting_update;
    g_inits = 0;
    scan_cell_for_marker(&g_g, 10, 11);
    CHECK_EQ(g_inits, 1); CHECK_EQ(g_g.object_count, 1);
    sk_slot *s = &g_g.objs.slot[0];
    CHECK_EQ(s->x, 10 * 16); CHECK_EQ(s->y, 11 * 16); CHECK_EQ(s->x0, s->x); CHECK_EQ(s->type4, 7 * 4);
    CHECK_EQ(g_g.objs.marker_param, 0x55); CHECK(s->draw != NULL); CHECK(s->draw->used); CHECK_EQ(g_g.objs.draw_count, 1);
    scan_cell_for_marker(&g_g, 10, 11);                        /* the marker is in use: no second object */
    CHECK_EQ(g_g.object_count, 1);
    scan_cell_for_marker(&g_g, 16, 11);                        /* an attribute cell is not a marker */
    CHECK_EQ(g_g.object_count, 1);
    free_object(&g_g, s);
    CHECK(s->x < 0); CHECK_EQ(g_g.object_count, 0); CHECK_EQ(g_g.objs.draw_count, 0); CHECK(!s->draw->used);
    g_updates = 0; update_objects(&g_g); CHECK_EQ(g_updates, 0);
    drop_game();
}

void test_spawn_marker_ignores_types_beyond_the_table(void) {   /* Review Focus 3 */
    CHECK(make_game());
    g_g.world.live[11 * 40 + 12] = (uint16_t)(6 << 9);          /* flag 6 -> PALS type 200 */
    g_g.world.tile[11 * 40 + 12] = SK_MARKER_BIT;
    scan_cell_for_marker(&g_g, 12, 11);
    CHECK_EQ(g_g.object_count, 0);
    scan_cell_for_marker(&g_g, 99, 99);                        /* outside the map: nothing */
    CHECK_EQ(g_g.object_count, 0);
    drop_game();
}

void test_slots_fill_up_at_sixteen(void) {
    CHECK(make_game());
    for (int i = 0; i < 20; i++) spawn_object(&g_g, 100, 100, 7);
    CHECK_EQ(g_g.object_count, 16);
    CHECK(find_free_slot(&g_g) == NULL);
    drop_game();
}

void test_despawn_if_far_uses_the_160_margin(void) {
    CHECK(make_game());
    object_init_table[7] = counting_init;
    g_g.camera_x = 0; g_g.camera_y = 0;
    sk_slot *s = spawn_object(&g_g, 320 + 159, 50, 7);
    s->draw->x = s->x; s->draw->y = s->y;
    despawn_if_far(&g_g, s); CHECK(s->x >= 0);
    s->x = 320 + 161; s->draw->x = s->x;
    despawn_if_far(&g_g, s); CHECK(s->x < 0);
    CHECK_EQ(object_offscreen(&g_g, -161, 0, 160), -1); CHECK_EQ(object_offscreen(&g_g, -160, 0, 160), 0);
    drop_game();
}

void test_step_animation_follows_the_script(void) {
    sk_slot s = {0}; sk_draw d = {0}; s.draw = &d;
    set_draw_size(&d, 16, 16, (const uint8_t *)"frames");
    CHECK_EQ(d.blit_size, (16 << 6) | 2); CHECK_EQ(d.plane_bytes, 32); CHECK_EQ(d.frame_bytes, 160);
    static const uint8_t script[] = { 0, 3, 0, 2, 1, 5, 0xFF, 2, 6, 0 };   /* count 3, ticks 2, frames 1 5 FF, facing-right list 2 6 0 */
    int wrapped = 0, changed;
    changed = step_animation(&s, script, 0, &wrapped); CHECK_EQ(changed, 0); CHECK_EQ(s.frame, 1);
    changed = step_animation(&s, script, 0, &wrapped); CHECK_EQ(changed, -1); CHECK_EQ(s.anim_index, 1); CHECK_EQ(s.frame, 5);
    CHECK(d.frame == d.frames + 5 * 160);
    step_animation(&s, script, 0, &wrapped); step_animation(&s, script, 0, &wrapped);
    CHECK(d.dflags & 4);                                         /* 0xFF hides */
    step_animation(&s, script, 0, &wrapped); step_animation(&s, script, 0, &wrapped);
    CHECK_EQ(wrapped, -1); CHECK_EQ(s.anim_index, 0); CHECK(!(d.dflags & 4));
    step_animation(&s, script, 1, &wrapped); CHECK_EQ(s.frame, 2);   /* the facing-right list */
}

void test_touch_player_box_and_add_score(void) {
    CHECK(make_game()); frames(20, 0);
    int px = g_g.player.x, py = g_g.player.y;
    CHECK_EQ(touch_player(&g_g, px - 8, py, 4, 0x26), -1);       /* a hit: knock-back */
    g_g.player.invulnerable = 0; g_g.player.state = 0;
    CHECK_EQ(touch_player(&g_g, px + 100, py, 4, 4), 0);
    g_g.player.score = 0x00199980; g_g.player.lives = 1;
    add_score(&g_g, 0x20);
    CHECK_EQ(g_g.player.score, 0x00200000); CHECK_EQ(g_g.player.lives, 2);   /* an even hundred-thousands digit: a life */
    add_score(&g_g, 0x00100000);
    CHECK_EQ(g_g.player.score, 0x00300000); CHECK_EQ(g_g.player.lives, 2);   /* odd: no life */
    drop_game();
}

void test_draw_all_objects_blits_fixture_frames(void) {
    CHECK(make_game());
    object_init_table[7] = counting_init;
    uint8_t *fd = NULL; size_t fn = 0;
    CHECK(load_fixture("frames_tiny.plain", &fd, &fn));         /* 3 frames of 16x16: planes + mask */
    sk_slot *s = spawn_object(&g_g, 100, 100, 7);
    set_draw_size(s->draw, 16, 16, fd); set_object_frame(s, 1);
    s->draw->x = 100; s->draw->y = 100;
    sk_fb fb; CHECK(fb_init(&fb, SK_FB_W, SK_FB_H));
    fb_clear(&fb, 0);
    draw_all_objects(&g_g, &fb, 80, 90);
    CHECK(fb.px[10 * SK_FB_W + 20] != 0);                        /* drawn at (100-80, 100-90) in group 0 */
    CHECK_EQ(fb.px[10 * SK_FB_W + 20] & 0xF0, 0);
    s->draw->dflags |= 0x40; fb_clear(&fb, 0);
    draw_all_objects(&g_g, &fb, 80, 90);
    CHECK_EQ(fb.px[10 * SK_FB_W + 20] & 0x0F, 0); CHECK(fb.px[10 * SK_FB_W + 20] & 0xF0);   /* playfield 2 */
    s->draw->dflags = 4; fb_clear(&fb, 0);
    draw_all_objects(&g_g, &fb, 80, 90);
    CHECK_EQ(fb.px[10 * SK_FB_W + 20], 0);                       /* hidden */
    fb_free(&fb); free(fd); drop_game();
}

void test_deferred_free_does_not_blank_a_reused_record(void) {  /* 5a #7 */
    CHECK(make_game());
    sk_slot *a = find_free_slot(&g_g);
    init_object_slot(&g_g, a, 100, 100, 4);
    sk_draw *d = a->draw;
    free_object_deferred(&g_g, a);
    sk_slot *b = find_free_slot(&g_g);
    init_object_slot(&g_g, b, 120, 100, 4);
    CHECK(b->draw == d);                                         /* the record is reused at once */
    objects_flush_deferred(&g_g);
    CHECK(b->draw->used);
    drop_game();
}

void test_alloc_draw_record_refuses_past_16(void) {             /* 5a #10 */
    CHECK(make_game());
    for (int i = 0; i < 16; i++) init_object_slot(&g_g, &g_g.objs.slot[i], 10 * i, 50, 4);
    CHECK_EQ(g_g.objs.draw_count, 16);
    init_object_slot(&g_g, &g_g.objs.slot[0], 0, 50, 4);         /* a seventeenth record */
    CHECK_EQ(g_g.objs.draw_count, 16); CHECK(g_g.objs.slot[0].draw == NULL); CHECK_EQ(g_g.objs.slot[0].draw_index, -1);
    drop_game();
}

static int g_spawns;
static void marking_init(sk_game *g, sk_slot *s, int x, int y) { init_object_slot(g, s, x, y, s->type4); g_spawns++; }

void test_a_marker_spawns_once_while_its_object_lives(void) {   /* bset #6 on the marker's cell (0x10D5E2) */
    CHECK(make_game());
    object_init_table[7] = marking_init;                          /* the fixture's marker (10, 11): flag 5 -> type 7 */
    g_spawns = 0;
    scan_cell_for_marker(&g_g, 10, 11); scan_cell_for_marker(&g_g, 10, 11);
    CHECK_EQ(g_spawns, 1);
    sk_slot *s = NULL;
    for (int i = 0; i < 16; i++) if (g_g.objs.slot[i].x >= 0 && g_g.objs.slot[i].type4 == 28) s = &g_g.objs.slot[i];
    CHECK(s != NULL);
    free_object(&g_g, s);                                          /* despawned: the marker is free again */
    scan_cell_for_marker(&g_g, 10, 11);
    CHECK_EQ(g_spawns, 2);
    for (int i = 0; i < 16; i++) if (g_g.objs.slot[i].x >= 0 && g_g.objs.slot[i].type4 == 28) s = &g_g.objs.slot[i];
    free_object_deferred(&g_g, s);                                 /* knocked out: stays marked until a respawn */
    for (int f = 0; f < 3; f++) { update_objects(&g_g); scan_cell_for_marker(&g_g, 10, 11); }
    CHECK_EQ(g_spawns, 2);
    init_player(&g_g);
    scan_cell_for_marker(&g_g, 10, 11);
    CHECK_EQ(g_spawns, 3);
    object_init_table[7] = obj_stub_init;
    drop_game();
}
