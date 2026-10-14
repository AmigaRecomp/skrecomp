#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "check.h"
#include "fixture.h"
#include "app/play.h"
#include "game/level_flow.h"
#include "gfx/display_model.h"
#include "gfx/png.h"

static uint8_t *g_pd; static sk_play g_p;
static bool make_play(void) {                                    /* no disks: the scene holds only the level */
    size_t n = 0; sk_err e = {0};
    memset(&g_p, 0, sizeof g_p);
    if (!load_fixture("level_play.bin", &g_pd, &n) || !level_parse(g_pd, n, &g_p.scene.level, &e)) return false;
    g_p.scene.has_level = true;
    g_p.scene.backdrop = true;
    if (!fb_init(&g_p.scene.fb, SK_FB_W, SK_FB_H)) return false;
    palette_from_tilesets(&g_p.scene.palette, &g_p.scene.level.tiles_a, &g_p.scene.level.tiles_b);
    g_p.game.sk = NULL; g_p.game.tables = NULL;
    g_p.level_pos = -1;
    game_new_game(&g_p.game);
    return game_setup_level(&g_p.game, &g_p.scene.level, 0);
}
static void drop_play(void) { game_close(&g_p.game); fb_free(&g_p.scene.fb); level_free(&g_p.scene.level); free(g_pd); }

void test_play_script_runs_headless_on_the_fixture(void) {
    CHECK(make_play());
    g_p.script = g_p.script_cursor = ".170,R60,RU20,.60";           /* KICK OFF holds the kid for about 165 frames */
    for (int i = 0; i < 311; i++) play_step(&g_p, 0);        /* 310 scripted frames, then the script is spent */
    CHECK(g_p.game.player.x > 3 * 16 + 100); CHECK_EQ(g_p.game.player.state, 0); CHECK_EQ(g_p.game.player.jumping, 0);
    CHECK(g_p.script_done);
    play_draw(&g_p);
    static uint32_t out[SK_SCREEN_W * SK_SCREEN_H];
    play_present(&g_p, out);
    int floor_line = 12 * 16 + 2 - g_p.game.camera_y;              /* a floor pixel row in the window */
    CHECK(out[SK_PLAY_X + 100 + (SK_PLAY_Y + floor_line) * SK_SCREEN_W] != 0);
    drop_play();
}

void test_play_script_survives_game_over(void) {                   /* Review Focus 4 */
    CHECK(make_play());
    g_p.game.player.lives = 0;
    g_p.game.player.x = 22 * 16;                                    /* past the wall at column 20 */
    g_p.script = g_p.script_cursor = "R2000";                       /* into the pit at column 26, then below the map */
    for (int i = 0; i < 400 && g_p.game.player.state != 20; i++) play_step(&g_p, 0);
    CHECK_EQ(g_p.game.player.state, 0);                             /* nothing kills him there in 5a: he stands below the screen */
    CHECK(g_p.game.player.y >= g_p.game.world.fg_max_y + 160);
    kill_player(&g_p.game);                                        /* a deadly tile (5b) would do this */
    for (int i = 0; i < 2000 && g_p.game.player.lives != 3; i++) play_step(&g_p, 0);
    CHECK_EQ(g_p.game.player.lives, 3);                             /* game over -> the run restarted the level */
    for (int i = 0; i < 400 && g_p.game.player.state == 68; i++) play_step(&g_p, 0);   /* KICK OFF again */
    CHECK_EQ(g_p.game.player.x, 3 * 16); CHECK_EQ(g_p.game.player.state, 0);
    drop_play();
}

void test_play_real_level0_screenshot(void) {                      /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 3, .section = -1, .frames = 470, .script = ".170,R150,RU20,R130" };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    if (!SK_OK(&e)) printf("play_open: %s\n", e.msg);
    CHECK_EQ(p.level_pos, 1); CHECK_EQ(p.game.level_timer, 0x45);
    for (int i = 0; i < a.frames; i++) play_step(&p, 0);
    CHECK(p.game.player.x > p.game.world.start_x + 100);         /* the enemies are live: a knock-back is possible */
    CHECK(p.game.player.state == 0 || p.game.player.state == 52);
    play_draw(&p);
    static uint32_t out[SK_SCREEN_W * SK_SCREEN_H]; play_present(&p, out);
    CHECK(p.sprites.spr[0].visible);
    const char *dir = getenv("SOCCERKID_SHOTS");
    if (dir) {
        char path[512]; snprintf(path, sizeof path, "%s/level0-run.png", dir);
        mkdir(dir, 0755);
        CHECK(png_write(path, SK_SCREEN_W, SK_SCREEN_H, out, &e));
    }
    play_close(&p);
}

