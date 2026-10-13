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
static const uint8_t strider_walk_script[] = { 0, 4, 0, 4, 0, 1, 2, 3, 5, 6, 7, 8 };          /* 0x112768 */
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

/* ---- type 38: the swinging platform (a param resource) on a chain of slot 46 links ---- */
static int16_t hi(int32_t v) { return (int16_t)(v >> 16); }
static int32_t with_hi(int32_t v, int h) { return (int32_t)((uint32_t)(uint16_t)h << 16 | ((uint32_t)v & 0xFFFF)); }
static int t0_hi(const sk_slot *s) { return (uint16_t)s->t0 >> 8; }                              /* byte +0x14 */
static void set_t0_hi(sk_slot *s, int v) { s->t0 = (int16_t)((v & 0xFF) << 8 | (s->t0 & 0xFF)); }

static void obj_swing_platform_init(sk_game *g, sk_slot *s, int x, int y) {    /* 0x117A04 */
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
    if (!pf || !sec || !links) { if (pf) free_platform(g, s); free_object(g, s); return; }   /* port guard */
    const uint8_t *b = g->res->param[(p & 0xC0) >> 6].bytes;
    pf->px = (int32_t)((uint32_t)(uint16_t)x << 16);
    pf->py = (int32_t)((uint32_t)(uint16_t)y << 16);
    pf->pspeed = 0x1E; pf->pangle = 0;
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
}
static void obj_swing_platform_update(sk_game *g, sk_slot *s, int index) {     /* 0x117B08 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    sk_platform *pf = slot_platform(g, s);
    if (!pf) return;
    int r = s->x0;
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

void obj_italy_install(void) {
    object_init_table[36] = obj_pendulum_init;         object_update_table[36] = obj_pendulum_update;
    object_init_table[37] = obj_jumper_init;           object_update_table[37] = obj_jumper_update;
    object_init_table[38] = obj_swing_platform_init;   object_update_table[38] = obj_swing_platform_update;
    object_init_table[43] = obj_strider_init;          object_update_table[43] = obj_strider_update;
    object_init_table[100] = obj_rider_init;           object_update_table[100] = obj_rider_update;
}
