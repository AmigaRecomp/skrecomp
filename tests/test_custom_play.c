#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "app/args.h"
#include "app/play.h"
#include "formats/custom.h"
#include "game/world.h"
#include "game/level_flow.h"
#include "game/player.h"

void test_args_custom(void) {
    sk_args a; sk_err e = {0};
    const char *ok[] = { "soccerkid", "--custom", "x.skl" };
    CHECK(args_parse(3, (char **)ok, &a, &e)); CHECK(a.custom_path && strcmp(a.custom_path, "x.skl") == 0);
    const char *lv[] = { "soccerkid", "--custom", "x.skl", "--level", "3" };
    CHECK(!args_parse(5, (char **)lv, &a, &e)); CHECK_STR(e.msg, "--custom cannot be combined with");
    const char *vw[] = { "soccerkid", "--custom", "x.skl", "--viewer" };
    CHECK(!args_parse(4, (char **)vw, &a, &e)); CHECK_STR(e.msg, "--custom cannot be combined with");
    const char *ng[] = { "soccerkid", "--new-game", "--custom", "x.skl" };
    CHECK(!args_parse(4, (char **)ng, &a, &e)); CHECK_STR(e.msg, "--custom cannot be combined with");
}

static bool copy_level(const sk_level *lv, int setting, sk_custom *c) {
    sk_err e = {0};
    if (!custom_alloc(c, setting, lv->map.width, lv->map.height, &e)) return false;
    memcpy(c->cells, lv->map.cells, (size_t)c->w * c->h * 2);
    memcpy(c->states, lv->map.states, (size_t)c->w * c->h);
    return true;
}

void test_custom_copy_plays_like_the_original(void) {       /* ROM-backed: LONDON1 against its own copy, frame by frame */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    static sk_play a, b; sk_err e = {0};
    const char *script = ".180,R400,RU20,R300,RF3,RU15,R600,.20,L100,R545";
    sk_args aa = { .roms = roms, .level = 4, .scale = 1, .section = -1, .script = script, .near_type = -1 };
    CHECK(play_open(&a, &aa, &e));
    sk_custom c;
    CHECK(copy_level(&a.scene.level, 4, &c));
    sk_args ab = aa; ab.custom = &c;
    CHECK(play_open(&b, &ab, &e));
    int diffs = 0, first = -1;
    for (int i = 0; i < 2000; i++) {
        play_step(&a, 0); play_step(&b, 0);
        bool same = a.game.player.x == b.game.player.x && a.game.player.y == b.game.player.y &&
                    a.game.player.score == b.game.player.score && a.game.object_count == b.game.object_count &&
                    a.game.player.state == b.game.player.state && a.game.camera_x == b.game.camera_x;
        if (!same && first < 0) first = i;
        diffs += !same;
    }
    if (diffs) printf("first difference at frame %d\n", first);
    CHECK_EQ(diffs, 0);
    CHECK(a.game.player.x > 600);                                  /* the run went somewhere */
    play_close(&a); play_close(&b); custom_free(&c);
}

void test_custom_exit_ends_the_run(void) {                     /* ROM-backed: a 22 x 13 level on LONDON1 with a near exit */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    static sk_play p; sk_err e = {0};
    sk_args a = { .roms = roms, .level = 4, .scale = 1, .section = -1, .near_type = -1 };
    CHECK(play_open(&p, &a, &e));                                  /* to read LONDON1's floor tile and exit flag */
    const sk_level *lv = &p.scene.level;
    int floor = -1, exit_flag = -1, sx = -1, sy = -1;
    for (int i = 0; i < lv->map.width * lv->map.height && sx < 0; i++)
        if ((lv->map.cells[i] & SK_INDEX_MASK) == 0x1FF) { sx = i % lv->map.width; sy = i / lv->map.width; }
    for (int y = sy + 1; y < lv->map.height && floor < 0; y++) {   /* the first solid tile below the start */
        int t = level_cell(lv, sx, y) & SK_INDEX_MASK;
        const uint8_t *tail = t < lv->tiles_a.count ? lv->tiles_a.tails + t * SK_TILE_TAIL : NULL;
        if (tail && tail[0]) floor = t;
    }
    for (int f = 33; f < 128 && exit_flag < 0; f++)
        if (lv->pals[SK_PALS_ATTR + f - 33] == 13) exit_flag = f;
    CHECK(floor >= 0); CHECK(exit_flag >= 0);
    sk_custom c; CHECK(custom_alloc(&c, 4, 22, 13, &e));
    for (int x = 0; x < 22; x++) { c.cells[11 * 22 + x] = (uint16_t)floor; c.cells[12 * 22 + x] = (uint16_t)floor; }
    c.cells[10 * 22 + 1] = 0x1FF;
    for (int y = 7; y <= 10; y++) c.cells[y * 22 + 8] = (uint16_t)(exit_flag << SK_FLAG_SHIFT);
    play_close(&p);
    a.custom = &c; a.script = ".180,R600";
    CHECK(play_open(&p, &a, &e));
    int i = 0;
    for (; i < 900 && !p.quit; i++) play_step(&p, 0);
    CHECK(p.custom_done); CHECK(p.quit);
    printf("custom level: %d frames, kid %d,%d state %d lives %d outcome %d\n", i, p.game.player.x, p.game.player.y, p.game.player.state, p.game.player.lives, (int)game_outcome(&p.game));
    play_close(&p); custom_free(&c);
}

