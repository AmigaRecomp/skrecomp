#include <string.h>
#include "check.h"
#include "game/between.h"

void test_play_level_entry_runs_maps_and_endings_on_their_own_slot(void) {
    sk_tables t = {0};
    uint8_t seq[] = { 30, 0, 1, 12, 31, 6 };
    memcpy(t.sequence, seq, sizeof seq);
    int ln = 0;
    sk_run r = play_level_entry(&t, &ln);                        /* world_map: level_number += 1 inside */
    CHECK_EQ(r.kind, SK_RUN_MAP); CHECK_EQ(r.map_index, 0); CHECK_EQ(ln, 1);
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_LEVEL); CHECK_EQ(r.data_index, 0); CHECK_EQ(ln, 1);   /* a level moves on by its exit walk */
    ln = 3;
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_LEVEL); CHECK_EQ(r.data_index, 12);  /* the scoreboard is a tile level */
    ln = 4;
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_MAP); CHECK_EQ(r.map_index, 1); CHECK_EQ(ln, 5);
}

void test_play_level_entry_walks_the_games_tail(void) {
    sk_tables t = {0};
    uint8_t seq[] = { 28, 12, 41, 29, 35, 42, 36 };
    memcpy(t.sequence, seq, sizeof seq);
    int ln = 2;
    sk_run r = play_level_entry(&t, &ln);                        /* ALIEN is 8b: its slot is passed over (ruling) */
    CHECK_EQ(r.kind, SK_RUN_ENDING); CHECK_EQ(ln, 4);            /* end_screens: level_number += 1 */
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_MAP); CHECK_EQ(r.map_index, 5); CHECK_EQ(ln, 5);
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_GAME_END); CHECK_EQ(ln, 5);          /* 42: play_level returns */
    ln = SK_LEVEL_SEQUENCE_LEN;                                  /* past the table: the end, no read */
    r = play_level_entry(&t, &ln);
    CHECK_EQ(r.kind, SK_RUN_GAME_END);
}
