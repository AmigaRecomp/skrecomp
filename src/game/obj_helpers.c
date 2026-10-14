#include "game/obj_helpers.h"
#include "game/ball.h"
#include "game/game.h"
#include "game/resources.h"

static int16_t hi(int32_t v) { return (int16_t)(v >> 16); }
static int32_t neg_hi(int32_t v) {               /* neg.w on the high word only */
    return (int32_t)((uint32_t)(uint16_t)(-(int16_t)(v >> 16)) << 16 | ((uint32_t)v & 0xFFFF));
}

/* The box test shared by ball_overlap and ball_overlap_box: unsigned compares, and the extent is 12 on the side
   where the ball is left of / above the point. */
static bool ball_in_box(const sk_game *g, int x, int y, int w, int h) {
    int dx = (int16_t)(hi(g->ball.x) - x), dy = (int16_t)(hi(g->ball.y) - y);
    if (dx < 0) { dx = -dx; w = 12; }
    if (dy < 0) { dy = -dy; h = 12; }
    return (uint16_t)w >= (uint16_t)dx && (uint16_t)h >= (uint16_t)dy;
}

static int last_speed;                           /* d2 after ball_overlap: |vx| + |vy| integer parts */

int ball_overlap(sk_game *g, int x, int y, int w, int h) {   /* 0x10E586 */
    sk_player *p = &g->player;
    last_speed = 0;
    if (!p->ball_free || g->ball.inactive || g->player_hidden || p->dying || p->ball_hold < 0) return 0;
    if (!ball_in_box(g, x, y, w, h)) return 0;
    int vx = hi(g->ball.vx), vy = hi(g->ball.vy);
    last_speed = (int16_t)((vx < 0 ? -vx : vx) + (vy < 0 ? -vy : vy));
    p->ball_free = 0;
    if (p->state == 16) return 2;
    p->ball_hold = 0;
    int a = atan2_256(x + ((uint16_t)w >> 1) - hi(g->ball.x), y + ((uint16_t)h >> 1) - hi(g->ball.y));
    if ((a >= 0x20 && a < 0x60) || (a >= 0xA0 && a < 0xE0)) { g->ball.vy = neg_hi(g->ball.vy); return 2; }
    g->ball.vx = neg_hi(g->ball.vx);
    return 1;
}

int ball_overlap_box(const sk_game *g, int x, int y, int w, int h) {   /* 0x10E65C */
    if (g->ball.inactive) return 0;
    return ball_in_box(g, x, y, w, h) ? 1 : 0;
}

static void count_kill(sk_game *g, uint32_t score) {
    if (g->kill_kind >= 1 && g->kill_kind <= 3) g->kills[g->kill_kind]++;
    add_score(g, score);
}

static int hit_common(sk_game *g, sk_slot *s, int dx, int dy, int w, int h) {   /* shared head: -1 none, 1 hit */
    sk_draw *d = s->draw;
    if ((int8_t)--s->timer >= 0) return -1;
    s->timer = 0;
    if (d) d->dflags &= (uint8_t)~2;
    int x = dx + (d ? d->x : s->x), y = dy + (d ? d->y : s->y);
    if (!ball_overlap(g, x, y, w, h) || last_speed < 1) return -1;
    play_sfx(g, 0x0B);
    s->timer = 4;
    if (d) d->dflags |= 2;
    return 1;
}

int ball_hits_object(sk_game *g, sk_slot *s, int dx, int dy, int w, int h, int dead_frame, uint32_t score) {   /* 0x10E3EA */
    if (hit_common(g, s, dx, dy, w, h) < 0) return -1;
    if (--s->hits != 0) return -2;
    sk_draw *d = s->draw;
    g->score_value = score;
    int cx = d ? d->x + ((uint16_t)d->w >> 1) : s->x, cy = d ? d->y + ((uint16_t)d->h >> 1) : s->y;
    spawn_object(g, cx, cy, 0x71);
    count_kill(g, score);
    s->vx = 0;
    s->vy = -10;
    s->flags |= 6;
    if (d) d->dflags &= (uint8_t)~2;
    set_object_frame(s, dead_frame);
    return 0;
}

