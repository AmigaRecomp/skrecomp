/* Italy's object types, transcribed from the listing. */
#include <string.h>
#include "game/game.h"
#include "game/obj_italy.h"
#include "game/obj_helpers.h"
#include "game/platforms.h"
#include "game/resources.h"
#include "game/attrs.h"
#include "game/ball.h"
#include "game/player.h"
#include "game/world.h"

static const uint8_t pendulum_link_dx[9] = { 1, 2, 3, 5, 9, 13, 15, 16, 16 };                   /* 0x111F7E (words) */
static const uint8_t pendulum_frame_pos[71] = {                                                 /* 0x111F90 */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t pendulum_frame_neg[71] = {                                                 /* 0x111FD7 */
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };
static const uint8_t jumper_fly_script[] = { 0, 4, 0, 2, 7, 8, 9, 10, 0, 1, 2, 3 };              /* 0x11222E */
static const uint8_t strider_walk_script[] = { 0, 4, 0, 4, 0, 1, 2, 3, 5, 6, 7, 8 };          /* 0x112768 */
static const uint8_t bouncer_script[] = { 0, 4, 0, 4, 0, 1, 2, 3, 3, 2, 1, 0 };               /* 0x112876 */
static const uint8_t rock_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 0, 1, 2, 3 };                   /* 0x112FFE */
static const int16_t rock_tremble[4] = { 1, 0, -1, 0 };                                       /* 0x11300A */
static const uint8_t hopper49_script[] = { 0, 4, 0, 2, 0, 1, 2, 3, 3, 2, 1, 0 };              /* 0x1130AA */
static const uint8_t thrower_walk_script[] = { 0, 6, 0, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };          /* 0x118682 */
static const uint8_t thrower_throw_script[] = { 0, 6, 0, 4, 16, 17, 18, 19, 19, 19, 12, 13, 14, 15, 15, 15 };  /* 0x118692 */
static const uint8_t thrown_script[] = { 0, 8, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };       /* 0x1186A2 */
static const uint8_t thrower68_walk_script[] = { 0, 6, 0, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };        /* 0x118DE6 */
static const uint8_t thrower68_throw_script[] = { 0, 4, 0, 6, 12, 13, 14, 15, 16, 17, 18, 19 };             /* 0x118DF6 */
static const uint8_t thrown68_script[] = { 0, 2, 0, 10, 0, 0, 1, 1 };                                        /* 0x118E02 */
static const int16_t flyer_bob[33] = { 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, -1, -1, -1, -1,
                                      -2, -2, -2, -2, -2, -1, -1, -1, -1, 0x1000 };                  /* 0x119214 */
static const uint8_t flyer_script[] = { 0, 4, 0, 10, 0, 1, 2, 3, 4, 5, 6, 7 };                  /* 0x119256 */
static const uint8_t flyer_bare_script[] = { 0, 4, 0, 10, 8, 9, 10, 11, 12, 13, 14, 15 };       /* 0x119262 */
static const uint8_t wings_script[] = { 0, 8, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };   /* 0x119316 */
static const int16_t frog_hops[25] = { 0, -10, 2, 1,  0, -10, 2, 0,  4, -18, 1, 0,  0, -10, 2, 0,
                                      0, -10, 2, 1,  4, -18, 1, 1,  -1 };                          /* 0x11114C: vx, vy, hops, facing */
static const uint8_t frog_rise_script[] = { 0, 2, 0, 4, 0, 1, 6, 7 };                           /* 0x11117E */
static const uint8_t frog_top_script[] = { 0, 2, 0, 4, 2, 3, 8, 9 };                            /* 0x111186 */
static const uint8_t frog_fall_script[] = { 0, 2, 0, 4, 4, 5, 10, 11 };                         /* 0x11118E */
static const uint8_t frog_spit_script[] = { 0, 2, 0, 4, 0, 0, 0, 0 };                           /* 0x111196 */
static const uint8_t rb_s7a[] = { 0, 1, 0, 1, 0, 0 }, rb_s80[] = { 0, 1, 0, 1, 1, 1 }, rb_s86[] = { 0, 1, 0, 1, 2, 3 };
static const uint8_t rb_s8c[] = { 0, 6, 0, 6, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22 }, rb_s9c[] = { 0, 2, 0, 6, 23, 24, 23, 24 };
static const uint8_t rb_sa4[] = { 0, 1, 0, 1, 4, 4 }, rb_saa[] = { 0, 4, 0, 6, 5, 6, 5, 5, 5, 6, 5, 5 };
static const uint8_t rb_sb6[] = { 0, 4, 0, 6, 7, 8, 9, 10, 7, 8, 9, 10 };
static const sk_phase rboss_phases[23] = {                                                    /* 0x115392 */
    { 0xFFFF, 0, 4 }, { 0xFFFF, 0x14, 8 }, { 100, 0x28, 0x0C }, { 50, 0x1E, 0x10 }, { 100, 0x28, 0x14 }, { 50, 0x1E, 0x18 },
    { 100, 0x28, 0x1C }, { 0xFFFF, 0x0A, 0x20 }, { 0xFFFF, 0x14, 0x24 }, { 25, 0x28, 0x28 }, { 0xFFFF, 0x50, 0x2C },
    { 0x68, 0x32, 0x30 }, { 50, 0x28, 0x34 }, { 100, 0x46, 0x38 }, { 50, 0x28, 0x3C }, { 0xFFFF, 0x3C, 0x40 },
    { 0x68, 0x32, 0x44 }, { 25, 0x28, 0x00 }, { 0xFFFF, 0x5A, 0x48 }, { 25, 0x64, 0x50 }, { 12, 0x6E, 0x54 },
    { 200, 0x78, 0x58 }, { 0xFFFF, 0x82, 0x58 },
};
static const sk_motion rboss_motions[14] = {                                                  /* 0x1153EE */
    { 0, 0, rb_s7a, 0x00 }, { 0, 0, rb_s7a, 0x04 }, { 0, 0, rb_s7a, 0x08 }, { 0, 0, rb_s80, 0x0C }, { 0, 0, rb_s7a, 0x10 },
    { 2, 0, rb_s8c, 0x14 }, { 0, 0, rb_s7a, 0x18 }, { 0, 0, rb_s86, 0x1C }, { 0, 0, rb_s7a, 0x20 }, { 0, 0, rb_s9c, 0x24 },
    { 0, -12, rb_sa4, 0x28 }, { 0, 0, rb_saa, 0x2C }, { 0, 0, rb_sb6, 0x30 }, { 0, 0, rb_sb6, 0x34 },
};
static const uint8_t lift_script[] = { 0, 4, 0, 3, 0, 1, 2, 3, 0, 1, 2, 3 };                  /* 0x1156D0 */
static const uint8_t shot_scripts[3][12] = {                                                  /* 0x1157D0, 0x1157DC, 0x1157E8 */
    { 0, 4, 0, 4, 0, 1, 2, 3, 0, 1, 2, 3 }, { 0, 4, 0, 4, 5, 6, 7, 8, 5, 6, 7, 8 }, { 0, 4, 0, 4, 10, 11, 12, 13, 10, 11, 12, 13 } };
