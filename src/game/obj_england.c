/* England 1 and 2's object types and the child types they spawn, transcribed from the listing (objects.md has the
   pseudo-code; where the two differ the listing wins and the ledger says so). Byte +0xC of the slot is aux_c,
   byte +0xD aux_d; byte-wide stores to +0x14 / +0x16 are kept in t0 / t1. Frames come from the level's resource
   slots (slot_table); a type whose slot is missing frees itself in its init. */
#include <string.h>
#include "game/game.h"
#include "game/attr_england.h"
#include "game/obj_england.h"
#include "game/obj_helpers.h"
#include "game/resources.h"

/* Scripts (count, ticks, frames facing left, frames facing right), copied from the image. */
static const uint8_t bird_flap_script[] = { 0, 4, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7 };           /* 0x10F0C6, 0x10F1C6 */
static const uint8_t bulldog_walk_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 4, 5, 6, 7 };        /* 0x10F632 */
static const uint8_t bonus_script[] = { 0, 3, 0, 6, 0, 1, 2, 0, 1, 2 };                     /* 0x110ED4 */
static const uint8_t butterfly_script[] = { 0, 4, 0, 9, 0, 1, 2, 3, 0, 1, 2, 3 };           /* 0x1120E4 */
static const int8_t butterfly_drift[4][9] = {                                               /* 0x11209C */
    { -5, -6, -7, -8, -8, -8, -7, -6, -5 }, { -4, -3, -2, -1, 0, 1, 2, 3, 4 },
    { 5, 6, 7, 8, 8, 8, 7, 6, 5 }, { 4, 3, 2, 1, 0, -1, -2, -3, -4 },
};
static const uint8_t workman_walk_script[] = { 0, 6, 0, 6, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };   /* 0x112B58 */
static const uint8_t workman_punch_script[] = { 0, 8, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };   /* 0x112B68 */
static const uint8_t rat_run_script[] = { 0, 3, 0, 6, 1, 2, 3, 8, 9, 10 };                  /* 0x113B5A */
static const uint8_t walker_walk_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 6, 7, 8, 9 };         /* 0x113D4C */
static const uint8_t walker_turn_script[] = { 0, 2, 0, 8, 4, 4, 5, 5 };                     /* 0x113D58 */
static const uint16_t walker_speed_table[16] = { 3, 3, 2, 2, 1, 1, 0, 0x8000, 2, 2, 4, 4, 4, 4, 4, 4 };   /* 0x113D60 */
static const uint8_t cyclist_ride_script[] = { 0, 8, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };   /* 0x1184E2 */
static const uint32_t popup_scores[19] = {                                                  /* 0x110098 */
    0x100, 0x150, 0x200, 0x250, 0x300, 0x400, 0x500, 0x750,
    0x1000, 0x1500, 0x2000, 0x2500, 0x3000, 0x4000, 0x5000,
    0x10000, 0x20000, 0x50000, 0x100000,
};

#define POPUP_SMALL 0x42AE8E                     /* chip bitmaps of the score popups and the star burst */
#define POPUP_MEDIUM 0x42B06E
#define POPUP_LARGE 0x42B4CE
#define STAR_BURST 0x428B8E
#define BANNER_FONT 0x207A00
#define BANNER_TEXTS 0x111B80

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static const uint8_t *chip(const sk_game *g, uint32_t addr, size_t n) { return g->sk ? image_ptr(g->sk, addr, n) : NULL; }
static sk_draw *D(sk_game *g, sk_slot *s) { return slot_draw_record(g, s); }

/* The port's guard for a missing resource slot: the original would draw from address 0. */
static bool need(sk_game *g, sk_slot *s, const uint8_t *frames) {
    if (frames) return true;
    free_object(g, s);
    return false;
}

static void touch(sk_game *g, int x, int y, int w, int h) { touch_player(g, x, y, w, h); }

/* ---- type 8: the dropping (slot 2) ---- */
static void obj_dropping_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x10ED76 */
    init_object_slot(g, s, x, y, s->type4);
    s->vx = 2; s->vy = 0; s->timer = 0;
    const uint8_t *f = res(g, 2);
    if (need(g, s, f)) frames_16x16(g, s, f, 1);
}
static void obj_dropping_update(sk_game *g, sk_slot *s, int index) {           /* 0x10ED9E */
    (void)index;
    sk_draw *d = D(g, s);
    apply_velocity(s);
    if (!(s->flags & 2)) {
        bool check = aux_c(s) == 1;
        if (!check) {
            if (!(g->frame_ticks & 1)) s->flags ^= 1;              /* the wobble */
            if (++s->timer == 0x35) { s->vx = 0; set_aux_c(s, 1); s->flags |= 4; check = true; }
        }
        if (check && (uint16_t)(d->y - s->y) >= 20) {
            touch(g, d->x, d->y, 16, 16);
            if (probe_rows_below(g, s, 8, 0) < 0) {                 /* landed */
                s->flags &= ~1; s->flags |= 2;
                s->aux = 0; s->vx = 2; s->vy = -8;
                spawn_object(g, d->x, d->y, 124);
                set_object_frame(s, 2);
                play_sfx(g, 15);
            }
        }
    }
    if (revive_offscreen(g, s, 100, 0) != 0) s->vx = 2;
}

