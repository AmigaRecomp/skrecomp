#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_bonus.h"
#include "game/obj_helpers.h"
#include "game/resources.h"
#include "audio/sound.h"
#include "game/ball.h"
#include "game/platforms.h"

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static bool need(sk_game *g, sk_slot *s, const uint8_t *f) { if (f) return true; free_object(g, s); return false; }
static int state_of(const sk_slot *s) { return aux_c(s); }                  /* byte +0xC */
static void set_state(sk_slot *s, int v) { set_aux_c(s, v); }

/* ---- type 81: the cup piece (slot 131, 32 x 22, frame = the world): shows when all 50 tokens are taken ---- */
static const int8_t piece_bob[16] = { 0, 0, 1, 1, 2, 2, 1, 1, 0, 0, -1, -1, -2, -2, -1, -1 };   /* 0x11A052 */

static void obj_cup_piece_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x119E94 */
    init_object_slot(g, s, x, y + 8, s->type4);
    s->t1 = (int16_t)g->objs.marker_param;
    s->vx = 0; s->vy = 0;
    const uint8_t *f = res(g, 131);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 22, f + (size_t)(s->t1 & 7) * 0x1B8, 1);
    s->draw->dflags |= 4;
}

static void obj_cup_piece_update(sk_game *g, sk_slot *s, int index) {          /* 0x119EDA (no despawn) */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    switch (state_of(s)) {
    case 0:
        if (g->bonus_tokens != 0x32) return;
        set_state(s, 1); s->t0 = 25;
        /* fall through */
    case 1:
        d->dflags ^= 4;                                          /* blinks in */
        if (--s->t0 != 0) return;
        s->flags |= 4;
        d->dflags &= (uint8_t)~4; set_state(s, 2); s->x0 = 10;
        /* fall through */
    case 2:
        s->vy = (int16_t)(2 * piece_bob[g->frame_ticks & 15]);
        apply_velocity_nograv(s);
        if (--s->x0 == 0) {                                      /* the flash: 60 frames off, 4 on */
            s->x0 = 60;
            d->dflags ^= 2;
            if (d->dflags & 2) s->x0 = 4;
        }
        if (!player_overlap(g, d->x, d->y, 0x20, 0x16)) return;
        g->timer_flags &= (uint8_t)~0x80;                        /* the clock stops */
        g->cup_pieces |= (uint8_t)(1u << (s->t1 & 7));
        g->piece_collected = 0xFF;
        game_music(g, SK_MUSIC_LEVEL, 6);
        g->score_value = 0x1000;
        spawn_object(g, d->x, d->y, 123);
        add_score(g, 0x1000);
        play_sfx(g, 9);
        d->dflags |= 4; set_state(s, 3); s->t0 = 200;
        /* fall through */
    default:
        if (--s->t0 != 0) return;
        free_object(g, s);
        g->bonus_pending = 0; g->timer_rate = 0x5DC; g->level_number = g->fulltime_level;
        g->level_outcome = 0x81; g->curtain_flags |= 0x10; g->timeout_armed = false;
        return;
    }
}

/* ---- type 82: the ALIEN (data 41). Without all five cup pieces a figure that sends the kid home (message 0); with them
   a boss of 35 hits: base pod (slot 135), head (type 116), two body pieces (type 84) and two arms on objects_c ---- */
