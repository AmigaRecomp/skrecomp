/* England London's object types, transcribed from the listing (objects.md and gameplay.md "Platform records").
   The platform types keep their state in the slot: t0 = the turn phase (attribute 23), t1's high byte the speed
   to reach and its low byte the new angle, aux_d = 4 while an attribute 23 cell is under the probe, anim (+0x1C)
   the param's y offset, timer (+0x21) dormant. */
#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/obj_london.h"
#include "game/platforms.h"
#include "game/player.h"
#include "game/resources.h"

static const uint16_t platform_margins[16] = {                 /* 0x1177D6 (the same at 0x117E9E): (x, y) by angle / 32 */
    0x280, 0xA0, 0x1E0, 0x1E0, 0xA0, 0x1E0, 0x1E0, 0x1E0, 0x280, 0xA0, 0x1E0, 0x1E0, 0xA0, 0x1E0, 0x1E0, 0x1E0,
};
static const int8_t probe_dirs[16] = { 1, 0, 1, 1, 0, 1, -1, 1, -1, 0, -1, -1, 0, -1, 1, -1 };   /* 0x11A562 */

static int16_t hi(int32_t v) { return (int16_t)(v >> 16); }
static int32_t with_hi(int32_t v, int h) { return (int32_t)((uint32_t)(uint16_t)h << 16 | ((uint32_t)v & 0xFFFF)); }
static int t1_hi(const sk_slot *s) { return (uint16_t)s->t1 >> 8; }
static int t1_lo(const sk_slot *s) { return s->t1 & 0xFF; }

void probe_tile_attr_dir(sk_game *g, sk_slot *s) {               /* 0x11A4FE */
    sk_platform *pf = slot_platform(g, s);
    if (!pf) return;
    int x = hi(pf->px), y = (int8_t)s->anim + hi(pf->py) + pf->half_h;
    int k = (pf->pangle >> 4) & 0xFE;
    if (probe_dirs[k] < 0) x -= pf->half_w + 8; else if (probe_dirs[k] > 0) x += pf->half_w + 8;
    if (probe_dirs[k + 1] < 0) y -= pf->half_h + 8; else if (probe_dirs[k + 1] > 0) y += pf->half_h + 8;
    g->attrs.probe_slot = s;
    probe_tile_attr(g, x, y);
    if (g->attrs.probe_kind != 0xFF) set_aux_d(s, 0);
}

/* The shared init of types 5 and 10 (0x117576 / 0x117C40). */
static sk_platform *platform_init(sk_game *g, sk_slot *s, int x, int y) {
    x += 8;
    init_object_slot(g, s, x, y, s->type4);
    s->timer = 0;
    int index = -1;
    sk_platform *pf = alloc_platform(g, &index);
    s->platform = index;
    s->flags = (int16_t)(index < 0 ? 0xFF : (index * 32) >> 8);
    int p = g->objs.marker_param & 0xFF;
    const sk_section *sec = g->res ? g->res->param[(p & 0xC0) >> 6].sec : NULL;
    if (!pf || !sec) { if (pf) free_platform(g, s); free_object(g, s); return NULL; }   /* port guard */
    const uint8_t *b = g->res->param[(p & 0xC0) >> 6].bytes;
    pf->px = (int32_t)((uint32_t)(uint16_t)x << 16);
    pf->py = (int32_t)((uint32_t)(uint16_t)y << 16);
    pf->pmode = (uint8_t)(p & 1);
    pf->pspeed = (uint8_t)((p & 0x0E) << 4);
    pf->pangle = (uint8_t)((p & 0x30) << 2);
    pf->half_w = b[0];
    pf->half_h = b[1];
    pf->px = with_hi(pf->px, hi(pf->px) + (int8_t)b[2]);
    s->anim = b[3];
    pf->py = with_hi(pf->py, hi(pf->py) + (int8_t)b[3]);
    pf->stand_dy = b[5];
    pf->frame_sel = b[4] & 0x7F;
    set_draw_size_at(g, s, 2 * pf->half_w, 2 * pf->half_h, sec->data);
    sk_draw *d = slot_draw_record(g, s);
    if (d) {
        d->frames_len = sec->len;
        d->dflags = (uint8_t)(!(b[4] & 0x80));                     /* move.b d2,$1b(a0): the clipped-blitter bit */
        int off = (uint16_t)d->frame_bytes - (pf->frame_sel ? 0 : (uint16_t)d->plane_bytes);
        pf->pframe = (size_t)off < sec->len ? d->frames + off : NULL;
    }
    return pf;
}