static const uint8_t spirit_in_script[] = { 0, 8, 0, 2, 7, 6, 5, 4, 3, 2, 1, 0, 7, 6, 5, 4, 3, 2, 1, 0 };   /* 0x115928 */
static const uint8_t spirit_out_script[] = { 0, 8, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };  /* 0x11593C */
static const int16_t spirit_vy[4] = { 0, 4, 0, -4 };                                          /* 0x115920 */
static const int16_t suction_jitter[8] = { 0, 24, 12, 36, 48, 24, 12, 36 };                   /* 0x1159F4 */
static const uint8_t suction_script[] = { 0, 4, 0, 1, 0, 1, 2, 3, 0, 1, 2, 3 };               /* 0x115A04 */
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
    if (touch_player(g, b->x + 8, b->y + 7, 16, 15) < 0)
        g->player.knock_dir = (int16_t)~(int16_t)(int8_t)s->flags;  /* along the swing: ext.w, not.w */
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

/* ---- type 38: the swinging platform (a param resource) on a chain of slot 46 links ---- */
static int16_t hi(int32_t v) { return (int16_t)(v >> 16); }
static int32_t with_hi(int32_t v, int h) { return (int32_t)((uint32_t)(uint16_t)h << 16 | ((uint32_t)v & 0xFFFF)); }
static int t0_hi(const sk_slot *s) { return (uint16_t)s->t0 >> 8; }                              /* byte +0x14 */
static void set_t0_hi(sk_slot *s, int v) { s->t0 = (int16_t)((v & 0xFF) << 8 | (s->t0 & 0xFF)); }

/* The chain platforms' shared parts: types 38 (0x117A04 / 0x117B08) and 6 (0x1177F6 / 0x117904) differ only in the angle step. */
sk_platform *chain_platform_init(sk_game *g, sk_slot *s, int x, int y) {
    x += 8;
    init_object_slot(g, s, x, y, s->type4);
    int p = g->objs.marker_param & 0xFF;
    int r = (p & 0x30) + 0x40;
    s->x0 = (int16_t)r;
    y += r;
    g->objs.chain_count = (r >> 4) - 2;
    int index = -1;
    sk_platform *pf = alloc_platform(g, &index);
    s->platform = index;
    s->flags = (int16_t)(index < 0 ? 0xFF : (index * 32) >> 8);   /* move.w d0,$a(a6): byte +0xA */
    const sk_section *sec = g->res ? g->res->param[(p & 0xC0) >> 6].sec : NULL;
    const uint8_t *links = res(g, 46);
    if (!pf || !sec || !links) { if (pf) free_platform(g, s); free_object(g, s); return NULL; }   /* port guard */
    const uint8_t *b = g->res->param[(p & 0xC0) >> 6].bytes;
    pf->px = (int32_t)((uint32_t)(uint16_t)x << 16);
    pf->py = (int32_t)((uint32_t)(uint16_t)y << 16);
    pf->pangle = 0;
    pf->half_w = b[0]; pf->half_h = b[1];
    pf->px = with_hi(pf->px, hi(pf->px) + (int8_t)b[2]);
    s->anim = b[3];
    pf->py = with_hi(pf->py, hi(pf->py) + (int8_t)b[3]);
    pf->stand_dy = b[5];
    pf->frame_sel = b[4];                                           /* the whole byte (type 5 masks it) */
    set_draw_size_at(g, s, 2 * pf->half_w, 2 * pf->half_h, sec->data);
    sk_draw *d = slot_draw_record(g, s);
    if (d) {
        d->frames_len = sec->len;
        int off = (uint16_t)d->frame_bytes - (pf->frame_sel ? 0 : (uint16_t)d->plane_bytes);
        pf->pframe = (size_t)off < sec->len ? d->frames + off : NULL;
    }
    for (int i = 0; i <= g->objs.chain_count && i < 8; i++) {
        chain_record(g, &g->objs.chain[i], s, 16, 16, links);
        g->objs.chain[i].frame_bytes = 0x80;
    }
    g->objs.chain_b = true;
    return pf;
}
void chain_platform_place(sk_game *g, sk_slot *s, sk_platform *pf) {
    sk_draw *d = slot_draw_record(g, s);
    int r = s->x0, a = pf->pangle;
    int c = cos_1024(a * 4), sn = sin_1024(a * 4);
    int dx = (int16_t)(((int16_t)(sn * r >> 8)) + s->x - hi(pf->px));   /* muls, lsr.l #8: the low word */
    int dy = (int16_t)(((int16_t)(c * r >> 8)) + s->y - hi(pf->py));
    platform_carry(g, pf, dx, dy);
    pf->px = with_hi(pf->px, hi(pf->px) + dx);
    pf->py = with_hi(pf->py, hi(pf->py) + dy);
    if (d) { d->x = (int16_t)(hi(pf->px) - pf->half_w); d->y = hi(pf->py); }
    int lx = s->x - ((uint16_t)pf->half_w >> 1), ly = s->y;
    int16_t ax = 0, ay = 0;
    for (int i = 0; i <= g->objs.chain_count && i < 8; i++) {
        ax = (int16_t)(ax + sn); ay = (int16_t)(ay + c);
        g->objs.chain[i].x = (int16_t)(trunc16(ax) + lx);
        g->objs.chain[i].y = (int16_t)(trunc16(ay) + ly);
    }
    if (object_offscreen(g, hi(pf->px), hi(pf->py), 160) < 0 && object_offscreen(g, s->x, s->y, 160) < 0) {
        free_platform(g, s);
        free_object(g, s);
        g->objs.chain_b = false;
    }
}