static const uint8_t alien_still[] = { 0, 1, 0, 1, 0, 0 };                     /* 0x1170D6 */
static const uint8_t alien_fall[] = { 0, 4, 0, 2, 0, 1, 2, 2, 0, 1, 2, 2 };    /* 0x1170DC */
static const sk_phase alien_phases[39] = {                                     /* alien_phases 0x116F36 */
    { 100, 0, 4 }, { 50, 10, 8 }, { 25, 20, 12 }, { 65535, 30, 16 }, { 10, 60, 20 }, { 65535, 70, 24 }, { 2, 20, 28 },
    { 90, 130, 32 }, { 10, 150, 36 }, { 65535, 80, 40 }, { 65535, 160, 44 }, { 10, 90, 48 }, { 65535, 80, 52 },
    { 65535, 160, 56 }, { 10, 90, 60 }, { 65535, 100, 64 }, { 10, 150, 68 }, { 65535, 140, 72 }, { 2, 20, 76 },
    { 65535, 40, 80 }, { 10, 60, 84 }, { 65535, 70, 88 }, { 65535, 170, 92 }, { 40, 180, 96 }, { 16, 190, 100 },
    { 50, 20, 104 }, { 65535, 200, 108 }, { 65535, 220, 112 }, { 65535, 210, 116 }, { 2, 20, 120 }, { 65535, 50, 124 },
    { 10, 60, 128 }, { 65535, 70, 132 }, { 25, 110, 136 }, { 65535, 120, 140 }, { 25, 110, 8 }, { 7, 230, 148 },
    { 200, 240, 152 }, { 65535, 250, 152 } };
static const sk_motion alien_motions[26] = {                                   /* 0x116FD2: only the jump moves */
    { 0, 0, NULL, 0x00 }, { 0, 0, NULL, 0x04 }, { 0, 0, alien_still, 0x08 }, { 0, 0, alien_still, 0x0C },
    { 0, 0, alien_still, 0x10 }, { 0, 0, alien_still, 0x14 }, { 0, 0, alien_still, 0x18 }, { 0, 0, alien_still, 0x1C },
    { 0, 0, alien_still, 0x20 }, { 0, 0, alien_still, 0x24 }, { 0, 0, alien_still, 0x28 }, { 0, 0, NULL, 0x2C },
    { 0, 0, NULL, 0x30 }, { 0, 0, alien_still, 0x34 }, { 0, 0, alien_still, 0x38 }, { 0, 0, alien_still, 0x3C },
    { 0, 0, alien_still, 0x40 }, { 0, 0, alien_still, 0x44 }, { 10, -20, alien_still, 0x48 }, { 0, 0, alien_still, 0x4C },
    { 0, 0, alien_still, 0x50 }, { 0, 0, alien_still, 0x54 }, { 0, 0, alien_still, 0x58 }, { 0, 0, alien_fall, 0x5C },
    { 0, 0, NULL, 0x60 }, { 0, 0, NULL, 0x64 } };
static const int8_t alien_bob_table[16] = { -1, -1, -2, -1, -1, 0, 0, 0, 1, 1, 2, 1, 1, 0, 0, 0 };   /* 0x1170E8 */
static const uint8_t hand_a_by_angle[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                             7, 7, 6, 6, 6, 5, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 };            /* 0x1171F6 */
static const uint8_t hand_a_off[8][2] = { { 1, 3 }, { 6, 3 }, { 9, 4 }, { 10, 6 }, { 10, 8 }, { 9, 9 }, { 8, 10 }, { 2, 16 } };   /* 0x117216 */
static const uint8_t hand_b_by_angle[32] = { 0, 0, 0, 13, 13, 13, 14, 14, 14, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 12, 10, 9, 8, 0 };          /* 0x117328 */
static const uint8_t hand_b_off[22][2] = { { 7, 2 }, { 7, 2 }, { 7, 2 }, { 7, 2 }, { 7, 2 }, { 7, 2 }, { 7, 2 }, { 7, 2 },
    { 9, 5 }, { 11, 8 }, { 15, 12 }, { 19, 17 }, { 15, 12 }, { 0, 6 }, { 0, 14 }, { 0, 14 }, { 0, 14 }, { 0, 14 },
    { 14, 12 }, { 14, 12 }, { 14, 12 }, { 14, 12 } };                                                     /* 0x117348 */

