/* Controls (a port addition): which keys, pad buttons and stick directions give the game's five inputs (up, down, left, right,
   fire; JUMP is a second "up") and the pause and back actions, saved as a small text file and changed on the remap screen. */
#ifndef SK_APP_CONTROLS_H
#define SK_APP_CONTROLS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include "core/err.h"

typedef enum { SK_ACT_UP, SK_ACT_DOWN, SK_ACT_LEFT, SK_ACT_RIGHT, SK_ACT_JUMP, SK_ACT_FIRE, SK_ACT_PAUSE, SK_ACT_BACK, SK_ACTIONS } sk_action;
enum { SK_BIND_NONE, SK_BIND_KEY, SK_BIND_BUTTON, SK_BIND_AXIS };
typedef struct { uint8_t kind; int16_t code; int8_t sign; } sk_bind;   /* a scancode, a gamepad button, or an axis and its direction */
#define SK_BINDS 6
typedef struct { sk_bind b[SK_ACTIONS][SK_BINDS]; } sk_controls;
typedef struct { bool present; bool button[SDL_GAMEPAD_BUTTON_COUNT]; int16_t axis[SDL_GAMEPAD_AXIS_COUNT]; } sk_pad;

#define SK_AXIS_THRESHOLD 16000         /* about half way: the stick's dead zone */

void controls_defaults(sk_controls *c);
const char *controls_action_name(sk_action a);                 /* "up" .. "back" */
bool controls_held(const sk_controls *c, sk_action a, const bool *keys, const sk_pad *pad);
uint8_t controls_bits(const sk_controls *c, const bool *keys, const sk_pad *pad);   /* the game's SK_IN_* bits */
/* A new binding for the action: a key replaces its keys, a button or axis its pad bindings. */
void controls_set(sk_controls *c, sk_action a, sk_bind b);
void controls_format(const sk_controls *c, char *out, size_t n);   /* "fire = key:Space, pad:b" lines */
bool controls_parse(sk_controls *c, const char *text, sk_err *e);  /* each known line replaces that action; bad entries skipped */
void controls_bind_name(sk_bind b, char *out, size_t n);           /* "key:Space", "pad:a", "pad:leftx-" */
typedef struct { bool armed[SDL_GAMEPAD_AXIS_COUNT]; } sk_axis_arm;
#define SK_AXIS_REST 8000               /* an axis re-arms only back inside this */
void controls_axes_arm(sk_axis_arm *arm, const int16_t *axes);   /* axes NULL: nothing armed yet */
/* A stick or trigger pushed out: an armed axis past the threshold and larger than its stick's other axis. */
bool controls_axis_press(sk_axis_arm *arm, const int16_t *axes, sk_bind *out);
bool controls_any_held(const bool *keys, const sk_pad *pad);    /* any key, button or axis out of rest */
bool controls_load(sk_controls *c, const char *path);              /* the defaults, then the file; a missing file is written */
bool controls_save(const sk_controls *c, const char *path);

#endif