static void obj_swing_platform_init(sk_game *g, sk_slot *s, int x, int y) {    /* 0x117A04 */
    sk_platform *pf = chain_platform_init(g, s, x, y);
    if (pf) pf->pspeed = 0x1E;
}
static void obj_swing_platform_update(sk_game *g, sk_slot *s, int index) {     /* 0x117B08 */
    (void)index;
    sk_platform *pf = slot_platform(g, s);
    if (!pf) return;
    int sp = pf->pspeed < 0x12 ? pf->pspeed : 0x12;
    int step = (uint8_t)(sp + 7) >> 3;
    if (t0_hi(s)) step = -step;
    pf->pangle = (uint8_t)(pf->pangle + step);
    int a = pf->pangle;
    bool turn = true;
    if (t1_hi(s)) {
        pf->pspeed++;
        if (a != 0) turn = false;
        else set_t1_hi(s, t1_hi(s) ^ 0xFF);
    }
    if (turn && --pf->pspeed == 0) { set_t0_hi(s, t0_hi(s) ^ 0xFF); set_t1_hi(s, t1_hi(s) ^ 0xFF); }
    chain_platform_place(g, s, pf);
}

/* ---- type 43: the strider (slots 53, 55; 54 when its rider is knocked off) ---- */
static void obj_strider_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x1124B4 */
    init_object_slot(g, s, x, y - 36, s->type4);
    s->vx = 8; s->vy = 0; s->hits = 2;
    s->flags |= 8;
    const uint8_t *f = res(g, 53);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 52, f, 1);                                  /* sub_107E56 */
}
static void obj_strider_update(sk_game *g, sk_slot *s, int index) {           /* 0x1124EA */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    switch (aux_c(s)) {
    case 1:                                                         /* slowing to turn */
        set_frame2(s, 4, 9, s->flags);
        if ((g->frame_ticks & 3) == 0) s->vx--;
        if (s->vx == 0) { s->flags ^= 1; set_aux_c(s, 0); }
        break;
    case 2:                                                         /* the rider knocked off: bounces */
        if (s->vy == -4) set_frame2(s, 0, 3, s->flags);
        if (probe_rows_below(g, s, 0, 0)) {
            set_frame2(s, 1, 4, s->flags);
            s->vy = -12;
            if (--s->t0 == 0) { set_aux_c(s, 3); s->flags |= 2; }
        }
        despawn_if_far(g, s);
        return;
    case 3:
        if (s->vy == -4) set_frame2(s, 0, 3, s->flags);
        despawn_if_far(g, s);
        return;
    case 4:                                                         /* leaping: lands near its start height */
        if (s->vy >= 0) {
            int dy = (int16_t)(s->y - d->y);
            if (dy < 0) dy = -dy;
            int rows;
            if (dy < 16 && (rows = probe_rows_below2(g, s, 0, 0)) != 0) {
                d->y = (int16_t)(d->y + rows + 6);
                reset_animation(s);
                size_frames(g, d, 48, 52, res(g, 53));
                s->flags &= ~4;
                set_aux_c(s, 0);
            }
        }
        break;
    default: {
        if (s->vx != 8) s->vx++;
        int wrapped = 0;
        step_animation(s, strider_walk_script, s->flags, &wrapped);
        int ahead = (int16_t)(d->x - s->x);
        if (!(s->flags & 1)) ahead = -ahead;
        if (ahead >= 160) { set_aux_c(s, 1); break; }
        if (ahead >= 100 || s->vx != 8) break;
        int k = (int16_t)(d->x + 24 - g->player.x);
        if (s->flags & 1) k = -k;
        if (k < 80 || k > 112) break;
        set_aux_c(s, 4);                                            /* leap at the kid */
        reset_animation(s);
        size_frames(g, d, 48, 58, res(g, 55));
        set_frame2(s, 0, 1, s->flags);
        d->y = (int16_t)(d->y - 6);
        s->vy = -16;
        s->flags |= 4;
        break;
    }
    }
    touch_player(g, d->x, d->y, 48, 40);
    if (ball_hits_object_quiet(g, s, 0, 0, 48, 48, 0, 0x500) < 0) { despawn_if_far(g, s); return; }
    size_frames(g, d, 32, 41, res(g, 54));                          /* killed: the rider flies off, the mount bounces */
    s->flags |= 4;
    s->vx = 4; s->vy = -4;
    d->dflags &= (uint8_t)~2;
    s->t0 = 3;
    set_aux_c(s, 2);
    g->objs.marker_param = s->flags & 0xFF;
    spawn_object(g, d->x, s->y + 27, 100);
    set_frame2(s, 0, 3, s->flags);
    g->score_value = 0x500;                                         /* the popup reads it */
    add_score(g, 0x500);
    spawn_object(g, d->x + ((uint16_t)d->w >> 1), d->y + ((uint16_t)d->h >> 1), 113);
    despawn_if_far(g, s);
}

/* ---- type 100: the strider's rider, thrown off (slot 54) ---- */
static void obj_rider_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x112774 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->flags = (int16_t)(g->objs.marker_param & 1);
    s->vx = 6; s->vy = 0; s->t0 = 50;
    const uint8_t *f = res(g, 54);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 41, f, 1);                                  /* sub_107C58 */
    set_frame2(s, 2, 5, s->flags);
}
static void obj_rider_update(sk_game *g, sk_slot *s, int index) {             /* 0x1127BA */
    (void)g; (void)index;
    apply_velocity(s);
    if (s->t0 && --s->t0 == 0) s->flags |= 4;
    /* object_offscreen's result is dropped: the original never frees it */
}

