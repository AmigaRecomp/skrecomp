/* Japan's object types, transcribed from the listing. */
#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/obj_japan.h"
#include "game/obj_italy.h"
#include "game/platforms.h"
#include "game/resources.h"
#include "game/world.h"

static const uint8_t insect_script[] = { 0, 4, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7 };                 /* 0x11088E */
static const uint8_t th67_idle_script[] = { 0, 2, 0, 12, 0, 1, 2, 3 };                         /* 0x118C1C */
static const uint8_t th67_walk_script[] = { 0, 6, 0, 6, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };   /* 0x118C24 */
static const uint8_t th67_throw_script[] = { 0, 8, 0, 4, 18, 19, 20, 21, 21, 21, 21, 21, 22, 23, 24, 25, 25, 25, 25, 25 };   /* 0x118C34 */
static const uint8_t th67_missile_script[] = { 0, 2, 0, 10, 0, 0, 1, 1 };                      /* 0x118C48 */
static const uint8_t th67_debris_script[] = { 0, 2, 0, 10, 0, 0, 0, 0 };                       /* 0x118C50 */

static const uint8_t crab_data[64] = {                                                         /* 0x10FC4E: the scripts overlap */
    0, 4, 0, 2, 0, 1, 0, 6, 0, 6, 0, 4, 0, 1, 2, 3, 4, 5, 0, 2, 0, 6, 0, 1, 0, 6, 0, 3, 0, 6, 1, 0,
    6, 6, 0, 1, 0, 4, 0, 6, 2, 3, 4, 5, 7, 8, 9, 10, 0, 6, 0, 4, 0, 2, 1, 0, 2, 1, 0, 2, 1, 0, 2, 1 };
#define CRAB_DIE (crab_data + 0)         /* 0x10FC4E */
#define CRAB_BURST (crab_data + 8)       /* 0x10FC56 */
#define CRAB_WAKE (crab_data + 18)       /* 0x10FC60 */
#define CRAB_TURN (crab_data + 26)       /* 0x10FC68 */
#define CRAB_WALK (crab_data + 36)       /* 0x10FC72 */
#define CRAB_BITE (crab_data + 48)       /* 0x10FC7E */

static const uint8_t popper_data[0x60] = {                                                      /* 0x1118B8: the spots, then the scripts */
    0, 0, 0, 0, 255, 216, 0, 14, 0, 0, 0, 8, 0, 48, 0, 20, 255, 224, 0, 12, 255, 240, 0, 8, 0, 30, 0, 12, 0, 16, 0, 8,
    0, 1, 0, 2, 0, 1, 0, 13, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0, 0, 17, 0, 4, 16, 13, 14, 15,
    16, 13, 14, 15, 16, 13, 14, 15, 16, 13, 14, 15, 16, 0, 0, 13, 0, 6, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0 };
#define POPPER_PIECE (popper_data + 0x20)   /* 0x1118D8 */
#define POPPER_RISE (popper_data + 0x26)    /* 0x1118DE */
#define POPPER_THROW (popper_data + 0x38)   /* 0x1118F0 */
#define POPPER_SINK (popper_data + 0x4E)    /* 0x111906 */

static const uint8_t walker80_script[] = { 0, 4, 0, 6, 4, 5, 6, 7, 0, 1, 2, 3 };              /* 0x119E88 */

static const uint8_t bubble_script[] = { 0, 4, 0, 4, 0, 1, 2, 3 };                            /* 0x1104CC */

static const uint8_t kicker_walk_script[] = { 0, 6, 0, 6, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13 };   /* 0x11131E */
static const uint8_t kicker_kick_script[] = { 0, 5, 0, 9, 0, 0, 14, 2, 14, 2, 15, 248, 0, 0, 7, 0, 16, 254, 16, 254, 17, 8, 7, 0 };   /* 0x11132E: (frame, dx) */
static const uint8_t fireball_data[0x3A] = {                                                     /* 0x1114F0: the scripts overlap */
    0, 3, 0, 4, 0, 1, 2, 3, 0, 1, 2, 3, 0, 15, 0, 2, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,
    4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 0, 4, 0, 3, 0, 1, 2, 3, 0, 1, 2, 3 };
#define FIREBALL_FLY (fireball_data + 0)      /* 0x1114F0 */
#define FIREBALL_FIRE (fireball_data + 0x0C)  /* 0x1114FC */
#define FIREBALL_BURST (fireball_data + 0x2E) /* 0x11151E */