/* The turn after attribute 23 (t0 1: slow to a stop, t0 2: speed back up the other way). */
static void platform_turn(sk_slot *s, sk_platform *pf) {
    if (s->t0 == 1) {
        pf->pspeed = (uint8_t)(pf->pspeed - 8);
        if (!((int8_t)pf->pspeed < 0)) { platform_velocity(pf->pangle, pf->pspeed, &pf->pvx, &pf->pvy); return; }
        s->t0 = 2;
        pf->pangle = (uint8_t)t1_lo(s);
    } else if (s->t0 != 2) return;
    pf->pspeed = (uint8_t)(pf->pspeed + 8);
    if (t1_hi(s) == pf->pspeed) s->t0 = 0;
    platform_velocity(pf->pangle, pf->pspeed, &pf->pvx, &pf->pvy);
}

static int platform_move(sk_game *g, sk_platform *pf) {
    int32_t nx = pf->px + pf->pvx, ny = pf->py + pf->pvy;
    int r = platform_carry(g, pf, (int16_t)(hi(nx) - hi(pf->px)), (int16_t)(hi(ny) - hi(pf->py)));
    pf->px = nx; pf->py = ny;
    return r;
}

static bool both_offscreen(sk_game *g, int x1, int y1, int x2, int y2, int mx, int my) {
    return point_offscreen(g, x1, y1, mx, my) < 0 && point_offscreen(g, x2, y2, mx, my) < 0;
}

/* ---- type 5: the moving platform ---- */
static void obj_platform_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x117576 */
    sk_platform *pf = platform_init(g, s, x, y);
    if (pf) platform_velocity(pf->pangle, pf->pspeed, &pf->pvx, &pf->pvy);
}
static void obj_platform_update(sk_game *g, sk_slot *s, int index) {           /* 0x11766C */
    (void)index;
    sk_platform *pf = slot_platform(g, s);
    if (!pf) return;
    if (s->timer) {                                                 /* dormant: wake when back in view */
        if (!both_offscreen(g, hi(pf->px), hi(pf->py), s->x, s->y, s->x0, s->y0)) {
            s->timer = 0;
            alloc_draw_record(g, s);
            s->draw = slot_draw_record(g, s);
            if (s->draw) s->draw->used = true;
            const uint8_t *base = pf->pframe;
            set_draw_size_at(g, s, 2 * pf->half_w, 2 * pf->half_h, base);
            sk_draw *d = slot_draw_record(g, s);
            if (d && base) pf->pframe = base + (uint16_t)d->frame_bytes - (uint16_t)d->plane_bytes;
        }
        return;
    }
    sk_draw *d = slot_draw_record(g, s);
    probe_tile_attr_dir(g, s);
    platform_turn(s, pf);
    platform_move(g, pf);
    if (!d) return;
    d->x = (int16_t)(hi(pf->px) - pf->half_w);
    d->y = hi(pf->py);
    int k = (pf->pangle >> 5) * 2;
    int mx = platform_margins[k], my = platform_margins[k + 1];
    if (both_offscreen(g, d->x, d->y, s->x, s->y, mx, my)) {       /* dormant: keep the slot and the record, drop the draw */
        s->x0 = (int16_t)mx; s->y0 = (int16_t)my;
        s->timer = 1;
        pf->pframe = d->frames;
        free_draw_record(g, d);
    }
}

