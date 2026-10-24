#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "app/scene.h"
#include "editor/doc.h"
#include "editor/pieces.h"

/* A 30 x 13 map: tiles 1, 2, 3 are the fillers (sky, wall, ground); the rest are placed on top of them. */
static uint16_t g_cells[30 * 13];
static sk_tilemap g_map = { 30, 13, g_cells, NULL };
static void put(int x, int y, int t) { g_cells[y * 30 + x] = (uint16_t)t; }
static void make_map(void) {
    for (int i = 0; i < 30 * 13; i++) g_cells[i] = (uint16_t)(i / 30 < 9 ? 1 : i / 30 < 11 ? 2 : 3);
    for (int i = 0; i < 2; i++) {
        int x = 2 + i * 6;
        put(x, 5, 10); put(x + 1, 5, 11); put(x, 6, 12); put(x + 1, 6, 13);        /* a 2 x 2 car, twice */
        put(x, 1, 20); put(x, 2, 21); put(x + 1, 2, 22);                            /* an L of three cells, twice */
        put(x, 8, 50); put(x + 1, 8, 51);                                            /* two cells, twice */
        for (int j = 0; j < 4; j++) put(20 + j, 1 + i * 2, 80);                      /* a run of one tile, twice */
        put(20, 6 + i, 0x1FF | 3 << 9); put(21, 6 + i, 60);                          /* the start (flagged) beside 60 */
    }
    put(16, 5, 30); put(17, 5, 31);                                                  /* once: not a piece */
    put(20, 9, 40); put(21, 9, 41); put(24, 9, 40); put(25, 9, 42);                  /* 40's neighbour varies */
}

/* Growing: a 2-wide crown (93 94) on a trunk of 92s, twice, a one-off crown (95) on 92 once, and 97 scattered
   7 times, once beside a crown: the trunk joins its crowns, 97 does not. */
static void make_trees(void) {
    make_map();
    for (int i = 0; i < 2; i++) {
        int x = 14 + i * 5;
        put(x, 4, 93); put(x + 1, 4, 94); put(x, 5, 92); put(x, 6, 92 + i * 4);      /* trunk 92 92, then 92 96 */
    }
    put(26, 4, 95); put(26, 5, 92);
    put(15, 5, 97);
    for (int j = 0; j < 6; j++) put(j * 4 + 1, 10, 97);
}

static const sk_piece *find(const sk_pieces *p, int corner) {
    for (int i = 0; i < p->n; i++) if (p->v[i].tiles[0] == corner) return &p->v[i];
    return NULL;
}

void test_edit_pieces_found_from_the_map(void) {
    make_map();
    sk_pieces p;
    CHECK(edit_find_pieces(&g_map, 3, &p));
    CHECK_EQ(p.n, 2);
    const sk_piece *car = find(&p, 10), *l = find(&p, 20);
    CHECK(car && l);
    if (car) {
        CHECK_EQ(car->w, 2); CHECK_EQ(car->h, 2); CHECK_EQ(car->cells, 4); CHECK_EQ(car->uses, 2);
        CHECK_EQ(car->tiles[1], 11); CHECK_EQ(car->tiles[2], 12); CHECK_EQ(car->tiles[3], 13);
        CHECK(car == &p.v[0]);                                                        /* largest first */
    }
    if (l) { CHECK_EQ(l->w, 2); CHECK_EQ(l->h, 2); CHECK_EQ(l->cells, 3); CHECK_EQ(l->tiles[1], -1); CHECK_EQ(l->tiles[3], 22); }
    edit_pieces_free(&p);
    CHECK_EQ(p.n, 0);
}

void test_edit_pieces_small_ones_on_request(void) {
    make_map();
    sk_pieces p;
    CHECK(edit_find_pieces(&g_map, 2, &p));
    CHECK_EQ(p.n, 3);                                     /* 50 51 as well; never 30 31, 40 41, the run of 80 or the start */
    CHECK(find(&p, 50) != NULL);
    CHECK(find(&p, 30) == NULL); CHECK(find(&p, 40) == NULL); CHECK(find(&p, 80) == NULL); CHECK(find(&p, 0x1FF) == NULL);
    edit_pieces_free(&p);
}

