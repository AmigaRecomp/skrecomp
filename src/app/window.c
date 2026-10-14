#include "app/audio_out.h"
#include <stdio.h>
#include <stdlib.h>
#include "app/window.h"
#include "app/frontend.h"
#include "game/input.h"
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
    SDL_SetHint(SDL_HINT_FORCE_RAISEWINDOW, "1");                /* started from a terminal: take the focus */
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "1");
    if (!SDL_Init(SDL_INIT_VIDEO)) return sk_fail(e, "SDL: %s", SDL_GetError());
    w->win = SDL_CreateWindow("Soccer Kid", SK_SCREEN_W * scale, SK_SCREEN_H * scale, SDL_WINDOW_RESIZABLE);
    if (!w->win) return sk_fail(e, "SDL window: %s", SDL_GetError());
    SDL_RaiseWindow(w->win);
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

uint8_t window_input_bits(const bool *keys) {
    uint8_t in = 0;
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) in |= SK_IN_UP;
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) in |= SK_IN_DOWN;
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) in |= SK_IN_LEFT;
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) in |= SK_IN_RIGHT;
    if (keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_X]) in |= SK_IN_FIRE;
    return in;
}

unsigned window_frontend_keys(const bool *keys) {
    static const SDL_Scancode f[10] = { SDL_SCANCODE_F1, SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4, SDL_SCANCODE_F5,
                                        SDL_SCANCODE_F6, SDL_SCANCODE_F7, SDL_SCANCODE_F8, SDL_SCANCODE_F9, SDL_SCANCODE_F10 };
    unsigned k = keys[SDL_SCANCODE_TAB] ? SK_FE_KEY_TAB : 0;
    for (int i = 0; i < 10; i++) if (keys[f[i]]) k |= SK_FE_KEY_F1 << i;
    return k;
}

int window_typed_key(SDL_Keycode k) {
    if (k >= SDLK_A && k <= SDLK_Z) return 'A' + (int)(k - SDLK_A);   /* the keymap gives uppercase only */
    if (k >= SDLK_0 && k <= SDLK_9) return '0' + (int)(k - SDLK_0);
    switch (k) {
    case SDLK_SPACE: return ' ';
    case SDLK_MINUS: return '-';
    case SDLK_PERIOD: return '.';
    case SDLK_BACKSPACE: case SDLK_DELETE: return 8;
    case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0D;
    default: return 0;
    }
}

static void present_pixels(sk_window *w) {
    SDL_UpdateTexture(w->tex, NULL, w->pixels, SK_SCREEN_W * (int)sizeof *w->pixels);
    SDL_RenderClear(w->ren);
    SDL_RenderTexture(w->ren, w->tex, NULL, NULL);
    SDL_RenderPresent(w->ren);
    w->frames_presented++;
}

/* Quit and one-shot keys: polled every iteration so the window stays responsive. s may be NULL (play mode); p is the
   game (NULL in the viewer): a save screen takes the typed keys and Escape. */
static bool poll_events(sk_window *w, sk_scene *s, sk_play *p) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT) return false;
        if (p && p->intro && ((ev.type == SDL_EVENT_KEY_DOWN && ev.key.key != SDLK_ESCAPE && ev.key.key != SDLK_F && ev.key.key != SDLK_M) ||
                                ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN)) {
            p->typed_key = 1;                                   /* frame_interrupt: any key, the mouse button or fire */
            continue;
        }
        if (ev.type == SDL_EVENT_KEY_DOWN && p && p->save.mode != SK_SAVE_OFF) {
            if (ev.key.key == SDLK_ESCAPE) { p->esc_key = true; continue; }
            int c = window_typed_key(ev.key.key);
            if (c) { p->typed_key = c; continue; }
        }
        if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
            switch (ev.key.key) {
            case SDLK_ESCAPE: if (!p) return false; p->esc_key = true; break;
            case SDLK_F10: case SDLK_P: if (p) p->pause_key = true; break;
            case SDLK_M: if (w->audio) audio_out_mute(w->audio, !w->audio->muted); break;
            case SDLK_PAGEUP: if (s) change_level(s, -1); break;
            case SDLK_PAGEDOWN: if (s) change_level(s, 1); break;
            case SDLK_LEFTBRACKET: if (s) change_section(s, -1); break;
            case SDLK_RIGHTBRACKET: if (s) change_section(s, 1); break;
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

int window_run_viewer(sk_window *w, sk_scene *s, int exit_after) {
    uint64_t last = SDL_GetTicksNS(), acc = 0;
    bool running = true;
    while (running) {
        uint64_t now = SDL_GetTicksNS();
        int steps = window_steps_for(&acc, now - last);
        last = now;
        running = poll_events(w, s, NULL);
        for (int i = 0; i < steps; i++) step_held_keys(s);
        scene_draw(s);
        scene_present(s, w->pixels);
        present_pixels(w);
        if (exit_after > 0 && w->frames_presented >= exit_after) break;
        if (steps == 0) SDL_Delay(1);
    }
    return 0;
}

int window_run(sk_window *w, sk_play *p, int exit_after) {
    static sk_audio_out audio;
    if (audio_out_open(&audio, 48000)) { p->audio = &audio; w->audio = &audio; }   /* no device: silent */
    uint64_t last = SDL_GetTicksNS(), acc = 0;
    bool running = true;
    while (running) {
        uint64_t now = SDL_GetTicksNS();
        int steps = window_steps_for(&acc, now - last);
        last = now;
        running = poll_events(w, NULL, p);
        for (int i = 0; i < steps; i++) {
            p->fe_keys = window_frontend_keys(SDL_GetKeyboardState(NULL));
            play_step(p, window_input_bits(SDL_GetKeyboardState(NULL)));
        }
        if (p->load_failed) { fprintf(stderr, "soccerkid: %s\n", p->load_err.msg); running = false; }
        if (p->quit) running = false;
        play_draw(p);
        play_present(p, w->pixels);
        present_pixels(w);
        if (exit_after > 0 && w->frames_presented >= exit_after) break;
        if (steps == 0) SDL_Delay(1);
    }
    p->audio = NULL; w->audio = NULL;
    audio_out_close(&audio);
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
