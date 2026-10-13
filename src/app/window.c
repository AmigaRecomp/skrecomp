#include <stdio.h>
#include <stdlib.h>
#include "app/window.h"
#include "gfx/display_model.h"

#define STEP_NS 20000000ull         /* 50 Hz */
#define MAX_CATCH_UP 5
#define SCROLL_PX 4                 /* camera pixels per step while an arrow is held */

void window_step_camera(sk_scene *s, int dx, int dy, int step) {
    int x = s->cam_x + dx * step, y = s->cam_y + dy * step;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > s->max_cam_x) x = s->max_cam_x;
    if (y > s->max_cam_y) y = s->max_cam_y;
    s->cam_x = x;
    s->cam_y = y;
}

bool window_open(sk_window *w, int scale, sk_err *e) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return sk_fail(e, "SDL: %s", SDL_GetError());
    w->win = SDL_CreateWindow("Soccer Kid", SK_SCREEN_W * scale, SK_SCREEN_H * scale, SDL_WINDOW_RESIZABLE);
    if (!w->win) return sk_fail(e, "SDL window: %s", SDL_GetError());
    w->ren = SDL_CreateRenderer(w->win, NULL);
    if (!w->ren) return sk_fail(e, "SDL renderer: %s", SDL_GetError());
    SDL_SetRenderVSync(w->ren, 1);
    SDL_SetRenderLogicalPresentation(w->ren, SK_SCREEN_W, SK_SCREEN_H, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    w->tex = SDL_CreateTexture(w->ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, SK_SCREEN_W, SK_SCREEN_H);
    if (!w->tex) return sk_fail(e, "SDL texture: %s", SDL_GetError());
    SDL_SetTextureScaleMode(w->tex, SDL_SCALEMODE_NEAREST);
    w->pixels = calloc((size_t)SK_SCREEN_W * SK_SCREEN_H, sizeof *w->pixels);
    if (!w->pixels) return sk_fail(e, "out of memory");
    w->frames_presented = 0;
    return true;
}

static void change_level(sk_scene *s, int delta) {
    sk_err e = {0};
    int idx = s->level_index + delta;
    while (idx >= 0 && idx < SK_LEVEL_RECORDS) {
        if (scene_load_level(s, idx, &e)) return;
        fprintf(stderr, "soccerkid: %s\n", e.msg);          /* skip entries that are not tile levels */
        idx += delta;
    }
}

static void change_section(sk_scene *s, int delta) {
    if (!s->has_sections) return;
    sk_err e = {0};
    int sec = s->section + delta;
    while (sec >= 0 && sec < s->sections.count) {
        if (scene_load_section(s, s->section_name, sec, 0, 0, &e)) return;
        sec += delta;
    }
}

/* Quit and one-shot keys: polled every iteration so the window stays responsive. */
static bool poll_events(sk_window *w, sk_scene *s) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT) return false;
        if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
            switch (ev.key.key) {
            case SDLK_ESCAPE: return false;
            case SDLK_PAGEUP: change_level(s, -1); break;
            case SDLK_PAGEDOWN: change_level(s, 1); break;
            case SDLK_LEFTBRACKET: change_section(s, -1); break;
            case SDLK_RIGHTBRACKET: change_section(s, 1); break;
            case SDLK_F: {
                bool full = (SDL_GetWindowFlags(w->win) & SDL_WINDOW_FULLSCREEN) != 0;
                SDL_SetWindowFullscreen(w->win, !full);
                break;
            }
            default: break;
            }
        }
    }
    return true;
}

/* Held keys: applied once per fixed step, so the scroll speed is 50 Hz whatever the display does. */
static void step_held_keys(sk_scene *s) {
    const bool *keys = SDL_GetKeyboardState(NULL);
    int dx = (keys[SDL_SCANCODE_RIGHT] ? 1 : 0) - (keys[SDL_SCANCODE_LEFT] ? 1 : 0);
    int dy = (keys[SDL_SCANCODE_DOWN] ? 1 : 0) - (keys[SDL_SCANCODE_UP] ? 1 : 0);
    if (dx || dy) window_step_camera(s, dx, dy, SCROLL_PX);
}

int window_steps_for(uint64_t *acc_ns, uint64_t elapsed_ns) {
    *acc_ns += elapsed_ns;
    int steps = 0;
    while (*acc_ns >= STEP_NS && steps < MAX_CATCH_UP) {
        *acc_ns -= STEP_NS;
        steps++;
    }
    if (*acc_ns > STEP_NS) *acc_ns = STEP_NS;
    return steps;
}

int window_run(sk_window *w, sk_scene *s, int exit_after) {
    uint64_t last = SDL_GetTicksNS(), acc = 0;
    bool running = true;
    while (running) {
        uint64_t now = SDL_GetTicksNS();
        int steps = window_steps_for(&acc, now - last);
        last = now;
        running = poll_events(w, s);
        for (int i = 0; i < steps; i++) step_held_keys(s);
        scene_draw(s);
        scene_present(s, w->pixels);
        SDL_UpdateTexture(w->tex, NULL, w->pixels, SK_SCREEN_W * (int)sizeof *w->pixels);
        SDL_RenderClear(w->ren);
        SDL_RenderTexture(w->ren, w->tex, NULL, NULL);
        SDL_RenderPresent(w->ren);
        w->frames_presented++;
        if (exit_after > 0 && w->frames_presented >= exit_after) break;
        if (steps == 0) SDL_Delay(1);
    }
    return 0;
}

void window_close(sk_window *w) {
    free(w->pixels);
    if (w->tex) SDL_DestroyTexture(w->tex);
    if (w->ren) SDL_DestroyRenderer(w->ren);
    if (w->win) SDL_DestroyWindow(w->win);
    SDL_Quit();
    w->win = NULL; w->ren = NULL; w->tex = NULL; w->pixels = NULL;
}
