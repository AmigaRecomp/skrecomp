/* Japan's object types, transcribed from the listing. */
#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/obj_japan.h"
#include "game/resources.h"

static const uint8_t insect_script[] = { 0, 4, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7 };                 /* 0x11088E */
static const uint8_t th67_idle_script[] = { 0, 2, 0, 12, 0, 1, 2, 3 };                         /* 0x118C1C */
static const uint8_t th67_walk_script[] = { 0, 6, 0, 6, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };   /* 0x118C24 */
static const uint8_t th67_throw_script[] = { 0, 8, 0, 4, 18, 19, 20, 21, 21, 21, 21, 21, 22, 23, 24, 25, 25, 25, 25, 25 };   /* 0x118C34 */
static const uint8_t th67_missile_script[] = { 0, 2, 0, 10, 0, 0, 1, 1 };                      /* 0x118C48 */
static const uint8_t th67_debris_script[] = { 0, 2, 0, 10, 0, 0, 0, 0 };                       /* 0x118C50 */

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

void obj_japan_install(void) {
    object_init_table[25] = obj_insect_init;           object_update_table[25] = obj_insect_update;
    object_init_table[67] = obj_th67_init;             object_update_table[67] = obj_th67_update;
    object_init_table[107] = obj_piece_init;           object_update_table[107] = obj_piece_update;
}
