#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "editor/skld_args.h"
#include "editor/playtest.h"
#include "game/input.h"

void test_skld_args(void) {
    skld_args a; sk_err e = {0};
    const char *n1[] = { "skld", "new", "london1", "a.skl" };
    CHECK(skld_args_parse(4, (char **)n1, &a, &e)); CHECK_EQ(a.mode, SKLD_NEW); CHECK_EQ(a.setting->index, 4);
    CHECK(strcmp(a.file, "a.skl") == 0); CHECK(a.copy);
    const char *n2[] = { "skld", "new", "venice2", "b.skl", "--size", "40x13", "--roms", "d" };
    CHECK(skld_args_parse(8, (char **)n2, &a, &e)); CHECK(!a.copy); CHECK_EQ(a.w, 40); CHECK_EQ(a.h, 13);
    CHECK(strcmp(a.roms, "d") == 0);
    const char *n3[] = { "skld", "new", "london1", "a.skl", "--size", "10x10" };
    CHECK(!skld_args_parse(6, (char **)n3, &a, &e)); CHECK_STR(e.msg, "at least 22x13");
    const char *n4[] = { "skld", "new", "nowhere", "a.skl" };
    CHECK(!skld_args_parse(4, (char **)n4, &a, &e)); CHECK_STR(e.msg, "unknown setting \"nowhere\""); CHECK_STR(e.msg, "skld --settings");
    const char *n5[] = { "skld", "new", "london1" };
    CHECK(!skld_args_parse(3, (char **)n5, &a, &e)); CHECK_STR(e.msg, "new SETTING FILE");
    const char *o1[] = { "skld", "--data", "x.dat", "level.skl" };
    CHECK(skld_args_parse(4, (char **)o1, &a, &e)); CHECK_EQ(a.mode, SKLD_OPEN); CHECK(strcmp(a.file, "level.skl") == 0);
    CHECK(strcmp(a.data_path, "x.dat") == 0);
    const char *s1[] = { "skld", "--settings" };
    CHECK(skld_args_parse(2, (char **)s1, &a, &e)); CHECK_EQ(a.mode, SKLD_SETTINGS);
    const char *h1[] = { "skld" };
    CHECK(skld_args_parse(1, (char **)h1, &a, &e)); CHECK_EQ(a.mode, SKLD_HELP);
    const char *b1[] = { "skld", "a.skl", "b.skl" };
    CHECK(!skld_args_parse(3, (char **)b1, &a, &e)); CHECK_STR(e.msg, "one level file");
    const char *b2[] = { "skld", "--frob", "a.skl" };
    CHECK(!skld_args_parse(3, (char **)b2, &a, &e)); CHECK_STR(e.msg, "unknown option --frob");
}

void test_playtest_leaves_the_document_alone(void) {      /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_args base = { .roms = roms, .level = 4, .scale = 1, .section = -1, .near_type = -1, .cup = -1 };
    static sk_scene s; sk_err e = {0};
    CHECK(scene_open(&s, &base, &e));
    static sk_doc d;
    CHECK(doc_new_copy(&d, &s.level, 4, &e));
    doc_paint(&d, 3, 3, 1);
    sk_custom before; CHECK(custom_copy(&before, &d.lvl));
    int past = d.past_n, xs[2] = { 0, 0 };
    static sk_play p;
    for (int run = 0; run < 2; run++) {
        CHECK(playtest_open(&p, &base, &d, run == 1, 40, 20, &e));
        for (int i = 0; i < 300; i++) play_step(&p, run == 0 ? 0 : SK_IN_RIGHT);
        xs[run] = p.game.player.x;
        if (run == 1) CHECK(p.game.player.x >= 40 * 16);          /* started at the cursor's cell, ran right */
        play_close(&p);
    }
    CHECK(xs[0] != 0);
    CHECK(memcmp(before.cells, d.lvl.cells, (size_t)before.w * before.h * 2) == 0);
    CHECK(memcmp(before.states, d.lvl.states, (size_t)before.w * before.h) == 0);
    CHECK_EQ(d.past_n, past); CHECK(d.dirty);
    custom_free(&before); doc_free(&d); scene_close(&s);
}

void test_skld_usage_lists_the_settings(void) {
    const char *u = skld_usage();
    CHECK_STR(u, "SETTING is one of");
    for (int i = 0; i < custom_setting_count(); i++) { CHECK_STR(u, custom_setting_at(i)->name); CHECK_STR(u, custom_setting_at(i)->title); }
    CHECK_STR(u, "Not available");                                /* the levels left out, and why */
    CHECK_STR(u, "skld new london1 mylevel.skl");                 /* an example */
}