/* ---- type 10: the platform that starts when stood on ---- */
static void obj_platform_trigger_init(sk_game *g, sk_slot *s, int x, int y) {  /* 0x117C40 */
    sk_platform *pf = platform_init(g, s, x, y);
    if (pf) { pf->pvx = 0; pf->pvy = 0; }
}
static void obj_platform_trigger_update(sk_game *g, sk_slot *s, int index) {   /* 0x117D2A */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    sk_platform *pf = slot_platform(g, s);
    if (!pf || !d) return;
    if (s->timer) {                                                 /* dormant: hidden until back in view */
        if (!both_offscreen(g, s->x, s->y, d->x, d->y, s->x0, s->y0)) { s->timer = 0; d->dflags &= ~4; }
        return;
    }
    if (pf->pvx || pf->pvy) probe_tile_attr_dir(g, s);
    platform_turn(s, pf);
    int r = platform_move(g, pf);
    if (!aux_c(s) && r) {                                           /* stood on: off it goes */
        set_aux_c(s, 0xFF);
        platform_velocity(pf->pangle, pf->pspeed, &pf->pvx, &pf->pvy);
    }
    d->x = (int16_t)(hi(pf->px) - pf->half_w);
    d->y = hi(pf->py);
    int k = (pf->pangle >> 5) * 2;
    int mx = platform_margins[k], my = platform_margins[k + 1];
    if (point_offscreen(g, hi(pf->px), hi(pf->py), mx, my) >= 0 || point_offscreen(g, s->x, s->y, mx, my) >= 0) return;
    if (!aux_c(s)) { free_platform(g, s); free_object(g, s); return; }
    s->x0 = (int16_t)mx; s->y0 = (int16_t)my;
    s->timer = 0xFF;
    d->dflags |= 4;
}

/* ---- type 26: the platform that sinks under the kid (COUNTRY2; 0x117EBE / 0x117F66) ---- */
static void obj_sinking_init(sk_game *g, sk_slot *s, int x, int y) {
    x += 8;
    init_object_slot(g, s, x, y, s->type4);
    s->timer = 0;
    int index = -1;
    sk_platform *pf = alloc_platform(g, &index);
    s->platform = index;
    s->flags = (int16_t)(index < 0 ? 0xFF : (index * 32) >> 8);
    const sk_section *sec = g->res ? g->res->param[2].sec : NULL;   /* move.w #$80,d0: always entry 2 */
    if (!pf || !sec) { if (pf) free_platform(g, s); free_object(g, s); return; }
    const uint8_t *b = g->res->param[2].bytes;
    pf->px = (int32_t)((uint32_t)(uint16_t)(x + (int8_t)b[2]) << 16);
    pf->py = (int32_t)((uint32_t)(uint16_t)(y + (int8_t)b[3]) << 16);
    pf->half_w = b[0]; pf->half_h = b[1];
    s->anim = b[3];
    s->y0 = hi(pf->py);                                            /* the rest height */
    pf->stand_dy = b[5];
    s->t0 = 2;
    set_draw_size_at(g, s, 2 * pf->half_w, 2 * pf->half_h, sec->data);
    sk_draw *d = slot_draw_record(g, s);
    if (d) { d->frames_len = sec->len; pf->pframe = (size_t)(4 * (uint16_t)d->plane_bytes) < sec->len ? d->frames + 4 * (uint16_t)d->plane_bytes : NULL; }
}
static void obj_sinking_update(sk_game *g, sk_slot *s, int index) {
    (void)index;
    sk_platform *pf = slot_platform(g, s);
    sk_draw *d = slot_draw_record(g, s);
    if (!pf || !d) return;
    int d4 = (int16_t)(s->t1 - s->t0), depth = (int16_t)(hi(pf->py) - s->y0), frame = 0, lift = 1;
    if (depth < 0) { if (d4 < 0) d4 = (g->frame_ticks & 7) ? 0 : 1; }
    else if (depth >= 32) { frame = 2; lift = 8; }
    else if (depth >= 16) { frame = 1; lift = 4; }
    set_object_frame(s, frame);
    pf->py = with_hi(pf->py, hi(pf->py) + d4);
    int r = platform_carry(g, pf, 0, d4);
    s->t1 = r ? 3 : 0;
    d->x = (int16_t)(hi(pf->px) - pf->half_w);
    d->y = (int16_t)(hi(pf->py) - lift);
    if (object_offscreen(g, s->x, s->y, 160) < 0) {
        g->mask.dirty = true; g->bw.mask_dirty = true;
        free_platform(g, s);
        free_object(g, s);
    }
}