/* ---- type 44: the bouncer (slot 52): hops along the ground, out of play 400 px from its marker ---- */
static void obj_bouncer_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x1127E4 */
    init_object_slot(g, s, x, y - 16, s->type4);
    s->vx = 6; s->vy = 0;
    s->flags |= 8 | 4;
    const uint8_t *f = res(g, 52);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
}
static void obj_bouncer_update(sk_game *g, sk_slot *s, int index) {           /* 0x11281A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    step_animation_simple(s, bouncer_script, s->flags);
    if (!(s->flags & 2)) {
        if (s->vy >= 0 && probe_rows_below(g, s, 12, 0) < 0) s->vy = -4;
        int dx = (int16_t)(s->x - d->x);
        if (dx < 0) dx = -dx;
        if (dx >= 400) s->flags |= 2;
        touch_player(g, d->x, d->y, 32, 32);
    }
    despawn_if_far(g, s);
}

/* ---- type 48: the loose rock (slot 128): trembles, drops when the kid passes under ---- */
static void obj_rock_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x112F5A */
    init_object_slot(g, s, x - 8, y - 24, s->type4);
    s->vy = 0; s->vx = 0;
    const uint8_t *f = res(g, 128);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
}
static void obj_rock_update(sk_game *g, sk_slot *s, int index) {              /* 0x112F84 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (aux_c(s) == 0) {
        d->x = (int16_t)(d->x + rock_tremble[g->frame_ticks & 3]);
        if ((int16_t)(d->y + 32 - g->player.y) >= 0) { despawn_if_far(g, s); return; }
        int dx = (int16_t)(d->x + 16 - g->player.x);
        if (dx < 0) dx = -dx;
        if (dx >= 32) { despawn_if_far(g, s); return; }
        s->flags |= 4 | 2;
        set_aux_c(s, 1);
    }
    int wrapped = 0;
    step_animation(s, rock_script, 0, &wrapped);
    touch_player(g, d->x, d->y, 32, 32);
    despawn_if_far(g, s);
}

/* ---- type 49: the hopper (slot 92): bounces along, out of play 380 px from its marker ---- */
static void obj_hopper49_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x113012 */
    init_object_slot(g, s, x, y - 16, s->type4);
    s->vx = 4; s->vy = -4;
    s->flags |= 8 | 4;
    const uint8_t *f = res(g, 92);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
}
static void obj_hopper49_update(sk_game *g, sk_slot *s, int index) {          /* 0x11304A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    int wrapped = 0;
    step_animation(s, hopper49_script, s->flags, &wrapped);
    if (!(s->flags & 2)) {
        int rows = probe_rows_below(g, s, 16, 0);
        if (rows < 0) { d->y = (int16_t)(d->y + rows); s->vy = -4; }
        int dx = (int16_t)(d->x - s->x);
        if (dx < 0) dx = -dx;
        if (dx >= 0x17C) s->flags |= 2;
        touch_player(g, d->x + 2, d->y + 2, 28, 28);
    }
    despawn_if_far(g, s);
}

/* ---- type 64: the thrower (slots 111, 112; its missile type 99 from slot 113) ---- */
static void obj_thrower_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x1184F6 */
    init_object_slot(g, s, x, y - 48, s->type4);
    s->vx = 2; s->vy = 0;
    s->flags |= 8;
    s->hits = 2; s->t0 = 0;
    const uint8_t *f = res(g, 111);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 64, f, 1);                                  /* frames_48x64 */
}
static void obj_thrower_update(sk_game *g, sk_slot *s, int index) {           /* 0x118534 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    bool attack = aux_c(s) == 1;
    if (!attack) {
        int wrapped = 0;
        step_animation(s, thrower_walk_script, s->flags, &wrapped);
        int dx = (int16_t)(d->x - s->x);
        if (dx < 0) dx = -dx;
        if (dx >= 150 || ground_ahead(g, s, s->flags) != 0) s->flags ^= 1;
        if (s->t0) s->t0--;
        else {
            int side = s->flags, k = (int16_t)(d->x - g->player.x);
            if (k < 0) { side ^= 1; k = -k; }
            if (k < 80 && !(side & 1)) { reset_animation(s); s->vx = 0; set_aux_c(s, 1); attack = true; }
        }
    }
    if (attack) {
        int wrapped = 0;
        if (step_animation(s, thrower_throw_script, s->flags, &wrapped) && s->anim_index == 3)
            spawn_child(g, s, d->x + 24, d->y + 16, 99, 24, res(g, 113), thrown_script, 32, 26, 12, 4);
        if (wrapped) {
            set_frame2(s, 1, 8, s->flags);
            reset_animation(s);
            s->vx = 2; set_aux_c(s, 0); s->t0 = 50;
        }
    }
    touch_player(g, d->x + 8, d->y + 8, 32, 56);
    if (ball_hits_object(g, s, 0, 0, 48, 64, 0, 0x500) >= 0) size_frames(g, d, 64, 58, res(g, 112));
    despawn_if_far(g, s);
}

/* ---- type 68: the second thrower (slots 102, 103; its missile type 99 from slot 104) ---- */
static void obj_thrower68_init(sk_game *g, sk_slot *s, int x, int y) {        /* 0x118C58 */
    init_object_slot(g, s, x, y - 48, s->type4);
    s->vx = 2; s->vy = 0; s->hits = 2;
    s->flags |= 8;
    const uint8_t *f = res(g, 102);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 64, f, 1);                                  /* frames_48x64 */
}
static void obj_thrower68_update(sk_game *g, sk_slot *s, int index) {         /* 0x118C92 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    bool attack = aux_c(s) == 1;
    if (!attack) {
        int wrapped = 0;
        step_animation(s, thrower68_walk_script, s->flags, &wrapped);
        int dx = (int16_t)(d->x + 24 - s->x);
        if (dx < 0) dx = -dx;
        if (dx >= 150 || ground_ahead(g, s, s->flags) != 0) s->flags ^= 1;
        if (s->t0) s->t0--;
        else {
            int side = s->flags, k = (int16_t)(d->x + 24 - g->player.x);
            if (k < 0) { k = -k; side ^= 1; }
            if (!(side & 1) && k < 90) { reset_animation(s); s->vx = 0; set_aux_c(s, 1); attack = true; }
        }
    }
    if (attack) {
        int wrapped = 0;
        if (step_animation(s, thrower68_throw_script, s->flags, &wrapped) && s->anim_index == 3)
            spawn_child(g, s, d->x + 24, d->y + 16, 99, 24, res(g, 104), thrown68_script, 32, 19, 10, 0);
        if (wrapped) {
            set_frame2(s, 0, 6, s->flags);
            reset_animation(s);
            s->t0 = 25; s->vx = 2; set_aux_c(s, 0);
        }
    }
    touch_player(g, d->x, d->y, 48, 64);
    if (ball_hits_object(g, s, 0, 0, 48, 64, 0, 0x500) >= 0) size_frames(g, d, 64, 57, res(g, 103));
    despawn_if_far(g, s);
}

/* ---- type 70: the flyer (slot 123): bobs, chases the kid in front of it, loses its wings (type 88) ---- */
static int t1_lo(const sk_slot *s) { return s->t1 & 0xFF; }                                    /* byte +0x17 */
static void set_t1_lo(sk_slot *s, int v) { s->t1 = (int16_t)(((uint16_t)s->t1 & 0xFF00) | (v & 0xFF)); }

static void obj_flyer_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x119006 */
    init_object_slot(g, s, x, y - 48, s->type4);
    s->vx = 0; s->vy = 0; s->t0 = 0; s->t1 = 0;
    s->flags |= 4 | 8;
    s->hits = 2;
    const uint8_t *f = res(g, 123);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 64, f, 1);                                  /* frames_48x64 */
    sk_draw *d = slot_draw_record(g, s);
    if (d && (int16_t)(d->x + 24 - g->player.x) < 0) s->flags |= 1;
}
static void obj_flyer_update(sk_game *g, sk_slot *s, int index) {             /* 0x119060 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (s->flags & 2) { apply_velocity(s); despawn_if_far(g, s); return; }
    s->x0 = d->x;
    apply_velocity_nograv(s);
    int wrapped = 0;
    step_animation(s, aux_d(s) ? flyer_bare_script : flyer_script, s->flags, &wrapped);
    int b = flyer_bob[(uint16_t)s->t0 < 33 ? (uint16_t)s->t0 : 32];   /* port guard: t0 stays inside the table */
    s->t0++;
    if (b == 0x1000) { b = flyer_bob[0]; s->t0 = 0; }
    s->vy = (int16_t)(b * 2);
    bool go = true;
    if (!(aux_c(s) & 1)) {
        int side = s->flags, k = (int16_t)(d->x + 24 - g->player.x);
        if (k < 0) { k = -k; side ^= 1; }
        if ((side & 1) || k >= 110) go = false;
        else { s->vx = 4; set_aux_c(s, aux_c(s) | 1); }
    }
    if (go) {
        set_aux_c(s, aux_c(s) | 2);
        int dx = (int16_t)(d->x - s->x);
        if (dx < 0) dx = -dx;
        if (dx >= 320) { d->x = s->x0; set_aux_c(s, aux_c(s) & ~2); }   /* held at 320 px from its marker */
        int right = 1, k = (int16_t)(g->player.x - d->x - 24);
        if (k < 0) { right = 0; k = -k; }
        if (k <= 2) set_t1_lo(s, 0);
        if (k >= 64) {
            int was = s->flags & 1;
            s->flags = (int16_t)((s->flags & ~1) | right);
            if (was != right) {
                set_t1_hi(s, t1_hi(s) + 1);
                if ((int8_t)t1_hi(s) > 4) { set_t1_lo(s, 0xFF); set_t1_hi(s, 0); }
            }
        }
        if (aux_c(s) & 2) {
            int dy = (int16_t)(g->player.y - 32 - d->y - (t1_lo(s) ? 0 : 32));
            s->vy = (int16_t)(s->vy + (dy < 0 ? -2 : 2));
        }
    }
    touch_player(g, d->x, d->y, 32, 54);
    int r = ball_hits_object(g, s, 0, 0, 48, 64, 8, 0x500);
    if (r == -2 && s->hits == 1) {                                  /* the first hit: its wings come off */
        set_aux_d(s, 0xFF);
        g->objs.marker_param = s->flags & 0xFF;
        spawn_object(g, d->x + 4, d->y, 88);
    } else if (r >= 0) {
        sk_slot *w = find_slot_by_type(g, 88);
        if (w) w->t0 = 1;                                           /* port guard: the original writes to whatever a1 holds */
    }
    if (object_offscreen(g, d->x, d->y, 160) < 0) free_object(g, s);
}

