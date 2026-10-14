#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_bonus.h"
#include "game/obj_helpers.h"
#include "game/resources.h"
#include "audio/sound.h"

static const uint8_t *res(const sk_game *g, int slot) { return g->res ? resource_slot(g->res, slot, NULL) : NULL; }
static bool need(sk_game *g, sk_slot *s, const uint8_t *f) { if (f) return true; free_object(g, s); return false; }
static int state_of(const sk_slot *s) { return aux_d(s); }                  /* aux.lo, byte +0xD */
static void set_state(sk_slot *s, int v) { set_aux_d(s, v); }

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

void obj_bonus_install(void) {
    object_init_table[81] = obj_cup_piece_init;        object_update_table[81] = obj_cup_piece_update;
}
