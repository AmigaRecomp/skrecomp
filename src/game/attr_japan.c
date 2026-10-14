/* Japan's tile attributes, transcribed from the listing (objects.md "Japan attributes"). */
#include "game/attr_japan.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/platforms.h"
#include "game/player.h"

static int16_t bx(const sk_game *g) { return (int16_t)(g->ball.x >> 16); }
static int16_t by(const sk_game *g) { return (int16_t)(g->ball.y >> 16); }
static bool in_open(int v, int lo, int hi) { return v > lo && v < hi; }   /* the cmpi/ble/bge pairs */

/* ---- attributes 11 and 12: the kit's dark shades by height (set_kit_colours / set_kit_extra_colours) ---- */
static void kit_by_y(sk_game *g, int cy, bool rev) {
    if (g->attrs.probe_kind > 1) return;
    bool ball = g->attrs.probe_kind == 1;
    int v = (int16_t)((ball ? g->ball.prev_y : g->player.y) - cy * 16);
    if (v < 0 || v >= 16) return;
    int d2 = rev ? 28 - (v & 12) : (v & 12) + 16;
    if (ball) g->kit_extra = d2 >> 2;
    else g->kit_current = d2 >> 2;
}
static void attr_shade_y_dark(sk_game *g, int cx, int cy) { (void)cx; kit_by_y(g, cy, false); }       /* 0x11AF96 */
static void attr_shade_y_dark_rev(sk_game *g, int cx, int cy) { (void)cx; kit_by_y(g, cy, true); }    /* 0x11AFD0 */

static bool kid_in_cell(const sk_game *g, int cx, int cy, int ylo, int yhi) {   /* x in (-8, 18), y in (ylo, yhi) */
    const sk_player *p = &g->player;
    return in_open((int16_t)(p->x - cx * 16), -8, 18) && in_open((int16_t)(p->y - cy * 16), ylo, yhi);
}

/* ---- attribute 18: holds the kick animation (anim_hold, read by sub_10B75A / sub_10B794) ---- */
static void attr_anim_hold(sk_game *g, int cx, int cy) {             /* 0x11B21A */
    if (g->attrs.probe_kind != 0) return;
    if (kid_in_cell(g, cx, cy, -40, 16)) g->player.anim_hold = 0xFF;
}

/* ---- attribute 21: a moving platform turns a quarter (+0x40 or -0x40 by its mode) ---- */
static void attr_platform_quarter(sk_game *g, int cx, int cy) {      /* 0x11B376 */
    (void)cx; (void)cy;
    if (g->attrs.probe_kind != 2) return;
    g->attrs.probe_kind = 0xFF;
    sk_slot *s = g->attrs.probe_slot;
    if (!s || s->t0 || aux_d(s) == 2) return;
    set_aux_d(s, 2);
    sk_platform *pf = slot_platform(g, s);
    if (!pf) return;
    pf->pangle = (uint8_t)(pf->pangle + (pf->pmode ? 0x40 : -0x40));
    platform_velocity(pf->pangle, pf->pspeed, &pf->pvx, &pf->pvy);
}

/* ---- attribute 45: the gap between TRAIN's cars carries the kid off (state 56, 10 px a frame) ---- */
static void attr_gap_carry(sk_game *g, int cx, int cy) {             /* 0x11BAEC */
    if (g->attrs.probe_kind != 0) return;
    if (!kid_in_cell(g, cx, cy, -40, 16)) return;
    g->player.state = 0x38;
    g->player.carry_dx = 10;
}

/* ---- attributes 46 and 47: belts, 2 px a frame left / right, for the kid on top and the ball ---- */
static void belt(sk_game *g, int cx, int cy, int dx) {
    int x = cx * 16, y = cy * 16;
    if (g->attrs.probe_kind == 1) {
        if (in_open((int16_t)(bx(g) - x), -8, 18) && in_open((int16_t)(by(g) - y), -16, 0)) g->bw.carry_dx = dx;
    } else if (g->attrs.probe_kind == 0) {
        if (kid_in_cell(g, cx, cy, -40, -14)) g->player.carry_dx = (int16_t)dx;
    }
}
static void attr_belt_left(sk_game *g, int cx, int cy) { belt(g, cx, cy, -2); }    /* 0x11BB32 */
static void attr_belt_right(sk_game *g, int cx, int cy) { belt(g, cx, cy, 2); }    /* 0x11BB2E */

