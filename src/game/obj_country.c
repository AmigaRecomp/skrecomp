/* England country's object types (27 53 54 55/56 57 59 62 and the mole's clod 89), transcribed from the listing. */
#include <string.h>
#include "game/game.h"
#include "game/obj_country.h"
#include "game/obj_helpers.h"
#include "game/resources.h"

static const uint8_t leaper_script[] = { 0, 4, 0, 4, 6, 7, 8, 9, 0, 1, 2, 3 };                     /* 0x110B3C */
static const uint8_t hopper_hop_script[] = { 0, 3, 0, 8, 5, 5, 6, 2, 2, 3 };                        /* 0x1136FC */
static const uint8_t hopper_turn_script[] = { 0, 6, 0, 6, 4, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 4 };     /* 0x113706 */
static const uint8_t goose_walk_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 5, 6, 7, 8 };                 /* 0x1137F0 */
static const uint8_t flytrap_grow_script[] = { 0, 7, 0, 2, 0, 1, 2, 3, 4, 5, 6, 13, 14, 15, 16, 17, 18, 19 };   /* 0x11396E */
static const uint8_t flytrap_shrink_script[] = { 0, 7, 0, 2, 6, 5, 4, 3, 2, 1, 0, 19, 18, 17, 16, 15, 14, 13 }; /* 0x113980 */
static const uint8_t flytrap_snap_script[] = { 0, 6, 0, 2, 7, 8, 9, 10, 11, 12, 20, 21, 22, 23, 24, 25 };       /* 0x113992 */
static const uint8_t flytrap_heights[26] = { 4, 8, 12, 16, 20, 24, 28, 32, 32, 32, 32, 32, 32,
                                             4, 8, 12, 16, 20, 24, 28, 32, 32, 32, 32, 32, 32 };              /* 0x1139A2 */
static const uint8_t farmer_walk_script[] = { 0, 4, 0, 8, 0, 1, 2, 3, 4, 5, 6, 7 };                /* 0x113A5A */
static const uint8_t flies_script_a[] = { 0, 31, 0, 10, 0, 1, 0, 0, 0, 2, 3, 3, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 2, 3, 3,
                                          0, 0, 8, 9, 10, 11, 8, 9, 10, 11 };                                   /* 0x113BD6 */
static const uint8_t flies_script_b[] = { 0, 21, 0, 10, 4, 4, 4, 4, 5, 4, 5, 4, 4, 4, 6, 7, 4, 4, 5, 4, 4, 6, 7, 4, 4 };   /* 0x113BFA */
static const uint8_t mole_walk_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 11, 12, 13, 14 };              /* 0x1140AA */
static const uint8_t mole_throw_script[] = { 0, 29, 0, 8, 4, 5, 4, 5, 4, 5, 4, 5, 6, 6, 6, 6, 6, 7, 8, 7, 8, 7, 8, 7, 8,
                                             8, 8, 8, 8, 9, 8, 8, 8, 15, 16, 15, 16, 15, 16, 15, 16, 17, 17, 17, 17, 17,
                                             18, 19, 18, 19, 18, 19, 18, 19, 19, 19, 19, 19, 20, 19, 19, 19 };     /* 0x1140B6 */
static const int16_t mole_targets[8] = { -64, 80, -16, 50, -90, 20, -30, 32 };                      /* 0x11409A */
static const uint8_t clod_script[] = { 0, 4, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7 };                       /* 0x11416C */

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static bool need(sk_game *g, sk_slot *s, const uint8_t *f) { if (f) return true; free_object(g, s); return false; }
static int iabs(int v) { return v < 0 ? -v : v; }