void test_play_step_reports_a_failed_load(void) {               /* 5a #9 */
    CHECK(make_play());
    g_p.scene.level_index = 99;                                   /* the next level must be read from disk: there is none */
    g_p.game.level_outcome = 0x81; g_p.game.level_end = true;
    for (int i = 0; i < 70 && !g_p.load_failed; i++) play_step(&g_p, 0);   /* after the 64-frame fade */
    CHECK(g_p.load_failed); CHECK(g_p.load_err.msg[0] != 0);
    int frames = g_p.game.frame_counter;
    play_step(&g_p, 0);
    CHECK_EQ(g_p.game.frame_counter, frames);                     /* stopped: no retry every frame */
    drop_play();
}

void test_play_home1_kick_scores(void) {                            /* ROM-backed: the ball kicked into the first walker */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 1, .section = -1, .frames = 320, .script = ".180,R20,RF3,R40,RF3,R40,RF3" };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    int kicks = 0;
    for (int i = 0; i < a.frames; i++) { play_step(&p, 0); kicks += p.game.player.state == 8 && p.game.player.timer == 1; }
    CHECK(kicks >= 1);
    CHECK(p.game.player.score > 0);
    CHECK(p.sprites.spr[1].visible || p.game.ball.inactive);        /* the ball is drawn */
    play_close(&p);
}

void test_country_london_run_without_a_crash(void) {               /* ROM-backed: levels 2..5 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    for (int lv = 2; lv <= 5; lv++) {
        sk_args a = { .roms = roms, .level = lv, .scale = 1, .section = -1, .frames = 2000,
                      .script = ".180,R300,RU20,R400,RF3,R600,RU20,R500" };
        static sk_play p; sk_err e = {0};
        CHECK(play_open(&p, &a, &e));
        for (int i = 0; i < a.frames; i++) play_step(&p, 0);
        play_draw(&p);
        static uint32_t out[SK_SCREEN_W * SK_SCREEN_H]; play_present(&p, out);
        CHECK(!p.load_failed);
        printf("level %d: kid %d,%d state %d lives %d score %06X objects %d platforms %d\n", lv, p.game.player.x, p.game.player.y,
               p.game.player.state, p.game.player.lives, p.game.player.score, p.game.object_count, p.game.platform_count);
        play_close(&p);
    }
}

void test_london1_kid_rides_a_platform(void) {                     /* ROM-backed: the kid placed on LONDON1's type 10 platform */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 4, .scale = 1, .section = -1, .frames = 0 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    int mx = -1, my = -1;
    for (int cy = 0; cy < p.game.world.h && mx < 0; cy++)
        for (int cx = 0; cx < p.game.world.w; cx++) {
            int param = 0;
            if (world_marker_type(&p.game.world, cx, cy, &param) == 10) { mx = cx; my = cy; break; }
        }
    CHECK(mx >= 0);
    for (int i = 0; i < 200; i++) play_step(&p, 0);                 /* KICK OFF */
    p.game.player.x = (int16_t)(mx * 16 + 8); p.game.player.y = (int16_t)(my * 16 - 60);
    int rode = 0;
    for (int i = 0; i < 600; i++) { play_step(&p, 0); rode |= p.game.player.on_platform; }
    CHECK(p.game.platform_count >= 1);
    CHECK(rode);
    play_close(&p);
}

void test_dark_kit_keeps_colour_49(void) {                         /* ROM-backed: kits 4..7 leave colour 49 as it was */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 1, .section = -1, .frames = 0 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    play_draw(&p);
    uint32_t c49 = p.scene.palette.xrgb[49];
    CHECK(c49 != 0);
    p.game.kit_current = 5;                                         /* a tunnel's dark shade */
    play_draw(&p);
    CHECK_EQ(p.scene.palette.xrgb[49], c49);
    play_close(&p);
}

