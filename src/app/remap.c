#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "app/remap.h"
#include "gfx/display_model.h"

#define FONT_8 0x204148u                /* the game's 8x8 font: 16 bytes a glyph, the first of each pair the face */

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

static void text(const sk_image *sk, uint32_t *out, const char *s, int x, int y, uint32_t colour) {
    for (int i = 0; s[i]; i++) {
        int c = toupper((unsigned char)s[i]);
        if (c < 0x20 || c > 0x5F) c = '?';
        for (int r = 0; r < 8 && sk; r++) {
            uint8_t face = image_u8(sk, FONT_8 + (uint32_t)(c - 0x20) * 16u + 2u * (uint32_t)r);
            for (int b = 0; b < 8; b++)
                if ((face >> (7 - b)) & 1) {
                    int px = x + 8 * i + b, py = y + r;
                    if (px >= 0 && px < SK_SCREEN_W && py >= 0 && py < SK_SCREEN_H) out[py * SK_SCREEN_W + px] = colour;
                }
        }
    }
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
    for (int i = 0; i < SK_SCREEN_W * SK_SCREEN_H; i++) out[i] = (out[i] >> 2) & 0x3F3F3F;   /* the game, dimmed */
    const uint32_t white = 0xF0F0F0, yellow = 0xF0D040, grey = 0x909090;
    text(sk, out, "CONTROLS", 128, 24, white);
    text(sk, out, "ACTION   KEY          PAD", 24, 48, grey);
    for (int a = 0; a < SK_ACTIONS; a++) {
        char k[32], p[32], line[64], name[16];
        snprintf(name, sizeof name, "%s", controls_action_name((sk_action)a));
        first_of(r->live, a, true, k, sizeof k);
        first_of(r->live, a, false, p, sizeof p);
        snprintf(line, sizeof line, "%-8.8s %-12.12s %-10.10s", name, k, p);
        text(sk, out, line, 24, 64 + 12 * a, a == r->step ? yellow : white);
    }
    char ask[64]; snprintf(ask, sizeof ask, "PRESS A KEY OR BUTTON FOR %s", controls_action_name((sk_action)r->step));
    text(sk, out, ask, 24, 172, yellow);
    text(sk, out, "BACKSPACE/PAD BACK KEEP F5 RESET ESC CANCEL", 8, 192, grey);
}
