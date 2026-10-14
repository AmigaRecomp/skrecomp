/* The USA's object types, transcribed from the listing. */
#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/obj_usa.h"
#include "game/resources.h"
#include "game/world.h"

static const uint8_t hedgehog_walk_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 4, 5, 6, 7 };         /* 0x119A98 */

static const uint8_t roller_script[] = { 0, 4, 0, 4, 3, 2, 1, 0, 0, 1, 2, 3 };                 /* 0x112F4E */
static const uint8_t gunman_walk_script[] = { 0, 6, 0, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };   /* 0x118FD2 */
static const uint8_t gunman_shoot_script[] = { 0, 6, 0, 4, 0, 1, 2, 2, 2, 2, 3, 4, 5, 5, 5, 5 };   /* 0x118FE2 */
static const uint8_t gunman_shot_script[] = { 0, 8, 0, 3, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7 };   /* 0x118FF2 */
static const int16_t rock_shake[4] = { 0, 1, -1, 0 };                                         /* 0x119972 */

static const int8_t skater_offsets[20][2] = {                                                  /* 0x200F12: (dx, dy) by frame */
    { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 }, { 16, 0 },
    { 8, 4 }, { 22, 4 }, { 8, 4 }, { 22, 4 }, { 28, 8 }, { 5, 6 }, { 28, 6 }, { 5, 5 } };

static const uint8_t bouncer_script[] = { 0, 8, 0, 4, 7, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7 };   /* 0x110FBA */
static const uint8_t hatchling_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 4, 5, 6, 7 };              /* 0x110CE2 */

static const uint8_t flyer_script[] = { 0, 2, 0, 2, 0, 1 };                                   /* 0x10F368 */
static const uint8_t flyer_drop_script[] = { 0, 2, 0, 4, 0, 0, 0, 0 };                        /* 0x10F36E */

static const uint8_t pot_script[] = { 0, 2, 0, 150, 0, 1 };                                   /* 0x10F472 */
static const uint8_t rat_data[18] = { 0, 3, 0, 6, 1, 2, 3, 8, 9, 10, 0, 2, 0, 7, 4, 5, 11, 12 };   /* 0x10F7EC: run, then jump */
#define RAT_RUN (rat_data + 0)      /* 0x10F7EC (0x10F57E is the same bytes) */
#define RAT_JUMP (rat_data + 10)    /* 0x10F7F6 */

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static bool need(sk_game *g, sk_slot *s, const uint8_t *f) { if (f) return true; free_object(g, s); return false; }

/* ---- type 3: the hedgehog (slots 121, 122 knocked out): walks, curls up and turns at an edge ---- */
static void obj_hedgehog_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x11997A */
    init_object_slot(g, s, x, y - 11, s->type4);
    s->vx = 2; s->vy = 0; s->t0 = 0;
    s->flags |= 8;
    s->hits = 1;
    const uint8_t *f = res(g, 121);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 27, f, 0);                                  /* frames_32x27 */
}
static void obj_hedgehog_update(sk_game *g, sk_slot *s, int index) {          /* 0x1199B8 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (!(s->flags & 2)) {
        bool curl_count = aux_c(s) == 1;
        if (!curl_count) {
            step_animation_simple(s, hedgehog_walk_script, s->flags);
            d->y = (int16_t)(d->y + probe_rows_below2(g, s, 16, 0));
            int run = 0;
            if ((uint16_t)ledge_distance(g, s, 16, 0, &run) >= 3) {
                if (s->flags & 1) run = -run;
                if (run < 0) s->flags ^= 1;                         /* the drop is behind it */
                else {                                              /* an edge ahead: curl up and turn */
                    set_frame2(s, 8, 9, s->flags);
                    d->y++;
                    s->vx = 0;
                    s->t0 = 25;
                    s->flags ^= 1;
                    set_aux_c(s, 1);
                    curl_count = true;
                }
            }
        }
        if (curl_count && --s->t0 == 0) { d->y--; s->vx = 2; set_aux_c(s, 0); }
        touch_player(g, d->x, d->y, 32, 27);
        if (ball_hits_object(g, s, 0, 0, 32, 27, 0, 0x400) >= 0) size_frames(g, d, 32, 31, res(g, 122));
    }
    despawn_if_far(g, s);
}