/* the bytes of the boss slot: +4 / +5 the arm angles (x0), +6 / +7 their lengths (y0), +0xB a counter, +0xD the aim */
static int ang_a(const sk_slot *s) { return (uint16_t)s->x0 >> 8; }
static int ang_b(const sk_slot *s) { return s->x0 & 0xFF; }
static int len_a(const sk_slot *s) { return (uint16_t)s->y0 >> 8; }
static int len_b(const sk_slot *s) { return s->y0 & 0xFF; }
static void set_ang_a(sk_slot *s, int v) { s->x0 = (int16_t)((v & 0xFF) << 8 | (s->x0 & 0xFF)); }
static void set_ang_b(sk_slot *s, int v) { s->x0 = (int16_t)(((uint16_t)s->x0 & 0xFF00) | (v & 0xFF)); }
static void set_len_b(sk_slot *s, int v) { s->y0 = (int16_t)(((uint16_t)s->y0 & 0xFF00) | (v & 0xFF)); }
static bool once(sk_slot *s) { if (aux_c(s) & 1) return false; set_aux_c(s, aux_c(s) | 1); return true; }   /* bset #0, $c */

static void shift(sk_draw *d, int dx, int dy) { if (d) { d->x = (int16_t)(d->x + dx); d->y = (int16_t)(d->y + dy); } }
static void move_body_head(sk_game *g, sk_draw *d, int dx, int dy) { shift(d, dx, dy); shift(g->alien.head_draw, dx, dy); }   /* 0x116E70 */
static void move_all(sk_game *g, sk_draw *d, int dx, int dy) {                 /* 0x116E44 */
    move_body_head(g, d, dx, dy); shift(g->alien.pa_draw, dx, dy); shift(g->alien.pb_draw, dx, dy);
}
static void head_frame(sk_game *g, const sk_slot *s, int a, int b) { if (g->alien.head) set_frame2(g->alien.head, a, b, s->flags); }   /* 0x116E84 */
static void hand_b_cycle(sk_game *g) { if (!(g->frame_ticks & 3)) g->alien.hand_b = (int8_t)((g->alien.hand_b + 1) & 3); }   /* 0x116EA4 */
static void bob(sk_game *g, sk_draw *d) { move_all(g, d, 0, alien_bob_table[g->frame_ticks & 15]); }   /* 0x116F18 */
static void pieces_pose(sk_game *g, int a, int b) { if (g->alien.piece_a) g->alien.piece_a->t1 = (int16_t)a; if (g->alien.piece_b) g->alien.piece_b->t1 = (int16_t)b; }
static void pieces_spread(sk_game *g, int sign) {                              /* (-13, -4) and (5, -2), or back */
    shift(g->alien.pa_draw, -13 * sign, -4 * sign); shift(g->alien.pb_draw, 5 * sign, -2 * sign);
}

static void hand_frame(sk_draw *h, int f) {                                    /* move.l frames + f * frame_bytes, $16 */
    size_t off = (size_t)f * (size_t)(uint16_t)h->frame_bytes;
    h->frame = (!h->frames || (h->frames_len && off + (size_t)(uint16_t)h->frame_bytes > h->frames_len)) ? NULL : h->frames + off;
}

static void alien_arm(sk_game *g, const sk_draw *d, int first, int a, int len, int bx, int by) {   /* alien_arm_a / _b */
    int s = sin_1024(a * 4), c = cos_1024(a * 4), lw = len << 8;
    for (int i = 0; i < 3; i++) {
        sk_draw *r = &g->objs.chain[first + i];
        int vx = i * s, vy = i * c;
        int px = (int)(((uint32_t)(vx < 0 ? -vx : vx) * (uint32_t)lw) >> 16), py = (int)(((uint32_t)(vy < 0 ? -vy : vy) * (uint32_t)lw) >> 16);
        r->x = (int16_t)(bx + (vx < 0 ? -px : px));
        r->y = (int16_t)(by + (vy < 0 ? -py : py));
        r->dflags = d->dflags;
    }
}