/* ---- type 27: the leaper (slot 28): jumps out of the water 160 px below its marker ---- */
static void obj_leaper_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x1109FE */
    init_object_slot(g, s, x - 8, y, s->type4);
    s->vx = 0; s->vy = -36;
    if (g->objs.marker_param & 1) s->flags |= 1;
    s->flags |= 0xC;
    s->hits = 1;
    const uint8_t *f = res(g, 28);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 36, f, 0);                                  /* sub_107C14, prio word 0 */
    sk_draw *d = slot_draw_record(g, s);
    d->y = (int16_t)(s->y + 0xA0);
}
static void obj_leaper_update(sk_game *g, sk_slot *s, int index) {             /* 0x110A4E */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    apply_velocity(s);
    if (g->ball_kicked_flag) ball_kill_check(g, s, 4, 0x200);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    if (aux_c(s) == 0) {
        int d5 = (int16_t)(d->y - s->y);
        if (s->vy <= -2) {                                          /* rising */
            int wrapped = 0;
            step_animation(s, leaper_script, s->flags, &wrapped);
            if (!s->vx && -d5 >= 0x50) s->vx = 2;
        } else {                                                    /* falling */
            int l = 11, r = 5;
            if ((uint16_t)++s->t0 < 4) { l = 10; r = 4; }
            set_frame2(s, l, r, s->flags);
            if (d5 >= 0xA0) { s->t0 = 40; set_aux_c(s, 1); s->flags &= ~4; }
            if (s->vx && -d5 <= 0x40) s->vx = 0;
        }
    } else if (--s->t0 == 0) {                                      /* under water: jump again */
        set_aux_c(s, 0);
        s->flags |= 4;
        s->vy = -36;
        d->x = s->x;
    }
    touch_player(g, d->x + 4, d->y + 6, 16, 28);
    ball_hits_object(g, s, 4, 6, 24, 28, 4, 0x200);
    despawn_if_far(g, s);
}

/* ---- type 53: the hopper (slot 79) ---- */
static void obj_hopper_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x11359A */
    init_object_slot(g, s, x, y - 16, s->type4);
    s->x0 = (int16_t)((g->objs.marker_param & 0xFF) + 2);           /* hops before it turns */
    s->vx = 0; s->vy = 0; s->t0 = 100; s->t1 = 0; s->hits = 1; s->flags |= 8;
    const uint8_t *f = res(g, 79);
    if (need(g, s, f)) frames_32x30(g, s, f, 1);
}
static void obj_hopper_update(sk_game *g, sk_slot *s, int index) {             /* 0x1135E8 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    int wrapped = 0;
    switch (aux_c(s)) {
    case 1:                                                         /* in the air */
        step_animation(s, hopper_hop_script, s->flags, &wrapped);
        if (!wrapped) break;
        d->x = (int16_t)(d->x - 8 + ((s->flags & 1) ? 16 : 0));
        set_frame2(s, 4, 1, s->flags);
        s->vx = 0; s->vy = 0; s->flags &= ~4;
        set_aux_c(s, 3);
        if (++s->t1 == s->x0) { s->t0 = 50; s->t1 = 0; set_aux_c(s, 2); }
        break;
    case 2:                                                         /* turning round */
        step_animation(s, hopper_turn_script, s->flags, &wrapped);
        if (!wrapped) break;
        s->flags ^= 1;
        set_frame2(s, 4, 1, s->flags);
        s->t0 = 25;
        set_aux_c(s, 0);
        break;
    case 0:                                                         /* sitting */
        set_frame2(s, 4, 1, s->flags);
        if (--s->t0 != 0) break;
        /* fall through: hop */
    default:
        s->vx = 6; s->vy = -12; s->flags |= 4;
        set_aux_c(s, 1);
        break;
    }
    touch_player(g, d->x, d->y + 14, 20, 16);
    ball_hits_object(g, s, 0, 0, 32, 30, 7, 0x200);
    despawn_if_far(g, s);
}

