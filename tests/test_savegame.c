#include <string.h>
#include "check.h"
#include "game/savegame.h"

void test_savegame_slot_round_trips_through_the_xor_and_checksum(void) {
    uint8_t file[SK_SAVE_SIZE];
    memset(file, 0, sizeof file);
    sk_save_slot in = { "SOCCER KID", 0x0012340, 21, 3, 0x05, 4, 10, 1 };
    savegame_put(file, 2, &in);
    CHECK_EQ(file[0], 1 << 2);
    CHECK(memcmp(file + 2 + 64, "SOCCER", 6) != 0);              /* stored XORed */
    sk_save_slot out;
    CHECK(savegame_get(file, sizeof file, 2, &out));
    CHECK(strcmp(out.name, "SOCCER KID") == 0);
    CHECK_EQ(out.score, 0x0012340u); CHECK_EQ(out.level_number, 21); CHECK_EQ(out.lives, 3);
    CHECK_EQ(out.cup_pieces, 5); CHECK_EQ(out.shirt, 4); CHECK_EQ(out.shorts, 10); CHECK_EQ(out.extra_heart, 1);
    file[2 + 64 + 5] ^= 1;                                       /* a damaged byte: the checksum fails */
    CHECK(!savegame_get(file, sizeof file, 2, &out));
    CHECK(!savegame_get(file, sizeof file, 3, &out));            /* an empty slot */
    CHECK(!savegame_get(file, 40, 2, &out));                     /* a short file */
}

void test_savegame_names_show_empty_slots(void) {               /* load_save_names 0x10340A: "N. name" rows */
    uint8_t file[SK_SAVE_SIZE];
    memset(file, 0, sizeof file);
    sk_save_slot in = { "KEVIN", 0x100, 4, 2, 0, 0, 0, 0 };
    savegame_put(file, 0, &in);
    char rows[5][24];
    savegame_names(file, sizeof file, rows);
    CHECK(strcmp(rows[0], "1. KEVIN           ") == 0);
    CHECK(strcmp(rows[1], "2. EMPTY           ") == 0);         /* dat_10349E, 16 wide */
    savegame_names(NULL, 0, rows);                               /* no file at all */
    CHECK(strcmp(rows[4], "5. EMPTY           ") == 0);
}

void test_savegame_rejects_a_level_past_the_sequence(void) {
    uint8_t file[SK_SAVE_SIZE];
    memset(file, 0, sizeof file);
    sk_save_slot in = { "X", 0, 58, 3, 0, 0, 0, 0 };
    savegame_put(file, 0, &in);
    sk_save_slot out;
    CHECK(!savegame_get(file, sizeof file, 0, &out));
}