void test_edit_pieces_never_join_the_fillers(void) {
    make_map();
    for (int x = 0; x < 30; x++) put(x, 12, x % 2 ? 3 : 2);                       /* 2 and 3 alternate: always each other */
    sk_pieces p;
    CHECK(edit_find_pieces(&g_map, 2, &p));
    for (int i = 0; i < p.n; i++) CHECK(p.v[i].tiles[0] != 2 && p.v[i].tiles[0] != 3);
    edit_pieces_free(&p);
}

void test_doc_stamp_a_piece(void) {
    make_map();
    sk_level lv; memset(&lv, 0, sizeof lv); lv.map = g_map;
    sk_doc d; sk_err e = {0};
    CHECK(doc_new_empty(&d, &lv, 4, 30, 13, &e));
    doc_place_flag(&d, 6, 4, 3);
    const int16_t l[4] = { 20, -1, 21, 22 };
    uint16_t before = d.lvl.cells[3 * 30 + 6];
    doc_stamp(&d, 5, 3, 2, 2, l);
    CHECK_EQ(d.lvl.cells[3 * 30 + 5], 20); CHECK_EQ(d.lvl.cells[3 * 30 + 6], before);   /* the hole left alone */
    CHECK_EQ(d.lvl.cells[4 * 30 + 5], 21); CHECK_EQ(d.lvl.cells[4 * 30 + 6], 3 << 9 | 22);   /* the marker kept */
    uint16_t corner = d.lvl.cells[12 * 30 + 29];
    doc_stamp(&d, 29, 12, 2, 2, l);                                                        /* clipped at the edge */
    CHECK_EQ(d.lvl.cells[12 * 30 + 29] & 0x1FF, 20);
    doc_stamp(&d, 40, 40, 2, 2, l);                                                        /* outside: no step */
    CHECK(doc_undo(&d)); CHECK_EQ(d.lvl.cells[12 * 30 + 29], corner);
    CHECK(doc_undo(&d)); CHECK_EQ(d.lvl.cells[3 * 30 + 5], 0); CHECK_EQ(d.lvl.cells[4 * 30 + 6], 3 << 9);   /* one step each */
    doc_free(&d);
}

void test_edit_pieces_of_london1(void) {                  /* ROM-backed: the same pieces as the survey in re/ */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args base = { .roms = roms, .level = 4, .scale = 1, .section = -1, .near_type = -1, .cup = -1 };
    static sk_scene s; sk_err e = {0};
    CHECK(scene_open(&s, &base, &e));
    sk_pieces p;
    CHECK(edit_find_pieces(&s.level.map, 3, &p));
    CHECK_EQ(p.n, 37);
    if (p.n) { CHECK_EQ(p.v[0].w, 19); CHECK_EQ(p.v[0].h, 13); }                         /* the bus */
    bool tree = false, taxi = false;
    for (int i = 0; i < p.n; i++) {
        bool crown = false, trunk = false;
        for (int j = 0; j < p.v[i].w * p.v[i].h; j++) { crown |= p.v[i].tiles[j] == 0x199; trunk |= p.v[i].tiles[j] == 0x184; }
        tree |= crown && trunk;
        taxi |= p.v[i].w == 9 && p.v[i].h == 4 && p.v[i].uses == 4;
    }
    CHECK(tree); CHECK(taxi);                                                               /* the tree with its trunk */
    edit_pieces_free(&p);
    scene_close(&s);
}

void test_edit_pieces_grow_the_trunk(void) {
    make_trees();
    sk_pieces p;
    CHECK(edit_find_pieces(&g_map, 3, &p));
    const sk_piece *a = NULL, *b = NULL;
    for (int i = 0; i < p.n; i++) if (p.v[i].tiles[0] == 93) { if (!a) a = &p.v[i]; else b = &p.v[i]; }
    CHECK(a && b);
    if (a && b) {
        const sk_piece *two = a->h == 3 ? a : b, *one = a->h == 3 ? b : a;                 /* 96 is used once: not grown */
        CHECK_EQ(two->w, 2); CHECK_EQ(two->h, 3); CHECK_EQ(two->tiles[2], 92); CHECK_EQ(two->tiles[4], 92);
        CHECK_EQ(two->tiles[3], -1); CHECK_EQ(two->tiles[5], -1);                         /* 97 stays out */
        CHECK_EQ(one->h, 2); CHECK_EQ(one->tiles[2], 92); CHECK_EQ(one->tiles[3], -1);
    }
    for (int i = 0; i < p.n; i++) CHECK(p.v[i].tiles[0] != 97 && p.v[i].tiles[0] != 95);
    edit_pieces_free(&p);
}