/* ---- type 88: the flyer's wings (slot 124): follow it, fly off when it dies ---- */
static void obj_wings_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x11926E */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->flags = (int16_t)(g->objs.marker_param & 1);
    s->vx = 0; s->vy = 0; s->t0 = 0;
    const uint8_t *f = res(g, 124);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 39, f, 1);                                  /* sub_107DF0 */
}
static void obj_wings_update(sk_game *g, sk_slot *s, int index) {             /* 0x1192AA */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    int wrapped = 0;
    step_animation(s, wings_script, 0, &wrapped);
    if (!(s->flags & 2)) {
        if (aux_c(s) == 0) {
            sk_slot *f = find_slot_by_type(g, 70);
            const sk_draw *fd = f ? slot_draw_record(g, f) : NULL;
            if (fd) { d->x = (int16_t)(fd->x + 4); d->y = fd->y; }   /* port guard: none, it stays */
        }
        if (s->t0) { set_aux_c(s, 0xFF); s->vy = -16; s->flags |= 4 | 2; }
    }
    if (object_offscreen(g, d->x, d->y, 160) < 0) free_object(g, s);
}

/* ---- type 31: the frog (slot 31): hops a fixed pattern, spits (type 111, slot 32) at the top of a hop ---- */
static void obj_frog_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x110FCE */
    init_object_slot(g, s, x, y - 5, s->type4);
    s->vx = 0; s->vy = -10; s->t0 = 10; s->t1 = 1;
    s->aux = 8;                                                     /* move.w #8,$c(a6): the pattern offset */
    s->flags |= 8 | 4;
    s->hits = 2;
    const uint8_t *f = res(g, 31);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 21, f, 1);                                  /* sub_107AE2 */
}
static void obj_frog_update(sk_game *g, sk_slot *s, int index) {              /* 0x11101A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (g->ball_kicked_flag) ball_kill_check(g, s, 12, 0x200);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    int v = s->vy;
    const uint8_t *script;
    if (v == 0 && (uint16_t)s->t0 >= 16) {                          /* the top of a high hop: spit */
        g->objs.child_frames = res(g, 32);
        g->objs.child_script = frog_spit_script;
        int x = d->x;
        g->objs.child_vx = 8;
        if (s->flags & 1) { g->objs.child_vx = (int16_t)(8 | 0x8000); x += 16; }
        g->objs.child_vy = -2;
        g->objs.child_w = 16; g->objs.child_h = 10;
        spawn_object(g, x, d->y, 111);
        script = frog_fall_script;
    } else if (v < 0 || (v == 0 && (int16_t)(s->t0 - 16) < 0)) {   /* bpl on the last compare's sign */
        v = -v;
        script = frog_rise_script;
    } else script = frog_fall_script;
    if (v <= 2) script = frog_top_script;
    step_animation_simple(s, script, s->flags);
    if (!(s->t0 > s->vy)) {                                         /* back at the launch speed: the next hop */
        s->vy = (int16_t)-s->t0;
        if (--s->t1 == 0) {
            s->aux = (int16_t)(s->aux + 8);
            int o = (uint16_t)s->aux / 2;
            if (o >= 24 || frog_hops[o] < 0) { o = 0; s->aux = 0; }
            s->vx = frog_hops[o];
            s->vy = frog_hops[o + 1];
            s->t0 = (int16_t)-frog_hops[o + 1];
            s->t1 = frog_hops[o + 2];
            s->flags &= ~1;
            if (frog_hops[o + 3]) s->flags |= 1;
        }
    }
    touch_player(g, d->x, d->y, 32, 20);
    ball_hits_object(g, s, 0, 0, 32, 30, 12, 0x200);
    despawn_if_far(g, s);
}

