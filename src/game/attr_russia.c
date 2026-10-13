/* Russia's tile attributes, transcribed from the listing. */
#include "game/attr_russia.h"
#include "game/attr_england.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/player.h"

static int16_t bx(const sk_game *g) { return (int16_t)(g->ball.x >> 16); }
static int16_t by(const sk_game *g) { return (int16_t)(g->ball.y >> 16); }
static bool in_open(int v, int lo, int hi) { return v > lo && v < hi; }   /* the cmpi/ble/bge pairs */
static bool in_half(int v, int lo, int hi) { return v >= lo && v < hi; }  /* cmpi/blt, cmpi/bge */

static void attr_tilt_plate(sk_game *g, int cx, int cy) {             /* 0x11B480: attribute 24 */
    if (g->attrs.probe_kind != 0) return;
    const sk_player *p = &g->player;
    int x = cx * 16, y = cy * 16, d2 = (int16_t)(p->x - x);
    if (!in_half(d2, -5, 27) || !in_half((int16_t)(p->y - y), -41, -31)) return;
    g->mask.dirty = true;
    if (g->flush_count < 16) g->flush_list[g->flush_count++] = cy * g->world.w + cx;   /* the restore list (port guard: 16) */
    set_cell_tile(g, cx, cy, d2 < -1 ? 0x1E8 : d2 > 14 ? 0x1E7 : 0x1E9, 0);
}

static void attr_spring(sk_game *g, int cx, int cy) {                 /* 0x11B81C: attribute 40 */
    if (g->attrs.probe_kind != 0) return;
    sk_player *p = &g->player;
    int x = cx * 16, y = cy * 16;
    if (!((int16_t)(p->prev_y - y) < -33)) return;                  /* coming down from above it */
    if (!in_open((int16_t)(p->x - x), 0, 16) || !in_half((int16_t)(p->y - y), -41, -33)) return;
    play_sfx(g, 0x12);
    p->vy = 26;
    p->jump_hold = 16;
    p->jump_y = p->y;
    p->jumping = 0xFF;
    p->falling = 0;
    p->landing = 0xFF;
    queue_tile_change(g, cx, cy, 2, 0);
}

static void attr_side_drain(sk_game *g, int cx, int cy) {             /* 0x11B952: attribute 41 */
    int x = cx * 16, y = cy * 16;
    if (g->attrs.probe_kind == 1) {
        int from = (int16_t)(g->ball.prev_x - x);
        if (!(from >= 16 || from <= -16)) return;                   /* entered from a side */
        if (in_open((int16_t)(bx(g) - x), -16, 16) && in_open((int16_t)(by(g) - y), -18, 18)) swallow_ball(g);
    } else if (g->attrs.probe_kind == 0) {
        const sk_player *p = &g->player;
        int from = (int16_t)(p->prev_x - x);
        if (!(from >= 18 || from <= -8)) return;
        if (in_open((int16_t)(p->x - x), -8, 18) && in_open((int16_t)(p->y - y), -40, 16)) hurt_player(g);
    }
}

static void attr_object_mark(sk_game *g, int cx, int cy) {            /* 0x11BCA4: attribute 52 */
    (void)cx; (void)cy;
    if (g->attrs.probe_kind != 2 || !g->attrs.probe_slot) return;
    g->attrs.probe_kind = 0xFF;
    g->attrs.probe_slot->flags |= 0x10;
}

static void attr_fall_trigger(sk_game *g, int cx, int cy) {           /* 0x11BEC2: attribute 60 */
    if (g->attrs.probe_kind != 0) return;
    const sk_player *p = &g->player;
    int x = cx * 16, y = cy * 16;
    if (!(p->x - x >= 0 && p->x - x <= 16) || !in_half((int16_t)(p->y - y), -54, -40) || p->vy >= 0) return;
    queue_tile_change(g, cx, cy, 3, 0);
    spawn_object(g, x + 2, y + 24, 104);
}

static void attr_deadly_drain(sk_game *g, int cx, int cy) {           /* 0x11BA50: attribute 61 */
    int x = cx * 16, y = cy * 16;
    if (g->attrs.probe_kind == 1) {
        if (in_open((int16_t)(bx(g) - x), -16, 16) && in_open((int16_t)(by(g) - y), -16, 16)) swallow_ball(g);
    } else if (g->attrs.probe_kind == 0) {
        const sk_player *p = &g->player;
        if (in_open((int16_t)(p->x - x), -8, 18) && in_open((int16_t)(p->y - y), -40, 16)) kill_player(g);
    }
}

static void attr_flutter_101(sk_game *g, int cx, int cy) {            /* 0x11C152: attribute 65, attribute 62 with type 101 */
    int x = cx * 16, y = cy * 16;
    const sk_player *p = &g->player;
    if (g->attrs.probe_kind == 1) { if (ball_hits_cell(g, x, y)) flutter_spawn(g, x, y, 101); return; }
    if (g->attrs.probe_kind != 0) return;
    int dx = (int16_t)(p->x - x), dy = (int16_t)(p->y - y);
    if (dx < 0 || dx > 16 || dy < -41 || dy > -30 || (uint16_t)p->speed < 4) return;
    flutter_spawn(g, x, y, 101);
}

static void attr_object_dig(sk_game *g, int cx, int cy) {             /* 0x11C518: attribute 77 */
    (void)cx; (void)cy;
    if (g->attrs.probe_kind != 2 || !g->attrs.probe_slot) return;
    g->attrs.probe_kind = 0xFF;
    sk_slot *s = g->attrs.probe_slot;
    s->aux = (int16_t)(5 << 8 | (s->aux & 0xFF));
}

void attr_russia_install(void) {
    attr_table[24] = attr_tilt_plate;
    attr_table[40] = attr_spring;
    attr_table[41] = attr_side_drain;
    attr_table[52] = attr_object_mark;
    attr_table[60] = attr_fall_trigger;
    attr_table[61] = attr_deadly_drain;
    attr_table[65] = attr_flutter_101;
    attr_table[77] = attr_object_dig;
}