void test_home1_cards_and_signs_animate(void) {                    /* ROM-backed: the level's cell states */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 1, .section = -1, .frames = 0 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    int cards = 0, signs = 0;
    for (int i = 0; i < p.game.world.w * p.game.world.h; i++) { cards += p.game.world.state[i] == 11; signs += p.game.world.state[i] == 19; }
    CHECK_EQ(cards, 5); CHECK_EQ(signs, 2);
    uint16_t first = p.game.tile_anim[11];
    int changes = 0;
    for (int i = 0; i < 48; i++) { play_step(&p, 0); if (p.game.tile_anim[11] != first) { changes++; first = p.game.tile_anim[11]; } }
    CHECK(changes >= 10);                                            /* a card turns every 4 frames */
    play_close(&p);
}

void test_italy_run_without_a_crash(void) {                        /* ROM-backed: levels 6..11 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    for (int lv = 6; lv <= 11; lv++) {
        sk_args a = { .roms = roms, .level = lv, .scale = 1, .section = -1, .frames = 2000,
                      .script = ".180,R300,RU20,R400,RF3,R600,RU20,R500" };
        static sk_play p; sk_err e = {0};
        CHECK(play_open(&p, &a, &e));
        for (int i = 0; i < a.frames; i++) play_step(&p, 0);
        play_draw(&p);
        static uint32_t out[SK_SCREEN_W * SK_SCREEN_H]; play_present(&p, out);
        CHECK(!p.load_failed);
        CHECK(p.game.object_count <= 16);
        printf("level %d: kid %d,%d state %d lives %d score %06X objects %d platforms %d\n", lv, p.game.player.x, p.game.player.y,
               p.game.player.state, p.game.player.lives, p.game.player.score, p.game.object_count, p.game.platform_count);
        play_close(&p);
    }
}

void test_play_starts_near_a_type(void) {                           /* ROM-backed: --near 51 on RIVIERA2, --start */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 11, .scale = 1, .section = -1, .near_type = 51, .near_n = 1 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    int mx = -1, my = -1;
    for (int cy = 0; cy < p.game.world.h && mx < 0; cy++)
        for (int cx = 0; cx < p.game.world.w; cx++) { int param = 0; if (world_marker_type(&p.game.world, cx, cy, &param) == 51) { mx = cx; my = cy; break; } }
    CHECK(mx >= 0);
    CHECK_EQ(p.game.player.x, mx * 16 - 128); CHECK_EQ(p.game.world.start_x, mx * 16 - 128);   /* a respawn comes back here too */
    (void)my;
    play_close(&p);
    sk_args b = { .roms = roms, .level = 6, .scale = 1, .section = -1, .near_type = -1, .has_start = true, .start_x = 1200, .start_y = 96 };
    CHECK(play_open(&p, &b, &e));
    CHECK_EQ(p.game.player.x, 1200); CHECK_EQ(p.game.player.y, 96);
    play_close(&p);
    sk_args c = { .roms = roms, .level = 6, .scale = 1, .section = -1, .near_type = 51, .near_n = 1 };
    CHECK(!play_open(&p, &c, &e));                                   /* no boss on RUINS1: a clear error */
    CHECK(strstr(e.msg, "51") != NULL);
}

void test_a_test_start_takes_the_layer_switches_on_its_left(void) {   /* ROM-backed: RUINS1's attribute 54 column at x 208 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 6, .scale = 1, .section = -1, .near_type = 48, .near_n = 1 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    CHECK_EQ(p.game.layer_flags, 3);                                 /* past the switch: in front, as when walked there */
    for (int i = 0; i < 2000 && p.game.player.state != 20; i++) play_step(&p, 0);
    kill_player(&p.game);
    for (int i = 0; i < 2000 && !(p.game.player.state == 68 || p.game.player.state == 0); i++) play_step(&p, 0);
    CHECK_EQ(p.game.layer_flags, 3);                                 /* and after a respawn there */
    play_close(&p);
    sk_args b = { .roms = roms, .level = 6, .scale = 1, .section = -1, .has_start = true, .start_x = 150, .start_y = 64 };
    CHECK(play_open(&p, &b, &e));
    CHECK_EQ(p.game.layer_flags, 0);                                 /* before it: behind */
    play_close(&p);
}

