/* The USA's object types, transcribed from the listing. */
#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_helpers.h"
#include "game/obj_usa.h"
#include "game/resources.h"
#include "game/world.h"

static const uint8_t hedgehog_walk_script[] = { 0, 4, 0, 6, 0, 1, 2, 3, 4, 5, 6, 7 };         /* 0x119A98 */

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

void obj_usa_install(void) {
    object_init_table[3] = obj_hedgehog_init;          object_update_table[3] = obj_hedgehog_update;
}