int ball_hits_object_quiet(sk_game *g, sk_slot *s, int dx, int dy, int w, int h, int dead_frame, uint32_t score) {   /* 0x10E4BE */
    (void)dead_frame;
    if (hit_common(g, s, dx, dy, w, h) < 0) return -1;
    if (--s->hits != 0) return -2;
    count_kill(g, score);
    return 0;
}

void ball_hits_object_simple(sk_game *g, sk_slot *s, int dx, int dy, int w, int h) {   /* 0x10E54A */
    sk_draw *d = s->draw;
    if ((int8_t)--s->timer >= 0) return;
    if (d) d->dflags &= (uint8_t)~2;
    s->timer = 0;
    int x = dx + (d ? d->x : s->x), y = dy + (d ? d->y : s->y);
    if (!ball_overlap(g, x, y, w, h)) return;
    s->timer = 4;
    if (last_speed < 4) return;
    play_sfx(g, 0x0B);
    if (d) d->dflags |= 2;
    s->hits--;
}

int point_offscreen(const sk_game *g, int x, int y, int mx, int my) {   /* 0x10D96E */
    int dx = (int16_t)(x - g->camera_x);
    if (dx < 0) dx = -dx + 320;
    if ((int16_t)(dx - 320) > mx) return -1;
    int dy = (int16_t)(y - g->camera_y);
    if (dy < 0) dy = -dy + 200;
    if ((int16_t)(dy - 200) > my) return -1;
    return 0;
}

bool ball_kill_check(sk_game *g, sk_slot *s, int dead_frame, uint32_t score) {   /* 0x10E108 */
    sk_draw *d = s->draw;
    if ((s->flags & 2) || !d) return false;
    if (point_offscreen(g, d->x, d->y, d->w, d->h) < 0) return false;
    add_score(g, score);
    set_object_frame(s, dead_frame);
    s->vx = 0;
    s->vy = -10;
    s->flags |= 6;
    d->dflags &= (uint8_t)~2;
    return true;
}

int revive_offscreen(sk_game *g, sk_slot *s, int frames, int frame) {   /* 0x10E20C */
    if (object_offscreen(g, s->x, s->y, 0xA0) < 0) { free_object(g, s); return 0; }
    if (!(s->flags & 2)) return 0;
    if (s->aux == 0) {
        sk_draw *d = s->draw;
        if (d && object_offscreen(g, d->x, d->y, 0x40) < 0) s->aux = (int16_t)frames;
        return 0;
    }
    if (--s->aux != 0) return 0;
    s->vx = s->vy = s->t0 = s->t1 = 0;
    s->frame = 0; s->anim_index = 0; s->anim_tick = 0; s->timer = 0;
    s->flags = (int16_t)(s->flags & 0x00FF & ~0xFF);   /* clr.b $a(a6): the flags byte */
    if (s->draw) { s->draw->x = s->x; s->draw->y = s->y; }
    set_object_frame(s, frame);
    return -1;
}

static bool row_word(const sk_game *g, int cx, int cy, int row, uint16_t *out) {
    if (!world_inside(&g->world, cx, cy)) { *out = 0; return false; }
    const uint8_t *t = world_tail(&g->world, cx, cy);
    *out = t ? (uint16_t)(t[row * 2] << 8 | t[row * 2 + 1]) : 0;
    return true;
}

static bool pixel(const sk_game *g, int x, int y) {
    uint16_t w;
    row_word(g, (x & 0xFFFF) >> 4, (y & 0xFFFF) >> 4, y & 15, &w);
    return w >> (15 - (x & 15)) & 1;
}

int ground_ahead(const sk_game *g, const sk_slot *s, int facing) {   /* 0x10DCFC */
    const sk_draw *d = s->draw;
    int x = d->x + ((facing & 1) ? d->w : 0), y = d->y + d->h - 1;
    int cx = (x & 0xFFF0) >> 4, cy = (y & 0xFFFF) >> 4, r = y & 15;
    uint16_t w;
    int res = 0;
    row_word(g, cx, cy, r, &w);
    if (w) res = -1;
    if (r == 15) row_word(g, cx, cy + 1, 0, &w); else row_word(g, cx, cy, r + 1, &w);
    if (w != 0xFFFF) res = -1;
    return res;
}