/* ---- type 111: a thrown blob (the child block), falls under gravity ---- */
static void obj_blob_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x10FE84 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 |= (int16_t)0x8000;                                   /* skip this frame's update */
    if (g->objs.child_vx & 0x8000) { g->objs.child_vx = (int16_t)(g->objs.child_vx & 0x7FFF); s->flags |= 1; }   /* bclr on the block */
    s->vx = g->objs.child_vx;
    s->vy = g->objs.child_vy;
    if (s->vy) s->flags |= 4;
    s->script = g->objs.child_script;
    set_draw_size_at(g, s, g->objs.child_w, g->objs.child_h, g->objs.child_frames);
}
static void obj_blob_update(sk_game *g, sk_slot *s, int index) {              /* 0x10FEDC */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->script && d->frames) step_animation_simple(s, s->script, s->flags);
    touch_player(g, d->x, d->y, d->w, d->h);
    free_if_far(g, s);
}

/* ---- type 51: RIVIERA2's boss (slot 69) and its arena: the wipe (96), the spirit (93), the lift (95),
   the shots (94) and the suction column (92) ---- */
static void music_slot(sk_game *g, int slot) { game_music_slot(g, slot); }   /* sub_103770 */
static bool once_bit(sk_slot *s, int bit) {                         /* bset.b #bit,aux.lo; bne */
    if (aux_c(s) & (1 << bit)) return false;
    set_aux_c(s, aux_c(s) | (1 << bit));
    return true;
}
static int y0_hi(const sk_slot *s) { return (uint16_t)s->y0 >> 8; }                            /* byte +0x6 */
static void clip_rise(sk_draw *d, int floor_y, int max_h, int words) {   /* the part above floor_y shows */
    int v = (int16_t)(floor_y - d->y);
    if (v <= 0) { d->dflags |= 4; return; }
    d->dflags &= (uint8_t)~4;
    if ((uint16_t)v >= (uint16_t)max_h) v = max_h;
    d->h = (int16_t)v;
    d->blit_size = (int16_t)(v << 6 | words);
}
static void lift_target(sk_game *g, int y) { sk_slot *l = find_slot_by_type(g, 95); if (l) l->t0 = (int16_t)y; }
static void lift_hold(sk_game *g) {                                  /* the lift stays where it is */
    sk_slot *l = find_slot_by_type(g, 95);
    const sk_draw *ld = l ? slot_draw_record(g, l) : NULL;
    if (ld) l->t0 = ld->y;
}