static void place_arms(sk_game *g, const sk_slot *s, const sk_draw *d) {        /* 0x116DE0 */
    int a = ang_a(s), b = ang_b(s);
    alien_arm(g, d, 0, a, len_a(s), d->x - 8, d->y + 10 + g->alien.arm_drop);
    sk_draw *h = &g->objs.chain[2];
    int t = hand_a_by_angle[((a + 4) & 0xFF) >> 3];                             /* the offset by the angle; the frame overridable */
    h->x = (int16_t)(h->x - hand_a_off[t][0]); h->y = (int16_t)(h->y - hand_a_off[t][1]);
    hand_frame(h, g->alien.hand_a >= 0 ? g->alien.hand_a : t);
    alien_arm(g, d, 3, b, len_b(s), d->x + 0x37, d->y + 10 + g->alien.arm_drop);
    h = &g->objs.chain[5];
    int f = g->alien.hand_b >= 0 ? g->alien.hand_b : hand_b_by_angle[((b + 4) & 0xFF) >> 3];
    if (f > 21) f = 21;                                                         /* port guard: the table ends at 21 */
    h->x = (int16_t)(h->x - hand_b_off[f][0]); h->y = (int16_t)(h->y - hand_b_off[f][1]);
    hand_frame(h, f);
}

static void alien_records(sk_game *g, const sk_slot *s, int first, int n, int w, int h, const uint8_t *frames) {   /* 0x1165E0 / 0x116620 */
    for (int i = first; i < first + n; i++) {
        sk_draw *r = &g->objs.chain[i];
        memset(r, 0, sizeof *r);
        size_frames(g, r, w, h, frames);
        r->x = s->x; r->y = s->y; r->prio = 1; r->dflags = 0x60; r->used = true;
    }
}

static void obj_alien_plain_init(sk_game *g, sk_slot *s, int x, int y) {        /* 0x11656A */
    init_object_slot(g, s, x, y - 0x2A, s->type4);
    const uint8_t *f = res(g, 139), *f2 = res(g, 140);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 60, f, 0);
    g->objs.child_frames = f2; g->objs.child_vx = 0; g->objs.child_vy = 0; g->objs.child_w = 0x40; g->objs.child_h = 0x3C;
    spawn_object(g, s->x, s->y, 116);
    if (g->objs.last_draw) g->objs.last_draw->dflags |= 0x20;
    g->push_x = s->x - 0xD4;
}

static void obj_alien_init(sk_game *g, sk_slot *s, int x, int y) {              /* 0x1163E4 */
    if (g->cup_pieces != 0x1F) { obj_alien_plain_init(g, s, x, y); return; }
    x -= 0x20; y -= 0x31;
    memset(&g->alien, 0, sizeof g->alien);                                      /* port: nothing left from an earlier fight */
    s->x = 0;                                                                   /* reserves the slot for the children */
    g->objs.marker_param = 0; g->alien.piece_a = spawn_object(g, x, y, 84);     /* port: the spawn's own result (last_slot can be stale) */
    g->objs.marker_param = 1; g->alien.piece_b = spawn_object(g, x, y, 84);
    init_object_slot(g, s, x, y, s->type4);
    s->x0 = 0; s->y0 = 0x0808; s->vx = 0; s->vy = 0; s->hits = 0x23;
    const uint8_t *f = res(g, 135), *parts = res(g, 136), *link = res(g, 137);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 64, 49, f, 1);
    s->draw->dflags |= 0x64;
    g->push_x = s->x - 0xD4;
    g->objs.child_frames = parts; g->objs.child_w = 0x20; g->objs.child_h = 0x20; g->objs.child_vx = 0; g->objs.child_vy = 0;
    g->alien.head = spawn_object(g, s->x + 0x10, s->y - 0x12, 116);
    g->alien.head_draw = g->alien.head ? g->objs.last_draw : NULL;
    if (g->alien.head_draw) g->alien.head_draw->dflags |= 0x60;
    if (link && parts) {
        alien_records(g, s, 0, 2, 16, 8, link); alien_records(g, s, 2, 1, 32, 32, parts + 0x2300);
        alien_records(g, s, 3, 2, 16, 8, link); alien_records(g, s, 5, 1, 32, 32, parts + 0x3700);
        g->objs.chain_c = true;
    }
    g->alien.hand_b = g->alien.hand_a = -1;
    g->alien.arm_drop = 0;
    g->objs.chain_count = 5;
}

