#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "app/scene.h"
#include "gfx/display_model.h"
#include "gfx/framebuffer.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/objects.h"
#include "game/player.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    return game_setup_level(&g_g, &g_lv, 0);
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static int g_order[64], g_n;
static void probe(sk_game *g, int step) { (void)g; if (g_n < 64) g_order[g_n++] = step; }

void test_frame_step_runs_the_steps_in_the_notes_order(void) {
    CHECK(make_game());
    g_g.step_probe = probe; g_n = 0;
    static sk_sprite kid;
    game_frame_step(&g_g, 0, &kid);
    int expect[] = { 1, 2, 3, 8, 10, 11, 13, 14, 15, 16, 18, 19, 21, 23, 24, 25, 26, 27, 29, 32 };   /* the steps the port runs, gameplay.md table */
    CHECK_EQ(g_n, (int)(sizeof expect / sizeof *expect));
    for (int i = 0; i < g_n && i < (int)(sizeof expect / sizeof *expect); i++) CHECK_EQ(g_order[i], expect[i]);
    CHECK_EQ(g_g.frame_ticks, 1); CHECK_EQ(g_g.frame_counter, 1);
    for (int i = 0; i < 30; i++) game_frame_step(&g_g, 0, &kid);
    CHECK_EQ(g_g.player.y, 12 * 16 - 41);                       /* the kid landed through the real loop */
    CHECK(kid.visible); CHECK_EQ(g_g.camera_x, 0);
    drop_game();
}

void test_timer_counts_down_in_bcd_and_times_out(void) {
    CHECK(make_game());
    g_g.level_timer = 0x10; g_g.timer_rate = 2; g_g.timer_flags = 0x80;
    timer_tick(&g_g); CHECK_EQ(g_g.level_timer, 0x10);
    timer_tick(&g_g); CHECK_EQ(g_g.level_timer, 0x09); CHECK(g_g.timer_flags & 1);   /* BCD */
    for (int i = 0; i < 9 * 2; i++) timer_tick(&g_g);
    CHECK_EQ(g_g.level_timer, 0); CHECK(g_g.timer_flags & 0x80);   /* the stop comes one period later */
    timer_tick(&g_g); timer_tick(&g_g);
    CHECK_EQ(g_g.timer_flags & 0x80, 0); CHECK(!g_g.timeout_armed);   /* a normal level just stops its clock */
    g_g.timeout_armed = true;                                     /* the scoreboard arms it (sub_11EF1C) */
    timeout_check(&g_g);
    CHECK_EQ(g_g.player.state, 68); CHECK(g_g.jingle_requested); CHECK_EQ(g_g.jingle, 2); CHECK(!g_g.timeout_armed);
    drop_game();
}

void test_curtain_opens_and_closes(void) {
    CHECK(make_game());
    CHECK_EQ(g_g.curtain_flags, 4);                              /* setup_level asks for the opening */
    run_curtain(&g_g); CHECK(g_g.curtain_flags & 2); CHECK_EQ(g_g.curtain_step, 0x62 - 2); CHECK_EQ(curtain_rows(&g_g), 98);
    for (int i = 0; i < 60; i++) run_curtain(&g_g);
    CHECK_EQ(g_g.curtain_flags & 0xA, 0); CHECK_EQ(curtain_rows(&g_g), 0);   /* fully open */
    g_g.curtain_flags |= 0x10;                                   /* close request */
    int n = 0;
    for (; n < 80 && !g_g.level_end; n++) run_curtain(&g_g);
    CHECK(g_g.level_end); CHECK(n >= 49 && n <= 52); CHECK(curtain_rows(&g_g) >= 100);
    drop_game();
}

void test_play_level_next_skips_non_tile_levels(void) {
    sk_tables t = {0};
    uint8_t seq[] = { 30, 0, 1, 12, 2, 29, 3, 42 };
    memcpy(t.sequence, seq, sizeof seq);
    CHECK_EQ(play_level_next(&t, 0), 1);                          /* 30 (world map) skipped -> position 1 (level 0) */
    CHECK_EQ(play_level_next(&t, 2), 4);                          /* 12 (scoreboard) skipped */
    CHECK_EQ(play_level_next(&t, 4), 6);                          /* 29 skipped */
    CHECK_EQ(play_level_next(&t, 6), -1);                         /* 42 = end */
    uint8_t usa[] = { 27, 28, 12, 41, 29, 35, 42 };               /* the game's tail: CITY 1, CITY 2, the ALIEN screen, the end */
    memcpy(t.sequence, usa, sizeof usa);
    CHECK_EQ(play_level_next(&t, 1), -1);                         /* 41 (ALIEN, not a tile level) skipped too */
}

