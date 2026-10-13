#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/objects.h"
#include "game/player.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e) || !world_build(&g_g.world, &g_lv)) return false;
    objects_reset(&g_g); attrs_reset(&g_g);
    init_player(&g_g);
    return true;
}
static void drop_game(void) { world_free(&g_g.world); level_free(&g_lv); free(g_d); }
static void frames(int n, uint8_t input) {
    for (int i = 0; i < n; i++) { g_g.frame_ticks++; g_g.input_state = input; g_g.mask.built = false; player_input_state(&g_g); }
}

void test_exit_attribute_starts_the_exit_walk(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.x = 38 * 16 + 4; g_g.player.y = 12 * 16 - 41;     /* on the exit cell (38, 11) */
    player_tile_attrs(&g_g);
    CHECK_EQ(g_g.player.state, 60); CHECK_EQ(g_g.sfx_last, 12);
    g_g.player.state = 0; g_g.level_outcome = 1;                 /* already ending: ignored */
    player_tile_attrs(&g_g); CHECK_EQ(g_g.player.state, 0);
    drop_game();
}

void test_checkpoint_moves_the_respawn_and_fires_once(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.x = 16 * 16 + 2; g_g.player.y = 12 * 16 - 41;
    g_g.layer_flags = 1;
    player_tile_attrs(&g_g);
    CHECK_EQ(g_g.world.start_x, 16 * 16 + 16); CHECK_EQ(g_g.world.start_y, 11 * 16 - 24);   /* feet on the row below the pole */
    CHECK_EQ(world_flag(&g_g.world, 16, 11) & 0x7E, 0);          /* marker bits stripped (andi.b #1) */
    CHECK_EQ(g_g.sfx_last, 12); CHECK_EQ(g_g.layer_flags_record, 1);   /* the respawn keeps the layers */
    g_g.sfx_last = 0; player_tile_attrs(&g_g); CHECK_EQ(g_g.sfx_last, 0);
    drop_game();
}

void test_layer_switch_changes_the_priority_word(void) {
    CHECK(make_game());
    frames(20, 0);
    CHECK_EQ(g_g.bplcon2_value, 0x20);
    g_g.player.x = 24 * 16 + 2; g_g.player.y = 12 * 16 - 41;     /* left half of cell 24: bit 0 set */
    player_tile_attrs(&g_g);
    CHECK_EQ(g_g.layer_flags & 1, 1); CHECK_EQ(g_g.bplcon2_value, 0x21); CHECK_EQ(g_g.layer_zone, 1);
    g_g.player.x = 24 * 16 + 12; player_tile_attrs(&g_g);        /* right half: cleared */
    CHECK_EQ(g_g.layer_flags & 1, 0); CHECK_EQ(g_g.bplcon2_value, 0x20);
    g_g.layer_flags = 3; select_layers(&g_g); CHECK_EQ(g_g.bplcon2_value, 0x22);
    drop_game();
}

void test_tile_queue_kinds_step_as_the_table_says(void) {
    CHECK(make_game());
    queue_tile_change(&g_g, 5, 12, 0);                           /* kind 0: 0x1FA.. then empty */
    process_tile_queue(&g_g);
    CHECK_EQ(world_tile_index(&g_g.world, 5, 12), 0x1FA); CHECK_EQ(g_g.world.state[12 * 40 + 5], 0x20);
    for (int i = 0; i < 8; i++) process_tile_queue(&g_g);
    CHECK_EQ(world_tile_index(&g_g.world, 5, 12), 0x1FB);
    for (int i = 0; i < 32; i++) process_tile_queue(&g_g);
    CHECK_EQ(g_g.world.live[12 * 40 + 5], 0); CHECK_EQ(world_tile_index(&g_g.world, 5, 12), 0); CHECK(!g_g.attrs.queue[0].used);
    queue_tile_change(&g_g, 6, 12, 2);                           /* kind 2: wobble four steps, back to the start tile */
    process_tile_queue(&g_g); CHECK_EQ(world_tile_index(&g_g.world, 6, 12), 1);
    for (int i = 0; i < 4; i++) process_tile_queue(&g_g); CHECK_EQ(world_tile_index(&g_g.world, 6, 12), 2);
    for (int i = 0; i < 16; i++) process_tile_queue(&g_g);
    CHECK_EQ(world_tile_index(&g_g.world, 6, 12), 1); CHECK(!g_g.attrs.queue[0].used);
    queue_tile_change(&g_g, 7, 12, 3);                           /* kind 3: three steps, stays, dirties the mask */
    g_g.mask.dirty = false;
    for (int i = 0; i < 13; i++) process_tile_queue(&g_g);
    CHECK_EQ(world_tile_index(&g_g.world, 7, 12), 4); CHECK(!g_g.attrs.queue[0].used); CHECK(g_g.mask.dirty);
    clear_cell(&g_g, 0, 12); CHECK_EQ(g_g.world.live[12 * 40], 0); CHECK_EQ(g_g.world.state[12 * 40], 0x20);
    set_cell_tile(&g_g, 1, 12, 2, 0); CHECK_EQ(world_tile_index(&g_g.world, 1, 12), 2); CHECK_EQ(g_g.world.state[12 * 40 + 1], 0x20);
    drop_game();
}