static const uint8_t fb_4e7a[] = { 0, 1, 0, 0, 0, 0 };   /* 0x114E7A */
static const uint8_t fb_4e80[] = { 0, 1, 0, 0, 13, 13 };   /* 0x114E80 */
static const uint8_t fb_4e86[] = { 0, 3, 0, 6, 2, 3, 4, 2, 3, 4 };   /* 0x114E86 */
static const uint8_t fb_4e90[] = { 0, 1, 0, 0, 1, 1 };   /* 0x114E90 */
static const uint8_t fb_4e96[] = { 0, 1, 0, 0, 5, 5 };   /* 0x114E96 */
static const uint8_t fb_4e9c[] = { 0, 3, 0, 10, 5, 5, 6, 5, 5, 7 };   /* 0x114E9C */
static const uint8_t fb_4ea6[] = { 0, 4, 0, 7, 19, 20, 21, 22, 15, 16, 17, 18 };   /* 0x114EA6 */
static const uint8_t fb_4eb2[] = { 0, 3, 0, 20, 12, 13, 13, 12, 13, 13 };   /* 0x114EB2 */
static const uint8_t fb_4ebc[] = { 0, 3, 0, 10, 5, 9, 11, 5, 10, 11 };   /* 0x114EBC */
static const uint8_t fb_4ec6[] = { 0, 1, 0, 0, 11, 11 };   /* 0x114EC6 */
static const uint8_t fb_4ecc[] = { 0, 4, 0, 4, 25, 25, 26, 26, 23, 23, 24, 24 };   /* 0x114ECC */
static const uint8_t fb_4ed8[] = { 0, 4, 0, 6, 27, 28, 29, 30, 27, 28, 29, 30 };   /* 0x114ED8 */
static const uint8_t fb_4ee4[] = { 0, 3, 0, 12, 12, 13, 13, 12, 13, 13 };   /* 0x114EE4 */
static const uint8_t fb_4eee[] = { 0, 1, 0, 0, 14, 14 };   /* 0x114EEE */
static const sk_phase fboss_phases[28] = {                                                  /* 0x114D4C */
    { 150, 0, 4 }, { 25, 10, 8 }, { 17, 20, 12 }, { 30, 170, 16 }, { 90, 50, 20 }, { 30, 60, 24 }, { 180, 40, 28 }, { 30, 60, 32 }, { 90, 50, 36 }, { 25, 30, 40 }, { 120, 80, 44 }, { 25, 170, 48 }, { 90, 50, 52 }, { 2, 30, 56 }, { 30, 60, 24 }, { 15, 110, 64 }, { 65535, 130, 28 }, { 65535, 140, 72 }, { 65535, 120, 76 }, { 25, 30, 16 }, { 25, 150, 84 }, { 200, 160, 88 }, { 65535, 180, 0 }, { 25, 30, 96 }, { 10, 100, 100 }, { 40, 70, 104 }, { 40, 90, 108 }, { 65535, 130, 28 },
};
static const sk_motion fboss_motions[19] = {                                                /* 0x114DBC */
    { 0, 0, fb_4e7a, 0x00 }, { 0, 0, fb_4e90, 0x04 }, { 0, 0, fb_4e86, 0x08 }, { 0, 0, fb_4e96, 0x0C }, { 2, 0, fb_4ea6, 0x10 }, { 2, 0, fb_4ea6, 0x14 }, { 0, 0, fb_4e9c, 0x18 }, { 2, -20, fb_4eb2, 0x1C }, { 0, 0, fb_4ebc, 0x20 }, { 0, 0, fb_4e80, 0x24 }, { 0, 0, fb_4ec6, 0x28 }, { 0, 0, fb_4ecc, 0x2C }, { 0, 0, fb_4ed8, 0x30 }, { 0, 0, fb_4ea6, 0x34 }, { 2, 0, fb_4ea6, 0x38 }, { 0, -12, fb_4ee4, 0x3C }, { 0, 0, fb_4eee, 0x40 }, { 0, 0, fb_4e96, 0x44 }, { 0, 0, fb_4eee, 0x48 },
};
static const uint8_t fboss_pair_script[] = { 0, 2, 0, 4, 0, 0, 1, 1 };                         /* 0x114D44 */
static const int16_t rock_delays[4] = { 0x50, 0x6E, 0x14, 0x32 };                           /* 0x114F42 */

static const uint8_t warning_script[] = { 0, 8, 0, 5, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF };   /* 0x111646 */
static const uint8_t passer_steps[] = { 5, 20, 4, 20, 3, 20, 0xFF, 0, 2, 0xEC, 1, 0xEC, 0, 0xEC, 0, 0xEC, 0, 0xEC, 0, 0xEC };   /* 0x111728: (frame, dx) */

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static bool need(sk_game *g, sk_slot *s, const uint8_t *f) { if (f) return true; free_object(g, s); return false; }
static void reset_animation(sk_slot *s) { s->anim_tick = 0; s->anim_index = 0; }   /* 0x10DF62 */
static int iabs(int v) { return v < 0 ? -v : v; }

/* ---- type 25: the insect (slot 23): homes on the kid, easing its speed down and up at each turn ---- */
static void obj_insect_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x1106BC */
    init_object_slot(g, s, x, y, s->type4);
    s->t0 = 8;
    s->flags |= 0xC;
    s->vx = s->t0; s->vy = s->t0; s->t1 = s->t0;
    s->hits = 1;
    const uint8_t *f = res(g, 23);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 27, f, 1);                                  /* frames_32x27 */
}
static void obj_insect_update(sk_game *g, sk_slot *s, int index) {            /* 0x1106FC */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (g->ball_kicked_flag) ball_kill_check(g, s, 8, 0x300);
    if (s->flags & 2) { apply_velocity(s); despawn_if_far(g, s); return; }
    step_animation_simple(s, insect_script, s->flags);
    int tx = g->player.x - 16; if (tx < 0) tx = 0;
    int ty = g->player.y - 50; if (ty < 0) ty = 0;
    int a = aux_c(s);
    if (a & 0x20) ty += 30;
    if (!(a & 1)) {                                                 /* x: turn when the kid is behind */
        int dx = (int16_t)(tx - d->x);
        if (dx < 0) { if (s->flags & 1) a |= 1; }
        else if (!(s->flags & 1)) a |= 1;
    } else if (!(g->frame_ticks & 3)) {
        if (!(a & 2)) {
            if (--s->vx == 0) {
                a |= 2;
                s->flags ^= 1;
                a &= ~0x20;
                set_aux_d(s, aux_d(s) + 1);
                if (aux_d(s) >= 6) { a |= 0x20; set_aux_d(s, 0); }  /* every sixth turn it flies 30 px lower */
            }
        } else if (++s->vx == s->t0) a &= ~3;
    }
    if (!(a & 4)) {                                                 /* y: the same with t1 and bit 4 = up */
        int dy = (int16_t)(ty - d->y);
        if (dy >= 0) { if (a & 0x10) a |= 4; }
        else if (!(a & 0x10)) a |= 4;
    } else if (!(a & 8)) {
        if (--s->t1 == 0) { a |= 8; a ^= 0x10; }
    } else if (++s->t1 == s->t0) a &= ~0xC;
    set_aux_c(s, a);
    s->vy = (int16_t)((a & 0x10) ? -s->t1 : s->t1);
    touch_player(g, d->x + 4, d->y + 4, 24, 16);
    ball_hits_object(g, s, 4, 4, 24, 16, 8, 0x300);
    apply_velocity_nograv(s);
    despawn_if_far(g, s);
}

