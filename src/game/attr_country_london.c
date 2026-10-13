/* England country and London's tile attributes, transcribed from the listing (objects.md). */
#include "game/attr_country_london.h"
#include "game/attr_england.h"
#include "game/attrs.h"
#include "game/player.h"
#include "game/resources.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/platforms.h"

/* attribute 23: a moving platform turns round here (0x11B426, probe_kind 2) */
static void attr_platform_reverse(sk_game *g, int cx, int cy) {
    (void)cx; (void)cy;
    if (g->attrs.probe_kind != 2) return;
    g->attrs.probe_kind = 0xFF;
    sk_slot *s = g->attrs.probe_slot;
    if (!s || s->t0 || aux_d(s) == 4) return;
    set_aux_d(s, 4);
    sk_platform *pf = slot_platform(g, s);
    if (!pf) return;
    int angle = (pf->pangle + 0x80) & 0xFF;                        /* +128 or -128: the same byte */
    s->t1 = (int16_t)(pf->pspeed << 8 | angle);
    s->t0 = 1;
    pf->pmode = (uint8_t)~pf->pmode;
}

/* attribute 26: a sign with no message (message_sign_check itself, 0x11B5A2) */
static void attr_bare_sign(sk_game *g, int cx, int cy) { message_sign_check(g, cx, cy); }

/* attribute 38: one way, no going back left (0x11B734) */
static void attr_oneway_left(sk_game *g, int cx, int cy) {
    (void)cy;
    int x = cx * 16;
    if (g->attrs.probe_kind == 1) {
        sk_ball *b = &g->ball;
        int d = (int16_t)((b->x >> 16) - x);
        if (d < -16 || d > 16) return;
        if ((int16_t)(b->prev_x - (b->x >> 16)) <= 0) return;
        b->x = (int32_t)((uint32_t)(uint16_t)b->prev_x << 16 | ((uint32_t)b->x & 0xFFFF));
        b->vx = -b->vx;                                            /* neg.l */
        return;
    }
    if (g->attrs.probe_kind != 0) return;
    sk_player *p = &g->player;
    int d = (int16_t)(p->x - x);
    if (d < 10 || d > 26) return;
    if ((int16_t)(p->prev_x - p->x) <= 0) return;
    p->x = p->prev_x;
    p->speed = 0;
    if (p->state == 16 && p->trick == 8) p->state = 0;
}

/* attribute 43: deadly (water): kill_player (0x11BA18, the kid's probe only) */
static void attr_deadly(sk_game *g, int cx, int cy) {
    if (g->attrs.probe_kind != 0) return;
    int dx = (int16_t)(g->player.x - cx * 16), dy = (int16_t)(g->player.y - cy * 16);
    if (dx > -8 && dx < 0x16 && dy > -40 && dy < 0x18) kill_player(g);
}

/* attribute 55: the splash, once until a respawn (0x11BDAC) */
static const uint8_t splash_script[] = { 0, 5, 0, 6, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4 };   /* 0x11BE38 */
static void attr_splash(sk_game *g, int cx, int cy) {
    if (g->attrs.probe_kind != 0 || g->splash_done) return;
    int x = cx * 16, y = cy * 16;
    int dx = (int16_t)(g->player.x - x), dy = (int16_t)(g->player.y - y);
    if (dx < 0 || dx > 16 || dy >= 0x1C || dy < -0x24) return;
    g->splash_done = 0xFF;
    sk_objects *o = &g->objs;
    o->child_frames = g->res ? resource_slot(g->res, 26, NULL) : NULL;
    o->child_script = splash_script;
    o->child_w = 0x40; o->child_h = 0x18; o->child_vx = 0; o->child_vy = 0;
    spawn_object(g, x - 0x18, y - 0xD, 115);
    play_sfx(g, 0x15);
}

/* attributes 70, 72, 73: more ball targets (0x11C34E, 0x11C372, 0x11C384) */
static void attr_ball_target_b(sk_game *g, int cx, int cy) { ball_target(g, cx, cy, &g->ball_target[3], 0); }
static void attr_ball_target_c(sk_game *g, int cx, int cy) { ball_target(g, cx, cy, &g->ball_target[5], 6); }
static void attr_ball_target_d(sk_game *g, int cx, int cy) { ball_target(g, cx, cy, &g->ball_target[6], 6); }

void attr_country_london_install(void) {
    attr_table[23] = attr_platform_reverse;
    attr_table[26] = attr_bare_sign;
    attr_table[38] = attr_oneway_left;
    attr_table[43] = attr_deadly;
    attr_table[55] = attr_splash;
    attr_table[70] = attr_ball_target_b;
    attr_table[72] = attr_ball_target_c;
    attr_table[73] = attr_ball_target_d;
}