static void alien_free_all(sk_game *g, sk_slot *s) {                           /* 0x116E12: the fight starts over */
    g->objs.chain_c = false;
    free_object(g, s);
    if (g->alien.head && g->alien.head->x >= 0) free_object(g, g->alien.head);
    if (g->alien.piece_a && g->alien.piece_a->x >= 0) free_object(g, g->alien.piece_a);
    if (g->alien.piece_b && g->alien.piece_b->x >= 0) free_object(g, g->alien.piece_b);
    memset(&g->alien, 0, sizeof g->alien);
}

static void forget_lost_parts(sk_game *g) {                                    /* port: a part freed under the boss (free_if_far) is let go */
    if (g->alien.head && (g->alien.head->x < 0 || (g->alien.head->type4 & 0x1FF) != 116 * 4)) { g->alien.head = NULL; g->alien.head_draw = NULL; }
    if (g->alien.piece_a && (g->alien.piece_a->x < 0 || (g->alien.piece_a->type4 & 0x1FF) != 84 * 4)) { g->alien.piece_a = NULL; g->alien.pa_draw = NULL; }
    if (g->alien.piece_b && (g->alien.piece_b->x < 0 || (g->alien.piece_b->type4 & 0x1FF) != 84 * 4)) { g->alien.piece_b = NULL; g->alien.pb_draw = NULL; }
}