/* ---- attributes 48..51: rail arrows for objects (types 21 / 22): the probe within t0/4 of the cell's middle turns it ---- */
static sk_slot *arrow_hit(sk_game *g, int off) {
    if (g->attrs.probe_kind != 2) return NULL;
    g->attrs.probe_kind = 0xFF;
    sk_slot *s = g->attrs.probe_slot;
    if (!s) return NULL;
    int q = (uint16_t)s->t0 >> 2;
    uint16_t v = (uint16_t)off;
    if (v < (uint16_t)(8 - q) || v >= (uint16_t)(8 + q)) return NULL;
    return s;
}
static void attr_arrow_up(sk_game *g, int cx, int cy) {              /* 0x11BBA4: attribute 48 */
    (void)cy;
    sk_slot *s = arrow_hit(g, g->attrs.probe_x - cx * 16);
    if (!s) return;
    s->flags |= 4; s->vx = 0; s->vy = (int16_t)-s->t0;
}
static void attr_arrow_down(sk_game *g, int cx, int cy) {            /* 0x11BBE4: attribute 50 */
    (void)cy;
    sk_slot *s = arrow_hit(g, g->attrs.probe_x - cx * 16);
    if (!s) return;
    s->flags |= 4; s->vx = 0; s->vy = s->t0;
}
static void attr_arrow_left(sk_game *g, int cx, int cy) {            /* 0x11BC20: attribute 51 */
    (void)cx;
    sk_slot *s = arrow_hit(g, g->attrs.probe_y - cy * 16);
    if (!s) return;
    s->flags &= ~4; s->vy = 0; s->vx = s->t0; s->flags &= ~1;
}
static void attr_arrow_right(sk_game *g, int cx, int cy) {           /* 0x11BC62: attribute 49 */
    (void)cx;
    sk_slot *s = arrow_hit(g, g->attrs.probe_y - cy * 16);
    if (!s) return;
    s->flags &= ~4; s->vy = 0; s->vx = s->t0; s->flags |= 1;
}

/* ---- attributes 56..59: the kid behind the scenery (player_hidden: no touches, the ball inert) by the half of the cell ---- */
static void hide_by(sk_game *g, int v, bool far_half_hides) {
    if (v < 0 || v >= 16) return;
    bool hide = far_half_hides;
    if (v < 8) hide = !hide;
    g->player_hidden = hide;
}
static void attr_hide_right(sk_game *g, int cx, int cy) { (void)cy; if (g->attrs.probe_kind == 0) hide_by(g, (int16_t)(g->player.x - cx * 16), true); }    /* 0x11BE46: 56 */
static void attr_hide_left(sk_game *g, int cx, int cy) { (void)cy; if (g->attrs.probe_kind == 0) hide_by(g, (int16_t)(g->player.x - cx * 16), false); }    /* 0x11BE4A: 57 */
static void attr_hide_below(sk_game *g, int cx, int cy) { (void)cx; if (g->attrs.probe_kind == 0) hide_by(g, (int16_t)(g->player.y - cy * 16), true); }    /* 0x11BE84: 58 */
static void attr_hide_above(sk_game *g, int cx, int cy) { (void)cx; if (g->attrs.probe_kind == 0) hide_by(g, (int16_t)(g->player.y - cy * 16), false); }   /* 0x11BE88: 59 */

/* ---- attributes 78 and 79: crossing to the near half (left for 78, right for 79) starts the oncoming train (type 23) ---- */
static void train_trigger(sk_game *g, int cx, int cy, bool left_half) {
    if (g->attrs.probe_kind != 0) return;
    int v = (int16_t)(g->player.x - cx * 16);
    if (v < 0 || v >= 16) return;
    uint8_t old = g->post_flag;
    bool set = left_half ? v < 8 : v >= 8;
    g->post_flag = (uint8_t)(set ? g->post_flag | 0x80 : g->post_flag & 0x7F);
    if (!g->post_flag || old) return;
    g->post_flag = 0xFF;
    spawn_object(g, cx * 16, cy * 16, 23);
}
static void attr_train_left(sk_game *g, int cx, int cy) { train_trigger(g, cx, cy, true); }     /* 0x11C534 */
static void attr_train_right(sk_game *g, int cx, int cy) { train_trigger(g, cx, cy, false); }   /* 0x11C58A */

void attr_japan_install(void) {
    attr_table[11] = attr_shade_y_dark;
    attr_table[12] = attr_shade_y_dark_rev;
    attr_table[18] = attr_anim_hold;
    attr_table[21] = attr_platform_quarter;
    attr_table[45] = attr_gap_carry;
    attr_table[46] = attr_belt_left;
    attr_table[47] = attr_belt_right;
    attr_table[48] = attr_arrow_up;
    attr_table[49] = attr_arrow_right;
    attr_table[50] = attr_arrow_down;
    attr_table[51] = attr_arrow_left;
    attr_table[56] = attr_hide_right;
    attr_table[57] = attr_hide_left;
    attr_table[58] = attr_hide_below;
    attr_table[59] = attr_hide_above;
    attr_table[78] = attr_train_left;
    attr_table[79] = attr_train_right;
}
