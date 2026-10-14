#include "game/between.h"

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
