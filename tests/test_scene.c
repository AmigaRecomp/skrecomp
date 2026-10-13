#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "check.h"
#include "fixture.h"
#include "app/args.h"
#include "app/scene.h"
#include "core/zipfile.h"
#include "gfx/display_model.h"
#include "gfx/palette.h"
#include "gfx/png.h"

void test_scene_draw_level_on_fixture(void) {
    uint8_t *d = NULL;
    size_t n = 0;
    sk_level lv = {0};
    sk_err e = {0};
    CHECK(load_fixture("level_tiny.bin", &d, &n));
    CHECK(level_parse(d, n, &lv, &e));
    sk_fb fb;
    CHECK(fb_init(&fb, SK_FB_W, SK_FB_H));
    /* foreground only: both cells use all-0xFF tiles -> value 15 in group A for x < 32 */
    scene_draw_level(&lv, &fb, 0, 0, false, NULL);
    CHECK_EQ(fb.px[0], 0x0F);
    CHECK_EQ(fb.px[16], 0x0F);
    CHECK_EQ(fb.px[15 * SK_FB_W + 31], 0x0F);
    CHECK_EQ(fb.px[32], 0x00);
    CHECK_EQ(fb.px[16 * SK_FB_W], 0x00);
    /* with the backdrop: cells 0 and 1 are out of range (one tile), cell 2 is tile 0 (0x0F bytes) */
    scene_draw_level(&lv, &fb, 0, 0, true, NULL);
    CHECK_EQ(fb.px[0], 0x0F);
    CHECK_EQ(fb.px[36], 0xF0);
    CHECK_EQ(fb.px[32], 0x00);
    CHECK_EQ(fb.px[16 * SK_FB_W + 36], 0xF0);
    /* camera offset: foreground cell 1 (x 16..31) lands at buffer x 6..21 for cam_x = 10 */
    scene_draw_level(&lv, &fb, 10, 0, false, NULL);
    CHECK_EQ(fb.px[21] & 0x0F, 0x0F);
    CHECK_EQ(fb.px[22] & 0x0F, 0x00);
    fb_free(&fb);
    level_free(&lv);
    free(d);
}

void test_scene_camera_clamps(void) {
    sk_level lv = {0};
    lv.map.width = 546;
    lv.map.height = 30;
    int x = -5, y = 999;
    scene_clamp_camera(&lv, &x, &y);
    CHECK_EQ(x, 0);
    CHECK_EQ(y, 30 * 16 - SK_CAM_MARGIN_H);
    x = 100000; y = -1;
    scene_clamp_camera(&lv, &x, &y);
    CHECK_EQ(x, 546 * 16 - SK_CAM_MARGIN_W);
    CHECK_EQ(y, 0);
    lv.map.width = 10;                      /* narrower than the buffer: never negative */
    lv.map.height = 5;
    x = 50; y = 50;
    scene_clamp_camera(&lv, &x, &y);
    CHECK_EQ(x, 0);
    CHECK_EQ(y, 0);
}

void test_scene_real_level0_screenshot(void) {       /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 3, .section = -1 };
    sk_scene s = {0};
    sk_err e = {0};
    CHECK(scene_open(&s, &a, &e));
    if (!SK_OK(&e)) printf("scene_open: %s\n", e.msg);
    CHECK_EQ(s.level.map.width, 546);
    CHECK(s.has_frames && s.frames.count > 0);        /* the record's first graphics set is shown */
    CHECK_EQ(s.max_cam_x, 546 * 16 - SK_CAM_MARGIN_W);
    scene_draw(&s);
    uint32_t *out = calloc(SK_SCREEN_W * SK_SCREEN_H, sizeof *out);
    scene_present(&s, out);
    CHECK_EQ(out[0], 0);                               /* border */
    CHECK_EQ(out[SK_PLAY_X], rgb12_to_xrgb(0x0BF));    /* backdrop tile 0 = colour 1 -> sky word 0 */
    char path[] = "/tmp/sk_shot_XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0);
    CHECK(png_write(path, SK_SCREEN_W, SK_SCREEN_H, out, &e));
    uint8_t *d = NULL;
    size_t n = 0;
    CHECK(file_read_all(path, &d, &n, &e));
    CHECK(n > 2000);                                   /* a 320x256 scene compresses to a few KB */
    free(d);
    close(fd);
    remove(path);
    free(out);
    /* --section with an explicit size and a bad one */
    CHECK(scene_load_section(&s, "file031", 4, 32, 36, &e));
    CHECK_EQ(s.frames.count, 12);
    CHECK(!scene_load_section(&s, "file031", 4, 32, 37, &e));
    CHECK_STR(e.msg, "whole number");
    CHECK(!scene_load_section(&s, "file031", 9, 0, 0, &e));
    CHECK_STR(e.msg, "section 9");
    CHECK(!scene_load_level(&s, 42, &e));
    CHECK_STR(e.msg, "42");
    scene_close(&s);
}

void test_bobs_estimate_real_sheets(void) {           /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 3, .section = -1 };
    sk_scene s = {0};
    sk_err e = {0};
    CHECK(scene_open(&s, &a, &e));
    CHECK(scene_load_section(&s, "file031", 0, 0, 0, &e));    /* estimated: 32x30 x 11 */
    CHECK_EQ(s.frames.width, 32);
    CHECK_EQ(s.frames.height, 30);
    CHECK_EQ(s.frames.count, 11);
    CHECK(scene_load_level(&s, 17, &e));                       /* a disk-3 level */
    CHECK(scene_load_section(&s, "file036", 0, 0, 0, &e));    /* the kid: 48x64 x 24 */
    CHECK_EQ(s.frames.width, 48);
    CHECK_EQ(s.frames.height, 64);
    CHECK_EQ(s.frames.count, 24);
    scene_close(&s);
}
