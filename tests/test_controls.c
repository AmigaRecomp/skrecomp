#include <string.h>
#include "check.h"
#include "app/controls.h"
#include "game/input.h"

static bool keys[SDL_SCANCODE_COUNT];
static sk_pad pad;

void test_controls_defaults_map_keys_and_pad_to_the_five_inputs(void) {
    sk_controls c; controls_defaults(&c);
    memset(keys, 0, sizeof keys); memset(&pad, 0, sizeof pad);
    CHECK_EQ(controls_bits(&c, keys, &pad), 0);
    keys[SDL_SCANCODE_D] = true; keys[SDL_SCANCODE_SPACE] = true;
    CHECK_EQ(controls_bits(&c, keys, &pad), SK_IN_RIGHT | SK_IN_FIRE);
    memset(keys, 0, sizeof keys);
    pad.present = true;
    pad.button[SDL_GAMEPAD_BUTTON_SOUTH] = true;                 /* A: jump, which is the game's up */
    pad.axis[SDL_GAMEPAD_AXIS_LEFTX] = -30000;                   /* the stick left */
    CHECK_EQ(controls_bits(&c, keys, &pad), SK_IN_UP | SK_IN_LEFT);
    pad.axis[SDL_GAMEPAD_AXIS_LEFTX] = -8000;                    /* inside the dead zone */
    pad.button[SDL_GAMEPAD_BUTTON_SOUTH] = false; pad.button[SDL_GAMEPAD_BUTTON_EAST] = true;
    CHECK_EQ(controls_bits(&c, keys, &pad), SK_IN_FIRE);
    pad.button[SDL_GAMEPAD_BUTTON_EAST] = false; pad.button[SDL_GAMEPAD_BUTTON_START] = true;
    CHECK(controls_held(&c, SK_ACT_PAUSE, keys, &pad));
    pad.present = false;                                          /* unplugged: its state counts for nothing */
    CHECK(!controls_held(&c, SK_ACT_PAUSE, keys, &pad));
}

void test_controls_file_round_trips_and_keeps_defaults_for_unknown_lines(void) {
    sk_controls c, d; controls_defaults(&c);
    controls_set(&c, SK_ACT_FIRE, (sk_bind){ SK_BIND_KEY, SDL_SCANCODE_K, 0 });
    controls_set(&c, SK_ACT_JUMP, (sk_bind){ SK_BIND_AXIS, SDL_GAMEPAD_AXIS_RIGHTY, -1 });
    char text[2048];
    controls_format(&c, text, sizeof text);
    CHECK(strstr(text, "fire = pad:b, pad:x, key:K\n") != NULL);   /* the kept pad buttons, then the new key */
    sk_err e = {0};
    controls_defaults(&d);
    CHECK(controls_parse(&d, text, &e));
    char again[2048];
    controls_format(&d, again, sizeof again);
    CHECK(strcmp(text, again) == 0);
    controls_defaults(&d);
    CHECK(controls_parse(&d, "# a comment\nnonsense line\nfire = key:Nope, pad:b\n", &e));   /* bad entries are skipped */
    memset(keys, 0, sizeof keys); memset(&pad, 0, sizeof pad); pad.present = true; pad.button[SDL_GAMEPAD_BUTTON_EAST] = true;
    CHECK(controls_held(&d, SK_ACT_FIRE, keys, &pad));
    keys[SDL_SCANCODE_SPACE] = true; pad.present = false;
    CHECK(!controls_held(&d, SK_ACT_FIRE, keys, &pad));          /* the line replaced fire's bindings */
}

void test_controls_set_replaces_one_device_and_keeps_the_other(void) {
    sk_controls c; controls_defaults(&c);
    controls_set(&c, SK_ACT_FIRE, (sk_bind){ SK_BIND_KEY, SDL_SCANCODE_J, 0 });
    memset(keys, 0, sizeof keys); memset(&pad, 0, sizeof pad);
    keys[SDL_SCANCODE_SPACE] = true;
    CHECK(!controls_held(&c, SK_ACT_FIRE, keys, &pad));          /* the old keys are gone */
    keys[SDL_SCANCODE_J] = true;
    CHECK(controls_held(&c, SK_ACT_FIRE, keys, &pad));
    memset(keys, 0, sizeof keys); pad.present = true; pad.button[SDL_GAMEPAD_BUTTON_EAST] = true;
    CHECK(controls_held(&c, SK_ACT_FIRE, keys, &pad));           /* the pad's B still fires */
}

#include <stdio.h>
#include <stdlib.h>
void test_controls_file_is_created_with_the_defaults_and_read_back(void) {
    char path[256];
    snprintf(path, sizeof path, "%s/sk_test_controls.cfg", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp");
    remove(path);
    sk_controls c;
    CHECK(controls_load(&c, path));                              /* missing: the defaults, written out */
    FILE *f = fopen(path, "r"); CHECK(f != NULL); if (f) fclose(f);
    controls_set(&c, SK_ACT_FIRE, (sk_bind){ SK_BIND_KEY, SDL_SCANCODE_K, 0 });
    CHECK(controls_save(&c, path));
    sk_controls d;
    CHECK(controls_load(&d, path));
    memset(keys, 0, sizeof keys); memset(&pad, 0, sizeof pad);
    keys[SDL_SCANCODE_K] = true;
    CHECK(controls_held(&d, SK_ACT_FIRE, keys, &pad));
    remove(path);
    CHECK(!controls_save(&c, "/nonexistent/dir/controls.cfg"));
}