/* ---- type 124: the dropping's splash (slot 2) ---- */
static void obj_splash_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x10EE5E */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->vx = 2; s->vy = -8; s->flags |= 5;
    const uint8_t *f = res(g, 2);
    if (!need(g, s, f)) return;
    frames_16x16(g, s, f, 1);
    set_object_frame(s, 1);
}
static void obj_splash_update(sk_game *g, sk_slot *s, int index) {             /* 0x10EE94 */
    (void)index;
    apply_velocity(s);
    free_if_far(g, s);
}

/* ---- type 123: the star burst (chip bitmap) and its score popup ---- */
static void obj_bonus_popup_init(sk_game *g, sk_slot *s, int x, int y) {       /* 0x10EEA2 */
    init_object_slot_nodraw(g, s, x - 8, y - 8, s->type4);
    s->vy = 0;
    sk_draw *d = D(g, s);
    const uint8_t *f = chip(g, STAR_BURST, 15 * 0x280);
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
    spawn_object(g, d->x + 16, d->y + 16, 113);
}
static void obj_bonus_popup_update(sk_game *g, sk_slot *s, int index) {        /* 0x10EED4 */
    (void)index;
    sk_draw *d = D(g, s);
    if (g->frame_ticks & 1) return;
    if (d->frames) d->frame = d->frames + (uint16_t)s->flags;        /* the flags word is the frame offset here */
    s->flags = (int16_t)(s->flags + d->frame_bytes);
    if (++s->aux == 15) free_object(g, s);
}

/* ---- types 12 and 28: the birds (slots 4 and 27) ---- */
static void bird_init(sk_game *g, sk_slot *s, int x, int y, int slot) {        /* 0x10EFD2 / 0x10F0D2 */
    init_object_slot(g, s, x, y, s->type4);
    int p = g->objs.marker_param & 0xFF;
    s->t0 = (int16_t)((p & 0x1C) >> 1);
    s->t1 = (int16_t)(p & 0xE0);
    s->hits = (uint8_t)(p & 3);
    s->flags |= 8;
    s->vx = s->t0;
    s->aux = 25;
    const uint8_t *f = res(g, slot);
    if (need(g, s, f)) frames_32x30(g, s, f, 1);
}
static void obj_bird_init(sk_game *g, sk_slot *s, int x, int y) { bird_init(g, s, x, y, 4); }
static void obj_bird2_init(sk_game *g, sk_slot *s, int x, int y) { bird_init(g, s, x, y, 27); }
static void obj_bird_update(sk_game *g, sk_slot *s, int index) {               /* 0x10F024 (type 28: 0x10F124) */
    (void)index;
    sk_draw *d = D(g, s);
    apply_velocity(s);
    if (g->ball_kicked_flag) ball_kill_check(g, s, 10, 0x300);
    if (!(s->flags & 2)) {
        s->aux++;
        if (s->vx == s->t0) step_animation_simple(s, bird_flap_script, s->flags);
        if ((uint16_t)s->t1 < (uint16_t)s->aux) {                    /* glide and turn */
            set_frame2(s, 8, 9, s->flags);
            if (--s->vx == 0) { s->flags ^= 1; s->aux = 0; }
        } else if (s->vx != s->t0) s->vx++;
        touch(g, d->x, d->y + 8, 32, 16);
        ball_hits_object(g, s, 0, 0, 32, 30, 10, 0x300);
    }
    despawn_if_far(g, s);
}

