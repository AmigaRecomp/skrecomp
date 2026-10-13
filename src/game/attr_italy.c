/* Italy's tile attributes, transcribed from the listing: two hazards that take the ball and hurt the kid,
   and three more ball targets. */
#include "game/attr_italy.h"
#include "game/attr_england.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/player.h"

static int16_t bx(const sk_game *g) { return (int16_t)(g->ball.x >> 16); }
static int16_t by(const sk_game *g) { return (int16_t)(g->ball.y >> 16); }
static bool in_open(int v, int lo, int hi) { return v > lo && v < hi; }   /* the cmpi/ble/bge pairs */
static bool in_half(int v, int lo, int hi) { return v >= lo && v < hi; }  /* cmpi/blt, cmpi/bge */

static void attr_hazard_up(sk_game *g, int cx, int cy) {             /* 0x11BFFE: attribute 63 */
    int x = cx * 16, y = cy * 16;
    const sk_player *p = &g->player;
    if (g->attrs.probe_kind == 1) {
        if (in_open(by(g) - y, -16, 16) && in_open(bx(g) - x, -16, 16) && (int16_t)(g->ball.vy >> 16) < 0) swallow_ball(g);
    } else if (g->attrs.probe_kind == 0) {
        if (in_open(p->x - x, -8, 16) && in_half(p->y - y, 0, 16) && p->vy >= 0) hurt_player(g);
    }
}
static void attr_hazard_down(sk_game *g, int cx, int cy) {           /* 0x11C0A8: attribute 64 */
    int x = cx * 16, y = cy * 16;
    const sk_player *p = &g->player;
    if (g->attrs.probe_kind == 1) {
        if (in_open(by(g) - y, -16, 16) && in_open(bx(g) - x, -16, 16) && (int16_t)(g->ball.vy >> 16) >= 0) swallow_ball(g);
    } else if (g->attrs.probe_kind == 0) {
        if (in_open(p->x - x, -8, 18) && in_half(p->y - y, -40, -24) && p->vy <= 0) hurt_player(g);
    }
}
static void attr_ball_target_69(sk_game *g, int cx, int cy) { ball_target(g, cx, cy, &g->ball_target[2], 4); }   /* 0x11C33C */
static void attr_ball_target_71(sk_game *g, int cx, int cy) { ball_target(g, cx, cy, &g->ball_target[4], 5); }   /* 0x11C360 */
static void attr_ball_target_74(sk_game *g, int cx, int cy) { ball_target(g, cx, cy, &g->ball_target[7], 4); }   /* 0x11C396 */

void attr_italy_install(void) {
    attr_table[63] = attr_hazard_up;
    attr_table[64] = attr_hazard_down;
    attr_table[69] = attr_ball_target_69;
    attr_table[71] = attr_ball_target_71;
    attr_table[74] = attr_ball_target_74;
}
