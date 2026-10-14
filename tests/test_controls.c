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

void test_controls_set_takes_the_input_from_other_actions(void) {
    sk_controls c; controls_defaults(&c);
    controls_set(&c, SK_ACT_BACK, (sk_bind){ SK_BIND_KEY, SDL_SCANCODE_SPACE, 0 });   /* Space was fire */
    memset(keys, 0, sizeof keys); memset(&pad, 0, sizeof pad);
    keys[SDL_SCANCODE_SPACE] = true;
    CHECK(controls_held(&c, SK_ACT_BACK, keys, &pad));
    CHECK(!controls_held(&c, SK_ACT_FIRE, keys, &pad));          /* one input, one action */
    controls_set(&c, SK_ACT_JUMP, (sk_bind){ SK_BIND_KEY, SDL_SCANCODE_UP, 0 });       /* up and jump may share */
    keys[SDL_SCANCODE_SPACE] = false; keys[SDL_SCANCODE_UP] = true;
    CHECK(controls_held(&c, SK_ACT_UP, keys, &pad)); CHECK(controls_held(&c, SK_ACT_JUMP, keys, &pad));
}

void test_controls_file_keeps_comma_keys_bad_lines_and_a_bom(void) {
    sk_controls c; controls_defaults(&c);
    controls_set(&c, SK_ACT_FIRE, (sk_bind){ SK_BIND_KEY, SDL_SCANCODE_COMMA, 0 });
    char text[2048]; controls_format(&c, text, sizeof text);
    sk_controls d; controls_defaults(&d); sk_err e = {0};
    CHECK(controls_parse(&d, text, &e));
    memset(keys, 0, sizeof keys); memset(&pad, 0, sizeof pad); keys[SDL_SCANCODE_COMMA] = true;
    CHECK(controls_held(&d, SK_ACT_FIRE, keys, &pad));           /* the comma key survives */
    controls_defaults(&d);
    CHECK(controls_parse(&d, "\xEF\xBB\xBF" "fire = key:Spacebar\njump = key:J\n", &e));   /* a BOM, a typo */
    memset(keys, 0, sizeof keys); keys[SDL_SCANCODE_SPACE] = true;
    CHECK(controls_held(&d, SK_ACT_FIRE, keys, &pad));           /* no valid entry: fire keeps its bindings */
    keys[SDL_SCANCODE_SPACE] = false; keys[SDL_SCANCODE_J] = true;
    CHECK(controls_held(&d, SK_ACT_JUMP, keys, &pad));           /* the BOM did not hide the first line's neighbour */
    static char longline[1200];
    memset(longline, ' ', sizeof longline - 20); memcpy(longline, "fire = pad:back", 15); strcpy(longline + sizeof longline - 20, "\n");
    controls_defaults(&d);
    CHECK(controls_parse(&d, longline, &e));
    memset(keys, 0, sizeof keys); keys[SDL_SCANCODE_SPACE] = true;
    CHECK(controls_held(&d, SK_ACT_FIRE, keys, &pad));           /* an overlong line is skipped whole */
}

void test_controls_stick_press_needs_a_dominant_axis_and_a_return(void) {
    sk_axis_arm arm; controls_axes_arm(&arm, NULL);              /* nothing armed until seen at rest */
    int16_t ax[SDL_GAMEPAD_AXIS_COUNT] = { 0 };
    sk_bind b;
    ax[SDL_GAMEPAD_AXIS_LEFTX] = -30000;
    CHECK(!controls_axis_press(&arm, ax, &b));                   /* held when the screen opened */
    ax[SDL_GAMEPAD_AXIS_LEFTX] = 0;
    CHECK(!controls_axis_press(&arm, ax, &b));                   /* back at rest: armed */
    ax[SDL_GAMEPAD_AXIS_LEFTX] = -24000; ax[SDL_GAMEPAD_AXIS_LEFTY] = -26000;   /* a diagonal, mostly up */
    CHECK(controls_axis_press(&arm, ax, &b));
    CHECK_EQ(b.code, SDL_GAMEPAD_AXIS_LEFTY); CHECK_EQ(b.sign, -1);
    CHECK(!controls_axis_press(&arm, ax, &b));                   /* not again until it returns */
    ax[SDL_GAMEPAD_AXIS_LEFTY] = -12000; ax[SDL_GAMEPAD_AXIS_LEFTX] = 0;
    CHECK(!controls_axis_press(&arm, ax, &b));                   /* drift around the threshold: not re-armed above 8000 */
    ax[SDL_GAMEPAD_AXIS_LEFTY] = -17000;
    CHECK(!controls_axis_press(&arm, ax, &b));
}