/* ---- type 16: the bulldog (slot 8) ---- */
static void obj_bulldog_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x10F588 */
    init_object_slot(g, s, x, y - 16, s->type4);
    s->vx = 4; s->vy = 0;
    s->hits = (uint8_t)(g->objs.marker_param + 1);
    s->flags |= 8;
    const uint8_t *f = res(g, 8);
    if (need(g, s, f)) frames_48x32(g, s, f, 1);
}
static void obj_bulldog_update(sk_game *g, sk_slot *s, int index) {            /* 0x10F5C2 */
    (void)index;
    sk_draw *d = D(g, s);
    apply_velocity(s);
    if (g->ball_kicked_flag) ball_kill_check(g, s, 8, 0x250);
    if (!(s->flags & 2)) {
        step_animation_simple(s, bulldog_walk_script, s->flags);
        if (ground_ahead(g, s, s->flags)) s->flags ^= 1;
        touch(g, d->x, d->y, 0x30, 0x14);
        ball_hits_object(g, s, 0, 8, 0x30, 0x18, 8, 0x250);
    }
    despawn_if_far(g, s);
}

/* ---- type 116: debris; type 99: the puff / punch (the child block) ---- */
static void obj_debris_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x10FC8E */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 |= (int16_t)0x8000;                                   /* skip this frame's update */
    s->flags |= 1;
    int vx = g->objs.child_vx;
    if (vx < 0) { vx = -vx; s->flags &= ~1; }
    s->vx = (int16_t)vx;
    s->vy = g->objs.child_vy;
    if (s->vy) s->flags |= 4;
    set_draw_size_at(g, s, g->objs.child_w, g->objs.child_h, g->objs.child_frames);
}
static void obj_debris_update(sk_game *g, sk_slot *s, int index) {             /* 0x10FCE0 */
    (void)index;
    apply_velocity(s);
    free_if_far(g, s);
}

static void obj_puff_init(sk_game *g, sk_slot *s, int x, int y) {              /* 0x10FF7A */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 |= (int16_t)0x8000;
    if (g->objs.child_vx & 0x8000) { g->objs.child_vx = (int16_t)(g->objs.child_vx & 0x7FFF); s->flags |= 1; }   /* bclr on the block */
    s->vx = g->objs.child_vx;
    s->vy = g->objs.child_vy;
    if (s->vy) s->flags |= 4;
    s->script = g->objs.child_script;                               /* the long at +0x14 */
    set_draw_size_at(g, s, g->objs.child_w, g->objs.child_h, g->objs.child_frames);
}
static void obj_puff_update(sk_game *g, sk_slot *s, int index) {               /* 0x10FFD2 */
    (void)index;
    sk_draw *d = D(g, s);
    apply_velocity_nograv(s);
    if (s->script && d->frames) step_animation_simple(s, s->script, s->flags);
    touch(g, d->x, d->y, d->w, d->h);
    free_if_far(g, s);
}

/* ---- type 115: the target's hit effect (the child block); plays once ---- */
const uint8_t obj_target_hit_script[] = {                                                   /* 0x11C4E0 */
    0, 24, 0, 2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};
static void obj_target_hit_init(sk_game *g, sk_slot *s, int x, int y) {        /* 0x10FCEE */
    obj_puff_init(g, s, x, y);
    sk_draw *d = D(g, s);
    if (d) d->dflags |= 4;
}
static void obj_target_hit_update(sk_game *g, sk_slot *s, int index) {         /* 0x10FD4C */
    (void)index;
    sk_draw *d = D(g, s);
    apply_velocity(s);
    d->dflags &= ~4;
    int wrapped = 0;
    if (s->script && d->frames) step_animation(s, s->script, s->flags, &wrapped);
    else wrapped = -1;
    if (wrapped) { free_object(g, s); return; }
    free_if_far(g, s);
}

/* ---- type 113: the rising score number ---- */
static void obj_score_popup_init(sk_game *g, sk_slot *s, int x, int y) {       /* 0x10FFFE */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->t0 = 50; s->vx = 0; s->vy = -8; s->flags |= 4;
    sk_draw *d = D(g, s);
    uint32_t v = g->score_value;
    int first;
    if ((int32_t)v < 0x875) {
        frames_wh(g, s, 16, 6, chip(g, POPUP_SMALL, 8 * 0x3C), 1);  /* sub_10783A */
        d->x = (int16_t)(d->x - 8);
        first = 0;
    } else if ((int32_t)v < 0x7500) {
        frames_wh(g, s, 32, 8, chip(g, POPUP_MEDIUM, 7 * 0xA0), 1);  /* sub_1079F4 */
        d->x = (int16_t)(d->x - 16);
        first = 8;
    } else {
        frames_wh(g, s, 48, 12, chip(g, POPUP_LARGE, 4 * 0x168), 1); /* sub_107D24 */
        d->x = (int16_t)(d->x - 24);
        first = 15;
    }
    int n = 0;                                                      /* the first entry >= the score (the table runs on) */
    while (first + n < 19 && (int32_t)v > (int32_t)popup_scores[first + n]) n++;
    set_object_frame(s, n);
}
static void obj_score_popup_update(sk_game *g, sk_slot *s, int index) {        /* 0x1100E4 */
    (void)index;
    sk_draw *d = D(g, s);
    apply_velocity_nograv(s);
    if (s->vy && !(g->frame_ticks & 1)) s->vy++;
    if ((uint16_t)s->t0 < 15) d->prio ^= 4;                          /* eori.w #4 on +0x1A: blinks out */
    if (--s->t0 < 0) { free_object(g, s); return; }
    free_if_far(g, s);
}

