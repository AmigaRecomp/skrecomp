/* England 1 and 2's tile attributes, transcribed from the listing (objects.md has the pseudo-code). Handlers are
   entered with the cell's pixel position (d0 = cx * 16, d1 = cy * 16) and probe_kind: 0 the kid (a6 = player),
   1 the ball (a6 = the ball record, so +0x10/+0x12 are its previous position). Also the bump-block object
   (type 122), the two tile-restore lists and the message box's state (its drawing is the HUD's, Task 9). */
#include <string.h>
#include "game/attr_england.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/hud.h"
#include "game/obj_helpers.h"
#include "game/player.h"
#include "game/resources.h"

#define TARGET_SCRIPT_SLOT 108

static int16_t bx(const sk_game *g) { return (int16_t)(g->ball.x >> 16); }
static int16_t by(const sk_game *g) { return (int16_t)(g->ball.y >> 16); }
static int16_t bvx(const sk_game *g) { return (int16_t)(g->ball.vx >> 16); }
static int16_t bvy(const sk_game *g) { return (int16_t)(g->ball.vy >> 16); }
static int iabs(int v) { return v < 0 ? -v : v; }
static bool in_open(int v, int lo, int hi) { return v > lo && v < hi; }   /* the cmpi/ble/bge pairs */
static int cell(const sk_game *g, int cx, int cy) { return cy * g->world.w + cx; }

/* the ball strictly within +-15 of the cell's corner (coins, cards) */
static bool ball_near15(const sk_game *g, int x, int y) { return in_open(bx(g) - x, -15, 15) && in_open(by(g) - y, -15, 15); }

/* ---- attributes 0 and 19: coins ---- */
static void coin(sk_game *g, int cx, int cy, int kind, int fallback) {   /* 0x11AD28 / 0x11ADAA */
    int x = cx * 16, y = cy * 16;
    if (g->attrs.probe_kind == 1) { if (!ball_near15(g, x, y)) return; }
    else if (g->attrs.probe_kind == 0) { if (!in_open(g->player.x - x, -8, 18)) return; }
    else return;
    add_score(g, 0x20);
    play_sfx(g, 1);
    queue_tile_change(g, cx, cy, kind, fallback);
}
static void attr_coin(sk_game *g, int cx, int cy) { coin(g, cx, cy, 0, 0); }
static void attr_coin_keep(sk_game *g, int cx, int cy) { coin(g, cx, cy, 1, 0x1EF); }

/* ---- attributes 37 and 66: card pieces ---- */
static void card(sk_game *g, int cx, int cy, bool keep_tile) {           /* 0x11AC50 / 0x11C240 */
    int x = cx * 16, y = cy * 16;
    const sk_player *p = &g->player;
    if (g->attrs.probe_kind == 1) { if (!ball_near15(g, x, y)) return; }
    else if (g->attrs.probe_kind == 0) {
        if (!in_open(p->x - x, -8, 18)) return;
        if ((p->crouching || p->sliding) && p->y - y > -10) return;
    } else return;
    g->hud_card_flag = 0xFF;
    g->score_value = 0x1000;
    spawn_object(g, x, y, 123);
    add_score(g, 0x1000);
    play_sfx(g, 9);
    if (keep_tile) g->world.live[cell(g, cx, cy)] &= 0x1FF;
    else clear_cell(g, cx, cy);
}
static void attr_card(sk_game *g, int cx, int cy) { card(g, cx, cy, false); }
static void attr_card_keep_tile(sk_game *g, int cx, int cy) { card(g, cx, cy, true); }

