/* soccerkid: plays a level (default) or runs the level viewer (--viewer). */
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include "game/player.h"
#include "game/world.h"
#include "app/args.h"
#include "app/play.h"
#include "app/scene.h"
#include "app/window.h"
#include "gfx/display_model.h"
#include "gfx/png.h"

static int write_shot(const char *path, uint32_t *out, sk_err *e) {
    if (png_write(path, SK_SCREEN_W, SK_SCREEN_H, out, e)) return 0;
    fprintf(stderr, "soccerkid: %s\n", e->msg);
    return 1;
}

static int run_viewer(const sk_args *a, sk_err *e) {
    sk_scene s;
    if (!scene_open(&s, a, e)) { fprintf(stderr, "soccerkid: %s\n", e->msg); return 1; }
    int rc = 0;
    if (a->screenshot) {
        uint32_t *out = calloc(SK_SCREEN_W * SK_SCREEN_H, sizeof *out);
        scene_draw(&s);
        scene_present(&s, out);
        rc = write_shot(a->screenshot, out, e);
        free(out);
    } else {
        sk_window w = {0};
        if (!window_open(&w, a->scale, e)) { fprintf(stderr, "soccerkid: %s\n", e->msg); rc = 1; }
        else rc = window_run_viewer(&w, &s, a->exit_after);
        window_close(&w);
    }
    scene_close(&s);
    return rc;
}

static int run_play(const sk_args *in, sk_err *e) {
    static sk_play p;
    sk_args args = *in, *a = &args;
    static char save[512];
    static char controls[512];
    if (!a->save_path || !a->controls_path) {                       /* SaveGame and controls.cfg in the user's data folder */
        char *dir = SDL_GetPrefPath("soccerkid", "soccerkid");
        if (dir && !a->save_path) { snprintf(save, sizeof save, "%sSaveGame", dir); a->save_path = save; }
        if (dir && !a->controls_path) { snprintf(controls, sizeof controls, "%scontrols.cfg", dir); a->controls_path = controls; }
        SDL_free(dir);
    }
    if (!play_open(&p, a, e)) { fprintf(stderr, "soccerkid: %s\n", e->msg); return 1; }
    int rc = 0;
    if (a->markers) {                                               /* --markers: the level's objects, left to right */
        const sk_world *w = &p.game.world;
        printf("level %d: %d x %d cells; type param  cell (x, y)  pixel (x, y)\n", a->level, w->w, w->h);
        for (int cx = 0; cx < w->w; cx++)
            for (int cy = 0; cy < w->h; cy++) {
                int param = 0, type = world_marker_type(w, cx, cy, &param);
                if (type > 0) printf("  %3d  0x%02X  (%d, %d)  (%d, %d)\n", type, param & 0xFF, cx, cy, cx * 16, cy * 16);
            }
        play_close(&p);
        return 0;
    }
    if (a->screenshot || a->frames) {
        for (int i = 0; i < a->frames; i++) play_step(&p, 0);
        if (a->screenshot) {
            uint32_t *out = calloc(SK_SCREEN_W * SK_SCREEN_H, sizeof *out);
            play_draw(&p);
            play_present(&p, out);
            rc = write_shot(a->screenshot, out, e);
            free(out);
        }
    } else {
        sk_window w = {0};
        if (!window_open(&w, a->scale, e)) { fprintf(stderr, "soccerkid: %s\n", e->msg); rc = 1; }
        else rc = window_run(&w, &p, a->exit_after);
        window_close(&w);
    }
    play_close(&p);
    return rc;
}

int main(int argc, char **argv) {
    sk_args a;
    sk_err e = {0};
    if (!args_parse(argc, argv, &a, &e)) {
        fprintf(stderr, "soccerkid: %s\n%s", e.msg, args_usage());
        return 2;
    }
    if (a.help) { fputs(args_usage(), stdout); return 0; }
    if (a.version) { puts("soccerkid 0.2"); return 0; }
    return a.viewer ? run_viewer(&a, &e) : run_play(&a, &e);
}