void test_level_setup_and_outcome(void) {
    CHECK(make_game());
    CHECK_EQ(g_g.player.lives, 3); CHECK_EQ(g_g.player.score, 0); CHECK_EQ(g_g.level_number, 1);
    CHECK_EQ(g_g.level_outcome, 0); CHECK_EQ(g_g.timer_rate, 0x5DC); CHECK_EQ(g_g.level_timer, 0x45); CHECK_EQ(g_g.cam_ease, 4);
    CHECK(!(g_g.timer_flags & 0x80)); CHECK_EQ(g_g.player.top_speed, 16); CHECK(g_g.view.full_redraw);   /* KICK OFF starts the clock */
    CHECK_EQ(game_outcome(&g_g), SK_OUTCOME_RUNNING);
    g_g.level_outcome = 0x81; g_g.level_end = true;
    CHECK_EQ(game_outcome(&g_g), SK_OUTCOME_NEXT_LEVEL);
    g_g.level_outcome = 0; g_g.player.state = 64; g_g.player.lives = -1;   /* the last life lost (0x10BBCA) */
    CHECK_EQ(game_outcome(&g_g), SK_OUTCOME_GAME_OVER);
    g_g.player.lives = 3;
    CHECK(game_setup_level(&g_g, &g_lv, 0));                     /* a second setup frees the first world */
    CHECK_EQ(g_g.player.state, 68); CHECK_EQ(g_g.player.lives, 3);   /* KICK OFF holds the kid (type 105) */
    drop_game();
}

void test_timer_restarts_after_a_respawn(void) {                /* 5a #6: the jingle object starts the clock */
    CHECK(make_game());
    static sk_sprite kid;
    CHECK(!(g_g.timer_flags & 0x80));
    for (int i = 0; i < 400 && !(g_g.timer_flags & 0x80); i++) game_frame_step(&g_g, 0, &kid);
    CHECK(g_g.timer_flags & 0x80); CHECK_EQ(g_g.player.state, 0);
    init_player(&g_g);                                           /* a respawn */
    CHECK(!(g_g.timer_flags & 0x80));
    for (int i = 0; i < 400 && !(g_g.timer_flags & 0x80); i++) game_frame_step(&g_g, 0, &kid);
    CHECK(g_g.timer_flags & 0x80);
    drop_game();
}

void test_tile_animation_groups_step(void) {                    /* sub_1094EA / sub_1095CE */
    CHECK(make_game());
    memset(g_g.tile_anim, 0, sizeof g_g.tile_anim);
    g_g.anim_phase = 0; memset(g_g.anim_wait, 0, sizeof g_g.anim_wait);
    uint16_t seen2[13];
    for (int i = 0; i < 13; i++) { seen2[i] = g_g.tile_anim[2]; tile_anim_step(&g_g); }
    CHECK_EQ(seen2[0], 0);
    CHECK_EQ(seen2[4], 0x80); CHECK_EQ(seen2[8], 0x100); CHECK_EQ(seen2[12], 0);   /* group 0 every 4th frame (phase 0 first at the 4th), 3 frames */
    CHECK_EQ(g_g.tile_anim[128 + 2], (uint16_t)(0x100 - g_g.tile_anim[2]));      /* the reversed half */
    memset(g_g.tile_anim, 0, sizeof g_g.tile_anim); g_g.anim_phase = 0; memset(g_g.anim_wait, 0, sizeof g_g.anim_wait);
    int changes = 0; uint16_t prev = 0;
    for (int i = 0; i < 64; i++) { tile_anim_step(&g_g); if (g_g.tile_anim[16 + 3] != prev) { changes++; prev = g_g.tile_anim[16 + 3]; } }
    CHECK_EQ(changes, 8);                                            /* group 1 every 8 frames */
    drop_game();
}

void test_animated_cells_draw_the_next_tile(void) {
    CHECK(make_game());
    static sk_fb fb; CHECK(fb_init(&fb, SK_FB_W, SK_FB_H));
    uint16_t tiles[40 * 16] = {0}; uint8_t states[40 * 16] = {0};
    tiles[2 * 40 + 3] = 0x1FE;                                       /* an empty image; its next frame 0x1FF is the striped start tile */
    states[2 * 40 + 3] = 2;
    uint16_t anim[256] = {0};
    int x = 3 * 16 + 16 + 4;                                         /* buffer column of world x 52 (+16): a lit stripe pixel of 0x1FF */
    fb_clear(&fb, 0);
    scene_draw_tiles_anim(&g_lv, &fb, tiles, states, anim, 0, 0);
    CHECK_EQ(fb.px[(2 * 16 + 2) * fb.w + x] & 15, 0);              /* frame 0: the empty tile */
    anim[2] = 0x80;
    fb_clear(&fb, 0);
    scene_draw_tiles_anim(&g_lv, &fb, tiles, states, anim, 0, 0);
    CHECK(fb.px[(2 * 16 + 2) * fb.w + x] & 15);                    /* frame 1: the next tile */
    fb_free(&fb);
    drop_game();
}
