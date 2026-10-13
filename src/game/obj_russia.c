/* Russia's object types, transcribed from the listing. */
#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/obj_russia.h"
#include "game/resources.h"
#include "game/platforms.h"
#include "game/world.h"
#include "formats/level.h"

static const uint8_t mine_script[] = { 0, 5, 0, 4, 0, 1, 2, 2, 255, 0, 1, 2, 2, 255 };          /* 0x111A38 */
static const uint8_t mine_piece_script[] = { 0, 1, 0, 2, 0, 1 };                               /* 0x111A32 */
static const uint8_t sled_scripts[3][8] = {                                                    /* 0x118886, 0x11888E, 0x118896 */
    { 0, 2, 0, 4, 0, 1, 6, 7 }, { 0, 2, 0, 4, 2, 3, 8, 9 }, { 0, 2, 0, 4, 4, 5, 10, 11 } };
static const uint8_t icicle_script[] = { 0, 8, 0, 3, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };   /* 0x1189D8 */

static const uint8_t burrow_wait_script[] = { 0, 4, 0, 20, 1, 0, 1, 2, 1, 0, 1, 2 };             /* 0x1196D4 */
static const uint8_t burrow_climb_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 12, 13, 14, 15 };         /* 0x1196E0 */
static const uint8_t burrow_walk_script[] = { 0, 4, 0, 6, 4, 5, 6, 7, 0, 1, 2, 3 };              /* 0x1196EC */
static const uint8_t burrow_hop_script[] = { 0, 2, 0, 6, 3, 4, 5, 6 };                           /* 0x1196F8 */
static const uint8_t burrow_dig_script[] = { 0, 4, 0, 4, 4, 5, 6, 7, 4, 5, 6, 7 };               /* 0x119700 */
static const uint8_t burrow_up_script[] = { 0, 4, 0, 4, 8, 9, 10, 11, 8, 9, 10, 11 };            /* 0x11970C */

static const uint8_t walker74_script[] = { 0, 4, 0, 8, 0, 1, 2, 3, 4, 5, 6, 7 };              /* 0x119DB6 */
static const uint8_t walker78_script[] = { 0, 5, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };        /* 0x119B6E */

static const uint8_t sprout_script[] = { 0, 10, 0, 3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 97, 0, 230, 2, 112, 0, 16, 57, 0, 48 };   /* 0x10EFC4 */

static const uint8_t shell_flash_script[] = { 0, 8, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };   /* 0x1103EA */
static const uint8_t shell_fly_script[] = { 0, 4, 0, 3, 0, 1, 2, 3, 4, 5, 6, 7 };              /* 0x1103FE */
static const uint8_t shell_burst_script[] = { 0, 5, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };       /* 0x11040A */

static const uint8_t belt_script[] = { 0, 4, 0, 2, 0, 1, 2, 3, 0, 1, 2, 3 };                  /* 0x1133AE */
static const uint8_t belt_fast_script[] = { 0, 4, 0, 3, 4, 5, 6, 7, 4, 5, 6, 7 };             /* 0x1133BA */

static const uint8_t gunner_walk_script[] = { 0, 4, 0, 10, 0, 1, 2, 3, 4, 5, 6, 7 };           /* 0x112D5A */
static const uint8_t gunner_fire_script[] = { 0, 16, 0, 4, 9, 9, 9, 10, 11, 12, 13, 9, 9, 10, 11, 12, 13, 9, 9, 9,
                                              14, 14, 14, 15, 16, 17, 18, 14, 14, 15, 16, 17, 18, 14, 14, 14 };   /* 0x112D66 */
static const uint8_t splash_script[] = { 0, 5, 0, 5, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4 };            /* 0x112D8A */
static const uint8_t bullet_script[] = { 0, 4, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7 };                 /* 0x112E54 */

static const uint8_t cannon_script[] = { 0, 8, 0, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };   /* 0x113488 */
static const uint8_t cannon_shot_script[] = { 0, 6, 0, 2, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27 };     /* 0x113516 */
static const uint8_t cannon_flash_script[] = { 0, 7, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };      /* 0x113588 */

static const uint8_t roller_script[] = { 0, 4, 0, 4, 0, 1, 2, 3 };                           /* 0x111AAC */

static const uint8_t rs_634e[] = { 0, 4, 0, 8, 3, 4, 5, 4, 3, 4, 5, 4 };   /* 0x11634E */
static const uint8_t rs_635a[] = { 0, 1, 0, 1, 6, 6 };   /* 0x11635A */
static const uint8_t rs_6360[] = { 0, 1, 0, 1, 4, 4 };   /* 0x116360 */
static const uint8_t rs_6366[] = { 0, 6, 0, 6, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };   /* 0x116366 */
static const uint8_t rs_6376[] = { 0, 1, 0, 1, 0, 6 };   /* 0x116376 */
static const uint8_t rs_637c[] = { 0, 1, 0, 1, 19, 19 };   /* 0x11637C */
static const uint8_t rs_6382[] = { 0, 8, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };   /* 0x116382 */
static const uint8_t rs_6396[] = { 0, 1, 0, 1, 20, 20 };   /* 0x116396 */
static const uint8_t rs_639c[] = { 0, 4, 0, 4, 1, 0, 0, 0, 1, 0, 0, 0 };   /* 0x11639C */
static const uint8_t rs_63a8[] = { 0, 2, 0, 8, 0, 1, 0, 1 };   /* 0x1163A8 */
static const uint8_t rs_63b0[] = { 0, 1, 0, 1, 8, 7 };   /* 0x1163B0 */
static const uint8_t rs_63b6[] = { 0, 1, 0, 1, 2, 2 };   /* 0x1163B6 */
static const uint8_t rs_63bc[] = { 0, 10, 0, 6, 12, 13, 14, 13, 18, 15, 16, 17, 16, 18, 15, 16, 17, 16, 18, 12, 13, 14, 13, 18 };   /* 0x1163BC */
static const uint8_t rs_63d4[] = { 0, 3, 0, 4, 21, 22, 23, 21, 22, 23 };   /* 0x1163D4 */
static const uint8_t rs_63de[] = { 0, 1, 0, 1, 23, 23 };   /* 0x1163DE */
static const sk_phase rboss2_phases[27] = {                                                  /* 0x116242 */
    { 100, 0, 4 }, { 65535, 10, 8 }, { 15, 150, 12 }, { 36, 30, 16 }, { 65535, 40, 20 }, { 25, 80, 24 }, { 64, 30, 28 }, { 65535, 40, 32 }, { 25, 80, 36 }, { 16, 30, 40 }, { 7, 50, 44 }, { 17, 60, 48 }, { 8, 70, 52 }, { 16, 30, 56 }, { 65535, 40, 60 }, { 25, 80, 64 }, { 32, 30, 68 }, { 15, 140, 72 }, { 12, 90, 76 }, { 50, 0, 80 }, { 65535, 10, 84 }, { 15, 150, 88 }, { 32, 30, 28 }, { 11, 110, 96 }, { 200, 120, 100 }, { 65535, 130, 100 }, { 65535, 100, 104 },
};
static const sk_motion rboss2_motions[16] = {                                                /* 0x1162AE */
    { 0, 0, rs_634e, 0x00 }, { 0, -2, rs_635a, 0x04 }, { 0, 0, rs_6360, 0x08 }, { 8, 0, rs_6366, 0x0C }, { 0, 0, rs_6376, 0x10 }, { 8, -16, rs_637c, 0x14 }, { 8, -9, rs_6382, 0x18 }, { 8, 8, rs_6396, 0x1C }, { 0, 0, rs_63b0, 0x20 }, { 0, -16, rs_63b6, 0x24 }, { 0, 0, rs_63bc, 0x28 }, { 0, 0, rs_63d4, 0x2C }, { 0, 0, rs_63de, 0x30 }, { 0, 0, rs_63de, 0x34 }, { 0, 0, rs_63a8, 0x38 }, { 0, 0, rs_639c, 0x3C },
};

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static bool need(sk_game *g, sk_slot *s, const uint8_t *f) { if (f) return true; free_object(g, s); return false; }
static void reset_animation(sk_slot *s) { s->anim_tick = 0; s->anim_index = 0; }   /* 0x10DF62 */
static int flags_word(const sk_slot *s) { return (uint16_t)((s->flags & 0xFF) << 8 | s->flags_b); }   /* the word at +0xA */
static void set_flags_word(sk_slot *s, int v) { s->flags = (int16_t)((v >> 8) & 0xFF); s->flags_b = (uint8_t)v; }
static int iabs(int v) { return v < 0 ? -v : v; }

