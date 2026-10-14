#include <string.h>
#include "check.h"
#include "app/remap.h"

static sk_bind k(SDL_Scancode s) { sk_bind b = { SK_BIND_KEY, (int16_t)s, 0 }; return b; }
static sk_bind btn(SDL_GamepadButton s) { sk_bind b = { SK_BIND_BUTTON, (int16_t)s, 0 }; return b; }

void test_remap_walks_the_actions_and_sets_each(void) {
    sk_controls c; controls_defaults(&c);
    sk_remap r; remap_open(&r, &c);
    CHECK(r.open); CHECK_EQ(r.step, SK_ACT_UP);
    for (int i = 0; i < 5; i++) CHECK_EQ(remap_input(&r, k(SDL_SCANCODE_BACKSPACE)), SK_REMAP_STAY);   /* keep up..jump */
    CHECK_EQ(r.step, SK_ACT_FIRE);
    CHECK_EQ(remap_input(&r, k(SDL_SCANCODE_J)), SK_REMAP_STAY);
    CHECK_EQ(remap_input(&r, btn(SDL_GAMEPAD_BUTTON_NORTH)), SK_REMAP_STAY);   /* pause on Y */
    CHECK_EQ(remap_input(&r, k(SDL_SCANCODE_BACKSPACE)), SK_REMAP_DONE);      /* back kept: the end */
    CHECK(!r.open);
    bool keys[SDL_SCANCODE_COUNT] = { false }; sk_pad pad = { 0 };
    keys[SDL_SCANCODE_J] = true;
    CHECK(controls_held(&c, SK_ACT_FIRE, keys, &pad));
    pad.present = true; pad.button[SDL_GAMEPAD_BUTTON_NORTH] = true; keys[SDL_SCANCODE_J] = false;
    CHECK(controls_held(&c, SK_ACT_PAUSE, keys, &pad));
    pad.button[SDL_GAMEPAD_BUTTON_NORTH] = false; pad.button[SDL_GAMEPAD_BUTTON_START] = true;
    CHECK(!controls_held(&c, SK_ACT_PAUSE, keys, &pad));         /* Start replaced */
}

void test_remap_escape_cancels_and_f5_restores_defaults(void) {
    sk_controls c, d; controls_defaults(&c); controls_defaults(&d);
    sk_remap r; remap_open(&r, &c);
    remap_input(&r, k(SDL_SCANCODE_Q));                          /* up = Q */
    CHECK_EQ(remap_input(&r, k(SDL_SCANCODE_ESCAPE)), SK_REMAP_CANCEL);
    char a[2048], b[2048]; controls_format(&c, a, sizeof a); controls_format(&d, b, sizeof b);
    CHECK(strcmp(a, b) == 0);                                    /* as before */
    controls_set(&c, SK_ACT_FIRE, k(SDL_SCANCODE_Z));
    remap_open(&r, &c);
    CHECK_EQ(remap_input(&r, k(SDL_SCANCODE_F5)), SK_REMAP_DONE);
    controls_format(&c, a, sizeof a);
    CHECK(strcmp(a, b) == 0);                                    /* the defaults again */
}

void test_remap_ignores_the_fixed_keys(void) {
    sk_controls c; controls_defaults(&c);
    sk_remap r; remap_open(&r, &c);
    static const SDL_Scancode fixed[] = { SDL_SCANCODE_F, SDL_SCANCODE_M, SDL_SCANCODE_TAB, SDL_SCANCODE_F1, SDL_SCANCODE_F10, SDL_SCANCODE_F12 };
    for (size_t i = 0; i < sizeof fixed / sizeof *fixed; i++) CHECK_EQ(remap_input(&r, k(fixed[i])), SK_REMAP_STAY);
    CHECK_EQ(r.step, SK_ACT_UP);                                 /* none of them was taken */
    CHECK_EQ(remap_input(&r, btn(SDL_GAMEPAD_BUTTON_BACK)), SK_REMAP_STAY);
    CHECK_EQ(r.step, SK_ACT_DOWN);                               /* the pad's Back keeps, as Backspace */
}
