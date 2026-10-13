#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/collision.h"
#include "game/world.h"

static uint8_t *g_d; static sk_level g_lv; static sk_world g_w;
static bool setup(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_lv, 0, sizeof g_lv); memset(&g_w, 0, sizeof g_w);
    return load_fixture("level_play.bin", &g_d, &n) && level_parse(g_d, n, &g_lv, &e) && world_build(&g_w, &g_lv);
}
static void teardown(void) { world_free(&g_w); level_free(&g_lv); free(g_d); }

void test_mask_rebuild_copies_tile_bitmasks(void) {
    CHECK(setup());
    sk_mask m = {0};
    mask_rebuild(&m, &g_w, 3 * 16 + 8, 10 * 16);              /* the kid near the start */
    CHECK_EQ(m.origin_x, 2 * 16); CHECK_EQ(m.origin_y, 9 * 16);
    CHECK(!mask_solid(&m, 3 * 16 + 8, 11 * 16 + 15));          /* empty above the floor */
    CHECK(mask_solid(&m, 3 * 16 + 8, 12 * 16));                /* floor row */
    CHECK(mask_solid(&m, 2 * 16, 14 * 16 + 15));               /* bottom row of the window (origin_y + 95) */
    CHECK(!mask_solid(&m, 3 * 16 + 8, 8 * 16));                /* above the window: outside */
    /* half tile: rows 8..15 of cell (14, 11) are solid */
    m.built = false;
    mask_rebuild(&m, &g_w, 14 * 16 + 4, 10 * 16);
    CHECK(!mask_solid(&m, 14 * 16 + 4, 11 * 16 + 7));
    CHECK(mask_solid(&m, 14 * 16 + 4, 11 * 16 + 8));
    teardown();
}

void test_mask_rebuild_clamps_to_the_map_edges(void) {        /* Review Focus 2 */
    CHECK(setup());
    sk_mask m = {0};
    mask_rebuild(&m, &g_w, 4, 0);                               /* left edge, above the map */
    CHECK_EQ(m.origin_x, 0); CHECK_EQ(m.origin_y, -16);
    CHECK(mask_solid(&m, 10, -5));                              /* rows above the map are solid */
    CHECK(!mask_solid(&m, 10, 5));
    CHECK(!mask_solid(&m, -1, 5));                              /* left of the window: false, no crash */
    m.built = false;
    mask_rebuild(&m, &g_w, 40 * 16 - 4, 15 * 16);               /* right/bottom corner */
    CHECK(mask_solid(&m, 40 * 16 - 4, 15 * 16 + 5));
    CHECK(!mask_solid(&m, 40 * 16 + 20, 15 * 16 + 5));          /* beyond the map: empty */
    CHECK(!mask_solid(&m, 40 * 16 - 4, 16 * 16 + 40));          /* below the map: empty (ruling) */
    teardown();
}

void test_mask_rebuild_is_lazy(void) {
    CHECK(setup());
    sk_mask m = {0};
    mask_rebuild(&m, &g_w, 100, 100);
    m.built = false;
    m.bits[0] = 0xAA;                                           /* poison */
    mask_rebuild(&m, &g_w, 101, 101);                           /* same origin, not dirty: untouched */
    CHECK_EQ(m.bits[0], 0xAA);
    m.built = false; m.dirty = true;
    mask_rebuild(&m, &g_w, 101, 101);
    CHECK(m.bits[0] != 0xAA);
    m.bits[0] = 0xAA;                                           /* already built this frame: untouched even when dirty */
    m.dirty = true;
    mask_rebuild(&m, &g_w, 300, 100);
    CHECK_EQ(m.bits[0], 0xAA);
    teardown();
}

void test_probes_on_fixture_geometry(void) {
    CHECK(setup());
    sk_mask m = {0};
    int fx = 3 * 16 + 8, fy = 12 * 16 - 41;                     /* resting: row y + 41 is the floor's top row */
    mask_rebuild(&m, &g_w, fx, fy);
    int gap = -1, push = -1;
    probe_floor(&m, fx, fy, &gap, &push);
    CHECK_EQ(gap, 0); CHECK_EQ(push, 0);
    probe_floor(&m, fx, fy - 6, &gap, &push); CHECK_EQ(gap, 6);
    probe_floor(&m, fx, fy + 3, &gap, &push); CHECK_EQ(push, 3);
    CHECK_EQ(probe_ground(&m, fx, fy), 16);                     /* 16 solid rows below the feet */
    CHECK_EQ(probe_slope(&m, fx, fy, 1), 16);                   /* flat */
    m.built = false; mask_rebuild(&m, &g_w, 20 * 16 - 7, fy);
    int16_t px = 20 * 16 - 7, speed = 16;                       /* probe column x + 8 = 321: inside wall column 20 */
    CHECK_EQ(probe_wall(&m, px + 8, fy, 0, 1, false, &px, &speed), -1);
    CHECK_EQ(speed, 0);
    CHECK_EQ(px, 20 * 16 - 9);                                  /* pushed left until the probe column is free */
    px = 20 * 16 - 7; speed = 16;
    CHECK_EQ(probe_wall(&m, px - 8, fy, -1, 1, false, &px, &speed), 0);   /* nothing on the left */
    CHECK_EQ(speed, 16);
    CHECK_EQ(probe_ceiling(&m, px, fy, 1, false), 0);           /* open air above */
    m.built = false; mask_rebuild(&m, &g_w, 6 * 16 + 8, 7 * 16 + 8);         /* head inside the ceiling block (6, 7) */
    CHECK_EQ(probe_ceiling(&m, 6 * 16 + 8, 7 * 16 + 8, 1, false) & SK_CEIL_BUMP, SK_CEIL_BUMP);
    CHECK_EQ(probe_ceiling(&m, 6 * 16 + 8, 7 * 16 + 20, 1, false) & SK_CEIL_BUMP, 0);   /* band leaves the block */
    m.built = false; mask_rebuild(&m, &g_w, 26 * 16 + 2, fy);
    CHECK(!probe_edge(&m, 26 * 16 + 2, fy, 1));                 /* the pit starts at column 26: nothing ahead */
    CHECK(probe_edge(&m, 26 * 16 + 2, fy, -1));                 /* floor behind */
    teardown();
}