/* ---- type 67: the thrower (slots 99, 100 its missile type 99, 101 the debris type 107) ---- */
static void obj_th67_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x1189EC */
    init_object_slot(g, s, x, y - 32, s->type4);
    s->vx = 0; s->vy = 0; s->t0 = 0;
    s->flags |= 8;
    s->hits = 3;
    const uint8_t *f = res(g, 99);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 48, f, 1);                                  /* sub_107E12 */
    sk_draw *d = slot_draw_record(g, s);
    if (d && (int16_t)(d->x + 24 - g->player.x) < 0) s->flags |= 1;
}
static void obj_th67_update(sk_game *g, sk_slot *s, int index) {              /* 0x118A3C */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    int wrapped = 0;
    switch (aux_c(s)) {
    case 0:                                                         /* idle until the kid is within 120 */
        step_animation(s, th67_idle_script, s->flags, &wrapped);
        if ((uint16_t)iabs((int16_t)(d->x - g->player.x)) >= 0x78) break;
        reset_animation(s);
        set_aux_c(s, 1);
        s->vx = 2;
        /* fall through */
    case 1: {                                                       /* patrol */
        step_animation(s, th67_walk_script, s->flags, &wrapped);
        if ((uint16_t)iabs((int16_t)(d->x - s->x)) >= 0x96 || ground_ahead(g, s, s->flags)) {
            set_aux_c(s, 3); s->t1 = 4; s->vx = 0;
            set_frame2(s, 16, 16, 0);
            break;
        }
        if (s->t0) { s->t0--; break; }
        int side = s->flags, k = (int16_t)(d->x - g->player.x);
        if (k < 0) { side ^= 1; k = -k; }
        if ((side & 1) || (uint16_t)k >= 0x50) break;
        reset_animation(s);
        set_aux_c(s, 2);
        s->vx = 0;
    }   /* fall through */
    case 2: {                                                       /* throw */
        int changed = step_animation(s, th67_throw_script, s->flags, &wrapped);
        if (changed && s->anim_index == 2)
            spawn_child(g, s, d->x + 24, d->y + 16, 99, 24, res(g, 100), th67_missile_script, 32, 6, 10, 2);
        if (wrapped) {
            reset_animation(s);
            s->t0 = 40;
            set_aux_c(s, 1);
            s->vx = 2;
            set_frame2(s, 0, 2, s->flags);
        }
        break;
    }
    case 3:                                                         /* a pause at the turn */
        if (--s->t1) break;
        s->vx = 2;
        s->flags ^= 1;
        set_aux_c(s, 1);
        break;
    default: break;
    }
    touch_player(g, d->x + 8, d->y, 32, 48);
    if (ball_hits_object(g, s, 0, 0, 48, 48, 17, 0x750) >= 0)
        spawn_child(g, s, d->x, d->y + 16, 107, 0, res(g, 101), th67_debris_script, 32, 28, 0, -14);
    despawn_if_far(g, s);
}

/* ---- type 107: a one-shot piece from the child block, under gravity, harmless ---- */
static void obj_piece_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x10FF08 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 |= (int16_t)0x8000;
    if (g->objs.child_vx & 0x8000) { g->objs.child_vx = (int16_t)(g->objs.child_vx & 0x7FFF); s->flags |= 1; }
    s->vx = g->objs.child_vx;
    s->vy = g->objs.child_vy;
    if (s->vy) s->flags |= 4;
    s->script = g->objs.child_script;
    set_draw_size_at(g, s, g->objs.child_w, g->objs.child_h, g->objs.child_frames);
}
static void obj_piece_update(sk_game *g, sk_slot *s, int index) {             /* 0x10FF60 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->script && d->frames) step_animation_simple(s, s->script, s->flags);
    free_if_far(g, s);
}

/* ---- type 6: the wheel platform: a chain platform turning full circles at (param bits 1..3) + 1 ---- */
static void obj_wheel_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x1177F6 */
    sk_platform *pf = chain_platform_init(g, s, x, y);
    if (pf) pf->pspeed = (uint8_t)(((g->objs.marker_param & 0xE) >> 1) + 1);
}
static void obj_wheel_update(sk_game *g, sk_slot *s, int index) {             /* 0x117904 */
    (void)index;
    sk_platform *pf = slot_platform(g, s);
    if (!pf) return;
    pf->pangle = (uint8_t)(pf->pangle + pf->pspeed);
    chain_platform_place(g, s, pf);
}

