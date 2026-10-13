/* SDL3 window: a 320x256 streaming texture presented with integer scaling, a fixed 50 Hz step. */
#ifndef SK_APP_WINDOW_H
#define SK_APP_WINDOW_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include "app/scene.h"
#include "core/err.h"

typedef struct {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    uint32_t *pixels;           /* SK_SCREEN_W * SK_SCREEN_H */
    int frames_presented;
} sk_window;

bool window_open(sk_window *w, int scale, sk_err *e);
/* Runs until Escape or the window closes; exit_after > 0 stops after that many presented frames. Returns 0. */
int window_run(sk_window *w, sk_scene *s, int exit_after);
void window_close(sk_window *w);

/* Moves the camera by (dx, dy) * step pixels, clamped to the scene's limits (pure, for tests). */
void window_step_camera(sk_scene *s, int dx, int dy, int step);

#endif