/* ---- type 35: the mine (slot 43): set off, it bursts into four pieces (type 111, slot 44) and leaves tiles 0x150/0x151 ---- */
static void obj_mine_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x111918 */
    init_object_slot(g, s, x, y - 10, s->type4);
    s->vx = 0; s->vy = 0;
    set_aux_c(s, 0);
    const uint8_t *f = res(g, 43);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
}
static void obj_mine_update(sk_game *g, sk_slot *s, int index) {              /* 0x111944 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d || !aux_c(s)) return;
    int wrapped = 0;
    if (step_animation(s, mine_script, 0, &wrapped) && s->frame == 1) {
        play_sfx(g, 0x13);
        g->shake_amplitude = 4;
        g->flash_timer = 16;
        g->flash_table = 0;                                         /* 0x3012BA */
        sk_objects *o = &g->objs;
        o->child_script = mine_piece_script;
        o->child_w = 16; o->child_h = 13;
        o->child_frames = res(g, 44);
        o->child_vx = 4; o->child_vy = -12;
        spawn_object(g, d->x, d->y, 111);
        o->child_vx = (int16_t)(o->child_vx | 0x8000);
        spawn_object(g, d->x + 16, d->y, 111);
        if (o->child_frames) o->child_frames += 0x104;
        o->child_vx = (int16_t)(o->child_vx | 0x8000);
        o->child_vy = -16;
        spawn_object(g, d->x + 16, d->y, 111);
        spawn_object(g, d->x, d->y, 111);
    }
    if (wrapped) {
        set_tile_at(g, d->x, d->y + 10, 0x150, 0);
        set_tile_at(g, d->x + 16, d->y + 10, 0x151, 0);
        free_object_keep(g, s);
    }
}

/* ---- type 65: the sled (slot 96): rides the slope, faster downhill; an uphill or a kick throws its rider (type 116) ---- */
static void sled_rider(sk_game *g, const sk_draw *d) {              /* sub_118844 */
    sk_objects *o = &g->objs;
    const uint8_t *f = res(g, 96);
    o->child_frames = f ? f + 0x28A0 : NULL;
    o->child_w = 32; o->child_h = 40;
    o->child_vx = 0; o->child_vy = -6;
    spawn_object(g, d->x, d->y, 116);
}
static void obj_sled_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x1186B6 */
    init_object_slot(g, s, x, y - 16, s->type4);
    s->flags = (int16_t)(g->objs.marker_param & 1);
    s->vx = 0; s->vy = 0; s->t0 = 0;
    if (s->flags & 1) s->t0 = 50;
    s->t1 = 16;
    s->hits = 1;
    const uint8_t *f = res(g, 96);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 40, f, 1);                                  /* sub_107C36 */
    set_frame2(s, 0, 6, s->flags);
}
static void obj_sled_update(sk_game *g, sk_slot *s, int index) {              /* 0x11871C */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (!(s->flags & 2)) {
        bool touch = true;
        if (s->t0) s->t0--;
        else {
            d->y = (int16_t)(d->y + probe_rows_below2(g, s, 16, -8));
            int slope = 0, steep = ledge_distance(g, s, 16, -8, &slope);
            int side = s->flags;
            if (slope > 0) side ^= 1;
            if (slope != 0 && !(side & 1)) {                        /* uphill: it stops dead, the rider flies */
                set_frame2(s, 12, 12, 0);
                sled_rider(g, d);
                s->vx = (int16_t)(s->vx - 6);
                if (s->vx < 0) s->vx = 0;
                s->vy = -10;
                s->flags |= 4 | 2;
                touch = false;
            } else {
                int k = (uint16_t)steep < 3 ? 0 : (uint16_t)steep < 8 ? 1 : 2;
                s->t1 = (int16_t)(s->t1 + k);
                if ((uint16_t)s->t1 >= 0x30) s->t1 = 0x30;
                s->vx = (int16_t)((uint16_t)s->t1 >> 2);
                int wrapped = 0;
                step_animation(s, sled_scripts[k], s->flags, &wrapped);
            }
        }
        if (touch) {
            touch_player(g, d->x, d->y, 32, 32);
            if (ball_hits_object(g, s, 0, 0, 32, 32, 12, 0x200) >= 0) { sled_rider(g, d); s->flags |= 8; }
        }
    }
    if (object_offscreen(g, d->x, d->y, 64) < 0) {
        if ((s->flags & 2) && (s->flags & 8)) free_object_deferred(g, s);
        else free_object(g, s);
    }
}