/* ---- type 24: the crate (slots 24 and 25) ---- */
static void obj_crate_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x11089A */
    init_object_slot(g, s, x, y - 4, s->type4);
    s->vx = 0; s->vy = 0; s->flags |= 8; s->hits = 2;
    s->t0 = (int16_t)(g->objs.marker_param & 0xFF);                 /* move.b to +0x14 */
    const uint8_t *f = res(g, 24);
    if (need(g, s, f)) frames_32x20(g, s, f, 1);
}
static void obj_crate_update(sk_game *g, sk_slot *s, int index) {              /* 0x1108D4 */
    (void)index;
    sk_draw *d = D(g, s);
    if (s->hits == 1) set_object_frame(s, 1);
    if (ball_hits_object_quiet(g, s, 0, 0, 0x20, 0x14, 0, 0) < 0) { despawn_if_far(g, s); return; }
    play_sfx(g, 8);
    sk_objects *o = &g->objs;
    o->child_frames = res(g, 25);
    o->child_w = 16; o->child_h = 12;
    int x = d->x + 16, y = d->y;
    static const int16_t pieces[4][4] = { { 0, 0, 4, -10 }, { 0, 10, 2, -8 }, { -16, -10, -4, -10 }, { 0, 10, -2, -8 } };
    for (int i = 0; i < 4; i++) {
        x += pieces[i][0]; y += pieces[i][1];
        if (i && o->child_frames) o->child_frames += 0x78;
        o->child_vx = pieces[i][2]; o->child_vy = pieces[i][3];
        spawn_object(g, x, y, 116);
    }
    int kind = s->t0 & 0xFF;
    if (kind == 7) {
        g->score_value = 0x10000;
        add_score(g, 0x10000);
        spawn_object(g, d->x, d->y, 123);
        play_sfx(g, 9);
    } else {
        o->marker_param = kind;
        spawn_object(g, x, y - 16, 112);
    }
    free_object(g, s);                                              /* free_object_keep: the record's bit stays (dynamic list) */
}

/* ---- type 112: the bonus (slot 29) ---- */
static void obj_bonus_init(sk_game *g, sk_slot *s, int x, int y) {             /* 0x110CFA */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->vx = 0; s->vy = -2; s->flags |= 4;
    s->t0 = (int16_t)((g->objs.marker_param & 0xFF) + 1);
    s->t1 = 100;
    const uint8_t *f = res(g, 29);
    if (need(g, s, f)) frames_32x24(g, s, f, 1);
}
static void obj_bonus_update(sk_game *g, sk_slot *s, int index) {              /* 0x110D3A */
    (void)index;
    sk_draw *d = D(g, s);
    sk_player *p = &g->player;
    apply_velocity_nograv(s);
    if (--s->t1 == 0) {                                             /* blink: on 4 frames, off 60 */
        s->t1 = 60;
        bool was = d->dflags & 2;
        d->dflags ^= 2;
        if (!was) s->t1 = 4;
    }
    switch (aux_c(s)) {
    case 0: {
        int wrapped = 0;
        step_animation(s, bonus_script, 0, &wrapped);
        if (wrapped) { set_object_frame(s, s->t0 + 2); set_aux_c(s, 1); }
        break;
    }
    case 1: if (--s->vy <= -4) set_aux_c(s, 2); break;
    default: if (++s->vy > 4) set_aux_c(s, 1); break;
    }
    if (!ball_overlap(g, d->x, d->y, 24, 24) && !player_overlap(g, d->x, d->y, 24, 24)) return;
    g->score_value = 0x2000;
    add_score(g, 0x2000);
    spawn_object(g, d->x, d->y, 123);
    play_sfx(g, 9);
    free_object(g, s);
    switch (s->t0) {
    case 1: p->invulnerable = 1000; break;
    case 2: g->timer_start = bcd_add32(g->timer_start, 0x15); g->timer_flags = 0xFF; break;
    case 4: p->powerup = 0xFF; g->powerup_timer = 750; p->invulnerable = 750; break;
    case 5: p->lives++; g->hud_redraw |= 2; break;
    case 6: p->health = 3; g->extra_heart = 0xFF; g->hud_redraw |= 1; break;
    case 7: p->health = g->extra_heart ? 3 : 2; g->hud_redraw |= 1; break;
    case 8: add_score(g, 0x5000); break;
    default: break;                                                 /* 3 nothing; 0 cannot happen (t0 = param + 1) */
    }
}