/* ---- type 18: the crab (slots 14; 15 the pieces, 16 the burst, 17 the bite): wakes on the kid's level, bites at close range ---- */
static void obj_crab_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x10F7FE */
    init_object_slot(g, s, x - 16, y - 16, s->type4);
    s->aux = 0;
    if (g->objs.marker_param & 1) set_aux_d(s, 0xFF);                /* patrols 80 px instead of to the edges */
    if (g->objs.marker_param & 2) s->y = (int16_t)(s->y + 8);
    s->vx = 0; s->vy = 0; s->hits = 3;
    const uint8_t *f = res(g, 14);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
}
static void crab_explode(sk_game *g, sk_slot *s, sk_draw *d) {      /* 0x10FAC4 */
    sk_objects *o = &g->objs;
    play_sfx(g, 0x13);
    g->score_value = 0x1000;
    spawn_object(g, d->x + ((uint16_t)d->w >> 1), d->y + ((uint16_t)d->h >> 1), 113);
    o->child_script = CRAB_BURST; o->child_w = 32; o->child_h = 22; o->child_frames = res(g, 16);
    o->child_vx = 0; o->child_vy = 0;
    spawn_object(g, d->x, d->y + 8, 115);
    const uint8_t *pieces = res(g, 15);
    o->child_frames = pieces ? pieces + 0x280 : NULL;
    o->child_vx = 0; o->child_vy = -14; o->child_w = 16; o->child_h = 16;
    spawn_object(g, d->x + 8, d->y, 116);
    static const struct { int dx, dy, vx, vy; } k[4] = { { 0, 0, -6, -8 }, { 16, 0, 6, -8 }, { 0, 16, -4, -6 }, { 16, 16, 4, -6 } };
    for (int i = 0; i < 4; i++) {
        o->child_frames = pieces ? pieces + 0xA0 * i : NULL;
        o->child_vx = (int16_t)k[i].vx; o->child_vy = (int16_t)k[i].vy;
        spawn_object(g, d->x + k[i].dx, d->y + k[i].dy, 116);
    }
    g->flash_timer = 16;
    g->flash_table = 0;                                             /* 0x3012BA, or 0x30169A in the train's dark (sub_1049C8) */
    if (g->train_fade < 0 || (g->train_fade == 0 && (g->train_flags & 0x20))) g->flash_table = 1;
    g->train_fade = 0;
    free_object_deferred(g, s);
}
static void obj_crab_update(sk_game *g, sk_slot *s, int index) {              /* 0x10F850 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    const sk_player *p = &g->player;
    bool die = false;
    if (g->ball_kicked_flag && !(s->flags & 2) && point_offscreen(g, d->x, d->y, d->w, d->h) >= 0) {   /* sub_10E156 */
        add_score(g, 0x1000);
        die = true;
    }
    if (!die && !(s->flags & 2)) {
        int wrapped = 0;
        switch (aux_c(s)) {
        case 0: {                                                   /* asleep: wakes when the kid is level and within 150 */
            s->flags &= ~1;
            int dx = (int16_t)(d->x + 16 - p->x);
            if (dx < 0) { dx = -dx; s->flags |= 1; }
            if ((uint16_t)dx >= 0x96) break;
            if ((uint16_t)iabs((int16_t)(p->y + 8 - d->y)) >= 16) break;
            set_aux_c(s, 1);
        }   /* fall through */
        case 1:
            step_animation(s, CRAB_WAKE, s->flags, &wrapped);
            if (wrapped) { reset_animation(s); set_aux_c(s, 2); s->vx = 4; }
            break;
        case 2: {
            apply_velocity(s);
            step_animation_simple(s, CRAB_WALK, s->flags);
            if ((uint16_t)++s->t1 >= 16 && (uint16_t)iabs((int16_t)(p->y + 8 - d->y)) < 16
                && (uint16_t)iabs((int16_t)(p->x - d->x - 16)) < 0x30) {
                set_aux_c(s, 4); s->t1 = 0;
                goto bite;
            }
            if ((uint16_t)++s->t0 < 25) break;
            bool turn = aux_d(s) ? (uint16_t)iabs((int16_t)(d->x - s->x)) >= 0x50 : ground_ahead(g, s, s->flags) != 0;
            if (!turn) break;
            reset_animation(s);
            set_aux_c(s, 3);
        }   /* fall through */
        case 3:
            step_animation(s, CRAB_TURN, s->flags, &wrapped);
            if (wrapped) { reset_animation(s); s->flags ^= 1; set_aux_c(s, 2); s->t0 = 0; }
            break;
        case 4:
        bite:
            s->t1++;
            if (s->t1 == 25) {
                sk_objects *o = &g->objs;
                o->child_frames = res(g, 17); o->child_script = CRAB_BITE;
                o->child_w = 32; o->child_h = 10;
                o->child_vx = 0; o->child_vy = 0;
                spawn_object(g, d->x + ((s->flags & 1) ? 32 : -32), d->y + 10, 114);
            } else if ((uint16_t)s->t1 >= 49) { s->t1 = 0; set_aux_c(s, 2); }
            break;
        default: break;
        }
        touch_player(g, d->x, d->y, 32, 32);
        int r = ball_hits_object_quiet(g, s, 0, 0, 32, 32, 0, 0x1000);
        if (r == 0) die = true;
        else { if (r == -2 && aux_c(s) == 0) set_aux_c(s, 1); despawn_if_far(g, s); return; }
    }
    if (die) { s->flags |= 2; s->t0 = 0; reset_animation(s); s->timer = 8; }
    if ((int8_t)--s->timer < 0) { d->dflags &= (uint8_t)~2; s->timer = 15; play_sfx(g, 0x14); }   /* 0x10FA8E: it smoulders */
    if ((uint16_t)++s->t0 < 50) {
        int wrapped = 0;
        step_animation(s, CRAB_DIE, 0, &wrapped);
        despawn_if_far(g, s);
        return;
    }
    crab_explode(g, s, d);
}

/* ---- type 33: the popper (slot 39): comes up at one of eight spots, throws two pieces (type 111), sinks ---- */
static void obj_popper_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x11173E */
    init_object_slot(g, s, x, y - 16, s->type4);
    s->flags |= 1;
    s->vx = 0; s->vy = 0; s->t0 = 4;
    const uint8_t *f = res(g, 39);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 28, f, 1);                                  /* sub_107B8C */
    slot_draw_record(g, s)->dflags |= 4;
}
static void obj_popper_update(sk_game *g, sk_slot *s, int index) {            /* 0x111778 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    int wrapped = 0;
    switch (aux_c(s)) {
    case 0: {
        if (--s->t0) break;
        set_aux_c(s, 1);
        d->dflags &= (uint8_t)~4;
        const uint8_t *q = popper_data + (game_random(g) & 7) * 4;
        d->x = (int16_t)(s->x + (int16_t)(q[0] << 8 | q[1]));
        d->y = (int16_t)(s->y + (int16_t)(q[2] << 8 | q[3]));
        reset_animation(s);
    }   /* fall through */
    case 1:
        step_animation(s, POPPER_RISE, 0, &wrapped);
        if (!wrapped) break;
        set_aux_c(s, 2);
        reset_animation(s);
        wrapped = 0;
        /* fall through */
    case 2: {
        if (step_animation(s, POPPER_THROW, 0, &wrapped)) {
            sk_objects *o = &g->objs;
            int x = d->x;
            o->child_vx = 6;
            bool fire = s->frame == 13;
            if (!fire && s->frame == 15) { x += 8; o->child_vx = (int16_t)(6 | 0x8000); fire = true; }
            if (fire) {
                o->child_script = POPPER_PIECE;
                o->child_w = 16; o->child_h = 13;
                const uint8_t *f = res(g, 44);
                o->child_frames = f ? f + 0x208 : NULL;
                o->child_vy = -8;
                spawn_object(g, x, d->y + 4, 111);
            }
        }
        if (wrapped) { set_aux_c(s, 3); s->t0 = 25; reset_animation(s); }
        break;
    }
    case 3: if (--s->t0 == 0) set_aux_c(s, 4); break;
    case 4:
        step_animation(s, POPPER_SINK, 0, &wrapped);
        if (wrapped) { set_aux_c(s, 0); s->t0 = 40; d->dflags |= 4; }
        break;
    default: break;
    }
    despawn_if_far(g, s);
}