/* ---- type 47: the roller (slot 63, READING a tumbleweed): bounces along to the left; the ball knocks it away ---- */
static void obj_roller_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x112E6E */
    init_object_slot(g, s, x, y - 12, s->type4);
    s->vx = 4; s->vy = -6;
    s->flags |= 4;
    const uint8_t *f = res(g, 63);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
}
static void obj_roller_update(sk_game *g, sk_slot *s, int index) {            /* 0x112EA0 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (!(s->flags & 2)) {
        if (aux_c(s)) {
            s->vx = (int16_t)(s->vx - 4);
            if (s->vx == 0) { set_aux_c(s, 0); s->flags &= ~1; s->vx = 4; }
        }
        int wrapped = 0;
        step_animation(s, roller_script, s->flags, &wrapped);
        int r = probe_rows_below(g, s, 16, -4);
        if (r < 0) { d->y = (int16_t)(d->y + r); s->vy = -6; }        /* lands: a bounce */
        if (ball_overlap(g, d->x, d->y, 32, 32)) {
            s->flags |= 1;
            if ((int16_t)(d->x + 16 - (int16_t)(g->ball.x >> 16)) < 0) s->flags &= ~1;   /* away from the ball */
            set_aux_c(s, 1);
            s->vx = 0x20;
        }
        int far = (int16_t)(s->x - d->x);
        if ((uint16_t)(far < 0 ? -far : far) >= 0x190) s->flags |= 2;   /* 400 px from home: done */
        touch_player(g, d->x, d->y, 32, 32);
    }
    despawn_if_far(g, s);
}

/* ---- type 69: the gunman (slots 118 walking, 119 shooting / hit, 120 the shot): shoots a kid in front within 70 px ---- */
static void gunman_child(sk_game *g, sk_slot *s, int type, int off, int vx, int vy) {   /* sub_118FAA */
    const sk_draw *d = s->draw;
    spawn_child(g, s, d->x + 0x18, d->y + 0x10, type, off, res(g, 120), gunman_shot_script, 32, 25, vx, vy);
}
static void obj_gunman_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x118E0A */
    init_object_slot(g, s, x, y - 48, s->type4);
    s->vx = 2; s->vy = 0; s->t0 = 0;
    s->hits = 3;
    s->flags |= 8;
    const uint8_t *f = res(g, 118);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 64, f, 1);                                  /* frames_48x64 */
}
static void obj_gunman_update(sk_game *g, sk_slot *s, int index) {            /* 0x118E48 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    bool shoot = aux_c(s) == 1;
    if (!shoot) {
        int wrapped = 0;
        step_animation(s, gunman_walk_script, s->flags, &wrapped);
        if (ground_ahead(g, s, s->flags)) s->flags ^= 1;
        if (s->t0) s->t0--;
        else {
            int facing = s->flags, k = (int16_t)(d->x + 0x18 - g->player.x);
            if (k < 0) { k = -k; facing ^= 1; }
            if (!(facing & 1) && (uint16_t)k < 0x46) {              /* the kid in front: draw */
                size_frames(g, d, 64, 64, res(g, 119));
                s->anim_tick = 0; s->anim_index = 0;
                s->vx = 0;
                set_aux_c(s, 1);
                shoot = true;
            }
        }
    }
    if (shoot) {
        int wrapped = 0;
        int changed = step_animation(s, gunman_shoot_script, s->flags, &wrapped);
        if (changed && s->anim_index == 2) gunman_child(g, s, 99, 0x18, 12, 4);
        if (wrapped) {                                              /* holster: walk on, 50 frames before the next draw */
            size_frames(g, d, 48, 64, res(g, 118));
            s->anim_tick = 0; s->anim_index = 0;
            set_frame2(s, 0, 6, s->flags);
            s->vx = 2;
            s->t0 = 50;
            set_aux_c(s, 0);
        }
    }
    touch_player(g, d->x, d->y, 48, 64);
    if (ball_hits_object(g, s, 8, 0, 48, 64, 0, 0x500) >= 0) {      /* down: the hit pose and his hat (type 107) */
        size_frames(g, d, 64, 64, res(g, 119));
        set_frame2(s, 6, 6, 0);
        d->x = (int16_t)(d->x - 16);
        gunman_child(g, s, 107, 0, 0, -14);
        d->x = (int16_t)(d->x + 8);
    }
    despawn_if_far(g, s);
}

