#include <string.h>
#include "check.h"
#include "app/args.h"

static bool parse(sk_args *a, sk_err *e, int argc, const char **argv) {
    return args_parse(argc, (char **)argv, a, e);
}

void test_args_defaults_and_errors(void) {
    sk_args a;
    sk_err e = {0};
    const char *ok[] = { "soccerkid", "--roms", "d" };
    CHECK(parse(&a, &e, 3, ok));
    CHECK(strcmp(a.roms, "d") == 0);
    CHECK_EQ(a.level, 0);
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
    CHECK(!parse(&a, &e, 3, noroms));
    CHECK_STR(e.msg, "--roms");
    const char *help[] = { "soccerkid", "--help" };
    CHECK(parse(&a, &e, 2, help));
    CHECK(a.help);
    const char *version[] = { "soccerkid", "--version" };
    CHECK(parse(&a, &e, 2, version));
    CHECK(a.version);
    CHECK_STR(args_usage(), "--roms");
}