bool wall_ahead(const sk_game *g, const sk_slot *s, int facing) {   /* sub_10DC8E */
    const sk_draw *d = s->draw;
    if (!d) return false;
    int x = d->x + ((facing & 1) ? d->w : 0);
    int cx = (x & 0xFFF0) >> 4, cy = ((d->y + d->h) & 0xFFF0) >> 4;
    uint16_t bit = (uint16_t)(0x8000u >> (x & 15));
    for (int r = 0; r < 16; r++) { uint16_t w; row_word(g, cx, cy, r, &w); if (w & bit) return true; }
    return false;
}

bool cells_have_gap(const sk_game *g, int x, int y, int n) {     /* sub_10DBC2: Z set when a cell is empty */
    int cx = (x & 0xFFF0) >> 4, cy = (y & 0xFFFF) >> 4;
    for (int k = 0; k < n; k++) {
        bool empty = true;
        for (int r = 0; r < 16 && empty; r++) { uint16_t w; row_word(g, cx + k, cy, r, &w); if (w) empty = false; }
        if (empty) return true;
    }
    return false;
}

int probe_rows_below(const sk_game *g, const sk_slot *s, int dx, int dy) {   /* 0x10D9A2 */
    const sk_draw *d = s->draw;
    int x = d->x + dx, y = d->y + d->h - 1 + dy;
    if (!pixel(g, x, y)) return 0;
    int n = 0;
    for (int k = 0; k < 5; k++) {
        n--;
        if (!pixel(g, x, y + n)) break;
    }
    return n;
}

int probe_rows_below2(const sk_game *g, const sk_slot *s, int dx, int dy) {   /* 0x10DA3A */
    int r = probe_rows_below(g, s, dx, dy);
    if (r < 0) return r;
    const sk_draw *d = s->draw;
    int x = d->x + dx, y = d->y + d->h - 1 + dy, n = 0;
    for (int k = 0; k < 5 && !pixel(g, x, y + n + 1); k++) n++;
    return n;
}

int ledge_distance(const sk_game *g, const sk_slot *s, int dx, int dy, int *signed_out) {   /* 0x10DD80 */
    const sk_draw *d = s->draw;
    int x = d->x + dx, y = d->y + d->h + dy;
    int cx = (x & 0xFFF0) >> 4, cy = (y & 0xFFFF) >> 4;
    uint16_t w;
    row_word(g, cx, cy, 0, &w);
    if (w == 0xFFFF) {                           /* solid below: measure in the cell above, flat when its last row is clear */
        cy--;
        row_word(g, cx, cy, 15, &w);
        if (w == 0) { if (signed_out) *signed_out = 0; return 0; }
    }
    int n = 0, r = 0;
    for (r = 0; r < 16; r++) { n++; row_word(g, cx, cy, r, &w); if (w & 0x8000) break; }
    for (r = 0; r < 16; r++) { n--; row_word(g, cx, cy, r, &w); if (w & 1) break; }
    if (signed_out) *signed_out = n;
    return n < 0 ? -n : n;
}

void frames_wh(sk_game *g, sk_slot *s, int w, int h, const uint8_t *frames, int prio) {   /* frames_WxH + set_frames_base */
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    set_draw_size(d, w, h, frames);
    d->frames_len = frames_room(g, frames);
    d->x = s->x;
    d->y = s->y;
    d->prio = (uint8_t)((unsigned)prio >> 8);                     /* move.w d0,$1a(a0): byte 0x1A, then the flags byte 0x1B */
    d->dflags = (uint8_t)prio;
}