/* ---- type 80: the small walker (slot 107; slot 105 knocked) ---- */
static void obj_walker80_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x119DC2 */
    init_object_slot(g, s, x, y, s->type4);
    s->vx = 4; s->vy = 0; s->hits = 2;
    s->flags |= 8;
    const uint8_t *f = res(g, 107);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 16, f, 1);                                  /* sub_107D68 */
}
static void obj_walker80_update(sk_game *g, sk_slot *s, int index) {          /* 0x119DF8 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (!(s->flags & 2)) {
        int wrapped = 0;
        step_animation(s, walker80_script, s->flags, &wrapped);
        if (ground_ahead(g, s, s->flags)) s->flags ^= 1;
        touch_player(g, d->x, d->y, 32, 16);
        if (ball_hits_object(g, s, 0, -16, 40, 32, 16, 0x300) >= 0) {
            d->y = (int16_t)(d->y - 16);
            size_frames(g, d, 32, 45, res(g, 105));
            set_frame2(s, 16, 16, 0);
        }
    }
    despawn_if_far(g, s);
}

/* ---- type 19: the press (slot 21): rises (param & 7) * 16, slams down; its shaft is tile 0x17F while it falls ---- */
static void obj_press_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x110122 */
    init_object_slot(g, s, x - 8, y, s->type4);
    s->vx = 0;
    s->t0 = (int16_t)((g->objs.marker_param & 7) << 4);
    s->vy = -2;
    set_aux_c(s, 0);
    const uint8_t *f = res(g, 21);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 16, f, 1);                                  /* sub_107A7C */
}
static void obj_press_update(sk_game *g, sk_slot *s, int index) {             /* 0x110160 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    int d6 = (int16_t)(d->y - s->y);
    if (d6 >= 8 && s->vy != -2) { play_sfx(g, 0x16); s->vy = -2; }  /* down: back up */
    else if (!(d6 > -(s->t0 + 1))) s->vy = 16;                       /* up: slam */
    int tile = 0x17F, state = 1;
    if (s->vy < 0) { tile = 0; state = 0; }
    int before = d->y;
    d->y = (int16_t)(d->y + (s->vy >> 1));
    int y = d->y, dr = (int16_t)((d->y & 0xFFF0) - (before & 0xFFF0));
    if (dr != 0) {                                                  /* a new row: fill it falling, clear the old rising */
        if (dr < 0) y = before;
        set_tile_at(g, d->x + 8, y, tile, state);
        g->mask.dirty = true;
        g->bw.mask_dirty = true;
    }
    if (s->vy >= 0) {
        g->player.fall_flags |= 1;                                  /* bset #0 at 0x302B6D: a hit squashes the kid */
        touch_player(g, d->x + 2, d->y, 28, 16);
        g->player.fall_flags &= (uint8_t)~1;
    }
    if (object_offscreen(g, d->x, d->y, 160) >= 0) return;
    fill_cells(g, s->x + 8, s->y - s->t0, 0x17F, 1, (uint16_t)s->t0 >> 4);   /* off the view: the whole shaft solid */
    if (s->marker_cell >= 0) {                                      /* ori.b #$c0 on the marker cell: it never comes back */
        g->world.live[s->marker_cell] |= 0xC000;
        g->world.tile[s->marker_cell] &= (uint16_t)~SK_MARKER_BIT;
    }
    free_object(g, s);
}

/* ---- type 21: the bubble (slot 22): blinks 100 frames, rises until an attribute-52 cell sends it back ---- */
static void obj_bubble_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x110418 */
    init_object_slot(g, s, x, y, s->type4);
    s->t0 = 4; s->vx = 0; s->vy = 0;
    s->timer = 100;
    const uint8_t *f = res(g, 22);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 15, f, 1);                                  /* sub_107906 */
}
static void obj_bubble_update(sk_game *g, sk_slot *s, int index) {            /* 0x110448 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    step_animation_simple(s, bubble_script, 0);
    bool probe = true;
    if (s->timer) {
        d->dflags ^= 2;
        if (--s->timer) probe = false;
        else { d->dflags &= (uint8_t)~2; s->vx = 0; s->vy = (int16_t)-s->t0; s->flags |= 4; }
    }
    if (probe) {
        g->attrs.probe_slot = s;
        probe_tile_attr(g, d->x + 8, d->y + 8);                     /* probe_tile_attr_saved */
        if (s->flags & 0x10) {
            s->flags &= ~0x10;
            d->x = s->x; d->y = s->y;
            s->vx = 0; s->vy = 0;
            s->timer = 100;
        } else touch_player(g, d->x, d->y, 16, 16);
    }
    apply_velocity_nograv(s);
    despawn_if_far(g, s);
}

/* ---- type 22: the lift tube (tiles only): opens and closes every 100 frames; open, it carries the kid up to an attribute-52 cell ---- */
static void tube_tiles(sk_game *g, sk_slot *s, int x, int y, bool open) {   /* sub_110636 / sub_11067C */
    if (open) {
        set_tile_at(g, x, y, 0x153, 0);
        set_tile_at(g, x, y + 16, 0x156, 0);
    } else {
        set_tile_at(g, x, y, 0x154, 0);
        set_tile_at(g, x, y + 16, 0x155, 0);
    }
    x -= 32; y += 32;
    int state = open ? 3 : 0;
    for (int k = 0; k <= s->t1; k++) {
        set_tile_at(g, x, y, open ? 0x1AF : 0, state);
        if (open) state ^= 0x80;                                    /* bchg #7: the two halves animate mirrored */
        set_tile_at(g, x + 16, y, open ? 0x1AF : 0, state);
        y += 16;
    }
}
static void obj_tube_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x1104D4 */
    init_object_slot_shared(g, s, x, y, s->type4, &g->objs.chain[7]);
    s->draw = NULL;
    set_aux_c(s, 1);
    s->t0 = 16;
    s->t1 = (int16_t)((g->objs.marker_param & 7) - 1);
    s->vx = 0; s->vy = 0;
}
static void obj_tube_update(sk_game *g, sk_slot *s, int index) {              /* 0x1104FE */
    (void)index;
    sk_player *p = &g->player;
    bool close_and_free = false;
    if (aux_c(s) != 2) {
        if (++s->timer >= 100) {
            s->timer = 0;
            set_aux_c(s, aux_c(s) ^ 1);
            tube_tiles(g, s, s->x + 32, s->y - 16, aux_c(s) != 0);
        }
        if (aux_c(s) != 0 && (uint16_t)iabs((int16_t)(s->y0 - p->y)) < 0x30
            && (uint16_t)(s->x0 - p->x + 16 + 4) < 8) {             /* the kid at its mouth: in he goes */
            set_aux_c(s, 2);
            p->state = 0x40;
            s->vy = (int16_t)-s->t0;
            g->cam_ease = 8;
        }
    }
    if (aux_c(s) == 2) {
        int dx = (uint16_t)s->vx >> 1;
        s->x0 = (int16_t)(s->x0 + ((s->flags & 1) ? dx : -dx));
        s->y0 = (int16_t)(s->y0 + (s->vy >> 1));
        p->anim = 0x11;
        if ((uint16_t)(-(int16_t)(s->y0 - s->y)) >= 0x28) p->exit_walk = 0xFF;   /* inside the tube: out of sight */
        p->x = (int16_t)(s->x0 + 16);
        p->y = s->y0;
        g->attrs.probe_slot = s;
        probe_tile_attr(g, s->x0 + 8, s->y0 + 8);
        if (s->flags & 0x10) {                                      /* the top: let him out */
            g->cam_ease = 4;
            p->state = 0; p->speed = 0; p->exit_walk = 0;
            close_and_free = true;
        }
    }
    if (!close_and_free && object_offscreen(g, s->x0, s->y0, 64) >= 0) return;
    tube_tiles(g, s, s->x + 32, s->y - 16, true);                   /* 0x11061A: left open */
    free_object(g, s);
}