static void alien_boss(sk_game *g, sk_slot *s, sk_draw *d) {                    /* alien_boss_body 0x116702 */
    forget_lost_parts(g);
    int ox = d->x, oy = d->y, changed = 0;
    int code = script_step(g, s, alien_phases, alien_motions, &changed);
    shift(g->alien.head_draw, d->x - ox, d->y - oy); shift(g->alien.pa_draw, d->x - ox, d->y - oy); shift(g->alien.pb_draw, d->x - ox, d->y - oy);
    enum { HIT, BALL, END } tail = HIT;
    int cnt = s->flags_b;
    switch (code >> 2) {
    case 0: tail = BALL; break;
    case 1: d->dflags ^= 4; tail = BALL; break;
    case 2:                                                                     /* the stance */
        head_frame(g, s, 0, 5); hand_b_cycle(g); pieces_pose(g, 1, 1);
        if (aux_c(s) & 4) { if (!(aux_c(s) & 8)) script_start(s, 36, alien_phases, alien_motions); tail = END; }
        break;
    case 3: case 4: case 5: {                                                   /* arm B swings out to the left */
        static const int stop[3] = { 0xD4, 0xDA, 0xE3 }, minlen[3] = { -1, 6, 4 };
        int k = (code >> 2) - 3;
        if (once(s)) g->alien.hand_b = -1;
        head_frame(g, s, 4, 4);
        if (minlen[k] >= 0 && len_b(s) != minlen[k]) set_len_b(s, len_b(s) - 1);
        set_ang_b(s, ang_b(s) - 1);
        if (ang_b(s) == stop[k]) script_end_next(s, alien_phases);
        break;
    }
    case 6:                                                                     /* the jab */
        if (once(s)) { g->alien.hand_b = 0x0C; set_len_b(s, len_b(s) + 2); cnt = 0; }
        if (++cnt == 8) { g->alien.hand_b = -1; set_len_b(s, len_b(s) - 2); }
        break;
    case 7:
        if (ang_b(s) >= 0xF8 && len_b(s) != 8) set_len_b(s, len_b(s) + 1);
        set_ang_b(s, ang_b(s) + 1);
        if (ang_b(s) == 0) script_end_next(s, alien_phases);
        break;
    case 8:
        head_frame(g, s, 0, 5); hand_b_cycle(g); bob(g, d);
        set_ang_a(s, ang_a(s) - 4);
        if (ang_a(s) == 0x84) script_end_next(s, alien_phases);
        break;
    case 9:                                                                     /* the shot from arm A's hand */
        if (once(s)) { g->objs.marker_param = ang_a(s); spawn_object(g, g->objs.chain[2].x, g->objs.chain[2].y, 83); }
        hand_b_cycle(g); bob(g, d);
        break;
    case 10:
        hand_b_cycle(g); bob(g, d);
        if (ang_a(s) == 0) script_end_next(s, alien_phases); else set_ang_a(s, ang_a(s) + 4);
        break;
    case 11: head_frame(g, s, 2, 2); d->dflags ^= 4; tail = BALL; break;
    case 12:                                                                    /* gone: home, turned round */
        d->dflags |= 4;
        if (!once(s)) { move_all(g, d, s->x - d->x, 0); s->flags ^= 1; script_end_next(s, alien_phases); }
        tail = BALL;
        break;
    case 13:                                                                    /* rises */
        if (once(s)) { pieces_pose(g, 2, -1); pieces_spread(g, 1); }
        head_frame(g, s, 1, 1); hand_b_cycle(g); move_all(g, d, 0, -1);
        break;
    case 14:                                                                    /* comes down */
        head_frame(g, s, 4, 4); hand_b_cycle(g); move_all(g, d, 0, 1);
        if (s->y == d->y) { pieces_pose(g, 1, 1); pieces_spread(g, -1); script_end_next(s, alien_phases); }
        break;
    case 15: hand_b_cycle(g); bob(g, d); break;
    case 16: {                                                                  /* aims arm A at the kid */
        if (once(s)) {
            int a = (atan2_256(g->player.y - d->y - 10, g->player.x - d->x + 8) + 4) & 0xFF;
            set_aux_d(s, (int8_t)a < 0 ? a : 0xFC);
        }
        hand_b_cycle(g); bob(g, d);
        int diff = (int8_t)(uint8_t)(aux_d(s) - ang_a(s));
        if ((uint8_t)(diff < 0 ? -diff : diff) >= 4) set_ang_a(s, ang_a(s) + 4); else script_end_next(s, alien_phases);
        break;
    }
    case 17: head_frame(g, s, 2, 2); hand_b_cycle(g); move_body_head(g, d, 0, 10); script_end_next(s, alien_phases); break;
    case 18:                                                                    /* the jump */
        head_frame(g, s, 2, 2);
        if (once(s)) { move_body_head(g, d, 0, -10); pieces_pose(g, 2, -1); pieces_spread(g, 1); }
        break;
    case 19:                                                                    /* the landing */
        if (once(s)) {
            s->flags ^= 1; pieces_pose(g, 1, 1); pieces_spread(g, -1);
            g->shake_amplitude = 8; move_body_head(g, d, 0, 10); cnt = 0;
        }
        head_frame(g, s, 0, 5);
        if (++cnt == 10) move_body_head(g, d, 0, -10);
        break;
    case 20:
        g->alien.hand_b = -1; head_frame(g, s, 0, 5);
        set_ang_b(s, ang_b(s) + 4);
        if (ang_b(s) == 0x38) script_end_next(s, alien_phases);
        break;
    case 21:
        g->alien.hand_b = -1; head_frame(g, s, 4, 4);
        set_ang_b(s, ang_b(s) - 4);
        if (ang_b(s) == 0) script_end_next(s, alien_phases);
        break;
    case 22:                                                                    /* the long reach */
        if (once(s)) { cnt = 0; set_aux_d(s, 0); }
        if (!(g->frame_ticks & 3)) { cnt = (cnt + 1) & 3; g->alien.hand_b = (int8_t)(cnt + 14); }   /* hand_b_open_cycle */
        if (aux_d(s) == 0) {
            set_len_b(s, len_b(s) + 4);
            if (len_b(s) != 0x50) break;
            set_aux_d(s, 0xFF);
        }
        set_len_b(s, len_b(s) - 4);
        if (len_b(s) == 8) script_end_next(s, alien_phases);
        break;
    case 23:                                                                    /* falls */
        if (once(s)) { d->dflags &= (uint8_t)~2; set_ang_a(s, 0xD0); set_ang_b(s, 0x2C); cnt = 0; }
        if (!(g->frame_ticks & 1)) { if (++cnt > 3) cnt = 3; g->alien.hand_b = (int8_t)(cnt + 18); }   /* hand_b_fall_cycle */
        g->alien.arm_drop = (int16_t)(g->alien.arm_drop + 4);
        d->y = (int16_t)(d->y + 2);
        shift(g->alien.head_draw, 0, 8);
        tail = END;
        break;
    case 24:                                                                    /* dies */
        if (once(s)) {
            head_frame(g, s, 6, 6);
            g->score_value = 0x100000; add_score(g, 0x100000);
            spawn_object(g, d->x + 0x20, d->y + 0x18, 113);
            g->shake_amplitude = 6; play_sfx(g, 0x11);
            game_music_slot(g, 2);                                              /* the level lists no slot 2: nothing changes */
        }
        tail = END;
        break;
    default:                                                                    /* 25 */
        if (once(s)) { g->level_outcome = 0x81; g->level_number++; g->curtain_flags |= 0x10; }
        tail = END;
        break;
    }
    s->flags_b = (uint8_t)cnt;
    if (tail == HIT) {                                                          /* 0x116D62 */
        touch_player(g, d->x, d->y, 0x40, 0x40);
        touch_player(g, g->objs.chain[5].x, g->objs.chain[5].y, 0x18, 0x18);
    }
    if (tail != END) {                                                          /* 0x116D94 */
        ball_hits_object_simple(g, s, 0, 0, 0x40, 0x40);
        if (!s->hits) set_aux_c(s, aux_c(s) | 4);
    }
    if (g->alien.head_draw) g->alien.head_draw->dflags = d->dflags;              /* 0x116DAE: all flash and hide together */
    if (g->alien.pa_draw) g->alien.pa_draw->dflags = d->dflags;
    if (g->alien.pb_draw) g->alien.pb_draw->dflags = d->dflags;
    place_arms(g, s, d);
    if (g->player.dying) set_aux_c(s, aux_c(s) | 2);
    if ((aux_c(s) & 2) && !g->player.dying) alien_free_all(g, s);
}

