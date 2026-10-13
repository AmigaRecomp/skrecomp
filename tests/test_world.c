#include <stdlib.h>
#include "check.h"
#include "fixture.h"
#include "game/world.h"

static bool load_play(sk_level *lv, uint8_t **d) {
    size_t n = 0; sk_err e = {0};
    if (!load_fixture("level_play.bin", d, &n)) return false;
    return level_parse(*d, n, lv, &e);
}

void test_world_builds_cells_and_start(void) {
    uint8_t *d = NULL; sk_level lv = {0}; sk_world w = {0};
    CHECK(load_play(&lv, &d));
    CHECK(world_build(&w, &lv));
    CHECK_EQ(w.w, 40); CHECK_EQ(w.h, 16);
    CHECK_EQ(w.start_x, 3 * 16); CHECK_EQ(w.start_y, 10 * 16 - 24);
    CHECK_EQ(world_tile_index(&w, 0, 12), 1);
    CHECK_EQ(world_tile_index(&w, 3, 10), 0x1FF);
    CHECK_EQ(world_flag(&w, 10, 11), 5);
    CHECK(w.tile[11 * 40 + 10] & SK_MARKER_BIT);           /* flags 1..31 are markers */
    CHECK(!(w.tile[11 * 40 + 16] & SK_MARKER_BIT));        /* flag 34 is an attribute */
    int param = -1;
    CHECK_EQ(world_marker_type(&w, 10, 11, &param), 7); CHECK_EQ(param, 0x55);
    CHECK_EQ(world_marker_type(&w, 16, 11, &param), -1);
    CHECK_EQ(world_attr(&w, 16, 11), 2); CHECK_EQ(world_attr(&w, 38, 11), 13); CHECK_EQ(world_attr(&w, 0, 0), -1);
    CHECK_EQ(world_tail(&w, 0, 12)[0], 0xFF); CHECK_EQ(world_tail(&w, 14, 11)[0], 0x00); CHECK_EQ(world_tail(&w, 14, 11)[16], 0xFF);
    CHECK_EQ(w.fg_max_x, 40 * 16 - 352); CHECK_EQ(w.fg_max_y, 16 * 16 - 200);
    CHECK_EQ(w.bg_max_x, 30 * 16 - 320); CHECK_EQ(w.bg_max_y, 14 * 16 - 200 < 0 ? 0 : 14 * 16 - 200);
    CHECK_EQ(w.state[0], 0);
    world_free(&w); level_free(&lv); free(d);
}

void test_world_without_start_tile_defaults(void) {
    uint8_t *d = NULL; size_t n = 0; sk_level lv = {0}; sk_world w = {0}; sk_err e = {0};
    CHECK(load_fixture("level_tiny.bin", &d, &n));
    CHECK(level_parse(d, n, &lv, &e));
    CHECK(world_build(&w, &lv));
    CHECK_EQ(w.start_x, 16); CHECK_EQ(w.start_y, 16 - 24);
    CHECK_EQ(w.fg_max_x, 0); CHECK_EQ(w.fg_max_y, 0);
    world_free(&w); level_free(&lv); free(d);
}
