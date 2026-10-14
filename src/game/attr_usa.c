/* The USA's tile attributes, transcribed from the listing (objects.md "USA attributes"). */
#include "game/attr_usa.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/objects.h"
#include "game/player.h"

static bool in_open(int v, int lo, int hi) { return v > lo && v < hi; }   /* the cmpi/ble/bge pairs */

/* ---- attribute 3: a ladder: a kid standing at it takes hold (state 7, or 6 with his feet 4 px or more up it) ---- */
static void attr_ladder(sk_game *g, int cx, int cy) {                 /* 0x11B2D6 */
    if (g->attrs.probe_kind != 0) return;
    sk_player *p = &g->player;
    int x = cx * 16, y = cy * 16;
    if (!in_open((int16_t)(p->x - x), -8, 18)) return;
    int d2 = (int16_t)(p->y + 0x28 - y);
    if (d2 <= 0 || p->state != 0) return;
    p->no_push_up = 0xFF;
    p->state = 0x1C;
    if ((uint16_t)d2 >= 4) p->state = 0x18;
}

/* ---- attribute 4: spikes ---- */
static void attr_spikes(sk_game *g, int cx, int cy) {                 /* 0x11B1E2 */
    if (g->attrs.probe_kind != 0) return;
    const sk_player *p = &g->player;
    if (in_open((int16_t)(p->x - cx * 16), -8, 18) && in_open((int16_t)(p->y - cy * 16), -40, 16)) hurt_player(g);
}

/* ---- attribute 36: a basketball hoop: the ball dropping through scores 10; the eighth basket gives a life and ends it ---- */
static void attr_hoop(sk_game *g, int cx, int cy) {                   /* 0x11B630 */
    if (g->attrs.probe_kind != 1) return;
    sk_player *p = &g->player;
    const sk_ball *b = &g->ball;
    if (!p->ball_free || g->hoop_count < 0) return;
    int x = cx * 16, y = cy * 16;
    if ((int16_t)(b->prev_y + 4 - y) >= 0) return;                  /* it was above the rim */
    if ((int16_t)((int16_t)(b->y >> 16) + 4 - y) < 0) return;       /* and is through it now */
    int dx = (int16_t)((int16_t)(b->x >> 16) - x);
    if (dx < -4 || dx > 4) return;
    if ((int16_t)(b->vy >> 16) < 0) return;                         /* falling */
    int sfx = 1;
    if (++g->hoop_count >= 8) {
        g->hoop_count = (int16_t)(0xFF00 | (g->hoop_count & 0xFF));   /* st.b on the high byte: no more */
        p->lives++;
        g->hud_redraw |= 2;
        sfx = 9;
    }
    play_sfx(g, sfx);
    add_score(g, 0x10);
}

void attr_usa_install(void) {
    attr_table[3] = attr_ladder;
    attr_table[4] = attr_spikes;
    attr_table[36] = attr_hoop;
}