/* ---- type 61: the stomper (slot 89; debris slot 98) ---- */
static const uint8_t stomper_walk_script[] = { 0, 6, 0, 8, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };   /* 0x113F1E */
static const uint8_t stomper_stomp_script[] = { 0, 11, 0, 3, 0, 0, 0, 1, 1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 0, 0, 0 };   /* 0x113F2E */
static void obj_stomper_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x113D80 */
    init_object_slot(g, s, x, y - 48, s->type4);
    s->vx = 2; s->vy = 0; s->flags |= 8; s->hits = 2; s->t0 = 0;
    const uint8_t *f = g->res ? resource_slot(g->res, 89, NULL) : NULL;
    if (!f) { free_object(g, s); return; }
    frames_64x64(g, s, f, 1);
}
static void obj_stomper_update(sk_game *g, sk_slot *s, int index) {           /* 0x113DBA */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    sk_player *p = &g->player;
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    bool stomp = aux_c(s) == 1;
    if (!stomp) {
        int wrapped = 0;
        step_animation(s, stomper_walk_script, s->flags, &wrapped);
        int dist = d->x - s->x;
        if (dist < 0) dist = -dist;
        if ((uint16_t)dist >= 200 || ground_ahead(g, s, s->flags)) s->flags ^= 1;
        if (s->t0) s->t0--;
        else {
            int dy = (int16_t)(p->y + 40 - d->y), dx = (int16_t)(d->x + 32 - p->x);
            if (dx < 0) dx = -dx;
            if (dy >= 0 && dy < 0x44 && dx < 0x5A) {
                s->anim_tick = 0; s->anim_index = 0;
                s->vx = 0; set_aux_c(s, 1); stomp = true;
            }
        }
    }
    if (stomp) {
        int wrapped = 0, changed = step_animation(s, stomper_stomp_script, 0, &wrapped);
        if (changed && s->anim_index == 7) {                       /* the stamp: the kid is thrown, the screen flashes */
            bounce_player(g);
            g->flash_timer = 16;
            g->flash_table = 0;
        }
        if (wrapped) { s->anim_tick = 0; s->anim_index = 0; s->vx = 2; set_aux_c(s, 0); s->t0 = 25; }
    }
    touch_player(g, d->x, d->y, 64, 64);
    if (ball_hits_object(g, s, 0, 0, 64, 64, 15, 0x500) >= 0) {
        sk_objects *o = &g->objs;
        const uint8_t *f = g->res ? resource_slot(g->res, 98, NULL) : NULL;
        o->child_frames = f; o->child_w = 32; o->child_h = 12; o->child_vx = 2; o->child_vy = -12;
        spawn_object(g, d->x + 16, d->y, 116);
        o->child_vx = -2; o->child_vy = -14;
        if (f) o->child_frames = f + 0xF0;
        spawn_object(g, d->x + 16, d->y + 32, 116);
    }
    despawn_if_far(g, s);
}

void obj_london_install(void) {
    object_init_table[26] = obj_sinking_init;          object_update_table[26] = obj_sinking_update;
    object_init_table[61] = obj_stomper_init;          object_update_table[61] = obj_stomper_update;
    object_init_table[5] = obj_platform_init;          object_update_table[5] = obj_platform_update;
    object_init_table[10] = obj_platform_trigger_init; object_update_table[10] = obj_platform_trigger_update;
}
