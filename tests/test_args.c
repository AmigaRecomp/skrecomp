#include <string.h>
#include "check.h"
#include "app/args.h"
#include "app/play.h"
#include "game/input.h"

static bool parse(sk_args *a, sk_err *e, int argc, const char **argv) {
    return args_parse(argc, (char **)argv, a, e);
}

void test_args_defaults_and_errors(void) {
    sk_args a;
    sk_err e = {0};
    const char *ok[] = { "soccerkid", "--roms", "d" };
    CHECK(parse(&a, &e, 3, ok));
    CHECK(strcmp(a.roms, "d") == 0);
    CHECK_EQ(a.level, -1);                                         /* no --level: the whole game from world map 0 */
    CHECK_EQ(a.cam_x, 0);
    CHECK_EQ(a.cam_y, 0);
    CHECK_EQ(a.scale, 3);
    CHECK_EQ(a.exit_after, 0);
    CHECK_EQ(a.section, -1);
    CHECK(a.section_file == NULL && a.screenshot == NULL && !a.no_backdrop && !a.help && !a.version);
    CHECK_EQ(a.frame_w, 0);

    const char *full[] = { "soccerkid", "--roms", "d", "--level", "5", "--cam", "100,20", "--section", "file031:4",
                           "--screenshot", "o.png", "--no-backdrop", "--scale", "2", "--exit-after", "3" };
    CHECK(parse(&a, &e, 16, full));
    CHECK_EQ(a.level, 5);
    CHECK_EQ(a.cam_x, 100);
    CHECK_EQ(a.cam_y, 20);
    CHECK(a.section_file && strcmp(a.section_file, "file031") == 0);
    CHECK_EQ(a.section, 4);
    CHECK(a.screenshot && strcmp(a.screenshot, "o.png") == 0);
    CHECK(a.no_backdrop);
    CHECK_EQ(a.scale, 2);
    CHECK_EQ(a.exit_after, 3);

    const char *sized[] = { "soccerkid", "--roms", "d", "--section", "file036:0:48x64", "--no-frames" };
    CHECK(parse(&a, &e, 6, sized));
    CHECK_EQ(a.frame_w, 48);
    CHECK_EQ(a.frame_h, 64);
    CHECK(a.no_frames && !a.no_backdrop);

    const char *badlevel[] = { "soccerkid", "--roms", "d", "--level", "x" };
    CHECK(!parse(&a, &e, 5, badlevel));
    CHECK_STR(e.msg, "--level");
    const char *badcam[] = { "soccerkid", "--roms", "d", "--cam", "5" };
    CHECK(!parse(&a, &e, 5, badcam));
    CHECK_STR(e.msg, "--cam");
    const char *badsection[] = { "soccerkid", "--roms", "d", "--section", "file031" };
    CHECK(!parse(&a, &e, 5, badsection));
    CHECK_STR(e.msg, "--section");
    const char *unknown[] = { "soccerkid", "--roms", "d", "--bogus" };
    CHECK(!parse(&a, &e, 4, unknown));
    CHECK_STR(e.msg, "--bogus");
    const char *missing[] = { "soccerkid", "--roms" };
    CHECK(!parse(&a, &e, 2, missing));
    CHECK_STR(e.msg, "--roms");
    const char *noroms[] = { "soccerkid", "--level", "1" };
    CHECK(parse(&a, &e, 3, noroms));                               /* the embedded data, or --data, can stand in */
    CHECK(a.roms == NULL && a.data_path == NULL);
    const char *data[] = { "soccerkid", "--data", "x.dat" };
    CHECK(parse(&a, &e, 3, data)); CHECK(strcmp(a.data_path, "x.dat") == 0);
    const char *help[] = { "soccerkid", "--help" };
    CHECK(parse(&a, &e, 2, help));
    CHECK(a.help);
    const char *version[] = { "soccerkid", "--version" };
    CHECK(parse(&a, &e, 2, version));
    CHECK(a.version);
    CHECK_STR(args_usage(), "--roms");
}

void test_args_script_and_frames(void) {
    sk_args a; sk_err e = {0};
    const char *ok[] = { "soccerkid", "--roms", "d", "--frames", "300", "--script", "R150,RU20,.30", "--viewer" };
    CHECK(args_parse(8, (char **)ok, &a, &e));
    CHECK_EQ(a.frames, 300); CHECK(a.viewer); CHECK(a.script && strcmp(a.script, "R150,RU20,.30") == 0);
    const char *s = a.script; uint8_t in; int n;
    CHECK(script_parse_step(&s, &in, &n, &e)); CHECK_EQ(in, SK_IN_RIGHT); CHECK_EQ(n, 150);
    CHECK(script_parse_step(&s, &in, &n, &e)); CHECK_EQ(in, SK_IN_RIGHT | SK_IN_UP); CHECK_EQ(n, 20);
    CHECK(script_parse_step(&s, &in, &n, &e)); CHECK_EQ(in, 0); CHECK_EQ(n, 30); CHECK_EQ(*s, 0);
    const char *bad[] = { "soccerkid", "--roms", "d", "--script", "X9" };
    CHECK(!args_parse(5, (char **)bad, &a, &e)); CHECK_STR(e.msg, "--script");
    const char *nocount[] = { "soccerkid", "--roms", "d", "--script", "R" };
    CHECK(!args_parse(5, (char **)nocount, &a, &e)); CHECK_STR(e.msg, "frame count");
    const char *defaults[] = { "soccerkid", "--roms", "d" };
    CHECK(args_parse(3, (char **)defaults, &a, &e)); CHECK(!a.viewer); CHECK_EQ(a.frames, 0); CHECK(a.script == NULL);
}

void test_args_start_near_and_markers(void) {
    sk_args a; sk_err e;
    const char *none[] = { "soccerkid", "--roms", "d" };
    CHECK(parse(&a, &e, 3, none));
    CHECK(!a.has_start); CHECK_EQ(a.near_type, -1); CHECK(!a.markers);
    const char *start[] = { "soccerkid", "--roms", "d", "--start", "1200,96" };
    CHECK(parse(&a, &e, 5, start));
    CHECK(a.has_start); CHECK_EQ(a.start_x, 1200); CHECK_EQ(a.start_y, 96);
    const char *near[] = { "soccerkid", "--roms", "d", "--near", "51", "--markers" };
    CHECK(parse(&a, &e, 6, near));
    CHECK_EQ(a.near_type, 51); CHECK_EQ(a.near_n, 1); CHECK(a.markers);
    const char *near2[] = { "soccerkid", "--roms", "d", "--near", "36,2" };
    CHECK(parse(&a, &e, 5, near2));
    CHECK_EQ(a.near_type, 36); CHECK_EQ(a.near_n, 2);
    const char *bad[] = { "soccerkid", "--roms", "d", "--near", "200" };
    CHECK(!parse(&a, &e, 5, bad));
    const char *bad2[] = { "soccerkid", "--roms", "d", "--start", "12" };
    CHECK(!parse(&a, &e, 5, bad2));
}

void test_args_cup_sets_the_pieces(void) {
    sk_args a; sk_err e = {0};
    const char *none[] = { "soccerkid", "--roms", "d" };
    CHECK(parse(&a, &e, 3, none)); CHECK_EQ(a.cup, -1);
    const char *all[] = { "soccerkid", "--roms", "d", "--cup", "31" };
    CHECK(parse(&a, &e, 5, all)); CHECK_EQ(a.cup, 31);
    const char *bad[] = { "soccerkid", "--roms", "d", "--cup", "32" };
    CHECK(!parse(&a, &e, 5, bad)); CHECK_STR(e.msg, "--cup");
}
