#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "app/remap.h"
#include "gfx/display_model.h"

#define FONT_8 0x204148u                /* the game's 8x8 font: 16 bytes a glyph, the first of each pair the face */

const sk_remap_line remap_help[SK_REMAP_HELP_LINES] = {
    { 40, 222, "BACKSPACE / PAD BACK" },
    { 40, 238, "F5" } };

void remap_open(sk_remap *r, sk_controls *live) {
    r->open = true; r->step = 0; r->live = live; r->before = *live;
}

sk_remap_result remap_input(sk_remap *r, sk_bind b) {
    if (!r->open) return SK_REMAP_STAY;
    if (b.kind == SK_BIND_KEY && b.code == SDL_SCANCODE_ESCAPE) { *r->live = r->before; r->open = false; return SK_REMAP_CANCEL; }
    if (b.kind == SK_BIND_KEY && b.code == SDL_SCANCODE_F5) { controls_defaults(r->live); r->open = false; return SK_REMAP_DONE; }
    if (b.kind == SK_BIND_KEY && (b.code == SDL_SCANCODE_F12 || b.code == SDL_SCANCODE_F || b.code == SDL_SCANCODE_M ||
                                  b.code == SDL_SCANCODE_TAB || (b.code >= SDL_SCANCODE_F1 && b.code <= SDL_SCANCODE_F10)))
        return SK_REMAP_STAY;                                    /* the fixed keys keep their own meaning */
    bool keep = (b.kind == SK_BIND_KEY && b.code == SDL_SCANCODE_BACKSPACE) || (b.kind == SK_BIND_BUTTON && b.code == SDL_GAMEPAD_BUTTON_BACK);
    if (!keep) controls_set(r->live, (sk_action)r->step, b);
    if (++r->step < SK_ACTIONS) return SK_REMAP_STAY;
    r->open = false;
    return SK_REMAP_DONE;
}

static void text_scaled(const sk_image *sk, uint32_t *out, const char *s, int x, int y, uint32_t colour, int scale) {
    for (int i = 0; s[i]; i++) {
        int c = toupper((unsigned char)s[i]);
        if (c < 0x20 || c > 0x5F) c = '?';
        for (int r = 0; r < 8 && sk; r++) {
            uint8_t face = image_u8(sk, FONT_8 + (uint32_t)(c - 0x20) * 16u + 2u * (uint32_t)r);
            for (int b = 0; b < 8; b++)
                if ((face >> (7 - b)) & 1)
                    for (int dy = 0; dy < scale; dy++)
                        for (int dx = 0; dx < scale; dx++) {
                            int px = x + (8 * i + b) * scale + dx, py = y + r * scale + dy;
                            if (px >= 0 && px < SK_SCREEN_W && py >= 0 && py < SK_SCREEN_H) out[py * SK_SCREEN_W + px] = colour;
                        }
        }
    }
}
static void text(const sk_image *sk, uint32_t *out, const char *s, int x, int y, uint32_t colour) { text_scaled(sk, out, s, x, y, colour, 1); }
static void centred(const sk_image *sk, uint32_t *out, const char *s, int y, uint32_t colour, int scale) {
    text_scaled(sk, out, s, (SK_SCREEN_W - 8 * scale * (int)strlen(s)) / 2, y, colour, scale);
}
static void fill(uint32_t *out, int x0, int y0, int w, int h, uint32_t colour) {
    for (int y = y0; y < y0 + h; y++) for (int x = x0; x < x0 + w; x++)
        if (x >= 0 && x < SK_SCREEN_W && y >= 0 && y < SK_SCREEN_H) out[y * SK_SCREEN_W + x] = colour;
}

static void first_of(const sk_controls *c, int a, bool keys, char *out, size_t n) {
    out[0] = 0;
    for (int i = 0; i < SK_BINDS; i++) {
        sk_bind b = c->b[a][i];
        if (b.kind == SK_BIND_NONE || (b.kind == SK_BIND_KEY) != keys) continue;
        char t[48]; controls_bind_name(b, t, sizeof t);
        snprintf(out, n, "%s", t + 4);                               /* without "key:" / "pad:" */
        return;
    }
    snprintf(out, n, "-");
}

void remap_draw(const sk_remap *r, const sk_image *sk, uint32_t *out) {
    if (!r->open) return;
    const uint32_t bg = 0x101828, bar = 0x2A3A6A, white = 0xF0F0F0, yellow = 0xF0D040, grey = 0x8890A0;
    fill(out, 0, 0, SK_SCREEN_W, SK_SCREEN_H, bg);               /* a plain screen: the text needs the contrast */
    centred(sk, out, "CONTROLS", 10, yellow, 2);
    text(sk, out, "ACTION", 32, 38, grey); text(sk, out, "KEY", 120, 38, grey); text(sk, out, "PAD", 224, 38, grey);
    fill(out, 24, 49, 272, 1, grey);
    for (int a = 0; a < SK_ACTIONS; a++) {
        int y = 56 + 16 * a;
        bool cur = a == r->step;
        if (cur) { fill(out, 20, y - 4, 280, 16, bar); text(sk, out, ">", 22, y, yellow); }
        char k[32], p[32], name[16];
        snprintf(name, sizeof name, "%s", controls_action_name((sk_action)a));
        first_of(r->live, a, true, k, sizeof k);
        first_of(r->live, a, false, p, sizeof p);
        k[12] = 0; p[9] = 0;                                     /* the columns' widths */
        uint32_t c = cur ? yellow : white;
        text(sk, out, name, 32, y, c); text(sk, out, k, 120, y, c); text(sk, out, p, 224, y, c);
    }
    centred(sk, out, "PRESS A KEY OR BUTTON FOR", 186, white, 1);
    centred(sk, out, controls_action_name((sk_action)r->step), 198, yellow, 2);
    text(sk, out, remap_help[0].text, remap_help[0].x, remap_help[0].y, white);
    text(sk, out, "KEEP", 216, remap_help[0].y, grey);
    text(sk, out, remap_help[1].text, remap_help[1].x, remap_help[1].y, white);
    text(sk, out, "DEFAULTS", 64, remap_help[1].y, grey);
    text(sk, out, "ESC", 168, remap_help[1].y, white);
    text(sk, out, "CANCEL", 200, remap_help[1].y, grey);
}
