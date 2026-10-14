#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/controls.h"
#include "game/input.h"

static const char *names[SK_ACTIONS] = { "up", "down", "left", "right", "jump", "fire", "pause", "back" };
const char *controls_action_name(sk_action a) { return a >= 0 && a < SK_ACTIONS ? names[a] : ""; }

static void add(sk_controls *c, sk_action a, sk_bind b) {
    for (int i = 0; i < SK_BINDS; i++) if (c->b[a][i].kind == SK_BIND_NONE) { c->b[a][i] = b; return; }
}
static sk_bind key(SDL_Scancode s) { sk_bind b = { SK_BIND_KEY, (int16_t)s, 0 }; return b; }
static sk_bind button(SDL_GamepadButton s) { sk_bind b = { SK_BIND_BUTTON, (int16_t)s, 0 }; return b; }
static sk_bind axis(SDL_GamepadAxis s, int sign) { sk_bind b = { SK_BIND_AXIS, (int16_t)s, (int8_t)sign }; return b; }

void controls_defaults(sk_controls *c) {
    memset(c, 0, sizeof *c);
    add(c, SK_ACT_UP, key(SDL_SCANCODE_UP)); add(c, SK_ACT_UP, key(SDL_SCANCODE_W));
    add(c, SK_ACT_UP, button(SDL_GAMEPAD_BUTTON_DPAD_UP)); add(c, SK_ACT_UP, axis(SDL_GAMEPAD_AXIS_LEFTY, -1));
    add(c, SK_ACT_DOWN, key(SDL_SCANCODE_DOWN)); add(c, SK_ACT_DOWN, key(SDL_SCANCODE_S));
    add(c, SK_ACT_DOWN, button(SDL_GAMEPAD_BUTTON_DPAD_DOWN)); add(c, SK_ACT_DOWN, axis(SDL_GAMEPAD_AXIS_LEFTY, 1));
    add(c, SK_ACT_LEFT, key(SDL_SCANCODE_LEFT)); add(c, SK_ACT_LEFT, key(SDL_SCANCODE_A));
    add(c, SK_ACT_LEFT, button(SDL_GAMEPAD_BUTTON_DPAD_LEFT)); add(c, SK_ACT_LEFT, axis(SDL_GAMEPAD_AXIS_LEFTX, -1));
    add(c, SK_ACT_RIGHT, key(SDL_SCANCODE_RIGHT)); add(c, SK_ACT_RIGHT, key(SDL_SCANCODE_D));
    add(c, SK_ACT_RIGHT, button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT)); add(c, SK_ACT_RIGHT, axis(SDL_GAMEPAD_AXIS_LEFTX, 1));
    add(c, SK_ACT_JUMP, button(SDL_GAMEPAD_BUTTON_SOUTH));
    add(c, SK_ACT_FIRE, key(SDL_SCANCODE_SPACE)); add(c, SK_ACT_FIRE, key(SDL_SCANCODE_LCTRL)); add(c, SK_ACT_FIRE, key(SDL_SCANCODE_X));
    add(c, SK_ACT_FIRE, button(SDL_GAMEPAD_BUTTON_EAST)); add(c, SK_ACT_FIRE, button(SDL_GAMEPAD_BUTTON_WEST));
    add(c, SK_ACT_PAUSE, key(SDL_SCANCODE_F10)); add(c, SK_ACT_PAUSE, key(SDL_SCANCODE_P)); add(c, SK_ACT_PAUSE, button(SDL_GAMEPAD_BUTTON_START));
    add(c, SK_ACT_BACK, button(SDL_GAMEPAD_BUTTON_BACK));
}

static bool bind_held(sk_bind b, const bool *keys, const sk_pad *pad) {
    switch (b.kind) {
    case SK_BIND_KEY: return keys && b.code >= 0 && b.code < SDL_SCANCODE_COUNT && keys[b.code];
    case SK_BIND_BUTTON: return pad && pad->present && b.code >= 0 && b.code < SDL_GAMEPAD_BUTTON_COUNT && pad->button[b.code];
    case SK_BIND_AXIS:
        if (!pad || !pad->present || b.code < 0 || b.code >= SDL_GAMEPAD_AXIS_COUNT) return false;
        return b.sign < 0 ? pad->axis[b.code] <= -SK_AXIS_THRESHOLD : pad->axis[b.code] >= SK_AXIS_THRESHOLD;
    default: return false;
    }
}

bool controls_held(const sk_controls *c, sk_action a, const bool *keys, const sk_pad *pad) {
    for (int i = 0; i < SK_BINDS; i++) if (bind_held(c->b[a][i], keys, pad)) return true;
    return false;
}