/* ---- type 73: the falling rock (slot 110, READING): shakes 100 frames, drops, breaks (type 116) and bounces off; back after 50 ---- */
static void obj_rock73_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x119806 */
    init_object_slot(g, s, x, y - 10, s->type4);
    s->vx = 0; s->vy = 0; s->t0 = 0;
    s->t1 = 100;
    const uint8_t *f = res(g, 110);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 16, 16, f, 1);                                  /* frames_16x16 */
}
static void obj_rock73_update(sk_game *g, sk_slot *s, int index) {            /* 0x11983C */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    int st = aux_c(s);
    bool touch = false;
    if (st == 0) {
        s->t0 = (int16_t)((s->t0 + 1) & 3);
        d->x = (int16_t)(s->x + rock_shake[s->t0]);
        if (--s->t1) touch = true;
        else { s->flags |= 4; set_aux_c(s, 1); st = 1; }
    }
    if (st == 1) {
        if (probe_rows_below(g, s, 0, 0) == 0) touch = true;        /* still falling */
        else {                                                      /* it lands and breaks */
            sk_objects *o = &g->objs;
            o->child_w = 16; o->child_h = 16; o->child_vx = -4; o->child_vy = -4;
            const uint8_t *f = res(g, 110);
            o->child_frames = f ? f + 0xA0 : NULL;
            spawn_object(g, d->x, d->y, 116);
            set_frame2(s, 2, 2, 0);
            s->vx = 4; s->flags |= 1; s->vy = -4;
            set_aux_c(s, 2);
            st = 2;
        }
    }
    if (st == 2) {
        if (object_offscreen(g, d->x, d->y, 64) < 0) {              /* sub_10D92C: gone from view, rest */
            s->vx = 0; s->vy = 0; s->flags &= ~4;
            s->t1 = 50;
            set_aux_c(s, 3);
            st = 3;
        }
    }
    if (st == 3 && --s->t1 == 0) {
        set_frame2(s, 0, 0, 0);
        d->x = s->x; d->y = s->y;
        s->t1 = 100;
        set_aux_c(s, 0);
    }
    if (touch) touch_player(g, d->x, d->y, 16, 16);
    despawn_if_far(g, s);
}