/* ---- type 66: the icicle (slot 97): drops when the kid passes under, shatters, comes back ---- */
static void obj_icicle_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x11889E */
    x -= 8;
    if (!(g->objs.marker_param & 1)) y += 8;
    init_object_slot(g, s, x, y, s->type4);
    s->vx = 0; s->vy = 0;
    const uint8_t *f = res(g, 97);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 26, f, 1);                                  /* sub_107B48 */
}
static void obj_icicle_update(sk_game *g, sk_slot *s, int index) {            /* 0x1188D4 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    switch (aux_c(s)) {
    case 0:
        if ((int16_t)(d->y - g->player.y) >= 0) break;
        if ((uint16_t)iabs((int16_t)(d->x + 16 - g->player.x)) >= 32) break;
        s->vx = 0; set_aux_c(s, 1); s->flags |= 4;
        break;
    case 1: {
        int r = probe_rows_below(g, s, 16, 0);
        if (!r) break;
        set_aux_c(s, 2);
        d->y = (int16_t)(d->y + r);
        s->flags &= ~4; s->vy = 0;
    }   /* fall through */
    case 2: {
        int wrapped = 0;
        step_animation(s, icicle_script, 0, &wrapped);
        if (!wrapped) break;
        d->dflags |= 4;
        s->t0 = 50;
        set_aux_c(s, 3);
    }   /* fall through */
    case 3:
        if (--s->t0) break;
        d->x = s->x; d->y = s->y;
        s->t0 = 25;
        set_aux_c(s, 4);
        /* fall through */
    case 4:
        d->dflags ^= 4;
        if (--s->t0 == 0) { d->dflags &= (uint8_t)~4; set_aux_c(s, 0); }
        break;
    default: break;
    }
    if (s->vy > 2) touch_player(g, d->x + 12, d->y, 8, 26);
    despawn_if_far(g, s);
}

/* ---- type 72: the burrower (slots 104 hole, 105 climbing, 106 hopping, 107 walking) ---- */
static void burrow_size(sk_game *g, sk_draw *d, int w, int h, int slot) { size_frames(g, d, w, h, res(g, slot)); }
static void obj_burrower_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x11932A */
    init_object_slot(g, s, x - 16, y - 16, s->type4);
    s->vx = 0; s->vy = 0; s->t0 = 50; s->hits = 2;
    s->flags |= 8;
    const uint8_t *f = res(g, 106);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
}
static void obj_burrower_update(sk_game *g, sk_slot *s, int index) {          /* 0x11936C */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    int wrapped = 0;
    switch (aux_c(s)) {
    case 0:                                                         /* in its hole */
        step_animation(s, burrow_wait_script, 0, &wrapped);
        if (--s->t0) break;
        burrow_size(g, d, 32, 45, 105);
        reset_animation(s);
        s->t0 = 24;
        set_aux_c(s, 1);
        /* fall through */
    case 1: {                                                       /* climbing: bit 5 of the flags = down */
        step_animation(s, burrow_climb_script, (s->flags >> 5) & 1, &wrapped);
        d->y = (int16_t)(d->y - ((s->flags & 0x20) ? -2 : 2));
        if (!((int16_t)d->y < s->y)) {                              /* back in the hole */
            reset_animation(s);
            burrow_size(g, d, 32, 32, 106);
            s->flags &= ~0x20;
            s->t0 = 50;
            set_aux_c(s, 0);
            break;
        }
        g->attrs.probe_slot = s;
        probe_tile_attr(g, d->x + 16, d->y + 26);                   /* probe_tile_attr_saved: an attribute may set state 5 */
        if (aux_c(s) == 5) { reset_animation(s); goto dig; }
        if (s->t0) { s->t0--; break; }
        s->flags &= ~1;
        int gy = d->y + ((s->flags & 0x20) ? 16 : 32);
        if (cells_have_gap(g, d->x - 64, gy, 4)) {
            s->flags |= 1;
            if (cells_have_gap(g, d->x + 32, gy, 4)) break;
        }
        burrow_size(g, d, 32, 32, 106);                             /* out: a hop */
        reset_animation(s);
        s->vx = 4; s->vy = -6;
        s->flags |= 4;
        set_aux_c(s, 2);
    }   /* fall through */
    case 2:
        step_animation(s, burrow_hop_script, s->flags, &wrapped);
        if (!wrapped) break;
        burrow_size(g, d, 48, 16, 107);
        reset_animation(s);
        s->vy = 0; s->flags &= ~4;
        d->y = (int16_t)(d->y + 10);
        set_aux_d(s, 0);
        set_aux_c(s, 3);
        /* fall through */
    case 3: {                                                       /* walking: turns 64 px out, hops back in at 22 */
        step_animation(s, burrow_walk_script, s->flags, &wrapped);
        bool first = !(aux_d(s) & 1);
        set_aux_d(s, aux_d(s) | 1);
        if (first) break;
        int dx = iabs((int16_t)(s->x - d->x));
        if ((uint16_t)dx >= 64) s->flags ^= 1;
        if (dx != 0x16) break;
        burrow_size(g, d, 32, 32, 106);
        reset_animation(s);
        s->vy = -6; s->flags |= 4;
        d->y = (int16_t)(d->y - 10);
        set_aux_c(s, 4);
    }   /* fall through */
    case 4:
        step_animation(s, burrow_hop_script, s->flags, &wrapped);
        if (!wrapped) break;
        burrow_size(g, d, 32, 45, 105);
        reset_animation(s);
        s->vx = 0; s->t0 = 12; s->vy = 0; s->flags &= ~4;
        set_aux_c(s, 1);
        break;
    case 5:
    dig:                                                            /* an attribute sent it digging: hidden, then up again */
        step_animation(s, burrow_dig_script, 0, &wrapped);
        if (!wrapped) break;
        reset_animation(s);
        d->dflags |= 4;
        set_aux_c(s, 6);
        s->t0 = 50;
        /* fall through */
    case 6:
        if (--s->t0) goto far;
        set_aux_c(s, 7);
        d->dflags &= (uint8_t)~4;
        /* fall through */
    case 7:
        step_animation(s, burrow_up_script, 0, &wrapped);
        if (!wrapped) break;
        reset_animation(s);
        set_frame2(s, 0, 12, s->flags);
        s->flags |= 0x20;
        set_aux_c(s, 1);
        break;
    default: break;
    }
    touch_player(g, d->x, d->y, 32, 16);
    if (ball_hits_object(g, s, 0, 0, 32, 32, 16, 0x300) >= 0) {
        burrow_size(g, d, 32, 45, 105);
        set_frame2(s, 16, 16, 0);
    }
far:
    if (point_offscreen(g, d->x, d->y, 160, 640) < 0) free_object(g, s);
}

