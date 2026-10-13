/* Italy's object types, transcribed from the listing. */
#include <string.h>
#include "game/game.h"
#include "game/obj_italy.h"
#include "game/obj_helpers.h"
#include "game/platforms.h"
#include "game/resources.h"

static const uint8_t pendulum_link_dx[9] = { 1, 2, 3, 5, 9, 13, 15, 16, 16 };                   /* 0x111F7E (words) */
static const uint8_t pendulum_frame_pos[71] = {                                                 /* 0x111F90 */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t pendulum_frame_neg[71] = {                                                 /* 0x111FD7 */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };
static const uint8_t jumper_fly_script[] = { 0, 4, 0, 2, 7, 8, 9, 10, 0, 1, 2, 3 };              /* 0x11222E */
static const uint8_t jumper_land_script[] = { 0, 4, 0, 4, 4, 5, 6, 6, 4, 5, 6, 6 };             /* 0x11223A */

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static bool need(sk_game *g, sk_slot *s, const uint8_t *f) { if (f) return true; free_object(g, s); return false; }
static void reset_animation(sk_slot *s) { s->anim_tick = 0; s->anim_index = 0; }   /* 0x10DF62 */
static int t1_hi(const sk_slot *s) { return (uint16_t)s->t1 >> 8; }                              /* byte +0x16 */
static void set_t1_hi(sk_slot *s, int v) { s->t1 = (int16_t)((v & 0xFF) << 8 | (s->t1 & 0xFF)); }
static int16_t trunc16(int v) { return (int16_t)(v < 0 ? -((-v) >> 4) : v >> 4); }   /* neg, lsr #4, neg */

static void chain_record(sk_game *g, sk_draw *d, const sk_slot *s, int w, int h, const uint8_t *frames) {   /* set_frames_base, d0 = 1 */
    size_frames(g, d, w, h, frames);
    d->x = s->x; d->y = s->y;
    d->prio = 0; d->dflags = 1;
    d->used = true;
}

/* ---- type 36: the pendulum (slots 46 and 45): a chain of 4 links and 3 shadows, the ball on objects_c[7] ---- */
static void obj_pendulum_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x111D66 */
    y += 8;
    if (g->objs.marker_param & 1) x += 8;
    g->objs.chain_a = true;
    init_object_slot_shared(g, s, x, y, s->type4, &g->objs.chain[7]);
    s->draw = NULL;                                                 /* the original sets no draw record */
    s->vx = 0; s->vy = 0; s->t0 = 0; s->t1 = 0x1C;
    const uint8_t *links = res(g, 46), *ball = res(g, 45);
    if (!links || !ball) { g->objs.chain_a = false; free_object(g, s); return; }
    for (int i = 0; i < 4; i++) chain_record(g, &g->objs.chain[i], s, 16, 16, links);
    for (int i = 4; i < 7; i++) chain_record(g, &g->objs.chain[i], s, 16, 16, links + 0x80);
    chain_record(g, &g->objs.chain[7], s, 32, 29, ball);
}
static void obj_pendulum_update(sk_game *g, sk_slot *s, int index) {           /* 0x111E34 */
    (void)index;
    int step = (uint16_t)(s->t1 + 7) >> 3;
    if (s->flags & 0xFF) step = -step;
    s->t0 = (int16_t)(s->t0 + step);
    int a = s->t0;
    bool turn = true;
    if (aux_c(s)) {
        s->t1++;
        if (a != 0) turn = false;
        else set_aux_c(s, aux_c(s) ^ 0xFF);
    }
    if (turn && --s->t1 == 0) { s->flags = (int16_t)(s->flags ^ 0xFF); set_aux_c(s, aux_c(s) ^ 0xFF); }
    const uint8_t *links = res(g, 46), *ball = res(g, 45);
    const uint8_t *tab = a < 0 ? pendulum_frame_neg : pendulum_frame_pos;
    int m = a < 0 ? -a : a;
    int fi = tab[m < 71 ? m : 70];                                  /* port guard: the swing stays inside 0..70 */
    const uint8_t *shadow = links + 0xA0 + fi * 0xA0;
    int ang = (a & 0xFF) * 4;
    int c = cos_1024(ang), sn = sin_1024(ang);
    int dx = 0, dy = 0;
    for (int k = 0; k < 4; k++) {
        sk_draw *l = &g->objs.chain[k], *sh = &g->objs.chain[k + 4];
        l->x = (int16_t)(trunc16(dx) + s->x);
        sh->x = (int16_t)(l->x + 8 - pendulum_link_dx[fi]);
        l->y = (int16_t)(trunc16(dy) + s->y);
        sh->y = (int16_t)(l->y + 8);
        set_draw_frame_ptr(sh, shadow);
        dx = (int16_t)(dx + sn); dy = (int16_t)(dy + c);
    }
    sk_draw *b = &g->objs.chain[7];
    b->x = (int16_t)(((int16_t)dx >> 4) + s->x - 8);
    b->y = (int16_t)(((int16_t)dy >> 4) + s->y - 8);
    set_draw_frame_ptr(b, ball);
    touch_player(g, b->x + 8, b->y + 7, 16, 15);                    /* the hurt side lands in 0x302B4A, never read */
    if (object_offscreen(g, s->x, s->y, 160) < 0) { g->objs.chain_a = false; free_object(g, s); }
}

