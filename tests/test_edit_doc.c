#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "editor/doc.h"
#include "game/world.h"

/* A 30 x 13 setting: tile 5 with state 2 three times and state 0 once, tile 7 solid (its tail), the start on
   tile 7 at (2, 9), flag 40 = the exit (attribute 13), flag 41 = a card (37), flag 3 = a marker (type 58). */
static sk_level g_lv; static uint8_t g_pals[200], g_tails[8 * SK_TILE_TAIL];
static uint16_t g_cells[30 * 13]; static uint8_t g_states[30 * 13];
static void make_setting(void) {
    memset(&g_lv, 0, sizeof g_lv); memset(g_cells, 0, sizeof g_cells); memset(g_states, 0, sizeof g_states);
    memset(g_tails, 0, sizeof g_tails);
    g_lv.map.width = 30; g_lv.map.height = 13; g_lv.map.cells = g_cells; g_lv.map.states = g_states;
    g_lv.pals = g_pals; g_lv.pals_len = sizeof g_pals;
    g_pals[SK_PALS_ATTR + 40 - 33] = 13; g_pals[SK_PALS_ATTR + 41 - 33] = 37; g_pals[SK_PALS_TYPE + 3] = 58;
    g_lv.tiles_a.count = 8; g_lv.tiles_a.tails = g_tails;
    memset(g_tails + 7 * SK_TILE_TAIL, 0xFF, SK_TILE_TAIL);
    for (int i = 0; i < 3; i++) { g_cells[i] = 5; g_states[i] = 2; }
    g_cells[3] = 5;
    g_cells[9 * 30 + 2] = 0x1FF; g_cells[10 * 30 + 2] = 7;
    g_cells[5 * 30 + 20] = 40 << SK_FLAG_SHIFT; g_cells[6 * 30 + 20] = 41 << SK_FLAG_SHIFT;
}

static uint16_t cell(const sk_doc *d, int x, int y) { return d->lvl.cells[y * d->lvl.w + x]; }
static uint8_t state(const sk_doc *d, int x, int y) { return d->lvl.states[y * d->lvl.w + x]; }

void test_doc_tile_states_and_flags(void) {
    make_setting();
    sk_doc d; sk_err e = {0};
    CHECK(doc_new_copy(&d, &g_lv, 4, &e));
    CHECK_EQ(d.tile_state[5], 2); CHECK_EQ(d.tile_state[6], 0);
    CHECK_EQ(doc_flag_for_attr(&g_lv, 13), 40); CHECK_EQ(doc_flag_for_attr(&g_lv, 37), 41); CHECK_EQ(doc_flag_for_attr(&g_lv, 2), -1);
    CHECK_EQ(doc_floor_tile(&g_lv), 7);
    CHECK(!d.dirty);
    doc_free(&d);
}

void test_doc_paint_flag_erase(void) {
    make_setting();
    sk_doc d; sk_err e = {0};
    CHECK(doc_new_copy(&d, &g_lv, 4, &e));
    doc_place_flag(&d, 10, 4, 41);
    doc_paint(&d, 10, 4, 5);
    CHECK_EQ(cell(&d, 10, 4), 41 << SK_FLAG_SHIFT | 5); CHECK_EQ(state(&d, 10, 4), 2);   /* the flag kept, the state from the table */
    doc_place_flag(&d, 10, 4, 3);
    CHECK_EQ(cell(&d, 10, 4), 3 << SK_FLAG_SHIFT | 5);                                  /* a marker replaces the attribute */
    doc_erase(&d, 10, 4, SK_LAYER_FLAG); CHECK_EQ(cell(&d, 10, 4), 5);
    doc_erase(&d, 10, 4, SK_LAYER_TILE); CHECK_EQ(cell(&d, 10, 4), 0); CHECK_EQ(state(&d, 10, 4), 0);
    doc_place_start(&d, 11, 4); CHECK_EQ(cell(&d, 11, 4) & SK_INDEX_MASK, 0x1FF);
    doc_paint(&d, 99, 99, 5);                                                             /* outside: ignored */
    CHECK(d.dirty);
    doc_free(&d);
}

void test_doc_fill_copy_paste(void) {
    make_setting();
    sk_doc d; sk_err e = {0};
    CHECK(doc_new_copy(&d, &g_lv, 4, &e));
    doc_fill(&d, 8, 6, 4, 4, SK_LAYER_TILE, 5, 0);                                         /* corners in either order */
    CHECK_EQ(cell(&d, 4, 4), 5); CHECK_EQ(cell(&d, 8, 6), 5); CHECK_EQ(cell(&d, 9, 6), 0); CHECK_EQ(state(&d, 6, 5), 2);
    sk_clip c = {0};
    CHECK(doc_copy(&d, 4, 4, 8, 6, &c)); CHECK_EQ(c.w, 5); CHECK_EQ(c.h, 3);
    doc_paste(&d, &c, 27, 11);                                                             /* clipped at the right and bottom */
    CHECK_EQ(cell(&d, 29, 12), 5); CHECK_EQ(state(&d, 29, 12), 2);
    clip_free(&c);
    doc_free(&d);
}