static void obj_alien_update(sk_game *g, sk_slot *s, int index) {              /* 0x116660 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    if (g->cup_pieces == 0x1F) { alien_boss(g, s, d); return; }
    if (!aux_c(s)) {
        if (s->x - 0x40 - g->player.x >= 0) return;                             /* until the kid is near */
        g->message_box_kind = 0; g->message_box_request = 0xFF;
        set_aux_c(s, 0xFF); s->t0 = 30;
        if (!g->game_flags) game_jingle_load(g);
        game_music(g, SK_MUSIC_JINGLE, 3);                                      /* the long jingle */
    }
    if (--s->t0 == 0) { g->music_stopped = true; if (g->snd) sound_stop(g->snd);   /* music_stop */
         g->level_outcome = 0x81; g->level_number++; g->curtain_flags |= 0x10; }
}

/* ---- type 84: a body piece of the ALIEN (slot 136, frames 7..13): a puppet the boss moves ---- */
static void obj_alien_piece_init(sk_game *g, sk_slot *s, int x, int y) {        /* 0x117374 */
    init_object_slot(g, s, x, y + 0x26, s->type4);
    s->t1 = 0;
    const uint8_t *f = res(g, 136);
    if (!need(g, s, f)) return;
    frames_wh(g, s, 32, 32, f, 1);
    s->draw->dflags |= 0x60;
    int base = 7;
    if (g->objs.marker_param) { base = 11; g->alien.pb_draw = s->draw; s->draw->x = (int16_t)(s->draw->x + 0x20); }
    else g->alien.pa_draw = s->draw;
    s->t0 = (int16_t)base;
    set_object_frame(s, base);
}
static void obj_alien_piece_update(sk_game *g, sk_slot *s, int index) {         /* 0x1173D6 */
    (void)index;
    if (!slot_draw_record(g, s)) return;
    set_object_frame(s, s->t0 + s->t1);
}