/* ---- type 103: the butterfly (slot 47) ---- */
static void obj_butterfly_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x11201E */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->type4 |= (int16_t)0x8000;
    s->t0 = 0;
    const uint8_t *f = res(g, 47);
    if (need(g, s, f)) set_draw_size_at(g, s, 16, 12, f);
}
static void obj_butterfly_update(sk_game *g, sk_slot *s, int index) {          /* 0x112046 */
    (void)index;
    sk_draw *d = D(g, s);
    d->x = (int16_t)(d->x - s->t0);
    d->y = (int16_t)(d->y + 1);
    step_animation_simple(s, butterfly_script, 0);
    s->t0 = butterfly_drift[s->frame & 3][s->anim_tick % 9];
    d->x = (int16_t)(d->x + s->t0);
    if (object_offscreen(g, d->x, d->y, 0x40) < 0) { free_object(g, s); g->flutter_count--; }
}

/* ---- type 105: the jingle banner (KICK OFF / GAME OVER / FULL TIME) ---- */
static void build_banner(sk_game *g, int which) {              /* 0x111B10: 13-row glyphs, 16 px apart */
    memset(g->banner, 0, sizeof g->banner);
    if (!g->sk) return;
    uint32_t text = BANNER_TEXTS + image_u16(g->sk, BANNER_TEXTS + 2u * (uint32_t)which);
    const int plane = 0x104, stride = 20;
    for (int col = 0; col < 10; col++) {
        int c = image_u8(g->sk, text + (uint32_t)col);
        if (!c) break;
        c -= 0x40;
        if (c < 0) continue;
        uint32_t glyph = BANNER_FONT + (uint32_t)c * 0x68;
        for (int r = 0; r < 13; r++) {
            uint16_t w0 = (uint16_t)~image_u16(g->sk, glyph + 8u * (uint32_t)r), w[3];
            for (int k = 0; k < 3; k++) w[k] = image_u16(g->sk, glyph + 8u * (uint32_t)r + 2u + 2u * (uint32_t)k);
            int o = r * stride + col * 2;
            g->banner[4 * plane + o] = (uint8_t)(w0 >> 8); g->banner[4 * plane + o + 1] = (uint8_t)w0;
            for (int k = 0; k < 3; k++) { g->banner[k * plane + o] = (uint8_t)(w[k] >> 8); g->banner[k * plane + o + 1] = (uint8_t)w[k]; }
        }
    }
}

static void obj_jingle_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x111AB4 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->x0 = -160; s->y0 = 0x4C;
    s->vx = 6; s->vy = 0; s->t0 = 50;
    set_aux_c(s, 0);
    int which = g->objs.marker_param & 0xFF;
    build_banner(g, which);
    frames_wh(g, s, 160, 13, g->banner, 1);                         /* sub_108098 */
    sk_draw *d = D(g, s);
    d->dflags |= 4 | 8 | 0x10;                                      /* hidden until the first update; screen-relative (ruling) */
    if (which == 0) { g->player.state = 68; g->player.invulnerable = 150; }
    s->t1 = (int16_t)which;
}
static void obj_jingle_update(sk_game *g, sk_slot *s, int index) {             /* 0x111BA6 */
    (void)index;
    sk_draw *d = D(g, s);
    d->x = s->x0; d->y = s->y0;
    d->dflags &= ~4;
    switch (aux_c(s)) {
    case 0: case 2:                                                 /* wait */
        if (--s->t0 == 0) { s->t0 = 75; set_aux_c(s, aux_c(s) + 1); }
        return;
    case 1:                                                         /* slide in; the jingle tune with KICK OFF */
        if (s->t1 == 0 && !(g->death_flags & 2)) {
            g->death_flags |= 2;
            if (g->death_flags & 1) g->death_flags &= ~1;
            if (!g->game_flags) { g->music_requested = true; g->music_request = -1; }   /* the jingle module (audio: hook) */
        }
        s->x0 = (int16_t)(s->x0 + s->vx);
        if (s->x0 >= 0x48) set_aux_c(s, 2);
        return;
    case 3:
        if ((g->death_flags & 2) && (g->death_flags &= ~2, !g->game_flags)) { g->music_requested = true; g->music_request = 0; }
        if (s->t1 == 1) { g->curtain_flags |= 0x10; return; }      /* GAME OVER: the level ends */
        if (s->t1 == 2) { g->level_number = g->fulltime_level; g->level_outcome = 0x81; g->curtain_flags |= 0x10; return; }
        if (s->t1 != 0) { g->curtain_flags |= 0x10; return; }
        g->player.state = 0;
        g->timer_flags |= 0x80;
        set_aux_c(s, 4);
        /* fall through: slide out */
    default:
        s->x0 = (int16_t)(s->x0 + s->vx);
        if (s->x0 < 0x140) return;
        if (s->t1 == 2) g->level_timer = 0x10;
        free_object(g, s);
        return;
    }
}

