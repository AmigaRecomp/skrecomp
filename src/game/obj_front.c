#include <string.h>
#include "game/attrs.h"
#include "game/game.h"
#include "game/obj_front.h"
#include "game/obj_helpers.h"

#define BIG_FONT 0x207A00u

/* ---- type 102: the DEMO banner, 64 x 13 at (0x70, 0xB4) of the view ---- */
static void obj_demo_banner_init(sk_game *g, sk_slot *s, int x, int y) {        /* 0x112374 */
    init_object_slot_nodraw(g, s, x, y, s->type4);
    s->x0 = 0x70; s->y0 = 0xB4; s->vy = 0; s->vx = 0; s->t0 = 0x28;
    memset(g->demo_banner, 0, sizeof g->demo_banner);
    static const char text[4] = { 'D', 'E', 'M', 'O' };          /* 0x112410 */
    for (int k = 0; k < 4 && g->sk; k++)                         /* glyph c - 0x40: planes 0..2 from words 1..3, the mask ~word 0 */
        for (int r = 0; r < 13; r++) {
            uint32_t a = BIG_FONT + (uint32_t)(text[k] - 0x40) * 0x68u + (uint32_t)r * 8u;
            uint8_t *row = g->demo_banner + r * 8 + k * 2;
            for (int p = 0; p < 3; p++) { uint16_t w = image_u16(g->sk, a + 2u + 2u * (uint32_t)p); row[p * 0x68] = (uint8_t)(w >> 8); row[p * 0x68 + 1] = (uint8_t)w; }
            uint16_t m = (uint16_t)~image_u16(g->sk, a);
            row[4 * 0x68] = (uint8_t)(m >> 8); row[4 * 0x68 + 1] = (uint8_t)m;
        }
    frames_wh(g, s, 64, 13, g->demo_banner, 1);                  /* sub_107EDE */
    if (s->draw) s->draw->dflags |= 8;
}
static void obj_demo_banner_update(sk_game *g, sk_slot *s, int index) {         /* 0x112416 */
    (void)index;
    sk_draw *d = slot_draw_record(g, s);
    if (!d) return;
    d->x = s->x0; d->y = s->y0;
}

/* ---- type 86: the tutorial, joystick moves shown as tiles round a stick picture ---- */
static const struct { uint8_t step, flag; uint16_t wait; } tutorial_script[42] = {                         /* 0x11A254 */
    { 3, 0, 70 }, { 8, 0, 40 }, { 2, 0, 100 }, { 8, 0, 40 }, { 3, 0, 50 }, { 8, 0, 50 }, { 0, 0, 80 }, { 8, 0, 50 },
    { 5, 0, 50 }, { 8, 0, 50 }, { 4, 0, 50 }, { 8, 0, 50 }, { 1, 0, 80 }, { 8, 0, 50 }, { 6, 0, 100 }, { 8, 0, 40 },
    { 7, 0, 100 }, { 8, 0, 50 }, { 2, 1, 50 }, { 8, 0, 80 }, { 3, 1, 50 }, { 8, 0, 100 }, { 8, 1, 50 }, { 8, 0, 50 },
    { 0, 0, 50 }, { 8, 0, 100 }, { 8, 1, 40 }, { 5, 0, 50 }, { 8, 0, 120 }, { 8, 1, 50 }, { 3, 0, 50 }, { 8, 0, 100 },
    { 8, 1, 50 }, { 2, 1, 140 }, { 1, 1, 140 }, { 8, 0, 140 }, { 8, 1, 50 }, { 0, 1, 100 }, { 8, 0, 50 }, { 3, 0, 100 },
    { 8, 0, 150 }, { 9, 0, 65535 } };
static const int16_t tutorial_at[13][2] = {                                                                  /* 0x11A11C */
    { 128, 160 }, { 128, 176 }, { 128, 192 }, { 128, 208 }, { 144, 160 }, { 144, 176 }, { 144, 208 }, { 160, 160 },
    { 160, 208 }, { 176, 160 }, { 176, 176 }, { 176, 192 }, { 176, 208 } };
static const uint8_t tutorial_tiles[10][13] = {                                                               /* 0x11A150 */
    { 0, 0, 0, 0, 17, 12, 0, 18, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 12, 19, 0, 20, 0, 0, 0, 0 },
    { 0, 21, 22, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 23, 24, 0 },
    { 16, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 12, 0, 0, 0, 13, 0, 0, 0 },
    { 0, 0, 0, 15, 0, 12, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 14 },
    { 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0 } };

static void obj_tutorial_init(sk_game *g, sk_slot *s, int x, int y) {           /* 0x11A072 */
    init_object_slot_invisible(g, s, x, y, s->type4);
    s->t0 = 0; s->t1 = 200;
    set_tile_at(g, 0x90, 0xB0, 12, 0);
}
static void obj_tutorial_update(sk_game *g, sk_slot *s, int index) {            /* 0x11A098 */
    (void)index;
    if (--s->t1 != 0) return;
    int i = s->t0 < 42 ? s->t0 : 41;
    int step = tutorial_script[i].step;
    s->t1 = (int16_t)tutorial_script[i].wait;
    if (step == 9 && g->curtain_flags == 0) g->curtain_flags |= 0x10;
    for (int k = 0; k < 13; k++) set_tile_at(g, tutorial_at[k][0], tutorial_at[k][1], tutorial_tiles[step][k], 0);
    if (tutorial_script[i].flag) set_tile_at(g, 0x90, 0xB0, 11, 0);
    s->t0++;
}

void obj_front_install(void) {
    object_init_table[102] = obj_demo_banner_init;     object_update_table[102] = obj_demo_banner_update;
    object_init_table[86] = obj_tutorial_init;         object_update_table[86] = obj_tutorial_update;
}
