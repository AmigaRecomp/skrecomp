/* SDL3 window: a 320x256 streaming texture presented with integer scaling, a fixed 50 Hz step. */
#ifndef SK_APP_WINDOW_H
#define SK_APP_WINDOW_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include "app/play.h"
#include "app/scene.h"
#include "core/err.h"

typedef struct {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    uint32_t *pixels;           /* SK_SCREEN_W * SK_SCREEN_H */
    int frames_presented;
    struct sk_audio_out *audio;  /* play mode: M mutes it */
} sk_window;

bool window_open(sk_window *w, int scale, sk_err *e);
/* Plays until Escape or the window closes; exit_after > 0 stops after that many presented frames. Returns 0. */
int window_run(sk_window *w, sk_play *p, int exit_after);
/* The level viewer (arrows scroll, PageUp/PageDown change level, [ ] change the section). */
int window_run_viewer(sk_window *w, sk_scene *s, int exit_after);
/* The joystick bits held on the keyboard: arrows or WASD, Space / Left Ctrl / X = fire (pure, for tests). */
uint8_t window_input_bits(const bool *keys);
void window_close(sk_window *w);

/* Moves the camera by (dx, dy) * step pixels, clamped to the scene's limits (pure, for tests). */
void window_step_camera(sk_scene *s, int dx, int dy, int step);

/* Fixed-step pacing: adds elapsed_ns to the accumulator and returns how many 20 ms steps to run
   (at most 5; a longer backlog is dropped so a stall never turns into a burst). Pure, for tests. */
int window_steps_for(uint64_t *acc_ns, uint64_t elapsed_ns);

#endif
