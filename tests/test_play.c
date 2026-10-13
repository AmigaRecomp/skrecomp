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
    g_p.script = g_p.script_cursor = "R60,RU20,.60";
    for (int i = 0; i < 141; i++) play_step(&g_p, 0);        /* 140 scripted frames, then the script is spent */
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
    CHECK_EQ(g_p.game.player.x, 3 * 16); CHECK_EQ(g_p.game.player.state, 0);
    drop_play();
}

void test_play_real_level0_screenshot(void) {                      /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args a = { .roms = roms, .level = 0, .scale = 3, .section = -1, .frames = 300, .script = "R150,RU20,R130" };
    static sk_play p; sk_err e = {0};
    CHECK(play_open(&p, &a, &e));
    if (!SK_OK(&e)) printf("play_open: %s\n", e.msg);
    CHECK_EQ(p.level_pos, 1); CHECK_EQ(p.game.level_timer, 0x45);
    for (int i = 0; i < a.frames; i++) play_step(&p, 0);
    CHECK(p.game.player.x > p.game.world.start_x + 200); CHECK_EQ(p.game.player.state, 0);
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