/* ---- type 32: the kicker (slot 33): kicks a fireball (type 110) at the kid on its level, and one as it is knocked out ---- */
static void obj_kicker_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x11119E */
    init_object_slot(g, s, x, y - 27, s->type4);
    s->vx = 2; s->vy = 0; s->hits = 3;
    s->flags |= 8;
    const uint8_t *f = res(g, 33);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 43, f, 1);                                  /* sub_107C7A */
}
static void obj_kicker_update(sk_game *g, sk_slot *s, int index) {            /* 0x1111D4 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (g->ball_kicked_flag) ball_kill_check(g, s, 19, 0x400);
    if (!(s->flags & 2)) {
        bool walk = true;
        if (aux_c(s) != 1) {
            if (s->t0) s->t0--;
            else if (d->y + 2 == g->player.y) {
                int k = (int16_t)(g->player.x - d->x - 16);
                if (!(s->flags & 1)) k = -k;
                if ((uint16_t)k < 0x5A) { s->vx = 0; set_aux_c(s, 1); reset_animation(s); walk = false; }
            }
            if (walk) {
                step_animation_simple(s, kicker_walk_script, s->flags);
                if (ground_ahead(g, s, s->flags)) s->flags ^= 1;
            }
        } else walk = false;
        if (!walk) {
            int wrapped = 0;
            int changed = step_animation_offset(s, kicker_kick_script, s->flags, &wrapped);
            if (wrapped) { s->vx = 2; set_aux_c(s, 0); reset_animation(s); }
            else if (changed && s->anim_index == 3) {
                g->objs.marker_param = s->flags & 0xFF & ~2;
                spawn_object(g, d->x + 16, d->y, 110);
                s->t0 = 100;
            }
        }
        touch_player(g, d->x, d->y, 32, 43);
        if (ball_hits_object(g, s, 0, 0, 32, 43, 19, 0x400) >= 0 && s->t0 == 0) {   /* knocked out: a last lob */
            int p = (int16_t)(g->player.x - d->x - 16) >= 0 ? 1 : 0;
            g->objs.marker_param = p | 2;
            spawn_object(g, d->x + 16, d->y, 110);
        }
    }
    despawn_if_far_640(g, s);
}

/* ---- type 110: the fireball (slot 34; the burst from slot 35, the fires type 114): lands, then lays 8 fires along the floor ---- */
static void obj_fireball_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x111346 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->timer = 0;
    if (g->objs.marker_param & 1) s->flags |= 1;
    s->vy = 6; s->vx = 10;
    if (g->objs.marker_param & 2) { s->vy = -4; s->vx = 0; s->t0 = 1; }   /* a lob under gravity */
    s->flags |= 4;
    const uint8_t *f = res(g, 34);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 16, f, 1);                                  /* frames_16x16 */
}
static void obj_fireball_update(sk_game *g, sk_slot *s, int index) {          /* 0x1113A2 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (s->t0 == 0) apply_velocity_nograv(s); else apply_velocity(s);
    sk_objects *o = &g->objs;
    if (!aux_c(s)) {
        step_animation_simple(s, FIREBALL_FLY, 0);
        int r = probe_rows_below2(g, s, 8, 0);
        if (r >= 0) { despawn_if_far(g, s); return; }
        d->y = (int16_t)(d->y + r);                                 /* landed: the burst */
        reset_animation(s);
        set_aux_c(s, 1);
        s->vx = 0; s->flags &= ~4; s->t0 = 9;
        d->dflags |= 4;
        o->child_frames = res(g, 35); o->child_script = FIREBALL_BURST;
        o->child_vx = 0; o->child_vy = 0; o->child_w = 32; o->child_h = 22;
        spawn_object(g, d->x - 8, d->y - 6, 114);
        play_sfx(g, 0x17);
        despawn_if_far(g, s);
        return;
    }
    if (--s->t0) { despawn_if_far(g, s); return; }
    if (!s->timer) play_sfx(g, 0x18);
    o->child_frames = res(g, 34); o->child_script = FIREBALL_FIRE;
    o->child_vx = (s->flags & 1) ? (int16_t)0x8000 : 0;
    o->child_vy = 0; o->child_w = 16; o->child_h = 16;
    spawn_object(g, d->x, d->y, 114);
    s->t0 = 10;
    if (++s->timer == 8) { free_object(g, s); return; }
    d->x = (int16_t)(d->x + ((s->flags & 1) ? 16 : -16));
    if (ground_ahead(g, s, s->flags)) { free_object(g, s); return; }
    despawn_if_far(g, s);
}