/* ---- type 7: the skater (slot 1, 32 x 44): rides the slopes, faster downhill, slower up and turning below 16; pushes on the flat ---- */
static void obj_skater_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x10EBBC */
    init_object_slot(g, s, x, y - 28, s->type4);
    int p = g->objs.marker_param & 0xFF;
    if (p & 1) s->flags |= 1;
    s->hits = (uint8_t)((p >> 1) + 1);
    s->flags |= 8;
    s->vx = 0x5A;                                                   /* the speed, not apply_velocity's */
    const uint8_t *f = res(g, 1);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 44, f, 1);                                  /* sub_107C9C */
}
static void obj_skater_update(sk_game *g, sk_slot *s, int index) {            /* 0x10EC00 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (g->ball_kicked_flag) ball_kill_check(g, s, 21, 0x500);
    if (s->flags & 2) {                                             /* knocked out: falls */
        d->y = (int16_t)(d->y + (s->vy >> 1));
        s->vy++;
        despawn_if_far(g, s);
        return;
    }
    d->y = (int16_t)(d->y - s->t0);
    d->y = (int16_t)(d->y + probe_rows_below2(g, s, s->t1, 0));
    int slope = 0, steep = ledge_distance(g, s, s->t1, 0, &slope);
    int dx, dy, mode;
    if (steep) {
        set_aux_c(s, 0);
        bool down = (s->flags & 1) ? slope >= 0 : slope < 0;
        int f;
        if (!down && (uint16_t)--s->vx >= 16) {                     /* uphill: slows */
            mode = 1; f = 0x12;
            if (steep >= 8) { f = 0x10; mode = 2; }
        } else {
            if (!down) s->flags ^= 1;                               /* too slow: turns and runs down */
            s->vx = (int16_t)(s->vx + 2);
            mode = 1; f = 0x0C;
            if (steep >= 8) { f = 0x0E; mode = 2; }
        }
        set_frame2(s, f, f + 1, s->flags);
        int k = s->frame < 20 ? s->frame : 19;                      /* port guard */
        dx = skater_offsets[k][0]; dy = skater_offsets[k][1];
    } else {
        if (aux_c(s) < 16) {                                        /* the flat: crouch, then push */
            set_aux_c(s, aux_c(s) + 1);
            set_frame2(s, 2, 8, s->flags);
        } else {
            s->vx++;
            int keep = s->frame;                                    /* sub_10DE14: the frame pointer by frame_ticks, $1E untouched */
            set_object_frame(s, (int)((uint32_t)(g->frame_ticks >> 2) % 6) + ((s->flags & 1) ? 6 : 0));
            s->frame = (uint8_t)keep;
        }
        mode = 3; dy = 1;
        dx = (s->flags & 1) ? 0x0C : 0x14;
    }
    d->x = (int16_t)(d->x + s->t1 - dx);
    s->t1 = (int16_t)dx; s->t0 = (int16_t)dy;
    d->y = (int16_t)(d->y + dy);
    if ((uint16_t)s->vx >= 0x5A) s->vx = 0x5A;
    int v = (uint16_t)s->vx;
    if (mode == 1) v += v >> 2;
    else if (mode == 3) v += v >> 1;
    v >>= 5;
    if (s->flags & 1) v = -v;
    d->x = (int16_t)(d->x + v);
    touch_player(g, d->x + 4, d->y, 24, 44);
    ball_hits_object(g, s, 0, 0, 32, 44, 21, 0x500);
    despawn_if_far(g, s);
}

/* ---- type 29: the bouncer (slot 30, 32 x 21, READING a beach ball): bounces along; the ball sends it back the other way ---- */
static void obj_bouncer_init(sk_game *g, sk_slot *s, int x, int y) {          /* 0x110EDE */
    init_object_slot(g, s, x, y - 5, s->type4);
    s->vx = 4; s->vy = 0;
    s->flags |= 8 | 4;
    s->hits = 1;
    const uint8_t *f = res(g, 30);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 21, f, 1);                                  /* sub_107AE2 */
}
static void obj_bouncer_update(sk_game *g, sk_slot *s, int index) {           /* 0x110F18 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    step_animation_simple(s, bouncer_script, s->flags);
    if (s->vy >= 0 && probe_rows_below(g, s, 12, 5) < 0) {         /* lands: a slope may turn it, then up again */
        int slope = 0;
        if (ledge_distance(g, s, 12, 0, &slope)) {
            if (!(s->flags & 1) ? slope < 0 : slope >= 0) s->flags ^= 1;
        }
        s->vy = -18;
    }
    if ((int8_t)--s->timer < 0) {
        s->timer = 0;
        if (ball_overlap(g, d->x, d->y, 24, 20) == 1) {            /* a side hit: off the other way */
            s->timer = 6;
            s->flags &= ~1;
            if ((int16_t)(g->ball.vx >> 16) < 0) s->flags |= 1;
        }
    }
    touch_player(g, d->x, d->y, 24, 20);
    despawn_if_far(g, s);
}