void test_play_runs_without_audio(void) {                           /* ROM-backed, Review Focus 5: no device, the sound still steps */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 1, .section = -1 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    CHECK(p.audio == NULL);
    CHECK(p.snd.slot[0] != NULL);                                    /* England 1's module loaded */
    for (int i = 0; i < 400; i++) play_step(&p, 0);
    CHECK(p.snd.pt.playing);                                         /* KICK OFF: the jingle, then the level tune */
    CHECK(p.snd.pt.mod == p.snd.slot[0]);
    long energy = 0;
    for (int k = 0; k < 2 * 960; k += 8) energy += p.audio_buf[k] < 0 ? -p.audio_buf[k] : p.audio_buf[k];
    CHECK(energy > 0);
    play_close(&p);
}

void test_music_survives_a_level_change(void) {                     /* ROM-backed, Review Focus 4 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 1, .section = -1 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    for (int i = 0; i < 400; i++) play_step(&p, 0);
    p.game.level_number++; p.game.level_outcome = 0x81; p.game.level_end = true;   /* the level is won (the exit walk) */
    int frames = 0;
    while (p.scene.level_index == 0 && frames++ < 400) play_step(&p, 0);
    CHECK(p.scene.level_index != 0);
    CHECK(frames >= 64);                                             /* the 64-frame fade first (0x1040F2) */
    CHECK(p.snd.slot[0] != NULL);
    for (int i = 0; i < 300; i++) play_step(&p, 0);                  /* plays the new level's module (ASan: no freed one) */
    CHECK(p.snd.pt.mod == NULL || p.snd.pt.mod == p.snd.slot[0] || p.snd.pt.mod == p.snd.jingle);
    play_close(&p);
}

void test_game_over_fades_the_music(void) {                          /* ROM-backed: fade_music_out 0x1040F2 on every level exit */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 1, .section = -1 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    for (int i = 0; i < 300; i++) play_step(&p, 0);
    p.game.player.state = 64; p.game.player.lives = -1;              /* GAME OVER */
    for (int i = 0; i < 70; i++) play_step(&p, 0);
    CHECK(p.snd.fading >= 64);
    for (int i = 0; i < 3; i++) CHECK_EQ(p.snd.paula.ch[i].vol, 0);
    play_close(&p);
}