/* ---- type 34: FACTORY 2's boss (slots 40, 41 for the pair it throws); stomps shake rocks (type 106) loose; dies on the 26th hit ---- */
static bool fonce(sk_slot *s) {
    if (aux_c(s) & 1) return false;
    set_aux_c(s, aux_c(s) | 1);
    return true;
}
static void obj_fboss_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x114942 */
    init_object_slot(g, s, x - 16, y - 48, s->type4);
    s->vx = 0; s->vy = 0; s->hits = 25;
    const uint8_t *f = res(g, 40);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 64, f, 1);                                  /* frames_64x64 */
    static const int dx[4] = { -0x50, -0x30, 0x60, 0x80 };
    for (int k = 0; k < 4; k++) {
        g->objs.marker_param = (uint8_t)(2 * k);
        spawn_object(g, s->x + dx[k], s->y - 0x50, 106);
    }
    g->push_x = s->x - 0x80;
    game_music_slot(g, 1);
}
static void obj_fboss_update(sk_game *g, sk_slot *s, int index) {             /* 0x1149D4 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    sk_objects *o = &g->objs;
    sk_player *p = &g->player;
    int changed = 0, code = script_step(g, s, fboss_phases, fboss_motions, &changed);
    enum { END, CHECKS, HITS } tail = CHECKS;
    switch (code) {
    case 0x00: case 0x04: case 0x3C: tail = END; break;
    case 0x08:                                                      /* throws the pair on frame 4 */
        if (changed && s->frame == 4) {
            o->child_frames = res(g, 41); o->child_script = fboss_pair_script;
            o->child_vx = 4; o->child_vy = -6; o->child_w = 32; o->child_h = 17;
            spawn_object(g, d->x, d->y + 16, 107);
            o->child_vx = (int16_t)(o->child_vx | 0x8000);
            spawn_object(g, d->x + 32, d->y + 16, 107);
        }
        tail = END;
        break;
    case 0x10: {                                                    /* the kid 70..79 px in front: an attack */
        int k = (int16_t)(p->x - d->x - 32);
        if (!(s->flags & 1)) k = -k;
        if ((uint16_t)k < 0x50 && (uint16_t)k >= 0x46) {
            s->x0 = s->t1; s->y0 = (int16_t)(s->t0 + 0x28);
            script_start(s, 23, fboss_phases, fboss_motions);
        }
        break;
    }
    case 0x14:                                                      /* the kid below and close: the grab */
        if ((int16_t)(p->y - d->y) >= 0) {
            int k = (int16_t)(p->x - d->x - 32);
            if (!(s->flags & 1)) k = -k;
            if ((uint16_t)k < 0x3C) set_aux_c(s, aux_c(s) | 2);
        }
        break;
    case 0x18: if (fonce(s)) s->flags ^= 1; break;
    case 0x1C: tail = HITS; break;
    case 0x20:                                                      /* stomping: rocks wobble loose */
        if (fonce(s)) g->fboss_wobble = 4;
        if (changed && s->anim_index == 0) { g->shake_amplitude = 4; play_sfx(g, 0x16); }
        break;
    case 0x24: if (fonce(s)) { g->fboss_drop = 4; play_sfx(g, 0x11); g->shake_amplitude = 6; } break;
    case 0x2C:                                                      /* the lunge */
        if (fonce(s)) {
            play_sfx(g, 0x10);
            int dx = (s->flags & 1) ? 32 : -32;
            s->anim = (uint8_t)dx;
            d->x = (int16_t)(d->x + dx);
        }
        tail = HITS;
        break;
    case 0x30:                                                      /* waits for the kid to be back */
        if ((uint16_t)s->t0 >= 0x60 && p->health) {
            set_aux_c(s, aux_c(s) & ~0x0C);
            script_end_next(s, fboss_phases);
        }
        break;
    case 0x34: script_start(s, (uint16_t)s->x0 >> 2, fboss_phases, fboss_motions); s->t0 = s->y0; break;   /* resume */
    case 0x38: {                                                    /* walks home */
        int dx = (int16_t)(d->x - s->x);
        s->flags &= ~1;
        if (dx == 0) script_end_next(s, fboss_phases);
        else if (dx < 0) s->flags |= 1;
        break;
    }
    case 0x40:
        if (fonce(s)) {
            g->score_value = 0x40000;
            add_score(g, 0x40000);
            spawn_object(g, d->x + 32, d->y + 24, 113);
            play_sfx(g, 0x11);
            g->fboss_drop = 4;
            g->shake_amplitude = 8;
            game_music_slot(g, 2);
        }
        tail = END;
        break;
    case 0x48: if (fonce(s)) { g->level_outcome = 0x81; g->level_number++; g->curtain_flags |= 0x10; } tail = END; break;
    default: break;                                                 /* 0x0C 0x28 0x44 */
    }
    if (tail == CHECKS) {                                           /* 0x114CB4 */
        if ((int8_t)s->hits < 0) {
            d->dflags &= (uint8_t)~2;
            script_start(s, 20, fboss_phases, fboss_motions);
            tail = END;
        } else {
            if (aux_c(s) & 2) {
                set_aux_c(s, aux_c(s) & ~2);
                s->x0 = s->t1; s->y0 = s->t0;
                script_start(s, 15, fboss_phases, fboss_motions);
            }
            if ((aux_c(s) & 4) && !(aux_c(s) & 8)) {                /* the kid died: it walks home */
                set_aux_c(s, aux_c(s) | 8);
                script_start(s, 17, fboss_phases, fboss_motions);
            }
            tail = HITS;
        }
    }
    if (tail == HITS) {                                             /* 0x114D10 */
        touch_player(g, d->x + 10, d->y + 8, 0x2C, 0x30);
        if (p->dying) set_aux_c(s, aux_c(s) | 4);
        ball_hits_object_simple(g, s, 8, 0, 0x30, 0x40);
    }
    despawn_if_far(g, s);
}

/* ---- type 106: a rock in FACTORY 2's roof (slot 42): drops on a stomp (0x300384) or wobbles first (0x300383), resets off the view ---- */
static void obj_rock_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x114EF4 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    int p = g->objs.marker_param & 0xFF;
    s->t0 = rock_delays[(p >> 1) & 3];
    s->vx = 0; s->vy = 0;
    s->t1 = s->t0;
    set_aux_c(s, 3);
    const uint8_t *f = res(g, 42);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 20, f, 1);                                  /* sub_10794A */
    set_frame2(s, p, p, 0);
}
static void obj_rock_update(sk_game *g, sk_slot *s, int index) {              /* 0x114F4A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (g->player.dying) { free_object(g, s); return; }
    enum { WAIT, WOBBLE, FALL, DROP, TOUCH, CHECK } st;
    switch (aux_c(s)) {
    case 1: st = WOBBLE; break;
    case 2: st = FALL; break;
    case 0: st = WAIT; break;
    case 4: st = CHECK; break;
    default:
        if (g->fboss_drop) { g->fboss_drop--; st = DROP; }
        else if (g->fboss_wobble) { g->fboss_wobble--; set_aux_c(s, 0); st = WAIT; }
        else return;
    }
    if (st == WAIT) {
        if (--s->t0) st = TOUCH;
        else { d->dflags &= (uint8_t)~4; set_aux_c(s, 1); s->vx = 4; s->t0 = 6; st = WOBBLE; }
    }
    if (st == WOBBLE) { s->flags ^= 1; st = --s->t0 ? TOUCH : DROP; }
    if (st == DROP) { set_aux_c(s, 2); s->vx = 0; set_frame2(s, s->frame + 1, s->frame + 1, 0); st = FALL; }
    if (st == FALL) {
        s->flags |= 4;
        if (probe_rows_below(g, s, 8, 0) < 0) { s->vy = -8; set_aux_c(s, 4); }   /* lands: a bounce */
        st = TOUCH;
    }
    if (st == TOUCH) touch_player(g, d->x, d->y, 16, 16);
    if (object_offscreen(g, d->x, d->y, 64) >= 0) return;
    s->t0 = s->t1;                                                  /* off the view: back in the roof */
    set_aux_c(s, 3);
    s->flags &= ~4;
    d->x = s->x; d->y = s->y;
    s->vx = 0; s->vy = 0;
    set_frame2(s, (s->frame + 1) & 7, (s->frame + 1) & 7, 0);
}

