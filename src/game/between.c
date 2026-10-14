#include "game/between.h"
#include "game/ball.h"
#include "game/platforms.h"

sk_run play_level_entry(const sk_tables *t, int *level_number) {   /* play_level 0x104060 (bonus_check: 8b) */
    for (;;) {
        sk_run r = { SK_RUN_GAME_END, 42, 0 };
        if (*level_number < 0 || *level_number >= SK_LEVEL_SEQUENCE_LEN) return r;
        int d = t->sequence[*level_number];
        r.data_index = d;
        if (d == 42) return r;                                   /* 0x1040B4: the game is over */
        if (d == 41) { ++*level_number; continue; }              /* ALIEN: type 82 is 8b (ruling: passed over) */
        if (d == 29) { ++*level_number; r.kind = SK_RUN_ENDING; return r; }               /* end_screens 0x11EFAC */
        if (d >= 30 && d < 36) { ++*level_number; r.kind = SK_RUN_MAP; r.map_index = d - 30; return r; }   /* world_map 0x11E45C */
        r.kind = SK_RUN_LEVEL;
        return r;
    }
}

void flight_init(sk_flight *f, int from_x, int from_y, int to_x, int to_y) {   /* plane_flight 0x11E778 */
    f->x = (int32_t)((uint32_t)from_x << 16);
    f->y = (int32_t)((uint32_t)from_y << 16);
    f->tx = to_x;
    int dx = to_x - from_x;
    if (dx < 0) dx += 320;                                       /* bpl / addi #$140: always east */
    int a = atan2_256(dx, to_y - from_y);
    f->big_off = 0x2AC; f->small_off = 0;                        /* level */
    if ((int8_t)a >= 0x17) { f->big_off = 0; f->small_off = 0x1D4; }        /* heading down */
    if ((int8_t)a <= -0x17) { f->big_off = 0x558; f->small_off = 0xEA; }    /* heading up */
    f->sx = (int32_t)((uint32_t)cos_1024(a * 4) << 8);          /* +-256 << 8: +-1.0 px a frame */
    f->sy = (int32_t)((uint32_t)sin_1024(a * 4) << 8);
}

bool flight_step(sk_flight *f) {                                 /* loc_11E842 */
    f->x += f->sx;
    f->y += f->sy;
    int x = (int16_t)(f->x >> 16);
    return x == f->tx || x - 320 == f->tx;
}

int flight_draw_x(const sk_flight *f) {
    int x = (int16_t)(f->x >> 16);
    return x >= 320 ? x - 320 : x;
}
