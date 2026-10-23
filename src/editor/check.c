#include <stdio.h>
#include <stdlib.h>
#include "editor/check.h"
#include "game/world.h"

static int attr_of(const sk_level *lv, int flag) {
    if (flag < 33 || SK_PALS_ATTR + (size_t)(flag - 33) >= lv->pals_len) return -1;
    return lv->pals[SK_PALS_ATTR + flag - 33];
}

static bool is_card(int attr) { return attr == 37 || attr == 66; }   /* the card piece, and the one in a solid tile */

static int count_attr(const uint16_t *cells, int n, const sk_level *lv, bool (*want)(int)) {
    int k = 0;
    for (int i = 0; i < n; i++) k += want(attr_of(lv, cells[i] >> SK_FLAG_SHIFT));
    return k;
}

static bool is_exit(int attr) { return attr == 13; }

static void add(sk_issue *out, int max, int *n, sk_issue_kind kind, bool error, int x, int count, const char *text) {
    if (*n >= max) return;
    sk_issue *is = &out[(*n)++];
    is->kind = kind; is->error = error; is->x = x; is->y = 0; is->count = count;
    snprintf(is->text, sizeof is->text, "%s", text);
}

int edit_check(const sk_doc *d, const sk_level *lv, sk_issue *out, int max) {
    const sk_custom *c = &d->lvl;
    int n = 0, cells = c->w * c->h;
    char t[96];
    int starts = 0, first_start = -1;
    for (int i = 0; i < cells; i++)
        if ((c->cells[i] & SK_INDEX_MASK) == SK_START_TILE && starts++ == 0) first_start = i % c->w;
    if (!starts) add(out, max, &n, SK_ISSUE_NO_START, true, 0, 0, "no start position");
    int setting_cells = lv->map.width * lv->map.height;
    if (count_attr(lv->map.cells, setting_cells, lv, is_exit) && !count_attr(c->cells, cells, lv, is_exit))
        add(out, max, &n, SK_ISSUE_NO_EXIT, true, 0, 0, "no exit: the level cannot be finished");
    if (starts > 1) {
        snprintf(t, sizeof t, "%d start positions: the game uses the first (column %d)", starts, first_start);
        add(out, max, &n, SK_ISSUE_MANY_STARTS, false, first_start, starts, t);
    }
    int cards = count_attr(c->cells, cells, lv, is_card), orig = count_attr(lv->map.cells, setting_cells, lv, is_card);
    if (orig && cards != orig) {
        snprintf(t, sizeof t, "%d card%s (the original has %d)", cards, cards == 1 ? "" : "s", orig);
        add(out, max, &n, SK_ISSUE_CARDS, false, 0, cards, t);
    }
    int *col = calloc((size_t)c->w, sizeof *col);                  /* markers per column */
    if (col) {
        for (int i = 0; i < cells; i++) { int f = c->cells[i] >> SK_FLAG_SHIFT; if (f >= 1 && f <= 31) col[i % c->w]++; }
        for (int s = 0; s < c->w; s++) {
            int k = 0, left = -1;
            for (int x = s; x < s + SK_CHECK_SCREEN_COLS && x < c->w; x++) { if (col[x] && left < 0) left = x; k += col[x]; }
            if (k > SK_CHECK_SLOTS) {
                snprintf(t, sizeof t, "%d objects within one screen at column %d: the game has %d slots", k, left, SK_CHECK_SLOTS);
                add(out, max, &n, SK_ISSUE_CROWDED, false, left, k, t);
                break;
            }
        }
        free(col);
    }
    return n;
}
