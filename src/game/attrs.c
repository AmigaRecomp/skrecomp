#include <string.h>
#include "game/attrs.h"
#include "game/attr_country_london.h"
#include "game/attr_italy.h"
#include "game/attr_russia.h"
#include "game/attr_japan.h"
#include "game/attr_england.h"
#include "game/game.h"

sk_attr_fn attr_table[95];

#define BPLCON2_TABLE 0x200BCA
static const uint8_t bplcon2_default[4] = { 0x20, 0x21, 0x21, 0x22 };

void attrs_reset(sk_game *g) {
    memset(&g->attrs, 0, sizeof g->attrs);
    attr_england_install();
    attr_country_london_install();
    attr_italy_install();
    attr_russia_install();
    attr_japan_install();
    if (!attr_table[2]) attr_table[2] = attr_checkpoint;
    if (!attr_table[13]) attr_table[13] = attr_exit;
    if (!attr_table[53]) attr_table[53] = attr_layer_switch_a;
    if (!attr_table[54]) attr_table[54] = attr_layer_switch_b;
}

void select_layers(sk_game *g) {                                /* 0x10BDAA */
    int i = g->layer_flags & 3;
    g->bplcon2_value = g->sk ? image_u8(g->sk, BPLCON2_TABLE + (uint32_t)i) : bplcon2_default[i];
}

static void run_cell(sk_game *g, int cx, int cy) {               /* the per-cell body of the probes */
    if (!world_inside(&g->world, cx, cy)) return;
    int attr = world_attr(&g->world, cx, cy);
    if (attr < 0 || attr >= 95 || !attr_table[attr]) return;
    attr_table[attr](g, cx, cy);
}

int probe_tile_attr(sk_game *g, int x, int y) {                 /* 0x11A57E */
    g->attrs.probe_kind = 2;
    g->attrs.probe_x = x; g->attrs.probe_y = y;
    int cx = x >> 4, cy = y >> 4;
    if (!world_inside(&g->world, cx, cy)) return -1;
    int attr = world_attr(&g->world, cx, cy);
    if (attr < 0) return -1;
    run_cell(g, cx, cy);
    return 0;
}

void player_tile_attrs(sk_game *g) {                            /* 0x11A5E2 */
    sk_player *p = &g->player;
    p->carry_dx = 0;
    if (p->state == 64 || p->dying) return;
    flush_tile_restores(g);
    g->attrs.probe_kind = 0;
    int x0 = p->x - 16;
    if (x0 < 0) x0 = 0;
    int cx0 = x0 >> 4;
    if (p->y < 0) {                                              /* above the map: one row at row 0 */
        for (int c = 0; c < 3; c++) run_cell(g, cx0 + c, 0);
        return;
    }
    attrs_run_cells(g, 0, cx0 * 16, p->y & ~15, 3, 4);
}

void attrs_run_cells(sk_game *g, int kind, int x0, int y0, int cols, int rows) {
    g->attrs.probe_kind = kind;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) run_cell(g, (x0 >> 4) + c, (y0 >> 4) + r);
}

static int cell_index(const sk_game *g, int cx, int cy) { return cy * g->world.w + cx; }

static void write_cell(sk_game *g, int cell, uint16_t word, int index, int state) {
    g->world.live[cell] = word;
    g->world.tile[cell] = (uint16_t)(index & SK_INDEX_MASK);     /* the cell long: the tile, marker bit dropped */
    if (state >= 0) g->world.state[cell] = (uint8_t)state;
}

void queue_tile_change(sk_game *g, int cx, int cy, int kind, int fallback) {  /* 0x11A8DC */
    int cell = cell_index(g, cx, cy);
    for (int i = 0; i < 32; i++) {
        sk_tile_change *q = &g->attrs.queue[i];
        if (q->used) continue;
        q->used = true; q->cell = cell; q->kind = kind; q->ticks = 0; q->start = 0;
        return;
    }
    write_cell(g, cell, (uint16_t)fallback, fallback, 0x20);    /* queue full: the fallback tile at once (loc_11AA3E) */
}

static void apply_tile_change(sk_game *g, sk_tile_change *q) {   /* 0x11A900 */
    uint16_t *w = &g->world.live[q->cell];
    g->world.state[q->cell] = 0x20;
    switch (q->kind) {
    case 0:                                                      /* a 6-frame pickup sparkle, then an empty cell */
        if (q->ticks == 0) { *w = 0x1FA; q->ticks = 8; }
        if (--q->ticks == 0) { q->ticks = 8; (*w)++; if (*w == 0x1FF) { q->used = false; *w = 0; } }
        break;
    case 1:
        if (q->ticks == 0) { *w = 0x1EA; q->ticks = 8; }
        if (--q->ticks == 0) { q->ticks = 8; (*w)++; if (*w == 0x1EF) q->used = false; }
        break;
    case 2:                                                      /* hit wobble: four tiles on, then the original back */
        if (q->ticks == 0) { q->start = *w; *w &= SK_INDEX_MASK; q->ticks = 4; }
        if (--q->ticks == 0) { q->ticks = 4; (*w)++; if (*w == ((q->start & SK_INDEX_MASK) + 4)) { q->used = false; *w = q->start; } }
        break;
    default:                                                     /* kind 3: a 4-frame transformation that stays */
        if (q->ticks == 0) { *w &= SK_INDEX_MASK; q->start = *w; q->ticks = 4; }
        if (--q->ticks == 0) { q->ticks = 4; (*w)++; if (*w == q->start + 3) q->used = false; }
        g->mask.dirty = true;
        break;
    }
    g->world.tile[q->cell] = (uint16_t)(*w & SK_INDEX_MASK);
}