/* ---- type 74: the big walker (slot 127, 64x64) ---- */
static void obj_walker74_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x119D16 */
    init_object_slot(g, s, x, y - 48, s->type4);
    s->vx = 2; s->vy = 0; s->hits = 3;
    s->flags |= 8;
    const uint8_t *f = res(g, 127);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 64, f, 1);                                  /* frames_64x64 */
}
static void obj_walker74_update(sk_game *g, sk_slot *s, int index) {          /* 0x119D50 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (!(s->flags & 2)) {
        int wrapped = 0;
        step_animation(s, walker74_script, s->flags, &wrapped);
        if (ground_ahead(g, s, s->flags)) s->flags ^= 1;
        touch_player(g, d->x, d->y, 64, 64);
        ball_hits_object(g, s, 0, 0, 64, 64, 8, 0x500);
    }
    despawn_if_far(g, s);
}

/* ---- type 78: the second walker (slots 116, 117 when knocked down) ---- */
static void obj_walker78_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x119AA4 */
    init_object_slot(g, s, x, y - 34, s->type4);
    s->vx = 4; s->vy = 0; s->hits = 2;
    s->flags |= 8;
    const uint8_t *f = res(g, 116);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 50, f, 1);                                  /* sub_107F88 */
}
static void obj_walker78_update(sk_game *g, sk_slot *s, int index) {          /* 0x119ADE */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (!(s->flags & 2)) {
        int wrapped = 0;
        step_animation(s, walker78_script, s->flags, &wrapped);
        if ((uint16_t)iabs((int16_t)(d->x - s->x)) >= 0xB4 || ground_ahead(g, s, s->flags)) s->flags ^= 1;
        touch_player(g, d->x + 10, d->y, 64, 40);
        if (ball_hits_object(g, s, 0, 10, 64, 40, 0, 0x400) >= 0) size_frames(g, d, 64, 64, res(g, 117));
    }
    despawn_if_far(g, s);
}

/* ---- type 11: the sprout (slot 3): grows, bursts, rests; its flags word counts the rest ---- */
static void obj_sprout_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x10EF0C */
    init_object_slot(g, s, x - 7, y - 22, s->type4);
    int p = g->objs.marker_param & 0xFF;
    s->t0 = (int16_t)((p & 0xF) ? (p & 0xF) : 1);
    s->t1 = (int16_t)(((p & 0xF0) ? (p & 0xF0) : 1) * 2);
    s->aux = s->t0;
    const uint8_t *f = res(g, 3);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 32, f, 1);                                  /* frames_16x32 */
}
static void obj_sprout_update(sk_game *g, sk_slot *s, int index) {            /* 0x10EF56 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    bool count = flags_word(s) != 0;
    if (!count) {
        int wrapped = 0;
        if (step_animation(s, sprout_script, 0, &wrapped) && s->frame == 0) {
            d->dflags |= 4;
            set_flags_word(s, 10);
            s->aux = (int16_t)(s->aux - 1);
            if (s->aux == 0) { s->aux = s->t0; set_flags_word(s, s->t1); }
            count = true;
        }
    }
    if (count) {
        set_flags_word(s, flags_word(s) - 1);
        if (flags_word(s)) { despawn_if_far(g, s); return; }
        d->dflags &= (uint8_t)~4;
    }
    if (s->frame >= 4) touch_player(g, d->x + 2, d->y, 12, 20);
    despawn_if_far(g, s);
}

/* ---- type 20: the cannon shell (slot 18; the flash from slot 19, the burst from slot 20: type 115) ---- */
static void obj_shell_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x110242 */
    init_object_slot(g, s, x + 7, y + 2, s->type4);
    if (!(g->objs.marker_param & 1)) { s->flags |= 1; s->x = (int16_t)(s->x - 14); }
    s->vx = 8; s->vy = 0;
    const uint8_t *f = res(g, 18);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 11, f, 1);                                  /* sub_107A16 */
    slot_draw_record(g, s)->dflags |= 4;
}
static void obj_shell_update(sk_game *g, sk_slot *s, int index) {             /* 0x110286 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    s->t0++;
    sk_objects *o = &g->objs;
    switch (aux_c(s)) {
    case 0:                                                         /* fire: the muzzle flash */
        d->dflags |= 4;
        o->child_frames = res(g, 19); o->child_script = shell_flash_script;
        o->child_w = 16; o->child_h = 16;
        o->child_vx = (s->flags & 1) ? 0 : (int16_t)0x8000;
        o->child_vy = 0;
        spawn_object(g, d->x, d->y - 3, 115);
        set_aux_c(s, 1);
        /* fall through */
    case 1:
        if ((uint16_t)s->t0 < 16) break;
        d->dflags &= (uint8_t)~4;
        set_aux_c(s, 2);
        /* fall through */
    case 2:
        apply_velocity(s);
        step_animation_simple(s, shell_fly_script, s->flags);
        if (object_offscreen(g, d->x, d->y, 64) >= 0) {
            if (!wall_ahead(g, s, s->flags)) { touch_player(g, d->x, d->y, 20, 10); break; }
            o->child_frames = res(g, 20); o->child_script = shell_burst_script;   /* into a wall: the burst */
            o->child_w = 16; o->child_h = 32;
            o->child_vx = 0;
            int bx = d->x + 15;
            if (!(s->flags & 1)) { bx -= 14; o->child_vx = (int16_t)0x8000; }
            o->child_vy = 0;
            spawn_object(g, bx, d->y - 10, 115);
        }
        set_aux_c(s, 3);
        /* fall through */
    case 3:
        d->dflags |= 4;
        if ((uint16_t)s->t0 < 100) break;
        s->t0 = 0;
        set_aux_c(s, 0);
        d->x = s->x;
        reset_animation(s);
        break;
    default: break;
    }
    despawn_if_far(g, s);
}

