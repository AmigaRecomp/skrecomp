/* Russia's object types, transcribed from the listing. */
#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/obj_russia.h"
#include "game/resources.h"

static const uint8_t mine_script[] = { 0, 5, 0, 4, 0, 1, 2, 2, 255, 0, 1, 2, 2, 255 };          /* 0x111A38 */
static const uint8_t mine_piece_script[] = { 0, 1, 0, 2, 0, 1 };                               /* 0x111A32 */
static const uint8_t sled_scripts[3][8] = {                                                    /* 0x118886, 0x11888E, 0x118896 */
    { 0, 2, 0, 4, 0, 1, 6, 7 }, { 0, 2, 0, 4, 2, 3, 8, 9 }, { 0, 2, 0, 4, 4, 5, 10, 11 } };
static const uint8_t icicle_script[] = { 0, 8, 0, 3, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };   /* 0x1189D8 */

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static bool need(sk_game *g, sk_slot *s, const uint8_t *f) { if (f) return true; free_object(g, s); return false; }
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

void obj_russia_install(void) {
    object_init_table[35] = obj_mine_init;             object_update_table[35] = obj_mine_update;
    object_init_table[65] = obj_sled_init;             object_update_table[65] = obj_sled_update;
    object_init_table[66] = obj_icicle_init;           object_update_table[66] = obj_icicle_update;
}
