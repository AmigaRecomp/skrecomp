#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "app/scene.h"
#include "editor/names.h"
#include "editor/view.h"
#include "game/world.h"
#include "gfx/display_model.h"

void test_edit_names(void) {
    CHECK(edit_type_name(58) && strcmp(edit_type_name(58), "Rat") == 0);
    CHECK(edit_attr_name(13) && strcmp(edit_attr_name(13), "Exit") == 0);
    CHECK(edit_type_name(127) == NULL); CHECK(edit_type_name(-1) == NULL); CHECK(edit_attr_name(95) == NULL);
    sk_level lv = {0}; uint8_t pals[200] = {0}; char t[64];
    lv.pals = pals; lv.pals_len = sizeof pals;
    pals[SK_PALS_TYPE + 3] = 58; pals[SK_PALS_PARAM + 3] = 4; pals[SK_PALS_TYPE + 4] = 126;
    pals[SK_PALS_ATTR + 40 - 33] = 13; pals[SK_PALS_ATTR + 41 - 33] = 94;
    edit_flag_label(&lv, 3, t, sizeof t); CHECK(strcmp(t, "Rat (58), param 4") == 0);
    edit_flag_label(&lv, 4, t, sizeof t); CHECK(strcmp(t, "type 126, param 0") == 0);
    edit_flag_label(&lv, 40, t, sizeof t); CHECK(strcmp(t, "Exit") == 0);
    edit_flag_label(&lv, 41, t, sizeof t); CHECK(strcmp(t, "attribute 94") == 0);
    edit_flag_label(&lv, 0, t, sizeof t); CHECK(strcmp(t, "nothing") == 0);
}

void test_edit_render_matches_the_viewer(void) {          /* ROM-backed: LONDON1's own map, the same camera */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 4, .scale = 1, .section = -1, .cam_x = 320, .cam_y = 64 };
    static sk_scene s; sk_err e = {0};
    CHECK(scene_open(&s, &a, &e));
    scene_set_camera(&s, 320, 64);
    scene_draw(&s);
    static uint32_t screen[SK_SCREEN_W * SK_SCREEN_H], view[SK_VIEW_W * SK_VIEW_H];
    scene_present(&s, screen);
    sk_custom c; CHECK(custom_alloc(&c, 4, s.level.map.width, s.level.map.height, &e));
    memcpy(c.cells, s.level.map.cells, (size_t)c.w * c.h * 2);
    CHECK(edit_render(&s.level, &s.palette, &c, s.cam_x + 16, s.cam_y, SK_VIEW_W, SK_VIEW_H, view));   /* the first visible pixel */
    int diffs = 0;
    for (int y = 0; y < SK_VIEW_H; y++)
        for (int x = 0; x < SK_VIEW_W; x++) diffs += view[y * SK_VIEW_W + x] != screen[(SK_PLAY_Y + y) * SK_SCREEN_W + SK_PLAY_X + x];
    CHECK_EQ(diffs, 0);
    uint32_t *big = malloc(900 * 500 * 4);
    CHECK(edit_render(&s.level, &s.palette, &c, 0, 0, 900, 500, big));   /* a window wider than the screen */
    free(big);
    custom_free(&c); scene_close(&s);
}