/* ---- type 42: the conveyor (no draw): pushes the kid and the ball right; type 98: its belt pieces (slot 67) ---- */
static int y0_hi(const sk_slot *s) { return (uint16_t)s->y0 >> 8; }                            /* byte +0x6 */
static void obj_conveyor_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x113256 */
    init_object_slot_shared(g, s, x + 16, y, s->type4, &g->objs.chain[7]);
    s->draw = NULL;                                                 /* the original sets no draw record */
    s->t0 = 18;
    s->t1 = 9 << 4;                                                 /* move.b #9,d0 overwrites the parameter: 144 px */
    s->x0 = 2; s->y0 = 0;
}
static void obj_conveyor_update(sk_game *g, sk_slot *s, int index) {          /* 0x113284 */
    (void)index;
    int d = (int16_t)(g->player.x - s->x);
    if (d >= 0 && (uint16_t)d < (uint16_t)s->t1) {
        int e = (int16_t)(g->player.y + 40 - (s->y - 16));
        if (e >= 0 && e < 32) g->player.carry_dx = s->x0;
    }
    d = (int16_t)((int16_t)(g->ball.x >> 16) + 8 - s->x);
    if (d >= 0 && (uint16_t)d < (uint16_t)s->t1) {
        int e = (int16_t)((int16_t)(g->ball.y >> 16) + 8 - (s->y - 16));
        if (e >= 0 && e < 32) g->bw.carry_dx = s->x0;
    }
    if (--s->t0 == 0) {
        int y = s->y;
        if (aux_c(s)) { s->y0 = (int16_t)((y0_hi(s) ^ 0xFF) << 8 | (s->y0 & 0xFF)); y -= 16; }
        int sp = y0_hi(s) ? 3 : 2;
        s->x0 = (int16_t)sp;
        g->objs.marker_param = sp * 2;
        spawn_object(g, s->x, y, 98);
        s->t0 = 18;
        set_aux_c(s, aux_c(s) ^ 0xFF);
    }
    if (object_offscreen(g, s->x, s->y, 160) < 0) free_object(g, s);
}
static void obj_belt_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x113342 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->vx = (int16_t)(g->objs.marker_param & 0xFF);
    s->vy = 0;
    s->flags |= 1;
    const uint8_t *f = res(g, 67);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 10, f, 1);                                  /* frames_16x10 */
}
static void obj_belt_update(sk_game *g, sk_slot *s, int index) {              /* 0x113376 */
    (void)index;
    if (!slot_draw_record(g, s)) return;
    apply_velocity(s);
    if (wall_ahead(g, s, s->flags)) { free_object(g, s); return; }
    step_animation_simple(s, s->vx == 4 ? belt_fast_script : belt_script, s->flags);
    free_if_far(g, s);
}

/* ---- type 46: the gunner (slots 60, 61 its debris, 62 the splash): fires both ways (type 97), splashes off the ship ---- */
static void obj_gunner_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x112B7C */
    init_object_slot(g, s, x, y - 45, s->type4);
    s->vx = 2; s->vy = 0; s->t0 = 0; s->hits = 4;
    s->flags |= 8;
    const uint8_t *f = res(g, 60);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 61, f, 1);                                  /* sub_107FCC */
}
static void obj_gunner_update(sk_game *g, sk_slot *s, int index) {            /* 0x112BB6 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    sk_objects *o = &g->objs;
    if (!(s->flags & 2)) {
        int wrapped = 0;
        if (!aux_c(s)) {
            step_animation(s, gunner_walk_script, s->flags, &wrapped);
            if (s->t0) s->t0--;
            else {
                int k = (int16_t)(d->x + 32 - g->player.x);
                if (s->flags & 1) k = -k;
                if (k >= 0x5A && k <= 0x64) { reset_animation(s); set_aux_c(s, 1); s->vx = 0; }
            }
            if ((uint16_t)iabs((int16_t)(s->x - d->x)) >= 0xA0) s->flags ^= 1;
        } else if (step_animation(s, gunner_fire_script, s->flags, &wrapped)) {
            int fx = d->x - 16;
            bool fire = s->frame == 13;
            if (!fire) { fx += 0x50; fire = s->frame == 18; }
            if (fire) { o->marker_param = s->flags & 0xFF; spawn_object(g, fx, d->y, 97); }
            if (s->anim_index == 15) { reset_animation(s); set_aux_c(s, 0); s->vx = 2; s->t0 = 50; }
        }
        touch_player(g, d->x + 12, d->y, 40, 61);
        if (ball_hits_object(g, s, 0, 0, 64, 61, 8, 0x1000) < 0) { despawn_if_far(g, s); return; }
        o->child_frames = res(g, 61);                               /* knocked: its debris */
        o->child_w = 32; o->child_h = 15;
        o->child_vx = 0; o->child_vy = -12;
        spawn_object(g, d->x + 16, d->y, 116);
        set_aux_c(s, 0);
    }
    if (!aux_c(s) && object_offscreen(g, d->x, d->y, 0x8C) < 0) {   /* off the ship: the splash, once */
        set_aux_c(s, 1);
        o->child_frames = res(g, 62); o->child_script = splash_script;
        o->child_w = 64; o->child_h = 48;
        o->child_vx = 0; o->child_vy = 0;
        spawn_object(g, d->x, g->camera_y + 0x98, 114);
    }
    despawn_if_far(g, s);
}

/* ---- type 97: the gunner's bullet (slot 66; the burst slot 68) ---- */
static void obj_bullet_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x112D98 */
    init_object_slot_nodraw(g, s, x, y + 16, s->type4);
    s->flags = (int16_t)(g->objs.marker_param & 1);
    s->vx = 8;
    set_aux_c(s, 0);
    const uint8_t *f = res(g, 66);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 11, f, 1);                                  /* sub_107A16 */
}
static void obj_bullet_update(sk_game *g, sk_slot *s, int index) {            /* 0x112DD0 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (aux_c(s)) {
        int wrapped = 0;
        step_animation(s, bullet_script, 0, &wrapped);
        if (wrapped) { free_object(g, s); return; }
    } else {
        int dx = (uint16_t)s->vx >> 1;                              /* sub_10E07C */
        d->x = (int16_t)(d->x + ((s->flags & 1) ? dx : -dx));
        step_animation_simple(s, bullet_script, s->flags);
        if (touch_player(g, d->x, d->y, 32, 11) != 0) {
            set_aux_c(s, 1);
            d->x = (int16_t)(d->x + ((s->flags & 1) ? 16 : -16));
            reset_animation(s);
            size_frames(g, d, 32, 28, res(g, 68));
        }
    }
    if (object_offscreen(g, d->x, d->y, 64) < 0) free_object(g, s);
}