void test_custom_bonus_keeps_its_timer_after_game_over(void) {   /* review 3: ROM-backed, ENGLAND BONUS as a setting */
    /* (named for the review finding: the bonus level's own end is a death, which ends a custom run) */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    static sk_play p; sk_err e = {0};
    sk_args a = { .roms = roms, .level = 36, .scale = 1, .section = -1, .near_type = -1, .cup = -1 };
    CHECK(play_open(&p, &a, &e));
    sk_custom c; CHECK(copy_level(&p.scene.level, 36, &c));
    play_close(&p);
    a.custom = &c;
    CHECK(play_open(&p, &a, &e));
    CHECK(p.game.timeout_armed);
    p.game.player.lives = 0;
    kill_player(&p.game);                                         /* a death ends a bonus level, as in the original */
    for (int i = 0; i < 2000 && !p.quit; i++) play_step(&p, 0);
    CHECK(p.quit); CHECK(!p.custom_done);                         /* the run ends: no title screen, no other level */
    CHECK_EQ(p.scene.level_index, 36); CHECK_EQ(p.scene.level.map.width, c.w);
    play_close(&p); custom_free(&c);
}

void test_custom_round_trip_through_the_file_plays_the_same(void) {   /* review 13: write, parse, then play */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    static sk_play a, b; sk_err e = {0};
    sk_args aa = { .roms = roms, .level = 4, .scale = 1, .section = -1, .script = ".180,R400,RU20,R700", .near_type = -1 };
    CHECK(play_open(&a, &aa, &e));
    sk_custom c, r = {0}; uint8_t *d = NULL; size_t n = 0;
    CHECK(copy_level(&a.scene.level, 4, &c));
    CHECK(custom_write(&c, &d, &n)); CHECK(custom_parse(d, n, &r, &e));
    sk_args ab = aa; ab.custom = &r;
    CHECK(play_open(&b, &ab, &e));
    int diffs = 0;
    for (int i = 0; i < 1300; i++) { play_step(&a, 0); play_step(&b, 0); diffs += a.game.player.x != b.game.player.x || a.game.player.y != b.game.player.y; }
    CHECK_EQ(diffs, 0);
    play_close(&a); play_close(&b); custom_free(&c); custom_free(&r); free(d);
}

void test_playtest_game_over_returns(void) {                  /* review 6: skld's play-test ends on game over */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    static sk_play p; sk_err e = {0};
    sk_args a = { .roms = roms, .level = 4, .scale = 1, .section = -1, .near_type = -1, .cup = -1 };
    CHECK(play_open(&p, &a, &e));
    sk_custom c; CHECK(copy_level(&p.scene.level, 4, &c));
    play_close(&p);
    a.custom = &c; a.custom_quit_on_game_over = true;
    CHECK(play_open(&p, &a, &e));
    p.game.player.lives = 0;
    kill_player(&p.game);
    for (int i = 0; i < 2000 && !p.quit; i++) play_step(&p, 0);
    CHECK(p.quit); CHECK(!p.custom_done); CHECK(p.custom_game_over);
    play_close(&p); custom_free(&c);
}