/* ---- type 37: the jumper (slot 48): thrown up from its marker, lands, blinks out and starts again ---- */
static void jumper_restart(sk_game *g, sk_slot *s, sk_draw *d) {               /* sub_112206 */
    (void)g;
    d->dflags &= (uint8_t)~6;
    set_aux_c(s, 0);
    d->x = s->x; d->y = s->y;
    s->vy = s->t0;
    reset_animation(s);
}
static void obj_jumper_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x1120F0 */
    init_object_slot(g, s, x - 8, y - 16, s->type4);
    s->t0 = -20; s->vx = 0; s->vy = s->t0;
    s->flags |= 4;
    const uint8_t *f = res(g, 48);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 21, f, 1);                                  /* sub_10796C */
}
static void obj_jumper_update(sk_game *g, sk_slot *s, int index) {             /* 0x112128 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (aux_c(s)) {
        s->x0++;
        if (s->x0 == 75) jumper_restart(g, s, d);
        else if ((uint16_t)s->x0 >= 25) {
            if ((uint16_t)s->x0 < 50) d->dflags ^= 4;
            else d->dflags |= 4;
        }
    } else {
        apply_velocity_sym(s);
        int v = s->vy < 0 ? -s->vy : s->vy;
        if (v <= 3) {
            if (!t1_hi(s)) reset_animation(s);
            set_t1_hi(s, 0xFF);
            step_animation_simple(s, jumper_land_script, 0);
        } else {
            if (t1_hi(s)) reset_animation(s);
            set_t1_hi(s, 0);
            step_animation_simple(s, jumper_fly_script, s->vy < 0);   /* rol.w #1: the sign bit */
            int rows = probe_rows_below(g, s, 0, 0);
            if (rows) { d->y = (int16_t)(d->y + rows); set_aux_c(s, 1); s->x0 = 0; }
        }
        if (s->vy >= 0) touch_player(g, d->x + 4, d->y, 8, 21);
    }
    if ((int16_t)(d->y - s->y) < 0) return;
    if (object_offscreen(g, d->x, d->y, 64) >= 0) return;          /* sub_10D92C */
    jumper_restart(g, s, d);
    if (object_offscreen(g, s->x, s->y, 64) < 0) free_object(g, s);
}

void obj_italy_install(void) {
    object_init_table[36] = obj_pendulum_init;         object_update_table[36] = obj_pendulum_update;
    object_init_table[37] = obj_jumper_init;           object_update_table[37] = obj_jumper_update;
}