static void obj_rboss_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x11505E */
    init_object_slot(g, s, x - 48, y + 16, s->type4);
    s->vx = 0; s->vy = 0; s->hits = 15;
    const uint8_t *f = res(g, 69);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 64, f, 1);                                  /* frames_64x64 */
    slot_draw_record(g, s)->dflags |= 4;
    g->push_x = s->x - 0x80;
    spawn_object(g, 0, 0, 96);
    spawn_object(g, s->x + 8, s->y, 95);
    save_backdrop(g);
    music_slot(g, 1);
}
static void obj_rboss_update(sk_game *g, sk_slot *s, int index) {             /* 0x1150CA */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (aux_c(s) & 2) {                                             /* woken by the wipe */
        int changed = 0, code = script_step(g, s, rboss_phases, rboss_motions, &changed);
        bool tail = true;
        switch (code) {
        case 0x00: s->x0 = (int16_t)(s->y - 0x6F); lift_target(g, s->x0 + 0x40); script_end_next(s, rboss_phases); break;
        case 0x04: lift_target(g, s->y); s->x0 = (int16_t)(s->y - 0x40); script_end_next(s, rboss_phases); break;
        case 0x08:                                                  /* rise or sink to x0 */
            if (s->x0 == d->y) script_end_next(s, rboss_phases);
            else if ((uint16_t)s->x0 < (uint16_t)d->y) d->y--;
            else d->y++;
            break;
        case 0x0C:                                                  /* shoot: a shot every 20 frames, kinds 0 1 2 .. */
            if (once_bit(s, 0)) { play_sfx(g, 25); s->x0 = 0; s->y0 = 0; }
            if (--s->x0 < 0) {
                g->objs.marker_param = y0_hi(s);
                spawn_object(g, d->x + 24, d->y + 8, 94);
                s->x0 = 20;
                s->y0 = (int16_t)(((y0_hi(s) + 2) & 0xFF) << 8 | (s->y0 & 0xFF));
            }
            break;
        case 0x18: s->flags ^= 1; script_end_next(s, rboss_phases); break;
        case 0x20:                                                  /* turn to the kid */
            script_end_next(s, rboss_phases);
            s->flags |= 1;
            if (!((uint16_t)(d->x + 32) < (uint16_t)g->player.x)) s->flags &= ~1;
            break;
        case 0x1C: {                                                /* suction */
            int pull = (s->flags & 1) ? -3 : 3;
            g->player.carry_dx = (int16_t)pull;
            g->bw.carry_dx = pull;
            if (once_bit(s, 0)) { g->objs.marker_param = s->flags & 0xFF; spawn_object(g, s->x, s->y - 0x58, 92); }
            break;
        }
        case 0x28: case 0x30: tail = false; break;
        case 0x2C:
            tail = false;
            if (once_bit(s, 0)) {
                g->score_value = 0x20000;
                add_score(g, 0x20000);
                spawn_object(g, d->x + 32, d->y + 24, 113);
                play_sfx(g, 17);
                g->shake_amplitude = 6;
                music_slot(g, 2);
            }
            break;
        case 0x34:
            tail = false;
            if (once_bit(s, 0)) { g->level_outcome = 0x81; g->level_number++; g->curtain_flags |= 0x10; }
            break;
        default: break;                                             /* 0x10 0x14 0x24 */
        }
        if (!tail) {
            if (object_offscreen(g, s->x, s->y, 64) < 0) free_object(g, s);
            return;
        }
    }
    clip_rise(d, s->y, 64, 5);
    touch_player(g, d->x + 4, d->y, 40, 56);
    if (g->player.dying && once_bit(s, 2)) {                        /* the kid lost: it gloats */
        restore_backdrop(g);
        lift_hold(g);
        script_start(s, 18, rboss_phases, rboss_motions);
    }
    ball_hits_object_simple(g, s, 8, 12, 48, 52);
    if (!s->hits && once_bit(s, 3)) {
        d->dflags &= (uint8_t)~2;
        lift_hold(g);
        script_start(s, 19, rboss_phases, rboss_motions);
    }
    if (object_offscreen(g, s->x, s->y, 64) < 0) free_object(g, s);
}

static void obj_wipe_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x115532 */
    (void)x; (void)y;
    init_object_slot_nodraw(g, s, g->world.bg_max_x + 16, 0, s->type4);
    const uint8_t *f = res(g, 71);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 304, 16, f, 0);                                 /* sub_1080DC */
    slot_draw_record(g, s)->dflags |= 0x10;                         /* backdrop coordinates */
}
static void obj_wipe_update(sk_game *g, sk_slot *s, int index) {              /* 0x115560 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d || g->camera_x != g->push_x) return;                     /* the camera at the arena wall */
    int before = d->y;
    d->y++;
    if ((before & 0xFFF0) != (d->y & 0xFFF0)) paint_bg_cells(g, d->x, d->y, 8, 19);
    int v = (int16_t)(d->y - s->y);
    if (v == 0x80) spawn_object(g, 0, 0, 93);
    if ((uint16_t)v >= 0xD0) {
        sk_slot *boss = find_slot_by_type(g, 51);
        if (boss) set_aux_c(boss, aux_c(boss) | 2);
        free_object(g, s);
    }
}

static void obj_lift_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x1155CA */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    const uint8_t *f = res(g, 70);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 16, f, 1);                                  /* sub_107F00 */
    sk_draw *d = slot_draw_record(g, s);
    d->y = (int16_t)(d->y + 0x40);
    s->t0 = d->y;
    d->dflags |= 4;
}
static void obj_lift_update(sk_game *g, sk_slot *s, int index) {              /* 0x1155F8 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    s->t1 = d->y;
    int left = 0, right = 0;
    bool moved = false;
    if (d->y != s->t0) {
        moved = true;
        if ((uint16_t)d->y >= (uint16_t)s->t0) { d->y--; left = 0x1BB; right = 0x1BC; }   /* rising: solid */
        else d->y++;                                                /* sinking: empty */
    }
    if (moved && (d->y & 0xFFF0) != (s->t1 & 0xFFF0) && (int16_t)(d->y + 8) < s->y) {
        set_tile_at(g, d->x + 8, d->y + 8, left, 0);
        set_tile_at(g, d->x + 24, d->y + 8, right, 0);
    }
    step_animation_simple(s, lift_script, 0);
    clip_rise(d, s->y, 16, 5);
    if (object_offscreen(g, s->x, s->y, 64) < 0) {
        fill_cells(g, s->x + 16, s->y - 32, 0, 0, 1);
        fill_cells(g, s->x + 32, s->y - 32, 0, 0, 1);
        free_object(g, s);
    }
}