void process_tile_queue(sk_game *g) {                           /* 0x11A8C2 */
    for (int i = 0; i < 32; i++) if (g->attrs.queue[i].used) apply_tile_change(g, &g->attrs.queue[i]);
}

void clear_cell(sk_game *g, int cx, int cy) { write_cell(g, cell_index(g, cx, cy), 0, 0, 0x20); }   /* 0x11AAA0 */

void set_cell_tile(sk_game *g, int cx, int cy, int index, int state) {   /* 0x11AA6A */
    int cell = cell_index(g, cx, cy);
    uint16_t word = (uint16_t)((g->world.live[cell] & 0xFE00) | (index & SK_INDEX_MASK));
    write_cell(g, cell, word, index, state ? state : 0x20);
}

void set_tile_at(sk_game *g, int x, int y, int index, int state) {       /* 0x11A3E2 */
    int cx = (x & 0xFFF0) >> 4, cy = (y & 0xFFF0) >> 4;
    if (cx >= g->world.w || cy >= g->world.h) return;                   /* port guard: the original writes anywhere */
    set_cell_tile(g, cx, cy, index, state);
}

void fill_cells(sk_game *g, int x, int y, int index, int state, int n) { /* sub_11A41E */
    int cx = (x & 0xFFF0) >> 4, cy = (y & 0xFFF0) >> 4;
    for (int k = 0; k <= n; k++)
        if (cx < g->world.w && cy + k < g->world.h) set_cell_tile(g, cx, cy + k, index, state);   /* port guard */
}

void attr_checkpoint(sk_game *g, int cx, int cy) {              /* 0x11B250 */
    if (g->attrs.probe_kind != 0) return;
    sk_player *p = &g->player;
    int d = p->x - cx * 16;
    if (d < 0 || d >= 16) return;
    g->world.start_x = cx * 16 + 16;
    g->kit = g->kit_current;
    g->layer_flags_record = g->layer_flags;
    int flag = world_flag(&g->world, cx, cy);
    int bottom = cy;
    while (bottom + 1 < g->world.h && world_flag(&g->world, cx, bottom + 1) == flag) bottom++;
    g->world.start_y = (bottom + 1) * 16 - 40;                  /* d1 has walked past the pole's last cell */
    for (int r = bottom; r >= 0 && world_flag(&g->world, cx, r) == flag; r--)
        g->world.live[cell_index(g, cx, r)] &= 0x01FF;           /* andi.b #1 on the high byte: the pole fires once */
    play_sfx(g, 12);
}

void attr_exit(sk_game *g, int cx, int cy) {                    /* 0x11AAEC */
    (void)cx; (void)cy;
    if (g->attrs.probe_kind != 0 || (g->level_outcome & 1)) return;
    play_sfx(g, 12);
    g->player.state = 60;
}

static void layer_switch(sk_game *g, int cx, int zone, bool set_in_left_half) {
    int d;
    if (g->attrs.probe_kind == 0) {
        d = g->player.x - cx * 16;
        if (d < 0 || d >= 16) return;
        g->layer_zone = (uint8_t)zone;
        if (set_in_left_half) { g->layer_flags &= (uint8_t)~1; if (d < 8) g->layer_flags |= 1; }
        else { g->layer_flags |= 1; if (d < 8) g->layer_flags &= (uint8_t)~1; }
    } else if (g->attrs.probe_kind == 1) {
        d = (int)(g->ball.x >> 16) - cx * 16;
        if (d <= 0 || d >= 16) return;
        if (set_in_left_half) { g->layer_flags &= (uint8_t)~2; if (d < 8) g->layer_flags |= 2; }
        else { g->layer_flags |= 2; if (d < 8) g->layer_flags &= (uint8_t)~2; }
    } else return;
    select_layers(g);
}

void attr_layer_switch_a(sk_game *g, int cx, int cy) { (void)cy; layer_switch(g, cx, 1, true); }    /* 0x11BCC0 */
void attr_layer_switch_b(sk_game *g, int cx, int cy) { (void)cy; layer_switch(g, cx, 2, false); }   /* 0x11BD36 */