/* ---- type 114: a one-shot effect from the child block that hurts (the gunner's splash) ---- */
static void obj_hurt_effect_init(sk_game *g, sk_slot *s, int x, int y) {      /* 0x10FD76 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 |= (int16_t)0x8000;
    if (g->objs.child_vx & 0x8000) { g->objs.child_vx = (int16_t)(g->objs.child_vx & 0x7FFF); s->flags |= 1; }
    s->vx = g->objs.child_vx;
    s->vy = g->objs.child_vy;
    if (s->vy) s->flags |= 4;
    s->script = g->objs.child_script;
    set_draw_size_at(g, s, g->objs.child_w, g->objs.child_h, g->objs.child_frames);
    sk_draw *d = slot_draw_record(g, s);
    if (d) d->dflags |= 4;
}
static void obj_hurt_effect_update(sk_game *g, sk_slot *s, int index) {       /* 0x10FDD4 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    d->dflags &= (uint8_t)~4;
    int wrapped = 0;
    if (s->script && d->frames) step_animation(s, s->script, s->flags, &wrapped);
    else wrapped = -1;
    if (wrapped) { free_object(g, s); return; }
    touch_player(g, d->x, d->y, d->w, d->h);
    free_if_far(g, s);
}

/* ---- type 52: the wall cannon (a fixed record, slot 82): its shot (type 91) and flash (type 90, slot 83) ---- */
static void obj_cannon_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x1133C6 */
    init_object_slot_fixed(g, s, x, y, s->type4);
    s->flags = (int16_t)(g->objs.marker_param & 1);
    s->t0 = 100;
    const uint8_t *f = res(g, 82);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 13, f, 1);                                  /* sub_1078C2 */
    set_frame2(s, 0, 8, s->flags);
    if (s->flags & 1) s->draw->x = (int16_t)(s->draw->x - 1);
}
static void obj_cannon_update(sk_game *g, sk_slot *s, int index) {            /* 0x11340C */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    bool run = true;
    if (!aux_c(s)) { if (--s->t0) run = false; else set_aux_c(s, 1); }
    if (run) {
        int wrapped = 0;
        int changed = step_animation(s, cannon_script, s->flags, &wrapped);
        if (wrapped) { reset_animation(s); s->t0 = 100; set_aux_c(s, 0); }
        if (changed && s->anim_index == 4) {
            g->objs.marker_param = s->flags & 0xFF;
            spawn_object(g, d->x, d->y, 91);
            g->objs.marker_param = s->flags & 0xFF;
            spawn_object(g, d->x, d->y, 90);
        }
    }
    if (object_offscreen(g, s->x, s->y, 64) < 0) free_fixed_object(g, s);
}
static void obj_cannon_shot_init(sk_game *g, sk_slot *s, int x, int y) {      /* 0x11349C */
    init_object_slot_fixed_nodraw(g, s, x, y, s->type4);
    s->flags = (int16_t)(g->objs.marker_param & 1);
    s->x = (int16_t)(s->x - 8 + ((s->flags & 1) ? 16 : 0));
    s->vx = 10; s->vy = 0;
    const uint8_t *f = res(g, 82);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 13, f, 1);                                  /* sub_1078C2 */
}
static void obj_cannon_shot_update(sk_game *g, sk_slot *s, int index) {       /* 0x1134DE */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    int dx = (uint16_t)s->vx >> 1;                                  /* sub_10E07C */
    d->x = (int16_t)(d->x + ((s->flags & 1) ? dx : -dx));
    int wrapped = 0;
    step_animation(s, cannon_shot_script, s->flags, &wrapped);
    touch_player(g, d->x, d->y, d->w, d->h);
    if (object_offscreen(g, d->x, d->y, 64) < 0) free_fixed_object(g, s);
}
static void obj_cannon_flash_init(sk_game *g, sk_slot *s, int x, int y) {     /* 0x113526 */
    init_object_slot_fixed_nodraw(g, s, x, y - 7, s->type4);
    s->flags = (int16_t)(g->objs.marker_param & 1);
    s->x = (int16_t)(s->x - 16 + ((s->flags & 1) ? 32 : 0));
    const uint8_t *f = res(g, 83);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 26, f, 1);                                  /* sub_10798E */
}
static void obj_cannon_flash_update(sk_game *g, sk_slot *s, int index) {      /* 0x113562 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    int wrapped = 0;
    step_animation(s, cannon_flash_script, s->flags, &wrapped);
    if (wrapped || object_offscreen(g, d->x, d->y, 64) < 0) free_fixed_object(g, s);
}

/* ---- type 71: the sea leaper (slot 109): jumps out of the sea 100 px under its marker, dives, waits 25 frames ---- */
static void obj_sea_leaper_init(sk_game *g, sk_slot *s, int x, int y) {       /* 0x119718 */
    init_object_slot(g, s, x, y + 100, s->type4);
    s->vx = 2; s->vy = -28; s->hits = 2;
    s->flags |= 4 | 8;
    const uint8_t *f = res(g, 109);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 40, f, 1);                                  /* sub_107F44 */
}
static void obj_sea_leaper_update(sk_game *g, sk_slot *s, int index) {        /* 0x11975A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (s->flags & 2) { apply_velocity(s); despawn_if_far(g, s); return; }
    if (!aux_c(s)) {
        apply_velocity(s);
        int fr = 0;
        if (!(s->vy < -4)) { s->vx = 4; fr = 1; }
        set_frame2(s, fr, fr, 0);
        if ((int16_t)(d->y - s->y) >= 0) {                          /* back in the sea */
            d->dflags |= 4;
            s->vx = 2; s->vy = -28;
            d->x = s->x;
            s->t0 = 25;
            set_aux_c(s, 1);
        }
    } else if (--s->t0 == 0) { d->dflags &= (uint8_t)~4; set_aux_c(s, 0); }
    touch_player(g, d->x + 4, d->y, 56, 32);
    ball_hits_object(g, s, 8, 0, 48, 32, 2, 0x400);
    despawn_if_far(g, s);
}

/* ---- types 40 and 41: the sprite occluders (no picture): a 32x13 mask from slot 50, or a blank column param * 16 - 4 rows high ---- */
static int alloc_occluder(sk_game *g) {                             /* sub_107502: -1 full (the original goes on regardless) */
    g->occl_count++;
    for (int i = 0; i < 4; i++) if (!g->occl[i].used) return i;
    return -1;
}
static void occluder_init(sk_game *g, sk_slot *s, int x, int y, bool shaped) {
    init_object_slot_shared(g, s, x, y + 4, s->type4, &g->objs.chain[7]);
    s->draw = NULL;
    int i = alloc_occluder(g);
    s->t0 = (int16_t)i;
    if (i < 0) return;                                              /* port guard: a fifth occluder writes nowhere */
    g->occl[i].used = true;
    g->occl[i].x = s->x; g->occl[i].y = s->y;
    if (shaped) {
        size_t len = 0;
        g->occl[i].mask = g->res ? resource_slot(g->res, 50, &len) : NULL;
        g->occl[i].mask_len = g->occl[i].mask ? len : 0;
        g->occl[i].h = 13;
    } else {
        g->occl[i].mask = NULL; g->occl[i].mask_len = 0;
        g->occl[i].h = (int16_t)((g->objs.marker_param & 0xFF) * 16 - 4);
    }
}
static void obj_occluder_mask_init(sk_game *g, sk_slot *s, int x, int y) { occluder_init(g, s, x, y, true); }    /* 0x1122E0 */
static void obj_occluder_rect_init(sk_game *g, sk_slot *s, int x, int y) { occluder_init(g, s, x, y, false); }   /* 0x11232A */
static void obj_occluder_update(sk_game *g, sk_slot *s, int index) {          /* 0x11230E / 0x112358 */
    (void)index;
    if (object_offscreen(g, s->x, s->y, 64) >= 0) return;
    g->occl_count--;                                                /* sub_107528 */
    if (s->t0 >= 0 && s->t0 < 4) g->occl[s->t0].used = false;
    free_object(g, s);
}