sk_slot *spawn_child(sk_game *g, const sk_slot *parent, int x, int y, int type, int off, const uint8_t *frames,
                     const uint8_t *script, int w, int h, int vx, int vy) {   /* sub_10CE92 */
    if (!find_free_slot(g)) return NULL;         /* the port's guard: the block is left as it was */
    x -= off;
    if (parent && (parent->flags & 1)) { x += 2 * off; vx |= 0x8000; }
    g->objs.child_frames = frames;
    g->objs.child_script = script;
    g->objs.child_w = (int16_t)w;
    g->objs.child_h = (int16_t)h;
    g->objs.child_vx = (int16_t)vx;
    g->objs.child_vy = (int16_t)vy;
    return spawn_object(g, x, y, type);
}

void step_animation_simple(sk_slot *s, const uint8_t *script, int facing) {   /* 0x10DEA0 */
    int count = script[0] << 8 | script[1], ticks = script[2] << 8 | script[3];
    if (++s->anim_tick == (uint8_t)ticks) {
        s->anim_tick = 0;
        if (++s->anim_index == (uint8_t)count) s->anim_index = 0;
    }
    const uint8_t *frames = script + 4 + ((facing & 1) ? count : 0);
    set_object_frame(s, frames[s->anim_index]);
}

void apply_velocity_halfstep(const sk_game *g, sk_slot *s) {   /* sub_10E0B6 */
    sk_draw *d = s->draw;
    if (!d) return;
    int ex = s->vx, ey = s->vy;
    if (!(g->frame_ticks & 1)) { ex &= 1; ey &= 1; }
    if (s->flags & 4) {
        int dy;
        if (s->vy < 0) dy = -((int)((uint16_t)-s->vy >> 1) + ey);
        else dy = ((uint16_t)s->vy >> 1) + ey;
        d->y = (int16_t)(d->y + dy);
    }
    int dx = ((uint16_t)s->vx >> 1) + ex;
    if (!(s->flags & 1)) dx = -dx;
    d->x = (int16_t)(d->x + dx);
}

void apply_velocity_sym(sk_slot *s) {                          /* sub_10E046 */
    sk_draw *d = s->draw;
    if (s->flags & 4) {
        s->vy++;
        int v = (uint16_t)(s->vy < 0 ? -s->vy : s->vy) >> 1;
        if (d) d->y = (int16_t)(d->y + (s->vy < 0 ? -v : v));
    }
    int dx = (uint16_t)s->vx >> 1;
    if (!(s->flags & 1)) dx = -dx;
    if (d) d->x = (int16_t)(d->x + dx);
}

void apply_velocity_nograv(sk_slot *s) {           /* 0x10E090 */
    sk_draw *d = s->draw;
    if (!d) return;
    if (s->flags & 4) d->y = (int16_t)(d->y + (s->vy >> 1));
    int dx = (uint16_t)s->vx >> 1;
    if (!(s->flags & 1)) dx = -dx;
    d->x = (int16_t)(d->x + dx);
}

void free_if_far(sk_game *g, sk_slot *s) {         /* 0x10E290 */
    const sk_draw *d = s->draw;
    if (object_offscreen(g, d ? d->x : s->x, d ? d->y : s->y, 160) < 0) free_object(g, s);
}

void set_frame2(sk_slot *s, int left, int right, int facing) { set_object_frame(s, (facing & 1) ? right : left); }

void set_draw_size_at(sk_game *g, sk_slot *s, int w, int h, const uint8_t *frames) {   /* 0x10816E */
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    set_draw_size(d, w, h, frames);
    d->frames_len = frames_room(g, frames);
    d->x = s->x;
    d->y = s->y;
    d->prio = 0;                                                    /* move.w #1,$1a(a0) */
    d->dflags = 1;
}

size_t frames_room(const sk_game *g, const uint8_t *p) {
    if (!p || !g->res) return 0;
    for (int i = 0; i < SK_SLOTS; i++) {
        const sk_section *sec = g->res->slot[i];
        if (sec && sec->data && p >= sec->data && p < sec->data + sec->len) return (size_t)(sec->data + sec->len - p);
    }
    return 0;
}

void size_frames(sk_game *g, sk_draw *d, int w, int h, const uint8_t *frames) {
    set_draw_size(d, w, h, frames);
    d->frames_len = frames_room(g, frames);
}