/* ---- type 30: the egg (slots 77 egg, 78 shell halves as type 116, 76 the hatchling 48 x 13): rocks 100 frames, hatches ---- */
static void obj_egg_init(sk_game *g, sk_slot *s, int x, int y) {              /* 0x110B48 */
    init_object_slot(g, s, x, y - 6, s->type4);
    s->vx = 2; s->vy = 0;
    s->flags |= 8;
    s->hits = 1;
    const uint8_t *f = res(g, 77);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 26, f, 1);                                  /* sub_107B48 */
}
static void obj_egg_update(sk_game *g, sk_slot *s, int index) {               /* 0x110B7C */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->flags & 2) { despawn_if_far(g, s); return; }
    int st = aux_c(s);
    if (st == 0) {                                                  /* rocking */
        if (!(g->frame_ticks & 1)) s->flags ^= 1;
        if ((uint16_t)++s->t0 < 100) { despawn_if_far(g, s); return; }
        set_frame2(s, 1, 1, 0);
        sk_objects *o = &g->objs;
        const uint8_t *halves = res(g, 78);
        o->child_frames = halves; o->child_vx = 4; o->child_vy = -10; o->child_w = 16; o->child_h = 15;
        spawn_object(g, d->x, d->y, 116);
        o->child_frames = halves ? halves + 0x78 : NULL;
        o->child_vx = -4; o->child_vy = -10;
        spawn_object(g, d->x, d->y, 116);
        s->vx = 2; s->vy = -10;
        set_aux_c(s, 1);
        s->flags |= 4;
        st = 1;
    }
    if (st == 1) {                                                  /* the hop out */
        if (s->vy < 10) { despawn_if_far(g, s); return; }
        s->flags &= ~4;
        set_aux_c(s, 2);
        s->vx = 2;
        d->x = (int16_t)(d->x - 8); d->y = (int16_t)(d->y + 9);
        size_frames(g, d, 48, 13, res(g, 76));
    }
    int wrapped = 0;                                                /* walking */
    step_animation(s, hatchling_script, s->flags, &wrapped);
    if (ground_ahead(g, s, s->flags)) s->flags ^= 1;
    touch_player(g, d->x, d->y, 48, 13);
    if (ball_hits_object(g, s, 0, 0, 48, 13, 0, 0x200) >= 0) {
        size_frames(g, d, 32, 26, res(g, 77));
        set_frame2(s, 1, 1, 0);
        d->y = (int16_t)(d->y - 13);
    }
    despawn_if_far(g, s);
}

/* ---- type 14: the flyer (slot 6, 48 x 53, READING): bobs and patrols, two drops (type 111) every 24 frames; down, a wreck (type 119) ---- */
static void obj_flyer_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x10F1D2 */
    init_object_slot(g, s, x, y - 0x26, s->type4);
    int p = g->objs.marker_param & 0xFF;
    s->t0 = (int16_t)((p & 0x18) >> 2);                             /* top speed */
    s->t1 = (int16_t)(p & 0xE0);                                    /* frames before it turns */
    s->hits = (uint8_t)(p & 7);
    s->vx = s->t0; s->vy = 2;
    s->flags |= 4 | 8;
    const uint8_t *f = res(g, 6);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 48, 53, f, 1);                                  /* sub_107E78 */
}
static void flyer_down(sk_game *g, sk_slot *s, const sk_draw *d) {
    s->flags |= 1;
    s->vx = 2;
    spawn_object(g, d->x, d->y, 119);
}
static void obj_flyer_update(sk_game *g, sk_slot *s, int index) {             /* 0x10F22E */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (g->ball_kicked_flag && ball_kill_check(g, s, 2, 0x750)) flyer_down(g, s, d);
    if (!(s->flags & 2)) {
        s->vy = (int16_t)-(s->vy - 1);                              /* the bob */
        int wrapped = 0;
        step_animation(s, flyer_script, 0, &wrapped);
        set_aux_c(s, aux_c(s) + 1);
        set_aux_d(s, aux_d(s) + 1);
        if (aux_d(s) >= 0x18) {                                     /* the drops */
            set_aux_d(s, 0);
            sk_objects *o = &g->objs;
            o->child_frames = res(g, 0); o->child_script = flyer_drop_script;
            o->child_w = 16; o->child_h = 8;
            o->child_vx = (int16_t)(4 | 0x8000); o->child_vy = -16;
            spawn_object(g, d->x + 0x20, d->y + 0x38, 111);
            o->child_vx = 4;
            spawn_object(g, d->x + 0x1C, d->y + 0x38, 111);
        }
        if ((uint8_t)s->t1 < (uint8_t)aux_c(s)) {                   /* past its patrol: slow down and turn */
            if (--s->vx == 0) { s->flags ^= 1; set_aux_c(s, 0); }
        } else if (s->vx != s->t0) s->vx++;
        touch_player(g, d->x + 8, d->y + 2, 48, 52);
        if (ball_hits_object(g, s, 8, 0, 48, 56, 2, 0x750) == 0) flyer_down(g, s, d);
    }
    despawn_if_far(g, s);
}