/* ---- type 104: the roller (slot 22) attribute 60 lets go: at an attribute-52 cell it sets the mine (type 35) off ---- */
static void obj_roller_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x111A46 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->flags |= 1;
    s->vx = 4; s->vy = 0;
    const uint8_t *f = res(g, 22);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 15, f, 1);                                  /* sub_107906 */
}
static void obj_roller_update(sk_game *g, sk_slot *s, int index) {            /* 0x111A72: it never despawns otherwise */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    step_animation_simple(s, roller_script, 0);
    g->attrs.probe_slot = s;
    probe_tile_attr(g, d->x - 8, d->y + 8);                         /* probe_tile_attr_saved */
    if (!(s->flags & 0x10)) return;
    s->flags &= ~0x10;
    sk_slot *mine = find_slot_by_type(g, 35);
    if (mine) set_aux_c(mine, 0xFF);                                /* port guard: the original writes through a1 regardless */
    free_object(g, s);
}

/* ---- type 101: the falling piece attribute 65 lets go (slot 51) ---- */
static void obj_falling_piece_init(sk_game *g, sk_slot *s, int x, int y) {    /* 0x112426 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 |= (int16_t)0x8000;
    s->vx = 0; s->vy = 0;
    s->t0 = (int16_t)((g->frame_ticks & 3) + 3);                    /* its top speed */
    const uint8_t *f = res(g, 51);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 12, f, 1);                                  /* sub_1078A0 */
    int fr = g->frame_ticks & 3;
    set_frame2(s, fr == 3 ? 2 : fr, 0, 0);
}
static void obj_falling_piece_update(sk_game *g, sk_slot *s, int index) {     /* 0x112480 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (!((uint16_t)s->t0 < (uint16_t)s->vy)) s->vy++;
    d->y = (int16_t)(d->y + ((uint16_t)s->vy >> 1));
    if (object_offscreen(g, d->x, d->y, 64) < 0) { free_object(g, s); g->flutter_count--; }
}

/* ---- type 76: RED SQUARE 2's boss (slots 133, 132, 134) with the dust puff (type 118); dies on the 21st hit ---- */
static bool once(sk_slot *s) {                                      /* bset.b #0,aux.lo; bne: the first frame of a phase */
    if (aux_c(s) & 1) return false;
    set_aux_c(s, aux_c(s) | 1);
    return true;
}
static void obj_rboss2_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x115F16 */
    init_object_slot(g, s, x, y + 4, s->type4);
    s->vx = 0; s->vy = 0; s->hits = 20;
    const uint8_t *f = res(g, 133);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 64, f, 1);                                  /* sub_107D02 */
    set_frame2(s, 3, 3, 0);
    g->push_x = s->x - 0xA0;
    spawn_object(g, s->x, s->y, 118);
    game_music_slot(g, 1);
}
static void obj_rboss2_update(sk_game *g, sk_slot *s, int index) {            /* 0x115F74 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    int changed = 0, code = script_step(g, s, rboss2_phases, rboss2_motions, &changed);
    enum { COMMON, CHECKS, LAST } tail = COMMON;
    switch (code) {
    case 0x04: {
        int r = probe_rows_below2(g, s, 16, 0);
        if (r < 0) { d->y = (int16_t)(d->y + r); script_end_next(s, rboss2_phases); }
        break;
    }
    case 0x0C: {                                                    /* the dust at its feet */
        sk_slot *puff = find_slot_by_type(g, 118);
        if (puff) { set_aux_c(puff, aux_c(puff) | 1); puff->t0 = (int16_t)(d->x + 32); puff->t1 = (int16_t)(d->y + 50); }
        if (once(s)) size_frames(g, d, 48, 64, res(g, 132));
        tail = CHECKS;
        break;
    }
    case 0x10: s->flags ^= 1; script_end_next(s, rboss2_phases); tail = CHECKS; break;
    case 0x18: if (once(s)) size_frames(g, d, 48, 48, res(g, 134)); break;
    case 0x1C: if (once(s)) { size_frames(g, d, 48, 64, res(g, 132)); set_frame2(s, 20, 20, 0); } break;
    case 0x20: if (once(s)) { size_frames(g, d, 32, 64, res(g, 133)); set_frame2(s, 8, 7, s->flags); } tail = CHECKS; break;
    case 0x28: if (once(s)) { size_frames(g, d, 48, 64, res(g, 132)); set_frame2(s, 12, 15, s->flags); } tail = LAST; break;
    case 0x2C:
        if (once(s)) {
            size_frames(g, d, 48, 64, res(g, 132));
            set_frame2(s, 21, 21, 0);
            g->score_value = 0x20000;
            add_score(g, 0x20000);
            spawn_object(g, d->x + 32, d->y + 24, 113);
            play_sfx(g, 0x11);
            g->shake_amplitude = 6;
            game_music_slot(g, 2);
        }
        tail = LAST;
        break;
    case 0x30: tail = LAST; break;
    case 0x34: if (once(s)) { g->level_outcome = 0x81; g->level_number++; g->curtain_flags |= 0x10; } tail = LAST; break;
    case 0x38: if (once(s)) size_frames(g, d, 32, 64, res(g, 133)); tail = CHECKS; break;
    case 0x3C: tail = CHECKS; break;
    default: break;                                                 /* 0x00 0x08 0x14 0x24 */
    }
    if (tail == CHECKS) {                                           /* 0x1161C2 */
        if (aux_c(s) & 2) script_start(s, 26, rboss2_phases, rboss2_motions);   /* the kid lost: it gloats */
        if (!(aux_c(s) & 4) && (int8_t)s->hits < 0) {
            d->dflags &= (uint8_t)~2;
            set_aux_c(s, aux_c(s) | 4);
            script_start(s, 23, rboss2_phases, rboss2_motions);
        }
    }
    if (tail != LAST) {                                             /* 0x116200 */
        if (!(aux_c(s) & 4)) {
            if (g->player.dying) set_aux_c(s, aux_c(s) | 2);
            touch_player(g, d->x + 8, d->y + 16, 32, 48);
        }
        ball_hits_object_simple(g, s, 8, 10, 32, 54);
    }
    despawn_if_far(g, s);
}