void spawn_jingle(sk_game *g, int tune) {                       /* 0x1045A6 */
    g->jingle_requested = true;
    g->jingle = tune;
    if (g->level_data_index == 12 || g->level_data_index == 0x29) return;
    g->objs.marker_param = tune;
    spawn_object(g, 0, 0, 105);
}

/* ---- type 125 / 126: the sign's sparks (slot 0) ---- */
static void spark_init(sk_game *g, sk_slot *s, int x, int y) {                 /* 0x10EB34 / 0x10EB78 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->vy = -6;
    const uint8_t *f = res(g, 0);
    if (need(g, s, f)) frames_wh(g, s, 16, 8, f, 1);                /* sub_10785C */
}
static void spark_update(sk_game *g, sk_slot *s, int dx) {
    sk_draw *d = D(g, s);
    int v = s->vy++;
    d->y = (int16_t)(d->y + (v >> 1));
    d->x = (int16_t)(d->x + dx);
    if (object_offscreen(g, d->x, d->y, 160) < 0) free_object(g, s);
}
static void spark_right_update(sk_game *g, sk_slot *s, int index) { (void)index; spark_update(g, s, 1); }    /* 0x10EB52 */
static void spark_left_update(sk_game *g, sk_slot *s, int index) { (void)index; spark_update(g, s, -1); }    /* 0x10EB96 */

/* ---- type 45: the workman (slots 56, 57, 58, 59, 64, 65) ---- */
static void obj_workman_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x112882 */
    init_object_slot(g, s, x, y - 48, s->type4);
    s->flags |= 8;
    s->vx = 2; s->vy = 0; s->t1 = 0; s->x0 = 0; s->hits = 2;
    const uint8_t *f = res(g, 56);
    if (need(g, s, f)) frames_48x64(g, s, f, 1);
}
static void obj_workman_update(sk_game *g, sk_slot *s, int index) {            /* 0x1128C0 */
    (void)index;
    sk_draw *d = D(g, s);
    sk_player *p = &g->player;
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    switch (aux_c(s)) {
    case 2:                                                         /* after the punch */
        if (--s->t0 == 0) { s->vx = 2; set_aux_c(s, 0); }
        break;
    case 1:                                                         /* the wind-up: then the punch */
        if (--s->t0 != 0) break;
        set_draw_size(d, 48, 64, res(g, 56));
        set_frame2(s, 12, 13, s->flags);
        {
            int x = d->x, y = d->y + 10;
            int vx = 12 | 0x8000;
            d->x = (int16_t)(d->x + 16);
            x += 48;
            if (!(s->flags & 1)) { vx &= 0x7FFF; x -= 64; d->x = (int16_t)(d->x - 32); }
            sk_objects *o = &g->objs;
            o->child_vx = (int16_t)vx;
            o->child_frames = res(g, 65); o->child_script = workman_punch_script;
            o->child_w = 16; o->child_h = 16; o->child_vy = 4;
            spawn_object(g, x, y, 99);
        }
        set_aux_c(s, 2); s->t0 = 8; s->t1 = 100;
        break;
    default: {                                                      /* walking */
        int wrapped = 0;
        step_animation(s, workman_walk_script, s->flags, &wrapped);
        int dist = d->x - s->x;
        if (dist < 0) dist = -dist;
        s->x0 = (int16_t)dist;
        if ((uint16_t)dist >= 200 || ground_ahead(g, s, s->flags)) s->flags ^= 1;
        if ((uint16_t)s->x0 >= 0xA8) break;
        if (s->t1) { s->t1--; break; }
        int dy = (int16_t)(d->y + 64 - p->y);
        if (dy < 0 || dy >= 64) break;
        int ahead = s->flags & 1, dx = (int16_t)(p->x - d->x - 24);
        if (dx < 0) { dx = -dx; ahead ^= 1; }
        if (!ahead || dx >= 0x5A) break;
        s->vx = 0; set_aux_c(s, 1); s->t0 = 8;
        set_draw_size(d, 64, 64, res(g, 64));
        set_frame2(s, 0, 1, s->flags);
        break;
    }
    }
    touch(g, d->x, d->y, 0x30, 0x40);
    if (ball_hits_object_quiet(g, s, 0, 0, 0x30, 0x40, 0, 0x500) >= 0) {   /* knocked out */
        set_draw_size(d, 48, 47, res(g, 57));
        s->flags |= 6; s->vx = 0; s->vy = -4;
        d->dflags &= ~2;
        g->score_value = 0x500;
        spawn_object(g, d->x + ((uint16_t)d->w >> 1), d->y + ((uint16_t)d->h >> 1), 113);
        sk_objects *o = &g->objs;
        o->child_frames = res(g, 58); o->child_w = 32; o->child_h = 21; o->child_vx = 0; o->child_vy = -8;
        spawn_object(g, d->x + 8, d->y, 116);
        o->child_frames = res(g, 59); o->child_w = 16; o->child_h = 11; o->child_vx = 0; o->child_vy = -12;
        spawn_object(g, d->x + 16, d->y + 16, 116);
    }
    despawn_if_far(g, s);
}