int player_overlap(const sk_game *g, int x, int y, int w, int h) {   /* 0x10E3A8 */
    const sk_player *p = &g->player;
    int dx = (int16_t)(p->x - 8 - x);
    if (dx < 0) { dx = -dx; w = 16; }
    int py = p->y, reach = 0x22;
    if (p->crouching || p->sliding) { py += 10; reach -= 10; }
    int dy = (int16_t)(py - y);
    if (dy < 0) { dy = -dy; h = reach; }
    return ((uint16_t)w >= (uint16_t)dx && (uint16_t)h >= (uint16_t)dy) ? -1 : 0;
}

/* ---- the script engine (sub_10E69C / sub_10E730 / sub_10E772): phases of (frames, motion offset, next offset),
   motion records of (vx, vy, animation script, code); t0 counts frames, t1 is the phase's byte offset ---- */
static void motion_apply(sk_slot *s, const sk_motion *m) {
    s->vx = m->vx;
    s->flags &= ~4;
    s->vy = m->vy;
    if (m->vy) s->flags |= 4;
}

int script_step(sk_game *g, sk_slot *s, const sk_phase *ph, const sk_motion *mo, int *changed) {   /* sub_10E69C */
    sk_draw *d = slot_draw_record(g, s);
    const sk_phase *cur = &ph[(uint16_t)s->t1 >> 2];
    const sk_motion *m;
    if ((uint16_t)++s->t0 == cur->frames) {
        if (d) d->x = (int16_t)(d->x - (int8_t)s->anim);
        s->anim = 0;
        set_aux_c(s, aux_c(s) & ~1);
        s->anim_tick = 0; s->anim_index = 0; s->t0 = 0;
        s->t1 = cur->next;
        m = &mo[ph[(uint16_t)s->t1 >> 2].motion / 10];
        motion_apply(s, m);
    } else m = &mo[cur->motion / 10];
    apply_velocity(s);
    *changed = 0;
    if (m->script) { int wrapped = 0; *changed = step_animation(s, m->script, s->flags, &wrapped); }
    return m->code;
}

void script_start(sk_slot *s, int idx, const sk_phase *ph, const sk_motion *mo) {   /* sub_10E730 */
    set_aux_c(s, aux_c(s) & ~1);
    s->anim_tick = 0; s->anim_index = 0; s->t0 = 0;
    s->t1 = (int16_t)(idx * 4);
    motion_apply(s, &mo[ph[idx].motion / 10]);
}

void script_end_next(sk_slot *s, const sk_phase *ph) {   /* sub_10E772 */
    s->t0 = (int16_t)(ph[(uint16_t)s->t1 >> 2].frames - 1);
}

int step_animation_offset(sk_slot *s, const uint8_t *script, int facing, int *wrapped) {   /* sub_10DEEA */
    int count = script[0] << 8 | script[1], ticks = script[2] << 8 | script[3];
    const uint8_t *frames = script + 4;
    int changed = 0;
    s->anim_tick++;
    if (s->anim_tick == ticks) {
        s->anim_tick = 0;
        s->anim_index++;
        changed = -1;
        if (s->anim_index == count) { s->anim_index = 0; *wrapped = -1; }
    }
    if (facing & 1) frames += 2 * count;
    const uint8_t *e = frames + 2 * s->anim_index;
    sk_draw *d = s->draw;
    if (e[0] & 0x80) { if (d) d->dflags |= 4; return changed; }
    if (d) d->dflags &= (uint8_t)~4;
    if (d) d->x = (int16_t)(d->x - (int8_t)s->anim + (int8_t)e[1]);
    s->anim = e[1];
    set_object_frame(s, e[0]);
    return changed;
}

void despawn_if_far_640(sk_game *g, sk_slot *s) {               /* sub_10E1C8 */
    const sk_draw *d = s->draw;
    if (!(s->flags & 2) && object_offscreen(g, s->x, s->y, 0x280) >= 0) return;
    if (object_offscreen(g, d ? d->x : s->x, d ? d->y : s->y, 0x280) >= 0) return;
    if ((s->flags & 2) && (s->flags & 8)) free_object_deferred(g, s);
    else free_object(g, s);
}