/* ---- type 119: the flyer's wreck (slot 75, 32 x 32): up and away under gravity ---- */
static void obj_wreck_init(sk_game *g, sk_slot *s, int x, int y) {            /* 0x10F376 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->vx = 2; s->vy = -10;
    s->flags |= 4;
    const uint8_t *f = res(g, 75);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);                                  /* sub_107BF2 */
}
static void obj_wreck_update(sk_game *g, sk_slot *s, int index) {             /* 0x10F3A2 */
    (void)index;
    if (!slot_draw_record(g, s)) return;
    apply_velocity(s);
    free_if_far(g, s);
}

/* ---- type 13: the pot (slot 5, 32 x 27, READING): rocks on its ledge; on frame 0 with the kid below, it falls on him ---- */
static void obj_pot_init(sk_game *g, sk_slot *s, int x, int y) {              /* 0x10F3B0 */
    init_object_slot(g, s, x, y + 8, s->type4);
    s->vx = 0; s->vy = 0;
    s->flags |= 8;
    const uint8_t *f = res(g, 5);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 27, f, 1);                                  /* frames_32x27 */
}
static void obj_pot_update(sk_game *g, sk_slot *s, int index) {               /* 0x10F3DA */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    apply_velocity(s);
    if (s->aux == 2) touch_player(g, d->x, d->y, 32, 32);           /* falling */
    else {
        s->vx = 0;
        if (!(g->frame_ticks & 3)) { s->vx = 2; s->flags ^= 1; }    /* the rock */
        int wrapped = 0;
        step_animation(s, pot_script, 0, &wrapped);
        uint16_t kx = (uint16_t)g->player.x, lo = (uint16_t)(d->x - 0x14);
        if (kx < lo || kx >= (uint16_t)(lo + 0x48)) s->aux = 0;
        else if (s->aux == 0) {
            if (s->frame == 0) { s->vy = 10; s->flags |= 6; s->aux = 2; }
            else s->aux = 1;                                        /* the kid stays under it: once only */
        }
    }
    despawn_if_far(g, s);
}