uint8_t controls_bits(const sk_controls *c, const bool *keys, const sk_pad *pad) {
    uint8_t in = 0;
    if (controls_held(c, SK_ACT_UP, keys, pad) || controls_held(c, SK_ACT_JUMP, keys, pad)) in |= SK_IN_UP;
    if (controls_held(c, SK_ACT_DOWN, keys, pad)) in |= SK_IN_DOWN;
    if (controls_held(c, SK_ACT_LEFT, keys, pad)) in |= SK_IN_LEFT;
    if (controls_held(c, SK_ACT_RIGHT, keys, pad)) in |= SK_IN_RIGHT;
    if (controls_held(c, SK_ACT_FIRE, keys, pad)) in |= SK_IN_FIRE;
    return in;
}

static bool same(sk_bind x, sk_bind y) { return x.kind == y.kind && x.code == y.code && (x.kind != SK_BIND_AXIS || x.sign == y.sign); }

void controls_set(sk_controls *c, sk_action a, sk_bind b) {
    for (int o = 0; o < SK_ACTIONS; o++) {                       /* one input, one action (up and jump may share) */
        if (o == (int)a || ((o == SK_ACT_UP || o == SK_ACT_JUMP) && (a == SK_ACT_UP || a == SK_ACT_JUMP))) continue;
        sk_bind keep[SK_BINDS]; int n = 0;
        for (int i = 0; i < SK_BINDS; i++) if (c->b[o][i].kind != SK_BIND_NONE && !same(c->b[o][i], b)) keep[n++] = c->b[o][i];
        memset(c->b[o], 0, sizeof c->b[o]);
        for (int i = 0; i < n; i++) c->b[o][i] = keep[i];
    }
    bool key_kind = b.kind == SK_BIND_KEY;
    sk_bind keep[SK_BINDS]; int n = 0;
    for (int i = 0; i < SK_BINDS; i++) {
        sk_bind o = c->b[a][i];
        if (o.kind == SK_BIND_NONE || (o.kind == SK_BIND_KEY) == key_kind) continue;
        keep[n++] = o;
    }
    memset(c->b[a], 0, sizeof c->b[a]);
    for (int i = 0; i < n; i++) c->b[a][i] = keep[i];
    add(c, a, b);
}

void controls_bind_name(sk_bind b, char *out, size_t n) {
    switch (b.kind) {
    case SK_BIND_KEY: {
        const char *name = SDL_GetScancodeName((SDL_Scancode)b.code);
        if (!name[0] || strchr(name, ',') || strchr(name, '#')) snprintf(out, n, "key:#%d", b.code);   /* the file's separators */
        else snprintf(out, n, "key:%s", name);
        break;
    }
    case SK_BIND_BUTTON: snprintf(out, n, "pad:%s", SDL_GetGamepadStringForButton((SDL_GamepadButton)b.code)); break;
    case SK_BIND_AXIS: snprintf(out, n, "pad:%s%c", SDL_GetGamepadStringForAxis((SDL_GamepadAxis)b.code), b.sign < 0 ? '-' : '+'); break;
    default: if (n) out[0] = 0; break;
    }
}

void controls_format(const sk_controls *c, char *out, size_t n) {
    size_t used = 0;
    if (n) out[0] = 0;
    for (int a = 0; a < SK_ACTIONS; a++) {
        int w = snprintf(out + used, n - used, "%s =", names[a]);
        if (w < 0 || (size_t)w >= n - used) return;
        used += (size_t)w;
        bool first = true;
        for (int i = 0; i < SK_BINDS; i++) {
            if (c->b[a][i].kind == SK_BIND_NONE) continue;
            char t[64];
            controls_bind_name(c->b[a][i], t, sizeof t);
            w = snprintf(out + used, n - used, "%s %s", first ? "" : ",", t);
            if (w < 0 || (size_t)w >= n - used) return;
            used += (size_t)w;
            first = false;
        }
        w = snprintf(out + used, n - used, "\n");
        if (w < 0 || (size_t)w >= n - used) return;
        used += (size_t)w;
    }
}

static void trim(char *s) {
    char *p = s; while (*p == ' ' || *p == '\t') p++;
    memmove(s, p, strlen(p) + 1);
    size_t l = strlen(s); while (l && (s[l - 1] == ' ' || s[l - 1] == '\t' || s[l - 1] == '\r')) s[--l] = 0;
}

static bool parse_bind(const char *t, sk_bind *b) {
    if (!strncmp(t, "key:#", 5)) {
        int code = atoi(t + 5);
        if (code <= 0 || code >= SDL_SCANCODE_COUNT) return false;
        *b = key((SDL_Scancode)code); return true;
    }
    if (!strncmp(t, "key:", 4)) {
        SDL_Scancode sc = SDL_GetScancodeFromName(t + 4);
        if (sc == SDL_SCANCODE_UNKNOWN) return false;
        *b = key(sc); return true;
    }
    if (strncmp(t, "pad:", 4)) return false;
    char name[48]; snprintf(name, sizeof name, "%s", t + 4);
    size_t l = strlen(name);
    if (l && (name[l - 1] == '-' || name[l - 1] == '+')) {
        int sign = name[l - 1] == '-' ? -1 : 1;
        name[l - 1] = 0;
        SDL_GamepadAxis ax = SDL_GetGamepadAxisFromString(name);
        if (ax == SDL_GAMEPAD_AXIS_INVALID) return false;
        *b = axis(ax, sign); return true;
    }
    SDL_GamepadButton bt = SDL_GetGamepadButtonFromString(name);
    if (bt == SDL_GAMEPAD_BUTTON_INVALID) return false;
    *b = button(bt); return true;
}