void test_russia_run_without_a_crash(void) {                        /* ROM-backed: levels 13..17 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    for (int lv = 13; lv <= 17; lv++) {
        sk_args a = { .roms = roms, .level = lv, .scale = 1, .section = -1, .frames = 2000,
                      .script = ".180,R300,RU20,R400,RF3,R600,RU20,R500" };
        static sk_play p; sk_err e = {0};
        CHECK(play_open(&p, &a, &e));
        for (int i = 0; i < a.frames; i++) play_step(&p, 0);
        play_draw(&p);
        static uint32_t out[SK_SCREEN_W * SK_SCREEN_H]; play_present(&p, out);
        CHECK(!p.load_failed);
        CHECK(p.game.object_count <= 16);
        printf("level %d: kid %d,%d state %d lives %d score %06X objects %d platforms %d\n", lv, p.game.player.x, p.game.player.y,
               p.game.player.state, p.game.player.lives, p.game.player.score, p.game.object_count, p.game.platform_count);
        play_close(&p);
    }
}

void test_japan_run_without_a_crash(void) {                         /* ROM-backed: levels 18..22 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    for (int lv = 18; lv <= 22; lv++) {
        sk_args a = { .roms = roms, .level = lv, .scale = 1, .section = -1, .frames = 2000,
                      .script = ".180,R300,RU20,R400,RF3,R600,RU20,L200,R300" };
        static sk_play p; sk_err e = {0};
        CHECK(play_open(&p, &a, &e));
        for (int i = 0; i < a.frames; i++) { play_step(&p, 0); if (i % 50 == 0) play_draw(&p); }
        play_draw(&p);
        static uint32_t out[SK_SCREEN_W * SK_SCREEN_H]; play_present(&p, out);
        CHECK(!p.load_failed);
        CHECK(p.game.object_count <= 16);
        printf("level %d: kid %d,%d state %d lives %d score %06X objects %d bg_x %d level %d\n", lv, p.game.player.x, p.game.player.y,
               p.game.player.state, p.game.player.lives, p.game.player.score, p.game.object_count, p.game.view.bg_x, p.game.train_level);
        play_close(&p);
    }
}

void test_usa_run_without_a_crash(void) {                         /* ROM-backed: levels 23..28 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    for (int lv = 23; lv <= 28; lv++) {
        sk_args a = { .roms = roms, .level = lv, .scale = 1, .section = -1, .frames = 2000,
                      .script = ".180,R300,RU20,R400,RF3,R600,RU20,L200,R300" };
        static sk_play p; sk_err e = {0};
        CHECK(play_open(&p, &a, &e));
        for (int i = 0; i < a.frames; i++) { play_step(&p, 0); if (i % 50 == 0) play_draw(&p); }
        play_draw(&p);
        static uint32_t out[SK_SCREEN_W * SK_SCREEN_H]; play_present(&p, out);
        CHECK(!p.load_failed);
        CHECK(p.game.object_count <= 16);
        printf("level %d: kid %d,%d state %d lives %d score %06X objects %d bg_x %d level %d\n", lv, p.game.player.x, p.game.player.y,
               p.game.player.state, p.game.player.lives, p.game.player.score, p.game.object_count, p.game.view.bg_x, p.game.train_level);
        play_close(&p);
    }
}

static void finish_level(sk_play *p) {                              /* what the exit walk (state 60) leaves */
    p->game.level_number++;
    p->game.level_outcome = 0x81;
    p->game.level_end = true;
}

void test_play_whole_game_runs_map_levels_and_the_scoreboard(void) {   /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = -1, .scale = 1, .section = -1 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    if (!SK_OK(&e)) printf("play_open: %s\n", e.msg);
    CHECK_EQ(p.pic.kind, SK_PIC_LOADING);
    for (int i = 0; i < 25; i++) play_step(&p, 0);
    CHECK_EQ(p.pic.kind, SK_PIC_MAP); CHECK_EQ(p.pic.map_index, 0); CHECK_EQ(p.game.level_number, 1);
    for (int i = 0; i < 201; i++) play_step(&p, 0);
    CHECK_EQ(p.pic.kind, SK_PIC_LOADING);
    for (int i = 0; i < 25; i++) play_step(&p, 0);
    CHECK_EQ(p.pic.kind, SK_PIC_OFF); CHECK_EQ(p.scene.level_index, 0); CHECK_EQ(p.level_pos, 1);
    play_step(&p, 0);
    finish_level(&p);                                               /* HOME1 done: HOME2 */
    for (int i = 0; i < 64 + 25 + 1; i++) play_step(&p, 0);
    CHECK_EQ(p.pic.kind, SK_PIC_OFF); CHECK_EQ(p.scene.level_index, 1);
    finish_level(&p);                                               /* HOME2 done: the scoreboard (position 3) */
    for (int i = 0; i < 64 + 25 + 1; i++) play_step(&p, 0);
    CHECK_EQ(p.scene.level_index, 12); CHECK_EQ(p.game.level_number, 3);
    p.game.level_number = 9;                                        /* the last scoreboard of England: map 1 at 10 */
    finish_level(&p);
    for (int i = 0; i < 64 + 25 + 1; i++) play_step(&p, 0);
    CHECK_EQ(p.pic.kind, SK_PIC_MAP); CHECK_EQ(p.pic.map_index, 1); CHECK_EQ(p.game.level_number, 11);
    play_close(&p);
}