void test_doc_resize(void) {
    make_setting();
    sk_doc d; sk_err e = {0};
    CHECK(doc_new_copy(&d, &g_lv, 4, &e));
    CHECK(doc_resize(&d, 2, 0, 0, 0, &e));
    CHECK_EQ(d.lvl.w, 32); CHECK_EQ(cell(&d, 4, 9) & SK_INDEX_MASK, 0x1FF); CHECK_EQ(cell(&d, 0, 9), 0);
    CHECK(doc_resize(&d, 0, 0, 0, 2, &e)); CHECK_EQ(d.lvl.h, 15);
    CHECK(doc_resize(&d, 0, -1, 0, 0, &e));
    CHECK_EQ(d.lvl.h, 14); CHECK_EQ(cell(&d, 4, 8) & SK_INDEX_MASK, 0x1FF);
    CHECK(!doc_resize(&d, 0, -2, 0, 0, &e)); CHECK_STR(e.msg, "at least 22x13"); CHECK_EQ(d.lvl.h, 14);
    CHECK(!doc_resize(&d, 0, 0, -11, 0, &e)); CHECK_STR(e.msg, "at least 22x13"); CHECK_EQ(d.lvl.w, 32);
    CHECK(!doc_resize(&d, 0, 0, 2000, 0, &e)); CHECK_STR(e.msg, "exceeds 16384 cells"); CHECK_EQ(d.lvl.w, 32);
    doc_free(&d);
}

void test_doc_undo_redo(void) {
    make_setting();
    sk_doc d; sk_err e = {0};
    CHECK(doc_new_copy(&d, &g_lv, 4, &e));
    doc_paint(&d, 1, 1, 5);
    doc_fill(&d, 0, 0, 3, 3, SK_LAYER_TILE, 6, 0);
    CHECK(doc_resize(&d, 1, 0, 0, 0, &e));
    CHECK(doc_undo(&d)); CHECK_EQ(d.lvl.w, 30); CHECK_EQ(cell(&d, 1, 1), 6);
    CHECK(doc_undo(&d)); CHECK_EQ(cell(&d, 1, 1), 5);
    CHECK(doc_redo(&d)); CHECK_EQ(cell(&d, 1, 1), 6);
    doc_paint(&d, 2, 2, 4);                                                                /* drops the redo tail */
    CHECK(!doc_redo(&d));
    CHECK(doc_undo(&d)); CHECK(doc_undo(&d)); CHECK(doc_undo(&d)); CHECK(!doc_undo(&d));
    CHECK_EQ(cell(&d, 1, 1), g_cells[30 + 1]);                                            /* back to the setting's copy */
    for (int i = 0; i < 300; i++) doc_paint(&d, i % 30, 0, i % 8);
    int n = 0; while (doc_undo(&d)) n++;
    CHECK_EQ(n, SK_UNDO_MAX);
    doc_begin_stroke(&d);
    for (int i = 0; i < 10; i++) doc_paint(&d, i, 12, 7);
    doc_end_stroke(&d);
    CHECK(doc_undo(&d)); CHECK_EQ(cell(&d, 5, 12), g_cells[12 * 30 + 5]);
    doc_free(&d);
}

void test_doc_new_empty(void) {
    make_setting();
    sk_doc d; sk_err e = {0};
    CHECK(!doc_new_empty(&d, &g_lv, 4, 21, 13, &e)); CHECK_STR(e.msg, "at least 22x13");
    CHECK(doc_new_empty(&d, &g_lv, 4, 40, 15, &e));
    int starts = 0, exits = 0;
    for (int i = 0; i < 40 * 15; i++) { starts += (d.lvl.cells[i] & SK_INDEX_MASK) == 0x1FF; exits += d.lvl.cells[i] >> SK_FLAG_SHIFT == 40; }
    CHECK_EQ(starts, 1); CHECK(exits >= 3);
    CHECK_EQ(cell(&d, 1, 12) & SK_INDEX_MASK, 0x1FF);
    CHECK_EQ(cell(&d, 10, 13), 7); CHECK_EQ(cell(&d, 10, 14), 7);                          /* the setting's floor */
    CHECK(d.dirty);
    doc_free(&d);
}
