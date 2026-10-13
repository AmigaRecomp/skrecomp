#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/camera.h"
#include "game/game.h"
#include "game/objects.h"
#include "game/player.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e) || !world_build(&g_g.world, &g_lv)) return false;
    init_player(&g_g);
    objects_reset(&g_g);
    g_g.cam_ease = 4;
    return true;
}
static void drop_game(void) { world_free(&g_g.world); level_free(&g_lv); free(g_d); }
static void frames(int n, uint8_t input) {
    for (int i = 0; i < n; i++) { g_g.frame_ticks++; g_g.input_state = input; g_g.mask.built = false; player_input_state(&g_g); }
}
static int g_scanned_cells, g_last_cx;
static void note_init(sk_game *g, sk_slot *s, int x, int y) { init_object_slot_invisible(g, s, x, y, s->type4); g_scanned_cells++; g_last_cx = x / 16; }

void test_camera_clamps_and_follows(void) {
    CHECK(make_game());
    g_g.view.full_redraw = true;
    update_camera(&g_g);                                        /* start (48, 136): x - 14 + 64 - 144 < 0 */
    CHECK_EQ(g_g.camera_x, 0); CHECK_EQ(g_g.camera_y, 36); CHECK(!g_g.view.full_redraw);
    g_g.player.x = 300; g_g.player.y = 150;
    update_camera(&g_g);
    CHECK_EQ(g_g.camera_x, 300 - 14 + 0x40 - 144); CHECK_EQ(g_g.camera_y, 50);
    g_g.player.x = 20000; update_camera(&g_g);
    CHECK_EQ(g_g.camera_x, g_g.world.fg_max_x);
    g_g.shake_amplitude = 5; update_camera(&g_g);
    CHECK_EQ(g_g.camera_y, g_g.camera_y_raw + 5);
    g_g.player.facing = -1;                                     /* the look-ahead eases towards -64 by a tenth per frame */
    update_camera(&g_g); CHECK_EQ(g_g.look_x, 64 - 12);
    drop_game();
}

void test_push_x_forces_the_camera(void) {
    CHECK(make_game());
    g_g.view.full_redraw = true; g_g.player.x = 94; update_camera(&g_g);
    CHECK_EQ(g_g.camera_x, 0);
    g_g.push_x = 200;
    update_camera(&g_g); CHECK_EQ(g_g.camera_x, 4);              /* the pusher drags the camera 4 px per frame */
    update_camera(&g_g); CHECK_EQ(g_g.camera_x, 8);
    drop_game();
}

void test_crossing_a_column_scans_the_new_column_and_three_beyond(void) {
    CHECK(make_game());
    object_init_table[7] = note_init;
    for (int cy = 0; cy < 16; cy++) { g_g.world.live[cy * 40 + 23] = (uint16_t)(5 << 9); g_g.world.tile[cy * 40 + 23] |= SK_MARKER_BIT; }  /* column 23: a marker in every row */
    g_g.view.full_redraw = true; g_g.player.x = 94; update_camera(&g_g);     /* camera 0: columns 0..20, rows 2..15 scanned */
    CHECK_EQ(g_g.camera_x, 0); CHECK_EQ(g_scanned_cells, 1); CHECK_EQ(g_last_cx, 10);   /* the fixture's own marker (10, 11) */
    for (int i = 0; i < 3; i++) edge_scan_columns(&g_g);        /* beyond a leftward strip: columns -3..-1, outside */
    CHECK_EQ(g_scanned_cells, 1);
    g_g.player.x = 110; update_camera(&g_g); CHECK_EQ(g_g.camera_x, 16);   /* the new buffer column is 21 */
    CHECK_EQ(g_scanned_cells, 1);
    edge_scan_columns(&g_g); CHECK_EQ(g_scanned_cells, 1);       /* column 22 */
    edge_scan_columns(&g_g); CHECK_EQ(g_scanned_cells, 16); CHECK_EQ(g_last_cx, 23);   /* column 23: 15 more until the 16 slots are full */
    CHECK_EQ(g_g.object_count, 16);
    edge_scan_columns(&g_g); CHECK_EQ(g_scanned_cells, 16);      /* column 24 */
    edge_scan_columns(&g_g); CHECK_EQ(g_scanned_cells, 16);      /* the three passes are spent */
    drop_game();
}

void test_camera_scans_stay_inside_a_tiny_level(void) {           /* Review Focus 5 */
    uint8_t *d = NULL; size_t n = 0; sk_level lv = {0}; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g);
    CHECK(load_fixture("level_tiny.bin", &d, &n)); CHECK(level_parse(d, n, &lv, &e)); CHECK(world_build(&g_g.world, &lv));
    init_player(&g_g); objects_reset(&g_g);
    g_g.view.full_redraw = true; update_camera(&g_g);
    for (int i = 0; i < 5; i++) { edge_scan_columns(&g_g); edge_scan_rows(&g_g); }
    g_g.player.y = 500; g_g.player.x = 900; update_camera(&g_g);
    for (int i = 0; i < 5; i++) { edge_scan_columns(&g_g); edge_scan_rows(&g_g); }
    CHECK_EQ(g_g.camera_x, 0); CHECK_EQ(g_g.camera_y, 0); CHECK_EQ(g_g.object_count, 0);
    world_free(&g_g.world); level_free(&lv); free(d);
}

void test_camera_holds_its_height_during_a_jump(void) {
    CHECK(make_game());
    g_g.view.full_redraw = true;
    frames(20, 0); update_camera(&g_g);                         /* standing at y 151: the camera target is 51, below the clamp (56) */
    for (int i = 0; i < 30; i++) update_camera(&g_g);
    CHECK_EQ(g_g.camera_y_raw, 51);
    for (int i = 0; i < 12; i++) {                              /* the rise: camera_target returns jump_y while jumping */
        frames(1, SK_IN_UP); update_camera(&g_g);
        CHECK_EQ(g_g.camera_y_raw, 51);
    }
    CHECK(g_g.player.y < 151 - 30);
    drop_game();
}