/* ---- type 54: the goose (slot 80) ---- */
static void obj_goose_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x113716 */
    init_object_slot(g, s, x, y - 12, s->type4);
    s->vx = 2; s->vy = 0; s->flags |= 8;
    s->hits = (uint8_t)((g->objs.marker_param & 1) + 1);
    const uint8_t *f = res(g, 80);
    if (need(g, s, f)) frames_48x28(g, s, f, 1);
}
static void obj_goose_update(sk_game *g, sk_slot *s, int index) {              /* 0x113756 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    bool wait = aux_c(s) != 0;
    if (!wait) {
        int wrapped = 0;
        step_animation(s, goose_walk_script, s->flags, &wrapped);
        if (ground_ahead(g, s, s->flags) < 0 || (uint16_t)iabs(s->x - d->x) >= 0x78) {
            s->vx = 0; set_aux_c(s, 1); s->t0 = 6; wait = true;
        }
    }
    if (wait) {
        set_object_frame(s, 4);
        if (--s->t0 == 0) { s->flags ^= 1; set_aux_c(s, 0); s->vx = 2; }
    }
    touch_player(g, d->x, d->y + 12, 0x30, 0x10);
    ball_hits_object(g, s, 0, 12, 0x30, 0x10, 9, 0x200);
    despawn_if_far(g, s);
}

/* ---- types 55 / 56: the flytraps (slot 86): 55 grows from the ground and spawns 56 hanging 32 px above ---- */
static void obj_flytrap_ground_init(sk_game *g, sk_slot *s, int x, int y) {    /* 0x1137FC */
    init_object_slot(g, s, x, y, s->type4);
    s->t0 = 0;
    const uint8_t *f = res(g, 86);
    if (!need(g, s, f)) return;
    frames_16x32(g, s, f, 1);
    slot_draw_record(g, s)->dflags |= 4;
    spawn_object(g, s->x, s->y, 56);
}
static void obj_flytrap_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x11382E */
    init_object_slot_nodraw(g, s, x, y - 32, s->type4);
    s->flags = 1;
    s->t0 = 0;
    const uint8_t *f = res(g, 86);
    if (!need(g, s, f)) return;
    frames_16x32(g, s, f, 1);
    slot_draw_record(g, s)->dflags |= 4;
}
static void obj_flytrap_update(sk_game *g, sk_slot *s, int index) {            /* 0x11385E */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    int wrapped = 0;
    switch (aux_c(s)) {
    case 0:                                                         /* grow */
        d->dflags &= ~4;
        step_animation(s, flytrap_grow_script, s->flags, &wrapped);
        if (!wrapped) break;
        set_aux_c(s, 1);
        /* fall through */
    case 1:                                                         /* snap six times */
        wrapped = 0;
        step_animation(s, flytrap_snap_script, s->flags, &wrapped);
        if (!wrapped || ++s->t0 < 6) break;
        set_aux_c(s, 2); s->t0 = 0;
        /* fall through */
    case 2:                                                         /* shrink */
        wrapped = 0;
        step_animation(s, flytrap_shrink_script, s->flags, &wrapped);
        if (!wrapped) break;
        set_aux_c(s, 3);
        d->dflags |= 4;
        /* fall through */
    default:                                                        /* hidden for 50 frames */
        if (++s->t0 >= 50) { set_aux_c(s, 0); s->t0 = 0; }
        break;
    }
    if (!(d->dflags & 4)) {
        int h = flytrap_heights[s->frame < 26 ? s->frame : 25], dy = (s->flags & 1) ? 0 : 32 - h;
        touch_player(g, d->x, d->y + dy, 16, h);
        if (ball_overlap_box(g, d->x, d->y + dy, 16, h) && !g->ball.vanishing) {   /* swallowed */
            g->ball.vanishing = 0xFF;
            g->ball.vx = g->ball.vy = 0;
            g->bw.roll_phase = 0;
            play_sfx(g, 14);
            if (g->player.state == 16) { g->player.timer = 0; g->player.state = 0; }
        }
    }
    despawn_if_far(g, s);
}

