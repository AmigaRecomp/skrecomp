/* England country and London's tile attributes, transcribed from the listing (objects.md). */
#include "game/attr_country_london.h"
#include "game/attrs.h"
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

void attr_country_london_install(void) {
    attr_table[23] = attr_platform_reverse;
}