/* ---- type 58: the rat (slot 88) ---- */
static void obj_rat_init(sk_game *g, sk_slot *s, int x, int y) {               /* 0x113A66 */
    init_object_slot(g, s, x, y - 8, s->type4);
    int p = g->objs.marker_param & 0xFF;
    s->flags = (int16_t)(p & 1);
    s->vx = (int16_t)(p & 6);
    s->t0 = (int16_t)(((p & 0x18) << 2) & 0xFF);                    /* the wait */
    s->t1 = (int16_t)(p & 0xE0);                                    /* the run */
    s->flags |= 8;
    s->vy = 0;
    set_aux_d(s, s->t0);
    s->hits = 1;
    const uint8_t *f = res(g, 88);
    if (need(g, s, f)) frames_32x24(g, s, f, 1);
}
static void obj_rat_update(sk_game *g, sk_slot *s, int index) {                /* 0x113ACA */
    (void)index;
    sk_draw *d = D(g, s);
    if (s->flags & 2) { apply_velocity(s); despawn_if_far(g, s); return; }
    if (aux_c(s) == 0) {
        set_frame2(s, 0, 7, s->flags);
        set_aux_d(s, aux_d(s) - 1);
        if (aux_d(s) == 0) { set_aux_d(s, s->t1); set_aux_c(s, 1); }
    } else {
        apply_velocity(s);
        int wrapped = 0;
        step_animation(s, rat_run_script, s->flags, &wrapped);
        set_aux_d(s, aux_d(s) - 1);
        if (aux_d(s) == 0) { set_aux_d(s, s->t0); set_aux_c(s, 0); s->flags ^= 1; }
    }
    touch(g, d->x, d->y, 0x14, 0x14);
    ball_hits_object(g, s, 6, 4, 0x14, 0x14, 14, 0x200);
    despawn_if_far(g, s);
}

/* ---- type 60: the walker (slots 84, 85) ---- */
static void obj_walker_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x113C14 */
    init_object_slot(g, s, x, y - 33, s->type4);
    s->vx = 4; s->vy = 0; s->hits = 1; s->flags |= 8;
    const uint8_t *f = res(g, 84);
    if (need(g, s, f)) frames_48x49(g, s, f, 1);
}
static void obj_walker_update(sk_game *g, sk_slot *s, int index) {             /* 0x113C4A */
    (void)index;
    sk_draw *d = D(g, s);
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    bool turning = aux_c(s) == 1;
    if (!turning) {
        int wrapped = 0;
        step_animation(s, walker_walk_script, s->flags, &wrapped);
        int dist = d->x - s->x;
        if (dist < 0) dist = -dist;
        if ((uint16_t)dist >= 0x96 || ground_ahead(g, s, s->flags)) {
            s->anim_tick = 0; s->anim_index = 0;                    /* reset_animation */
            s->t0 = 0; set_aux_c(s, 1); turning = true;
        }
    }
    if (turning) {
        uint16_t v = walker_speed_table[((uint16_t)s->t0 >> 1) & 15];
        if (v & 0x8000) s->flags ^= 1;
        s->vx = (int16_t)(v & 0x7FFF);
        s->t0 = (int16_t)(s->t0 + 2);
        int wrapped = 0;
        step_animation(s, walker_turn_script, s->flags, &wrapped);
        if (wrapped) {
            s->anim_tick = 0; s->anim_index = 0;
            set_aux_c(s, 0);
            set_frame2(s, 0, 6, s->flags);
        }
    }
    touch(g, d->x + 8, d->y, 0x20, 0x30);
    if (ball_hits_object(g, s, 0, 0, 0x30, 0x31, 10, 0x300) >= 0) {
        sk_objects *o = &g->objs;
        o->child_frames = res(g, 85); o->child_w = 32; o->child_h = 21; o->child_vx = 0; o->child_vy = -14;
        spawn_object(g, d->x, d->y, 116);
    }
    despawn_if_far(g, s);
}