/* ---- type 57: the farmer (slot 87) ---- */
static void obj_farmer_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x1139BC */
    init_object_slot(g, s, x, y - 48, s->type4);
    s->vx = 2; s->vy = 0; s->hits = 2; s->flags |= 8;
    const uint8_t *f = res(g, 87);
    if (need(g, s, f)) frames_64x64(g, s, f, 1);
}
static void obj_farmer_update(sk_game *g, sk_slot *s, int index) {             /* 0x1139F2 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    apply_velocity(s);
    if (!(s->flags & 2)) {
        if (ground_ahead(g, s, s->flags) < 0 || (uint16_t)iabs(d->x - s->x) >= 200) s->flags ^= 1;
        int wrapped = 0;
        step_animation(s, farmer_walk_script, s->flags, &wrapped);
        touch_player(g, d->x + 8, d->y + 4, 0x30, 0x38);
        ball_hits_object(g, s, 0, 4, 0x3C, 0x40, 8, 0x500);
    }
    despawn_if_far(g, s);
}

/* ---- type 59: the flies (slot 81, a fixed record) ---- */
static void obj_flies_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x113B64 */
    init_object_slot_fixed(g, s, x, y, s->type4);
    s->t0 = (int16_t)(g->objs.marker_param & 0xFF);
    const uint8_t *f = res(g, 81);
    sk_draw *d = slot_draw_record(g, s);
    if (!d || !f) { free_fixed_object(g, s); return; }
    frames_16x16(g, s, f, 1);
    d->x = (int16_t)(d->x + (s->t0 ? 12 : 7));
    d->y = (int16_t)(d->y + (s->t0 ? 7 : -2));
    d->dflags |= 4;
}
static void obj_flies_update(sk_game *g, sk_slot *s, int index) {              /* 0x113BA6 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    d->dflags &= ~4;
    int wrapped = 0;
    step_animation(s, s->t0 ? flies_script_b : flies_script_a, 0, &wrapped);
    if (object_offscreen(g, d->x, d->y, 0x40) < 0) free_fixed_object(g, s);
}

/* ---- type 62: the mole (slot 90) and its clod (type 89, slot 91) ---- */
static void obj_mole_init(sk_game *g, sk_slot *s, int x, int y) {              /* 0x113F48 */
    init_object_slot(g, s, x, y - 16, s->type4);
    s->vx = 2; s->vy = 0; s->flags |= 8; s->hits = 2;
    s->t0 = 0; s->t1 = 0; s->x0 = 0;                                /* the target offset, its index, the pause */
    const uint8_t *f = res(g, 90);
    if (need(g, s, f)) frames_64x32(g, s, f, 1);
}
static void obj_mole_update(sk_game *g, sk_slot *s, int index) {               /* 0x113F8A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    bool aim = false, throw_ = aux_c(s) == 1;
    if (!throw_) {
        if (s->x0 == 0) {
            int wrapped = 0;
            step_animation(s, mole_walk_script, s->flags, &wrapped);
            if ((uint16_t)iabs(d->x - s->x) >= 0x96 || ground_ahead(g, s, s->flags)) {
                s->x0 = 6; s->flags ^= 1; s->vx = 0; set_object_frame(s, 10);
            } else aim = true;
        }
        if (!aim && s->x0) {
            if (--s->x0 == 0) { s->vx = 2; aim = true; }
        }
        if (aim && (int16_t)(s->x + s->t0 - d->x) == 0) {           /* at the target: dig and throw */
            s->anim_tick = 0; s->anim_index = 0;
            s->vx = 0; set_aux_c(s, 1); throw_ = true;
        }
    }
    if (throw_) {
        int wrapped = 0, changed = step_animation(s, mole_throw_script, s->flags, &wrapped);
        if (changed && s->anim_index == 25) {
            g->objs.marker_param = s->flags & 0xFF;
            spawn_object(g, d->x, d->y, 89);
        }
        if (wrapped) {
            s->anim_tick = 0; s->anim_index = 0;
            set_frame2(s, 8, 19, s->flags);
            set_aux_c(s, 0); s->vx = 2;
            s->t1 = (int16_t)((s->t1 + 2) & 15);
            s->t0 = mole_targets[s->t1 >> 1];
        }
    }
    touch_player(g, d->x, d->y, 0x30, 0x20);
    ball_hits_object(g, s, 0, 0, 0x40, 0x20, 21, 0x200);
    despawn_if_far(g, s);
}

static void obj_clod_init(sk_game *g, sk_slot *s, int x, int y) {              /* 0x1140F4 */
    init_object_slot_nodraw(g, s, x - 8, y + 10, s->type4);
    int facing = g->objs.marker_param & 1;
    if (facing) s->x = (int16_t)(s->x + 64);
    s->flags = (int16_t)facing;
    s->vx = 8; s->vy = 0; s->t0 = 100;
    const uint8_t *f = res(g, 91);
    if (need(g, s, f)) frames_16x10(g, s, f, 1);
}
static void obj_clod_update(sk_game *g, sk_slot *s, int index) {               /* 0x11413A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    apply_velocity(s);
    int wrapped = 0;
    step_animation(s, clod_script, 1, &wrapped);
    if (--s->t0 == 0) s->flags |= 4;                                /* then it falls */
    touch_player(g, d->x, d->y, 16, 12);
    despawn_if_far(g, s);
}

void obj_country_install(void) {
    object_init_table[27] = obj_leaper_init;           object_update_table[27] = obj_leaper_update;
    object_init_table[53] = obj_hopper_init;           object_update_table[53] = obj_hopper_update;
    object_init_table[54] = obj_goose_init;            object_update_table[54] = obj_goose_update;
    object_init_table[55] = obj_flytrap_ground_init;   object_update_table[55] = obj_flytrap_update;
    object_init_table[56] = obj_flytrap_init;          object_update_table[56] = obj_flytrap_update;
    object_init_table[57] = obj_farmer_init;           object_update_table[57] = obj_farmer_update;
    object_init_table[59] = obj_flies_init;            object_update_table[59] = obj_flies_update;
    object_init_table[62] = obj_mole_init;             object_update_table[62] = obj_mole_update;
    object_init_table[89] = obj_clod_init;             object_update_table[89] = obj_clod_update;
}