/* ---- types 15 and 17: the rat (slot 7, 32 x 24, READING): rests, then runs; 17 also jumps onto a wall ahead ---- */
static void rat_init(sk_game *g, sk_slot *s, int x, int y) {
    init_object_slot(g, s, x, y, s->type4);
    int p = g->objs.marker_param & 0xFF;
    s->flags = (int16_t)((s->flags & ~0xFF) | (p & 1));             /* move.b: the whole flags byte */
    s->vx = (int16_t)(p & 6);
    s->t0 = (int16_t)((uint8_t)((p & 0x18) << 2));                  /* the rest, frames (a byte at +0x14) */
    s->t1 = (int16_t)(p & 0xE0);                                    /* the run */
    s->flags |= 8;
    s->vy = 0;
    set_aux_d(s, s->t0);
    s->hits = 1;
    const uint8_t *f = res(g, 7);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 24, f, 0);                                  /* frames_32x24 */
}
static void rat_turn_check(sk_slot *s) {
    set_aux_d(s, aux_d(s) - 1);
    if (aux_d(s)) return;
    set_aux_d(s, s->t0);
    set_aux_c(s, 0);
    s->flags ^= 1;
}
static void rat_rest(sk_slot *s) {
    set_frame2(s, 0, 7, s->flags);
    set_aux_d(s, aux_d(s) - 1);
    if (aux_d(s)) return;
    set_aux_d(s, s->t1);
    set_aux_c(s, 1);
}
static void obj_rat_init(sk_game *g, sk_slot *s, int x, int y) { rat_init(g, s, x, y - 8); }               /* 0x10F478 */
static void obj_rat_update(sk_game *g, sk_slot *s, int index) {               /* 0x10F4DA */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (g->ball_kicked_flag) ball_kill_check(g, s, 14, 0x200);
    if (s->flags & 2) { apply_velocity(s); despawn_if_far(g, s); return; }
    if (!aux_c(s)) rat_rest(s);
    else {
        apply_velocity(s);
        int wrapped = 0;
        step_animation(s, RAT_RUN, s->flags, &wrapped);
        rat_turn_check(s);
    }
    touch_player(g, d->x, d->y, 20, 20);
    ball_hits_object(g, s, 6, 4, 20, 20, 14, 0x200);
    despawn_if_far(g, s);
}
static void obj_jumper_init(sk_game *g, sk_slot *s, int x, int y) { rat_init(g, s, x - 8, y - 8); }     /* 0x10F63E */
static void obj_jumper_update(sk_game *g, sk_slot *s, int index) {            /* 0x10F6A2 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (g->ball_kicked_flag) ball_kill_check(g, s, 14, 0x400);
    if (s->flags & 2) { apply_velocity(s); despawn_if_far(g, s); return; }
    if (aux_c(s) == 1) {
        int r = probe_rows_below2(g, s, 16, 0);
        if (r > 0) {                                                /* off a ledge: falling, no faster than its drop */
            set_frame2(s, 6, 13, s->flags);
            s->flags |= 4;
            if ((uint16_t)(2 * r) < (uint16_t)s->vy) s->vy = (int16_t)(2 * r);
        } else {
            s->vy = 0;
            s->flags &= ~4;
            step_animation_simple(s, RAT_RUN, s->flags);
        }
        apply_velocity(s);
        if (solid_column_ahead(g, s, s->flags & 1, 16 + ((s->flags & 1) ? 30 : -30)) && aux_d(s) >= 20) {
            s->vy = -12;                                            /* a wall 30 px ahead: up onto it */
            s->flags |= 4;
            set_aux_c(s, 2);
            s->anim_tick = 0; s->anim_index = 0;
        }
        rat_turn_check(s);
    } else if (aux_c(s) == 2) {
        apply_velocity(s);
        step_animation_simple(s, RAT_JUMP, s->flags);
        if (s->vy >= 0) set_aux_c(s, 1);
        rat_turn_check(s);
    } else rat_rest(s);
    touch_player(g, d->x, d->y, 20, 20);
    ball_hits_object(g, s, 6, 4, 20, 20, 14, 0x400);
    despawn_if_far(g, s);
}

void obj_usa_install(void) {
    object_init_table[3] = obj_hedgehog_init;          object_update_table[3] = obj_hedgehog_update;
    object_init_table[7] = obj_skater_init;            object_update_table[7] = obj_skater_update;
    object_init_table[13] = obj_pot_init;              object_update_table[13] = obj_pot_update;
    object_init_table[15] = obj_rat_init;              object_update_table[15] = obj_rat_update;
    object_init_table[17] = obj_jumper_init;           object_update_table[17] = obj_jumper_update;
    object_init_table[14] = obj_flyer_init;            object_update_table[14] = obj_flyer_update;
    object_init_table[119] = obj_wreck_init;           object_update_table[119] = obj_wreck_update;
    object_init_table[29] = obj_bouncer_init;          object_update_table[29] = obj_bouncer_update;
    object_init_table[30] = obj_egg_init;              object_update_table[30] = obj_egg_update;
    object_init_table[47] = obj_roller_init;           object_update_table[47] = obj_roller_update;
    object_init_table[69] = obj_gunman_init;           object_update_table[69] = obj_gunman_update;
    object_init_table[73] = obj_rock73_init;           object_update_table[73] = obj_rock73_update;
}