void test_play_game_end_starts_over(void) {                          /* ROM-backed: the ending, map 5, then 42 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 28, .scale = 1, .section = -1 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    p.game.level_number = 48;                                       /* ALIEN done: 49 the ending */
    finish_level(&p);
    for (int i = 0; i < 64 + 25 + 1; i++) play_step(&p, 0);
    CHECK_EQ(p.pic.kind, SK_PIC_ENDING); CHECK_EQ(p.game.level_number, 50);
    CHECK(p.snd.slot[0] != NULL); CHECK(p.snd.music_use4);       /* file090 on four channels */
    for (int i = 0; i < 100; i++) play_step(&p, 0);
    CHECK_EQ(p.pic.kind, SK_PIC_ENDING);                            /* waits for fire */
    play_step(&p, SK_IN_FIRE);
    for (int i = 0; i < 64 + 25; i++) play_step(&p, 0);
    CHECK_EQ(p.pic.kind, SK_PIC_MAP); CHECK_EQ(p.pic.map_index, 5);
    CHECK(!p.snd.music_use4); CHECK(p.snd.slot[0] == NULL);
    for (int i = 0; i < 2000 && p.pic.kind == SK_PIC_MAP; i++) play_step(&p, 0);
    for (int i = 0; i < 25 + 25 + 201; i++) play_step(&p, 0);      /* 42: a new game, map 0 */
    CHECK_EQ(p.game.player.lives, 3); CHECK(p.game.level_number <= 1);
    play_close(&p);
}

void test_play_draws_the_board_text_into_playfield_one(void) {
    CHECK(make_play());
    g_p.game.board.active = true;
    g_p.game.board.text_set[5][1] = 0xC0; g_p.game.board.text[5][1] = 0x80;   /* window x 8 lit, x 9 written dark */
    play_draw(&g_p);
    CHECK_EQ(g_p.scene.fb.px[5 * SK_FB_W + SK_VIEW_X + 8] & 1, 1);
    CHECK_EQ(g_p.scene.fb.px[5 * SK_FB_W + SK_VIEW_X + 9] & 1, 0);
    g_p.game.board.active = false;
    drop_play();
}

void test_scoreboard_level_counts_and_moves_on(void) {             /* ROM-backed: the board after HOME2 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 1, .scale = 1, .section = -1 };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    p.game.cards_taken = 4; p.game.recalls = 2; p.game.level_timer = 0x31; p.game.kills[1] = 3;
    finish_level(&p);                                               /* HOME2 done: TIME += 31, BALLS += 10 - 2 */
    for (int i = 0; i < 64 + 25 + 1; i++) play_step(&p, 0);
    CHECK_EQ(p.scene.level_index, 12);
    CHECK_EQ(p.game.stat_time, 0x31); CHECK_EQ(p.game.stat_balls, 0x08); CHECK_EQ(p.game.cards_taken, 4);
    uint32_t score = p.game.player.score;
    int n = 0;
    while (p.scene.level_index == 12 && n < 6000) { play_step(&p, 0); n++; }
    CHECK_EQ(p.scene.level_index, 2);                               /* ENG COUNTRY1 next */
    CHECK_EQ(p.game.player.score - score, 0x6200u);               /* 4 x 300 + 8 x 200 + 31 x 100 + 3 x 100, BCD */
    CHECK_EQ(p.game.cards, 0); CHECK_EQ(p.game.stat_time, 0);
    play_close(&p);
}

void test_bonus_levels_and_alien_load_and_run(void) {               /* ROM-backed: data 36..41 are files 136..141 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    for (int lv = 36; lv <= 41; lv++) {
        sk_args a = { .roms = roms, .level = lv, .scale = 1, .section = -1, .script = ".170,R1500,.330" };
        static sk_play p; sk_err e = {0};
        CHECK(play_open(&p, &a, &e));
        if (!SK_OK(&e)) { printf("level %d: %s\n", lv, e.msg); continue; }
        int want = lv == 41 ? 82 : 81, found = 0;
        for (int i = 0; i < 2000; i++) {
            play_step(&p, 0);
            for (int k = 0; k < 16; k++) found |= p.game.objs.slot[k].x >= 0 && (p.game.objs.slot[k].type4 & 0x1FF) == want * 4;
        }
        if (lv == 41) CHECK(found);                                 /* the alien is beside the start */
        CHECK(!p.load_failed);
        play_close(&p);
    }
}