static void obj_shot_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x1156DC */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->t0 = (int16_t)((g->objs.marker_param & 0xFF) >> 1);
    s->flags |= 4;
    sk_slot *boss = find_slot_by_type(g, 51);
    int dy = (int16_t)((boss ? boss->y : s->y) - 16 - s->y);       /* port guard: no boss, level */
    int dx = (int16_t)(g->player.x - s->x - 8);
    int a = atan2_256(dx, dy) * 4;
    int c = cos_1024(a);
    s->flags |= 1;
    if (c < 0) { s->flags &= ~1; c = -c; }
    s->vx = (int16_t)((uint16_t)c >> 6);
    s->vy = (int16_t)((int16_t)sin_1024(a) >> 6);
    const uint8_t *f = res(g, 72);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 16, f, 1);
    slot_draw_record(g, s)->dflags |= 4;
}
static void obj_shot_update(sk_game *g, sk_slot *s, int index) {              /* 0x11576C */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity_halfstep(g, s);
    int k = s->t0 == 0 ? 0 : s->t0 == 1 ? 1 : 2;
    int wrapped = 0;
    step_animation(s, shot_scripts[k], 0, &wrapped);
    d->dflags &= (uint8_t)~4;
    touch_player(g, d->x, d->y, 16, 16);
    if (object_offscreen(g, d->x, d->y, 64) < 0 || object_offscreen(g, s->x, s->y, 64) < 0) free_object(g, s);
}

static void obj_spirit_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x1157F4 */
    (void)x; (void)y;
    init_object_slot_nodraw(g, s, g->world.bg_max_x + 0x96, 0x40, s->type4);
    s->vx = 6; s->vy = 0;
    s->flags |= 1 | 4;
    set_aux_c(s, 3);
    s->t0 = 0x113; s->t1 = 0;
    const uint8_t *f = res(g, 73);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 31, f, 0);                                  /* sub_107BD0 */
    slot_draw_record(g, s)->dflags |= 0x10 | 4;                     /* backdrop coordinates, hidden */
}
static void obj_spirit_update(sk_game *g, sk_slot *s, int index) {            /* 0x115852 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    int wrapped = 0;
    switch (aux_c(s)) {
    case 3:                                                         /* appearing */
        d->dflags &= (uint8_t)~4;
        step_animation(s, spirit_in_script, 0, &wrapped);
        if (wrapped) { set_frame2(s, 0, 0, 0); reset_animation(s); set_aux_c(s, 0); }
        break;
    case 4:                                                         /* fading: then gone */
        step_animation(s, spirit_out_script, 0, &wrapped);
        if (wrapped) { free_object(g, s); return; }
        break;
    default:
        apply_velocity_nograv(s);
        if (aux_c(s) == 1) {                                        /* slowing to turn */
            if (--s->vx == 0) {
                set_aux_c(s, 2);
                s->flags ^= 1;
                s->t1 = (int16_t)((s->t1 + 1) & 3);
                s->vy = spirit_vy[s->t1];
            }
        } else if (aux_c(s) == 2) {                                 /* speeding up */
            if (++s->vx == 6) set_aux_c(s, 0);
        } else {
            int dx = (int16_t)(d->x - s->x);
            if (dx < 0) dx = -dx;
            if ((uint16_t)dx >= 50) { set_aux_c(s, 1); s->vx--; }
        }
        break;
    }
    if (--s->t0 == 0) set_aux_c(s, 4);
}

static void obj_suction_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x115950 */
    x -= 0x70;
    if (!(g->objs.marker_param & 1)) x += 0x110;
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->t0 = 150; s->t1 = 0;
    const uint8_t *f = res(g, 74);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 64, f, 1);                                  /* sub_107D02 */
}
static void obj_suction_update(sk_game *g, sk_slot *s, int index) {           /* 0x11598E */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    d->y = (int16_t)(d->y + s->t1);
    s->t1 = suction_jitter[g->frame_ticks & 7];
    d->y = (int16_t)(d->y - s->t1);
    step_animation_simple(s, suction_script, 0);
    if (--s->t0 == 0) { free_object(g, s); return; }
    if (touch_player(g, d->x, d->y, 16, 64) == -1) g->player.crushed = 0xFF;
    if (object_offscreen(g, s->x, s->y, 64) < 0) free_object(g, s);
}

void obj_italy_install(void) {
    object_init_table[31] = obj_frog_init;             object_update_table[31] = obj_frog_update;
    object_init_table[36] = obj_pendulum_init;         object_update_table[36] = obj_pendulum_update;
    object_init_table[37] = obj_jumper_init;           object_update_table[37] = obj_jumper_update;
    object_init_table[38] = obj_swing_platform_init;   object_update_table[38] = obj_swing_platform_update;
    object_init_table[43] = obj_strider_init;          object_update_table[43] = obj_strider_update;
    object_init_table[44] = obj_bouncer_init;          object_update_table[44] = obj_bouncer_update;
    object_init_table[48] = obj_rock_init;             object_update_table[48] = obj_rock_update;
    object_init_table[49] = obj_hopper49_init;         object_update_table[49] = obj_hopper49_update;
    object_init_table[51] = obj_rboss_init;            object_update_table[51] = obj_rboss_update;
    object_init_table[64] = obj_thrower_init;          object_update_table[64] = obj_thrower_update;
    object_init_table[68] = obj_thrower68_init;        object_update_table[68] = obj_thrower68_update;
    object_init_table[70] = obj_flyer_init;            object_update_table[70] = obj_flyer_update;
    object_init_table[88] = obj_wings_init;            object_update_table[88] = obj_wings_update;
    object_init_table[92] = obj_suction_init;          object_update_table[92] = obj_suction_update;
    object_init_table[93] = obj_spirit_init;           object_update_table[93] = obj_spirit_update;
    object_init_table[94] = obj_shot_init;             object_update_table[94] = obj_shot_update;
    object_init_table[95] = obj_lift_init;             object_update_table[95] = obj_lift_update;
    object_init_table[96] = obj_wipe_init;             object_update_table[96] = obj_wipe_update;
    object_init_table[100] = obj_rider_init;           object_update_table[100] = obj_rider_update;
    object_init_table[111] = obj_blob_init;            object_update_table[111] = obj_blob_update;
}