/* ---- type 1: the rope (objects_b, links from tiles 496 / 497 with their tables as the masks); the kid's side is state 12 ---- */
static void rope_layout(sk_game *g, bool tiles_b) {                 /* rope_layout_b 0x10E95A (rope_layout_a: param bit 0 clear, unused by the levels) */
    sk_objects *o = &g->objs;
    int p = g->objs.marker_param & 0xFF;
    int link_h = ((((p << 3) | (p >> 5)) & 7) + 1) * 2, end_h = (((p >> 2) & 7) + 1) * 2;   /* rope_link_sizes 0x10EA2C */
    const sk_level *lv = g->world.lv;
    const uint8_t *img = NULL, *tail = NULL;
    for (int i = 0; i < o->rope_links && i < 8; i++) {
        sk_draw *d = &o->rope[i];
        bool last = i == o->rope_links - 1;
        int tile = last ? 497 : 496;
        img = lv && lv->tiles_a.tiles && tile < lv->tiles_a.count ? lv->tiles_a.tiles + (size_t)tile * SK_TILE_BYTES : NULL;
        tail = lv && lv->tiles_a.tails && tile < lv->tiles_a.count ? lv->tiles_a.tails + (size_t)tile * SK_TILE_TAIL : NULL;
        memset(d, 0, sizeof *d);
        d->w = 16; d->h = (int16_t)(last ? end_h : link_h);
        d->blit_size = (int16_t)(d->h << 6 | 2);
        d->plane_bytes = 0x20; d->frame_bytes = 0x80;
        d->frames = d->frame = img;                                 /* the tile's 128 bytes read as four planes of 32 */
        d->mask = tail;                                             /* the next 32: the tile's collision table */
        d->prio = 0; d->dflags = tiles_b ? 1 : 0;
        d->used = true;
    }
}
static void obj_rope_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x10E7A2 */
    sk_objects *o = &g->objs;
    if (o->rope_active) { obj_stub_init(g, s, x, y); free_object(g, s); return; }   /* obj_none_init: one rope at a time */
    init_object_slot_shared(g, s, x, y, s->type4, &o->chain[7]);
    s->draw = NULL;
    s->x = (int16_t)(s->x - 8);
    o->rope_active = 1;
    o->rope_link_step = 15;
    o->rope_links = 6;
    rope_layout(g, g->objs.marker_param & 1);
    if (o->strip_row) { o->rope_angle = 0x1000; o->rope_swing = 0; }
    else { o->rope_angle = 0x4000; o->rope_swing = 0x280; }
}
static int16_t shr8_toward_zero(int v) { return (int16_t)(v < 0 ? -((-v) >> 8) : (v & 0xFFFF) >> 8); }   /* neg, lsr.l, neg / lsr.w */
static void obj_rope_update(sk_game *g, sk_slot *s, int index) {              /* 0x10E816 */
    (void)index;
    sk_objects *o = &g->objs;
    o->rope_active = -1;
    o->rope_base_x = s->x; o->rope_base_y = s->y;
    int r = o->rope_link_step;                                      /* rope_place_links 0x10E8D6 */
    uint16_t a = o->rope_angle;
    int bend = (int16_t)(((o->rope_angle >> 8) - 0x40) << 3);
    int lx = 0, ly = 0;
    for (int i = 0; i < o->rope_links && i < 8; i++) {
        int sn = sin_1024(a >> 6), cs = cos_1024(a >> 6);
        lx = (int16_t)(shr8_toward_zero(cs * r) + o->rope_base_x);
        ly = (int16_t)(shr8_toward_zero(sn * r) + o->rope_base_y);
        o->rope[i].x = (int16_t)lx; o->rope[i].y = (int16_t)ly;
        r += o->rope_link_step;
        a = (uint16_t)(a + bend);
    }
    o->rope_end_x0 = o->rope_end_x; o->rope_end_y0 = o->rope_end_y;  /* rope_step 0x10E88C */
    o->rope_end_x = (int16_t)lx; o->rope_end_y = (int16_t)ly;
    o->rope_swing = (int16_t)(o->rope_swing + 16);
    if ((uint16_t)(o->rope_angle + 0x4000) > 0x8000) o->rope_swing = (int16_t)(o->rope_swing - 32);
    o->rope_angle = (uint16_t)(o->rope_angle + (o->rope_swing & 0xFFF0));
    if (g->player.on_rope < 0) { g->player.x = o->rope_end_x; g->player.y = o->rope_end_y; }   /* the kid hangs on the end */
    int hi = o->rope_angle >> 8;
    if (object_offscreen(g, o->rope_base_x, o->rope_base_y, 64) < 0 && hi >= 0x3C && hi < 0x44
        && object_offscreen(g, o->rope_end_x, o->rope_end_y, 64) < 0) {
        o->rope_active = 0;
        free_object(g, s);
    }
}

void obj_russia_install(void) {
    object_init_table[11] = obj_sprout_init;           object_update_table[11] = obj_sprout_update;
    object_init_table[20] = obj_shell_init;            object_update_table[20] = obj_shell_update;
    object_init_table[42] = obj_conveyor_init;         object_update_table[42] = obj_conveyor_update;
    object_init_table[98] = obj_belt_init;             object_update_table[98] = obj_belt_update;
    object_init_table[46] = obj_gunner_init;           object_update_table[46] = obj_gunner_update;
    object_init_table[97] = obj_bullet_init;           object_update_table[97] = obj_bullet_update;
    object_init_table[114] = obj_hurt_effect_init;     object_update_table[114] = obj_hurt_effect_update;
    object_init_table[52] = obj_cannon_init;           object_update_table[52] = obj_cannon_update;
    object_init_table[91] = obj_cannon_shot_init;      object_update_table[91] = obj_cannon_shot_update;
    object_init_table[90] = obj_cannon_flash_init;     object_update_table[90] = obj_cannon_flash_update;
    object_init_table[71] = obj_sea_leaper_init;       object_update_table[71] = obj_sea_leaper_update;
    object_init_table[40] = obj_occluder_mask_init;    object_update_table[40] = obj_occluder_update;
    object_init_table[41] = obj_occluder_rect_init;    object_update_table[41] = obj_occluder_update;
    object_init_table[104] = obj_roller_init;          object_update_table[104] = obj_roller_update;
    object_init_table[101] = obj_falling_piece_init;   object_update_table[101] = obj_falling_piece_update;
    object_init_table[76] = obj_rboss2_init;           object_update_table[76] = obj_rboss2_update;
    object_init_table[1] = obj_rope_init;              object_update_table[1] = obj_rope_update;
    object_init_table[35] = obj_mine_init;             object_update_table[35] = obj_mine_update;
    object_init_table[65] = obj_sled_init;             object_update_table[65] = obj_sled_update;
    object_init_table[66] = obj_icicle_init;           object_update_table[66] = obj_icicle_update;
    object_init_table[74] = obj_walker74_init;         object_update_table[74] = obj_walker74_update;
    object_init_table[78] = obj_walker78_init;         object_update_table[78] = obj_walker78_update;
    object_init_table[72] = obj_burrower_init;         object_update_table[72] = obj_burrower_update;
}