/* ---- type 83: the ALIEN's shot (slot 138, 16 x 16), along arm A's angle ---- */
static void obj_alien_shot_init(sk_game *g, sk_slot *s, int x, int y) {         /* 0x1173F0 */
    init_object_slot(g, s, x, y, s->type4);
    int a = g->objs.marker_param & 0xFF;
    s->vx = (int16_t)(cos_256(a) / 4); s->vy = (int16_t)(sin_256(a) / 4);      /* 0x105E72 / 0x105E6E, toward zero */
    const uint8_t *f = res(g, 138);
    if (!need(g, s, f)) return;
    frames_16x16(g, s, f, 1);
    s->draw->dflags |= 0x40;
    int fr = 6 - ((((-a) & 0xFF) + 8) >> 4);
    set_object_frame(s, fr < 0 ? 0 : fr);                                       /* ruling: clamped (the original reads before the sheet) */
}
static void obj_alien_shot_update(sk_game *g, sk_slot *s, int index) {          /* 0x117472 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    d->x = (int16_t)(d->x + s->vx); d->y = (int16_t)(d->y + s->vy);
    touch_player(g, d->x, d->y, 0x10, 0x10);
    if (object_offscreen(g, d->x, d->y, 64) < 0) free_object(g, s);
}

/* ---- type 85: the tokens still to take, two big digits at the top left of the bonus levels (setup_level 0x104270) ---- */
#define BIG_FONT 0x207A00u                                                      /* 13 rows of 4 words a glyph: ~mask, three planes */
static void obj_tokens_left_init(sk_game *g, sk_slot *s, int x, int y) {        /* 0x11A2FC */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->x0 = 16; s->y0 = 16; s->vx = 6; s->vy = 0; s->t0 = 0x32; set_aux_c(s, 0);
    memset(g->token_bits, 0, sizeof g->token_bits);
    frames_wh(g, s, 32, 13, g->token_bits, 1);                                  /* sub_107A5A */
    if (s->draw) s->draw->dflags |= 4 | 8;
}
static void obj_tokens_left_update(sk_game *g, sk_slot *s, int index) {         /* 0x11A344 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    d->x = s->x0; d->y = s->y0;
    d->dflags &= (uint8_t)~4;
    int left = 0x32 - g->bonus_tokens;
    if (left < 0) left = 0;                                                     /* port guard: two digits */
    g->token_digits[0] = (uint8_t)(0x5D + left / 10 % 10);
    g->token_digits[1] = (uint8_t)(0x5D + left % 10);
    for (int c = 0; c < 2 && g->sk; c++) {
        const uint8_t *glyph = image_ptr(g->sk, BIG_FONT + (uint32_t)(g->token_digits[c] - 0x40) * 0x68u, 0x68);
        if (!glyph) continue;
        for (int r = 0; r < 13; r++) {
            uint8_t *row = g->token_bits + r * 4 + c * 2;
            const uint8_t *w = glyph + r * 8;
            row[4 * 0x34] = (uint8_t)~w[0]; row[4 * 0x34 + 1] = (uint8_t)~w[1];   /* the mask */
            row[0] = w[2]; row[1] = w[3];
            row[0x34] = w[4]; row[0x34 + 1] = w[5];
            row[2 * 0x34] = w[6]; row[2 * 0x34 + 1] = w[7];
            row[3 * 0x34] = 0; row[3 * 0x34 + 1] = 0;
        }
    }
}

void obj_bonus_install(void) {
    object_init_table[81] = obj_cup_piece_init;        object_update_table[81] = obj_cup_piece_update;
    object_init_table[82] = obj_alien_init;            object_update_table[82] = obj_alien_update;
    object_init_table[83] = obj_alien_shot_init;       object_update_table[83] = obj_alien_shot_update;
    object_init_table[85] = obj_tokens_left_init;      object_update_table[85] = obj_tokens_left_update;
    object_init_table[84] = obj_alien_piece_init;      object_update_table[84] = obj_alien_piece_update;
}
