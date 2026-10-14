#include <string.h>
#include "check.h"
#include "app/savescreen.h"
#include "game/input.h"

static sk_savescreen ss;
static uint8_t pic[16960];
static char rows[5][24] = { "1. EMPTY           ", "2. EMPTY           ", "3. EMPTY           ", "4. EMPTY           ", "5. EMPTY           " };

static int step(uint8_t in, int key) { return savescreen_step(&ss, in, key, false); }

void test_save_prompt_cross_leaves_without_saving(void) {      /* save_game_screen 0x11DD80 */
    sk_err e = {0};
    CHECK(savescreen_open(&ss, SK_SAVE_ASK, NULL, 0, pic, sizeof pic, rows, &e));
    CHECK_EQ(ss.tick, 0);                                        /* the cross is highlighted */
    CHECK_EQ(step(SK_IN_FIRE, 0), SK_SAVE_RESULT_NONE);          /* fire on the cross: at once, nothing saved */
}

void test_save_prompt_saves_a_typed_name_in_a_slot(void) {
    sk_err e = {0};
    CHECK(savescreen_open(&ss, SK_SAVE_ASK, NULL, 0, pic, sizeof pic, rows, &e));
    step(SK_IN_LEFT, 0); CHECK_EQ(ss.tick, 1);
    CHECK_EQ(step(SK_IN_FIRE, 0), SK_SAVE_RESULT_STAY);
    step(SK_IN_FIRE, 0);                                         /* waits for the release */
    CHECK_EQ(ss.mode, SK_SAVE_ASK);
    step(0, 0); CHECK_EQ(ss.mode, SK_SAVE_LIST);
    step(0, 0); step(SK_IN_DOWN, 0); CHECK_EQ(ss.slot, 1);
    step(0, 0); step(SK_IN_UP, 0); step(0, 0); step(SK_IN_UP, 0); CHECK_EQ(ss.slot, 4);   /* wraps */
    step(0, 0); step(SK_IN_FIRE, 0); CHECK_EQ(ss.mode, SK_SAVE_TYPE);
    CHECK(strcmp(ss.rows[4], "5.                 ") == 0);       /* the row blanked for the name */
    step(0, 'A'); step(0, 'B'); step(0, 8); step(0, 'C');
    CHECK(strncmp(ss.rows[4] + 3, "AC ", 3) == 0);
    for (int i = 0; i < 20; i++) step(0, 'Z');                   /* sixteen at most */
    CHECK_EQ(ss.typed, 16);
    for (int i = 0; i < 14; i++) step(0, 8);
    int r = step(0, 0x0D), n = 0;                                /* Return: SAVING GAME, then the result */
    while (r == SK_SAVE_RESULT_STAY && n < 200) { r = step(0, 0); n++; }
    CHECK_EQ(r, SK_SAVE_RESULT_SAVE); CHECK_EQ(ss.slot, 4);
    CHECK(strcmp(ss.name, "AC") == 0);
}

void test_resume_list_escape_cancels_and_fire_picks(void) {     /* resume_game_screen 0x11E1EA */
    sk_err e = {0};
    CHECK(savescreen_open(&ss, SK_SAVE_RESUME, NULL, 0, pic, sizeof pic, rows, &e));
    CHECK_EQ(ss.mode, SK_SAVE_LIST);
    CHECK_EQ(savescreen_step(&ss, 0, 0, true), SK_SAVE_RESULT_NONE);
    CHECK(savescreen_open(&ss, SK_SAVE_RESUME, NULL, 0, pic, sizeof pic, rows, &e));
    step(0, 0); step(SK_IN_DOWN, 0); step(0, 0); step(SK_IN_DOWN, 0);
    CHECK_EQ(step(SK_IN_FIRE, 0), SK_SAVE_RESULT_LOAD); CHECK_EQ(ss.slot, 2);
}
