/* soccerkid: level viewer entry point. */
#include <stdio.h>
#include <stdlib.h>
#include "app/args.h"
#include "app/scene.h"
#include "app/window.h"
#include "gfx/display_model.h"
#include "gfx/png.h"

int main(int argc, char **argv) {
    sk_args a;
    sk_err e = {0};
    if (!args_parse(argc, argv, &a, &e)) {
        fprintf(stderr, "soccerkid: %s\n%s", e.msg, args_usage());
        return 2;
    }
    if (a.help) { fputs(args_usage(), stdout); return 0; }
    if (a.version) { puts("soccerkid 0.1"); return 0; }
    sk_scene s;
    if (!scene_open(&s, &a, &e)) {
        fprintf(stderr, "soccerkid: %s\n", e.msg);
        return 1;
    }
    int rc = 0;
    if (a.screenshot) {
        uint32_t *out = calloc(SK_SCREEN_W * SK_SCREEN_H, sizeof *out);
        scene_draw(&s);
        scene_present(&s, out);
        if (!png_write(a.screenshot, SK_SCREEN_W, SK_SCREEN_H, out, &e)) {
            fprintf(stderr, "soccerkid: %s\n", e.msg);
            rc = 1;
        }
        free(out);
    } else {
        sk_window w = {0};
        if (!window_open(&w, a.scale, &e)) {
            fprintf(stderr, "soccerkid: %s\n", e.msg);
            rc = 1;
        } else {
            rc = window_run(&w, &s, a.exit_after);
        }
        window_close(&w);
    }
    scene_close(&s);
    return rc;
}