/* ---- attributes 5 6 7 9 10: kit shades (set_kit_colours / set_kit_extra_colours with d2 = kit * 4) ---- */
static void set_kit(sk_game *g, int d2) {
    if (g->attrs.probe_kind == 1) g->kit_extra = d2 >> 2;
    else g->kit_current = d2 >> 2;
}
static bool shade_d(const sk_game *g, int cx, int cy, bool by_y, int *d) {
    if (g->attrs.probe_kind > 1) return false;
    bool ball = g->attrs.probe_kind == 1;
    int px = ball ? g->ball.prev_x : g->player.x, py = ball ? g->ball.prev_y : g->player.y;
    int v = (int16_t)(by_y ? py - cy * 16 : px - cx * 16);
    if (v < 0 || v >= (by_y ? 16 : 32)) return false;
    *d = by_y ? (v & 12) : ((v >> 1) & 12);
    return true;
}
static void attr_shade_x(sk_game *g, int cx, int cy) { int d; if (shade_d(g, cx, cy, false, &d)) set_kit(g, d); }             /* 0x11AE30 */
static void attr_shade_x_rev(sk_game *g, int cx, int cy) { int d; if (shade_d(g, cx, cy, false, &d)) set_kit(g, 12 - d); }    /* 0x11AE68 */
static void attr_shade_y(sk_game *g, int cx, int cy) { int d; if (shade_d(g, cx, cy, true, &d)) set_kit(g, d); }              /* 0x11AEA6 */
static void attr_shade_x_dark(sk_game *g, int cx, int cy) { int d; if (shade_d(g, cx, cy, false, &d)) set_kit(g, d + 16); }   /* 0x11AF18 */
static void attr_shade_x_dark_rev(sk_game *g, int cx, int cy) { int d; if (shade_d(g, cx, cy, false, &d)) set_kit(g, 28 - d); }   /* 0x11AF54 */

/* ---- attributes 1 and 42: drains ---- */
static void swallow_ball(sk_game *g) {
    sk_ball *b = &g->ball;
    if (b->vanishing) return;
    b->vx = b->vy = 0;
    b->vanishing = 0xFF;
    g->bw.roll_phase = 0;
    play_sfx(g, 14);
    if (g->player.state == 16) { g->player.timer = 0; g->player.state = 0; }
}
static void drain_hurt(sk_game *g, int x, int y) {
    const sk_player *p = &g->player;
    if (in_open(p->x - x, -8, 18) && in_open(p->y - y, -40, 16)) hurt_player(g);
}
static void attr_drain(sk_game *g, int cx, int cy) {                    /* 0x11B146 */
    int x = cx * 16, y = cy * 16;
    if (g->attrs.probe_kind == 1) { if (in_open(bx(g) - x, -16, 16) && in_open(by(g) - y, -16, 16)) swallow_ball(g); }
    else if (g->attrs.probe_kind == 0) drain_hurt(g, x, y);
}
static void attr_drain_deep(sk_game *g, int cx, int cy) {               /* 0x11B88C */
    int x = cx * 16, y = cy * 16;
    if (g->attrs.probe_kind == 1) {
        if ((int16_t)(g->ball.prev_y - y) >= 18) return;
        if (in_open(by(g) - y, -18, 18) && in_open(bx(g) - x, -16, 16)) swallow_ball(g);
    } else if (g->attrs.probe_kind == 0) {
        if ((int16_t)(g->player.prev_y - y) >= 16) return;
        drain_hurt(g, x, y);
    }
}

/* ---- attribute 25: the block that falls when the kid stands on it ---- */
static void attr_bump_block(sk_game *g, int cx, int cy) {               /* 0x11B530 */
    const sk_player *p = &g->player;
    int x = cx * 16, y = cy * 16;
    if (g->attrs.probe_kind != 0 || p->vy > 0) return;
    if (!in_open(p->x - x, 0, 16)) return;
    int dy = (int16_t)(p->y - y);
    if (dy < -41 || dy >= -36) return;
    int c = cell(g, cx, cy);
    if (g->bump_restores < SK_BUMP_RESTORES) {
        g->bump_list[g->bump_restores].cell = c;
        g->bump_list[g->bump_restores].word = g->world.live[c];
        g->bump_restores++;
    }
    int tile = g->world.live[c] & 0x1FF;                            /* copy_tile_image (0x11AAC4) */
    if (tile != g->bump_tile && g->world.lv && tile < g->world.lv->tiles_a.count) {
        g->bump_tile = tile;
        const uint8_t *t = g->world.lv->tiles_a.tiles + (size_t)tile * SK_TILE_BYTES;
        for (int r = 0; r < 16; r++)
            for (int pl = 0; pl < 4; pl++) { g->bump_bob[pl * 32 + r * 2] = t[r * 8 + pl * 2]; g->bump_bob[pl * 32 + r * 2 + 1] = t[r * 8 + pl * 2 + 1]; }
        memset(g->bump_bob + 0x80, 0xFF, 0x20);                     /* dat_427D7E: a full mask */
    }
    spawn_object(g, x, y, 122);
    g->mask.dirty = true;
    clear_cell(g, cx, cy);
}

