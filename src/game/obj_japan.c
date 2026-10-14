/* Japan's object types, transcribed from the listing. */
#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/obj_japan.h"
#include "game/obj_italy.h"
#include "game/platforms.h"
#include "game/resources.h"

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

void obj_japan_install(void) {
    object_init_table[6] = obj_wheel_init;             object_update_table[6] = obj_wheel_update;
    object_init_table[18] = obj_crab_init;             object_update_table[18] = obj_crab_update;
    object_init_table[25] = obj_insect_init;           object_update_table[25] = obj_insect_update;
    object_init_table[67] = obj_th67_init;             object_update_table[67] = obj_th67_update;
    object_init_table[107] = obj_piece_init;           object_update_table[107] = obj_piece_update;
}
