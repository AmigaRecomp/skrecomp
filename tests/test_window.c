#include <stdio.h>
#include <stdlib.h>
#include "check.h"
#include "app/args.h"
#include "app/scene.h"
#include "app/window.h"

void test_window_runs_headless_frames(void) {         /* ROM-backed; SDL's dummy video driver */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    setenv("SDL_VIDEO_DRIVER", "dummy", 1);
    sk_args a = { .roms = roms, .level = 0, .scale = 1, .section = -1, .exit_after = 3 };
    sk_scene s = {0};
    sk_err e = {0};
    CHECK(scene_open(&s, &a, &e));
    sk_window w = {0};
    CHECK(window_open(&w, 1, &e));
    if (!SK_OK(&e)) printf("window_open: %s\n", e.msg);
    CHECK_EQ(window_run(&w, &s, 3), 0);
    CHECK_EQ(w.frames_presented, 3);
    window_close(&w);
    scene_close(&s);
}

void test_window_camera_step_clamps(void) {
    sk_level lv = {0};
    lv.map.width = 30;                                /* 480 px wide: max camera x = 128 */
    lv.map.height = 20;                               /* 320 px tall: max camera y = 120 */
    sk_scene s = {0};
    s.level = lv;
    s.has_level = true;
    s.max_cam_x = 128;
    s.max_cam_y = 120;
    window_step_camera(&s, 1, 0, 100);                /* right by 100 px */
    CHECK_EQ(s.cam_x, 100);
    window_step_camera(&s, 1, 0, 100);
    CHECK_EQ(s.cam_x, 128);
    window_step_camera(&s, 0, -1, 500);
    CHECK_EQ(s.cam_y, 0);
    window_step_camera(&s, 0, 1, 121);
    CHECK_EQ(s.cam_y, 120);
}

void test_window_fixed_step_accumulator(void) {
    uint64_t acc = 0;
    CHECK_EQ(window_steps_for(&acc, 50000000ull), 2);     /* 50 ms -> two 20 ms steps, 10 ms left */
    CHECK_EQ(acc, 10000000ull);
    CHECK_EQ(window_steps_for(&acc, 5000000ull), 0);      /* 15 ms: not yet a step */
    CHECK_EQ(acc, 15000000ull);
    CHECK_EQ(window_steps_for(&acc, 1000000000ull), 5);   /* a one-second stall: capped at 5 steps */
    CHECK(acc <= 20000000ull);                            /* and the backlog is dropped */
}