void replay_bump_restores(sk_game *g) {                          /* init_player 0x10BC82 */
    for (int i = 0; i < g->bump_restores; i++) {
        int c = g->bump_list[i].cell;
        g->world.live[c] = g->bump_list[i].word;
        g->world.tile[c] = (uint16_t)((g->world.tile[c] & SK_MARKER_BIT) | (g->bump_list[i].word & SK_INDEX_MASK));
    }
    g->bump_restores = 0;
}

void flush_tile_restores(sk_game *g) {                           /* 0x11B504: attribute 24's list (not in England 1/2) */
    for (int i = 0; i < g->flush_count; i++) set_cell_tile(g, g->flush_list[i] % g->world.w, g->flush_list[i] / g->world.w, 0x1E6, 0);
    g->flush_count = 0;
}

/* type 122: the fallen block (0x1174B0 / 0x117518); it rides a platform record in the original (5c), here it
   only falls: the kid is not carried */
static void obj_bumped_block_init(sk_game *g, sk_slot *s, int x, int y) {
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->t1 = 0;                                                      /* the platform record's counter (+0x10) */
    s->aux = (int16_t)y;                                            /* the platform's y */
    s->anim = 0;
    frames_16x16(g, s, g->bump_bob, 1);
}
static void obj_bumped_block_update(sk_game *g, sk_slot *s, int index) {
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (++s->t0 < 25) {
        s->t1++;
        s->aux = (int16_t)(s->aux + ((uint16_t)s->t1 >> 2));
        d->x = s->x; d->y = s->aux;
        if (object_offscreen(g, s->x, s->y, 160) >= 0) return;
    }
    g->mask.dirty = true;
    g->bw.mask_dirty = true;
    free_object(g, s);
}

/* ---- attributes 27 and 28: the message signs ---- */
static int message_sign_check(sk_game *g, int cx, int cy) {             /* 0x11B5A2: -1 when the ball hits it hard */
    int x = cx * 16, y = cy * 16;
    if (g->attrs.probe_kind != 1) return 0;
    int dx = (int16_t)(bx(g) - x), dy = (int16_t)(by(g) - y);
    if (dx < -16 || dx > 16 || !in_open(dy, -16, 16)) return 0;
    if ((uint16_t)(iabs(bvx(g)) + iabs(bvy(g))) < 4) return 0;
    spawn_object(g, x, y, 125);
    spawn_object(g, x - 4, y, 126);
    clear_cell(g, cx, cy);
    g->mask.dirty = true;
    play_sfx(g, 8);
    return -1;
}
static void message_request(sk_game *g, int cx, int cy, int kind) {     /* message_request_tail 0x11B71C */
    g->message_box_kind = kind;
    if (g->message_box_timer) return;
    if (message_sign_check(g, cx, cy)) g->message_box_request = 1;
}
static void attr_message_0(sk_game *g, int cx, int cy) { message_request(g, cx, cy, 0); }   /* 0x11B6BC */
static void attr_message_8(sk_game *g, int cx, int cy) { message_request(g, cx, cy, 8); }   /* 0x11B6C8 */