/* ---- type 63: the cyclist (slots 93, 94, 95) ---- */
static void obj_cyclist_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x118362 */
    init_object_slot(g, s, x, y - 40, s->type4);
    s->vx = 4; s->vy = 0; s->flags |= 8; s->hits = 2;
    const uint8_t *f = res(g, 93);
    if (need(g, s, f)) frames_64x56(g, s, f, 1);
}
static void obj_cyclist_update(sk_game *g, sk_slot *s, int index) {            /* 0x11839C */
    (void)index;
    sk_draw *d = D(g, s);
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    if (aux_c(s) == 1) {                                            /* stopped: turn round after 4 frames */
        if (--s->t0 == 0) {
            s->flags ^= 1; s->vx = 4;
            d->x = (int16_t)(d->x - 8); d->y = (int16_t)(d->y + 2);
            set_draw_size(d, 64, 56, res(g, 93));
            set_frame2(s, 0, 8, s->flags);
            set_aux_c(s, 0);
        }
    } else {
        int wrapped = 0;
        step_animation(s, cyclist_ride_script, s->flags, &wrapped);
        int dist = d->x - s->x;
        if (dist < 0) dist = -dist;
        if ((uint16_t)dist >= 200 || ground_ahead(g, s, s->flags)) {
            set_aux_c(s, 1);
            set_draw_size(d, 32, 58, res(g, 94));
            set_frame2(s, 0, 1, s->flags);
            s->t0 = 4; s->vx = 0;
            d->x = (int16_t)(d->x + 8); d->y = (int16_t)(d->y - 2);
        }
    }
    touch(g, d->x, d->y + 4, 0x30, 0x34);
    if (ball_hits_object(g, s, 0, 0, 0x40, 0x38, 0, 0x300) >= 0) {
        const uint8_t *f = res(g, 95);
        set_draw_size(d, 48, 36, f);
        sk_objects *o = &g->objs;
        o->child_frames = f ? f + 0x438 : NULL; o->child_w = 48; o->child_h = 36; o->child_vx = 0; o->child_vy = -6;
        spawn_object(g, d->x, d->y, 116);
    }
    despawn_if_far(g, s);
}

void obj_england_install(void) {
    object_init_table[8] = obj_dropping_init;          object_update_table[8] = obj_dropping_update;
    object_init_table[12] = obj_bird_init;             object_update_table[12] = obj_bird_update;
    object_init_table[16] = obj_bulldog_init;          object_update_table[16] = obj_bulldog_update;
    object_init_table[24] = obj_crate_init;            object_update_table[24] = obj_crate_update;
    object_init_table[28] = obj_bird2_init;            object_update_table[28] = obj_bird_update;
    object_init_table[45] = obj_workman_init;          object_update_table[45] = obj_workman_update;
    object_init_table[58] = obj_rat_init;              object_update_table[58] = obj_rat_update;
    object_init_table[60] = obj_walker_init;           object_update_table[60] = obj_walker_update;
    object_init_table[63] = obj_cyclist_init;          object_update_table[63] = obj_cyclist_update;
    object_init_table[99] = obj_puff_init;             object_update_table[99] = obj_puff_update;
    object_init_table[103] = obj_butterfly_init;       object_update_table[103] = obj_butterfly_update;
    object_init_table[105] = obj_jingle_init;          object_update_table[105] = obj_jingle_update;
    object_init_table[112] = obj_bonus_init;           object_update_table[112] = obj_bonus_update;
    object_init_table[113] = obj_score_popup_init;     object_update_table[113] = obj_score_popup_update;
    object_init_table[115] = obj_target_hit_init;     object_update_table[115] = obj_target_hit_update;
    object_init_table[116] = obj_debris_init;          object_update_table[116] = obj_debris_update;
    object_init_table[123] = obj_bonus_popup_init;     object_update_table[123] = obj_bonus_popup_update;
    object_init_table[124] = obj_splash_init;          object_update_table[124] = obj_splash_update;
    object_init_table[125] = spark_init;               object_update_table[125] = spark_right_update;
    object_init_table[126] = spark_init;               object_update_table[126] = spark_left_update;
}