/* ---- type 23: the oncoming train (slot 36, 16 x 176; READING): a blinking warning (type 108), then it sweeps by along the
   bottom with a second piece (type 109); spawned by attributes 78 / 79 ---- */
static void obj_post_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x11152A */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->flags |= 1;
    s->vx = 0; s->vy = 0;
    s->t0 = 75;
    const uint8_t *f = res(g, 36);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 176, f, 1);                                 /* sub_1079D2 */
    s->draw->dflags |= 4;
}
static void obj_post_update(sk_game *g, sk_slot *s, int index) {              /* 0x111560 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->t0) {
        if (--s->t0 == 40) {
            sk_objects *o = &g->objs;
            o->child_frames = res(g, 37); o->child_script = warning_script;
            o->child_vx = 8; o->child_vy = 8; o->child_w = 32; o->child_h = 32;   /* type 108 reads these as its position */
            spawn_object(g, d->x, d->y, 108);
        }
        if (!s->t0) {                                               /* it comes */
            s->vx = 24;
            d->dflags &= (uint8_t)~4;
            d->x = (int16_t)(g->camera_x - 16);
            d->y = (int16_t)(g->world.fg_max_y + 30);
            spawn_object(g, d->x - 8, d->y - 2, 109);
        }
    } else if (object_offscreen(g, d->x, d->y, 32) < 0) {
        if (!(g->post_flag & 0x80)) { g->post_flag = 0; free_object(g, s); return; }
        s->vx = 0; s->t0 = 75;                                      /* still on the near side: again */
        d->dflags |= 4;
    }
    if (g->player.dying) free_object(g, s);
}

/* ---- type 108: a fixed effect at (child_vx, child_vy) running the child script once ---- */
static void obj_fixed_effect_init(sk_game *g, sk_slot *s, int x, int y) {     /* 0x10FE10 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 = (int16_t)(s->type4 | 0x8000);
    sk_objects *o = &g->objs;
    s->x0 = o->child_vx; s->y0 = o->child_vy;
    s->script = o->child_script;
    set_draw_size_at(g, s, o->child_w, o->child_h, o->child_frames);
    if (s->draw) s->draw->dflags |= 4 | 8;
}
static void obj_fixed_effect_update(sk_game *g, sk_slot *s, int index) {      /* 0x10FE5A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    d->x = s->x0; d->y = s->y0;
    d->dflags &= (uint8_t)~4;
    int wrapped = 0;
    if (s->script) step_animation(s, s->script, s->flags, &wrapped);
    if (wrapped) free_object(g, s);
}

/* ---- type 109: the train's second piece (slot 38): steps through its frames as the view passes ---- */
static void obj_post_piece_init(sk_game *g, sk_slot *s, int x, int y) {       /* 0x11165A */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 = (int16_t)(s->type4 | 0x8000);
    s->flags |= 1;
    s->vx = 24; s->vy = 0; s->t0 = 0;
    const uint8_t *f = res(g, 38);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
    set_frame2(s, 5, 5, 0);
    s->draw->x = (int16_t)(s->draw->x + 20);
    s->anim = 20;
}
static void obj_post_piece_update(sk_game *g, sk_slot *s, int index) {        /* 0x1116A4 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    int v = (int16_t)(d->x - g->camera_x + 32);
    if ((int16_t)(v - s->t0) > 0x30) {
        s->t0 = (int16_t)v;
        s->anim_index++;
        d->dflags |= 4;
        int i = s->anim_index < (int)sizeof passer_steps / 2 ? s->anim_index : (int)sizeof passer_steps / 2 - 1;   /* port guard: the table's end */
        const uint8_t *e = passer_steps + 2 * i;
        if (!(e[0] & 0x80)) {
            d->x = (int16_t)(d->x - (int8_t)s->anim + (int8_t)e[1]);
            s->anim = e[1];
            d->dflags &= (uint8_t)~4;
            set_frame2(s, e[0], e[0], 0);
        }
    }
    touch_player(g, d->x, d->y, 32, 32);
    if (object_offscreen(g, d->x, d->y, 32) < 0) free_object(g, s);
}

void obj_japan_install(void) {
    object_init_table[6] = obj_wheel_init;             object_update_table[6] = obj_wheel_update;
    object_init_table[18] = obj_crab_init;             object_update_table[18] = obj_crab_update;
    object_init_table[33] = obj_popper_init;           object_update_table[33] = obj_popper_update;
    object_init_table[80] = obj_walker80_init;         object_update_table[80] = obj_walker80_update;
    object_init_table[19] = obj_press_init;            object_update_table[19] = obj_press_update;
    object_init_table[21] = obj_bubble_init;           object_update_table[21] = obj_bubble_update;
    object_init_table[22] = obj_tube_init;             object_update_table[22] = obj_tube_update;
    object_init_table[32] = obj_kicker_init;           object_update_table[32] = obj_kicker_update;
    object_init_table[110] = obj_fireball_init;        object_update_table[110] = obj_fireball_update;
    object_init_table[34] = obj_fboss_init;            object_update_table[34] = obj_fboss_update;
    object_init_table[106] = obj_rock_init;            object_update_table[106] = obj_rock_update;
    object_init_table[23] = obj_post_init;             object_update_table[23] = obj_post_update;
    object_init_table[108] = obj_fixed_effect_init;    object_update_table[108] = obj_fixed_effect_update;
    object_init_table[109] = obj_post_piece_init;      object_update_table[109] = obj_post_piece_update;
    object_init_table[25] = obj_insect_init;           object_update_table[25] = obj_insect_update;
    object_init_table[67] = obj_th67_init;             object_update_table[67] = obj_th67_update;
    object_init_table[107] = obj_piece_init;           object_update_table[107] = obj_piece_update;
}