void message_box_tick(sk_game *g) {                              /* 0x11C66E */
    if (g->message_box_timer) { g->message_box_timer--; return; }
    if (!(g->message_box_request & 1)) return;
    g->message_box_request &= ~1;
    int k = (g->message_box_kind >> 3) & 7;                         /* show_message_box: each kind once a level */
    if (g->message_shown[k]) return;
    g->message_shown[k] = 1;
    g->message_box_shown_kind = k;
    message_box_build(g, k);                                        /* the table at 0x200DA4: kind k shows message k */
    play_sfx(g, 10);
    g->message_wait = 1;                                            /* 0x3001DE = 150: step 31 holds the game until fire */
    g->message_box_timer = 250;
}

/* ---- attribute 62: butterflies from a bush; the ball targets share the tail ---- */
static void flutter(sk_game *g, int x, int y) {
    if (x == g->flutter_last_cell || g->flutter_count >= g->flutter_max) return;
    g->flutter_count++;
    g->flutter_last_cell = x;
    spawn_object(g, x + (g->frame_ticks & 15), y, 103);
}
static bool ball_hits_cell(const sk_game *g, int x, int y) {
    int dx = (int16_t)(bx(g) - x), dy = (int16_t)(by(g) - y);
    return dx >= 0 && dx <= 16 && dy >= 0 && dy <= 16 && iabs(bvx(g)) + iabs(bvy(g)) > 2;
}
static void attr_butterflies(sk_game *g, int cx, int cy) {              /* 0x11BF10 */
    int x = cx * 16, y = cy * 16;
    const sk_player *p = &g->player;
    if (g->attrs.probe_kind == 1) { if (ball_hits_cell(g, x, y)) flutter(g, x, y); return; }
    if (g->attrs.probe_kind != 0) return;
    int dx = (int16_t)(p->x - x), dy = (int16_t)(p->y - y);
    if (dx < 0 || dx > 16 || dy < -41 || dy > -30 || (uint16_t)p->speed < 4) return;
    flutter(g, x, y);
}

static void ball_target(sk_game *g, int cx, int cy, uint8_t *count, int param) {   /* ball_target_tail 0x11C3A4 */
    int x = cx * 16, y = cy * 16;
    g->objs.marker_param = param;
    if (g->attrs.probe_kind != 1 || !ball_hits_cell(g, x, y)) return;
    if (g->player.ball_free && (int8_t)*count >= 0) {
        g->player.ball_free = 0;
        ++*count;
        if (*count != 8) {
            sk_objects *o = &g->objs;
            o->child_vx = 0; o->child_vy = 0; o->child_w = 32; o->child_h = 24;
            o->child_frames = g->res ? resource_slot(g->res, TARGET_SCRIPT_SLOT, NULL) : NULL;
            o->child_script = obj_target_hit_script;
            spawn_object(g, x - 8, y - 8, 115);
        }
        if (*count == 4) {
            g->score_value = 0x4000;
            add_score(g, 0x4000);
            spawn_object(g, x, y, 123);
            play_sfx(g, 9);
        }
        if (*count >= 8) { *count = 0xFF; spawn_object(g, x - 8, y, 112); }
    }
    flutter(g, x, y);
}
static void attr_ball_target_a(sk_game *g, int cx, int cy) { ball_target(g, cx, cy, &g->ball_target[0], 0); }   /* 0x11C318 */

void attr_england_install(void) {
    attr_table[0] = attr_coin;               attr_table[1] = attr_drain;
    attr_table[5] = attr_shade_x;            attr_table[6] = attr_shade_x_rev;
    attr_table[7] = attr_shade_y;            attr_table[9] = attr_shade_x_dark;
    attr_table[10] = attr_shade_x_dark_rev;  attr_table[19] = attr_coin_keep;
    attr_table[25] = attr_bump_block;        attr_table[27] = attr_message_0;
    attr_table[28] = attr_message_8;         attr_table[37] = attr_card;
    attr_table[42] = attr_drain_deep;        attr_table[62] = attr_butterflies;
    attr_table[66] = attr_card_keep_tile;    attr_table[67] = attr_ball_target_a;
    object_init_table[122] = obj_bumped_block_init;
    object_update_table[122] = obj_bumped_block_update;
}
