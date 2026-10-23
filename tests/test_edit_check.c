#include <string.h>
#include "check.h"
#include "editor/check.h"
#include "game/world.h"

/* A 30 x 13 setting with one start, a 2-cell exit (flag 40 = attribute 13), two cards (flag 41 = 37) and marker flag 3. */
static sk_level g_lv; static uint8_t g_pals[200]; static uint16_t g_cells[30 * 13]; static uint8_t g_states[30 * 13];
static void make_setting(bool with_exit) {
    memset(&g_lv, 0, sizeof g_lv); memset(g_cells, 0, sizeof g_cells); memset(g_pals, 0, sizeof g_pals);
    g_lv.map.width = 30; g_lv.map.height = 13; g_lv.map.cells = g_cells; g_lv.map.states = g_states;
    g_lv.pals = g_pals; g_lv.pals_len = sizeof g_pals;
    g_pals[SK_PALS_ATTR + 40 - 33] = 13; g_pals[SK_PALS_ATTR + 41 - 33] = 37; g_pals[SK_PALS_TYPE + 3] = 58;
    g_cells[9 * 30 + 2] = 0x1FF;
    if (with_exit) { g_cells[5 * 30 + 28] = 40 << SK_FLAG_SHIFT; g_cells[6 * 30 + 28] = 40 << SK_FLAG_SHIFT; }
    g_cells[3 * 30 + 10] = 41 << SK_FLAG_SHIFT; g_cells[3 * 30 + 11] = 41 << SK_FLAG_SHIFT;
}

static bool has(const sk_issue *is, int n, sk_issue_kind k, const char *text) {
    for (int i = 0; i < n; i++) if (is[i].kind == k && strstr(is[i].text, text)) return true;
    return false;
}

void test_check_clean_level(void) {
    make_setting(true);
    sk_doc d; sk_err e = {0}; sk_issue is[8];
    CHECK(doc_new_copy(&d, &g_lv, 4, &e));
    CHECK_EQ(edit_check(&d, &g_lv, is, 8), 0);
    doc_free(&d);
}

void test_check_start_exit_cards_crowding(void) {
    make_setting(true);
    sk_doc d; sk_err e = {0}; sk_issue is[8];
    CHECK(doc_new_copy(&d, &g_lv, 4, &e));
    doc_erase(&d, 2, 9, SK_LAYER_TILE);
    int n = edit_check(&d, &g_lv, is, 8);
    CHECK(has(is, n, SK_ISSUE_NO_START, "no start position")); CHECK(is[0].error);
    doc_place_start(&d, 5, 5); doc_place_start(&d, 9, 5);
    n = edit_check(&d, &g_lv, is, 8);
    CHECK(has(is, n, SK_ISSUE_MANY_STARTS, "2 start positions: the game uses the first (column 5)")); CHECK(!is[0].error);
    doc_undo(&d);
    doc_erase(&d, 28, 5, SK_LAYER_FLAG); doc_erase(&d, 28, 6, SK_LAYER_FLAG);
    n = edit_check(&d, &g_lv, is, 8);
    CHECK(has(is, n, SK_ISSUE_NO_EXIT, "no exit"));
    doc_undo(&d); doc_undo(&d);
    doc_erase(&d, 10, 3, SK_LAYER_FLAG);
    n = edit_check(&d, &g_lv, is, 8);
    CHECK(has(is, n, SK_ISSUE_CARDS, "1 card (the original has 2)"));
    doc_undo(&d);
    for (int i = 0; i < 17; i++) doc_place_flag(&d, 12 + i % 18, 1 + i / 18, 3);
    n = edit_check(&d, &g_lv, is, 8);
    CHECK(has(is, n, SK_ISSUE_CROWDED, "17 objects within one screen at column 12"));
    doc_free(&d);
}

void test_check_skips_the_exit_without_one_in_the_setting(void) {   /* the bonus levels end on their timer */
    make_setting(false);
    sk_doc d; sk_err e = {0}; sk_issue is[8];
    CHECK(doc_new_copy(&d, &g_lv, 36, &e));
    CHECK_EQ(edit_check(&d, &g_lv, is, 8), 0);
    doc_free(&d);
}