bool controls_parse(sk_controls *c, const char *text, sk_err *e) {
    (void)e;
    char line[512];
    if (!strncmp(text, "\xEF\xBB\xBF", 3)) text += 3;              /* a UTF-8 byte order mark */
    while (*text) {
        size_t l = strcspn(text, "\n");
        bool too_long = l >= sizeof line;
        snprintf(line, sizeof line, "%.*s", (int)(too_long ? 0 : l), text);
        text += l + (text[l] == '\n');
        if (too_long) continue;                                  /* skipped whole: a cut entry could read as another */
        char *eq = strchr(line, '=');
        if (line[0] == '#' || !eq) continue;
        *eq = 0;
        char name[32]; snprintf(name, sizeof name, "%s", line); trim(name);
        int a = -1;
        for (int k = 0; k < SK_ACTIONS; k++) if (!strcmp(name, names[k])) a = k;
        if (a < 0) continue;
        sk_bind got[SK_BINDS]; int n = 0;
        for (char *tok = strtok(eq + 1, ","); tok; tok = strtok(NULL, ",")) {
            char t[64]; snprintf(t, sizeof t, "%s", tok); trim(t);
            sk_bind b;
            if (n < SK_BINDS && parse_bind(t, &b)) got[n++] = b;
        }
        if (!n) continue;                                        /* nothing valid: the action keeps what it had */
        memset(c->b[a], 0, sizeof c->b[a]);
        for (int i = 0; i < n; i++) c->b[a][i] = got[i];
    }
    return true;
}
bool controls_save(const sk_controls *c, const char *path) {
    char text[4096];
    controls_format(c, text, sizeof text);
    FILE *f = path ? fopen(path, "w") : NULL;
    if (!f) return false;
    bool ok = fputs("# Soccer Kid controls: action = key:<name>, pad:<button>, pad:<axis>+ or - (F12 in the game remaps)\n", f) >= 0 &&
              fputs(text, f) >= 0;
    return fclose(f) == 0 && ok;
}

bool controls_load(sk_controls *c, const char *path) {
    controls_defaults(c);
    FILE *f = path ? fopen(path, "r") : NULL;
    if (!f) return controls_save(c, path);
    char text[8192];
    size_t n = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[n] = 0;
    sk_err e = {0};
    return controls_parse(c, text, &e);
}
static int iabs(int v) { return v < 0 ? -v : v; }
static int partner(int a) {                                      /* the other axis of the same stick, -1 for a trigger */
    switch (a) {
    case SDL_GAMEPAD_AXIS_LEFTX: return SDL_GAMEPAD_AXIS_LEFTY;
    case SDL_GAMEPAD_AXIS_LEFTY: return SDL_GAMEPAD_AXIS_LEFTX;
    case SDL_GAMEPAD_AXIS_RIGHTX: return SDL_GAMEPAD_AXIS_RIGHTY;
    case SDL_GAMEPAD_AXIS_RIGHTY: return SDL_GAMEPAD_AXIS_RIGHTX;
    default: return -1;
    }
}

void controls_axes_arm(sk_axis_arm *arm, const int16_t *axes) {
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; a++) arm->armed[a] = axes && iabs(axes[a]) < SK_AXIS_REST;
}

bool controls_axis_press(sk_axis_arm *arm, const int16_t *axes, sk_bind *out) {
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; a++) if (iabs(axes[a]) < SK_AXIS_REST) arm->armed[a] = true;
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; a++) {
        if (!arm->armed[a] || iabs(axes[a]) < SK_AXIS_THRESHOLD) continue;
        int o = partner(a);
        if (o >= 0 && iabs(axes[o]) >= iabs(axes[a])) continue;  /* the stick's dominant direction only */
        arm->armed[a] = false;
        if (o >= 0) arm->armed[o] = false;
        *out = axis((SDL_GamepadAxis)a, axes[a] < 0 ? -1 : 1);
        return true;
    }
    return false;
}

bool controls_any_held(const bool *keys, const sk_pad *pad) {
    for (int k = 0; keys && k < SDL_SCANCODE_COUNT; k++) if (keys[k]) return true;
    if (!pad || !pad->present) return false;
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) if (pad->button[b]) return true;
    for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; a++) if (iabs(pad->axis[a]) >= SK_AXIS_REST) return true;
    return false;
}
