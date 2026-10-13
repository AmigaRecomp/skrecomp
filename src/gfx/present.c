#include <stdbool.h>
#include <stdlib.h>
#include "gfx/present.h"
#include "game/hud.h"
#include "gfx/display_model.h"

static inline uint32_t resolve(const sk_palette *pal, uint8_t v, uint32_t c17, uint32_t c18, int has_sky) {
#if SK_DUAL_PLAYFIELD
    int pf1 = v & 15, pf2 = v >> 4;
    if (pf1) return pal->xrgb[pf1];
    if (has_sky) {
        if (pf2 == 1) return c17;
        if (pf2 == 2) return c18;
    }
    return pal->xrgb[SK_PF2_COLOUR_BASE + pf2];
#else
    (void)c17; (void)c18; (void)has_sky;
    return pal->xrgb[v];
#endif
}

static uint32_t sky_word(const uint16_t *t, int len, int i) { return rgb12_to_xrgb(t[i < len ? i : len - 1]); }

void present_rows(const sk_fb *fb, const sk_palette *pal, const sk_sky *sky,
                  int src_x, int src_y, int w, int h, uint32_t *out) {
    bool has = sky && sky->c17 && sky->c17_len > 0 && sky->c18 && sky->c18_len > 0;
    for (int y = 0; y < h; y++) {
        uint32_t c17 = 0, c18 = 0;
        if (has) {
            c17 = sky_word(sky->c17, sky->c17_len, sky->c17_off + y);
            c18 = sky_word(sky->c18, sky->c18_len, sky->c18_off + y);
        }
        const uint8_t *row = fb->px + (size_t)(src_y + y) * (size_t)fb->w + src_x;
        uint32_t *dst = out + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) dst[x] = resolve(pal, row[x], c17, c18, has);
    }
}

void present_screen(const sk_fb *fb, const sk_palette *pal, const sk_sky *sky, const sk_sprite_layer *spr, int curtain_rows,
                    const struct sk_hud *hud, uint32_t *out) {
    for (size_t i = 0; i < (size_t)SK_SCREEN_W * SK_SCREEN_H; i++) out[i] = pal->xrgb[0];
    static uint32_t window[SK_VIEW_W * SK_VIEW_H];
    present_rows(fb, pal, sky, SK_VIEW_X, 0, SK_VIEW_W, SK_VIEW_H, window);
    for (int k = 0; spr && k < 2; k++) {
        const sk_sprite *sp = &spr->spr[k == 0 ? 1 - (spr->front & 1) : (spr->front & 1)];   /* the back one first */
        if (!sp->visible) continue;
        for (int r = 0; r < SK_SPR_H; r++) {
            int wy = sp->y + r;
            if (wy < 0 || wy >= SK_VIEW_H) continue;
            for (int cx = 0; cx < SK_SPR_W; cx++) {
                int wx = sp->x - SK_VIEW_X + cx;
                if (wx < 0 || wx >= SK_VIEW_W) continue;
                uint8_t v = sp->px[r * SK_SPR_W + cx];
                if (!v) continue;
                if (!sp->in_front && (fb->px[(size_t)wy * (size_t)fb->w + (size_t)(wx + SK_VIEW_X)] & 15)) continue;
                window[(size_t)wy * SK_VIEW_W + (size_t)wx] = pal->xrgb[SK_SPR_COLOUR_BASE + v];
            }
        }
    }
    if (curtain_rows > SK_VIEW_H / 2) curtain_rows = SK_VIEW_H / 2;
    for (int y = 0; y < curtain_rows; y++)
        for (int x = 0; x < SK_VIEW_W; x++) window[(size_t)y * SK_VIEW_W + x] = window[(size_t)(SK_VIEW_H - 1 - y) * SK_VIEW_W + x] = pal->xrgb[0];
    for (int y = 0; y < SK_VIEW_H; y++) {
        uint32_t *dst = out + (size_t)(SK_PLAY_Y + y) * SK_SCREEN_W + SK_PLAY_X;
        const uint32_t *src = window + (size_t)y * SK_VIEW_W;
        for (int x = 0; x < SK_VIEW_W; x++) dst[x] = src[x];
    }
    if (!hud) return;
    uint32_t colours[16];
    for (int i = 0; i < 16; i++) colours[i] = i ? rgb12_to_xrgb(hud->palette12[i]) : pal->xrgb[0];
    for (int y = 0; y < SK_HUD_H; y++)
        for (int x = 0; x < SK_SCREEN_W; x++)                     /* DDFSTRT 0x30: panel pixel x + 16 shows at frame x (as the play area) */
            out[(size_t)(SK_HUD_Y + y) * SK_SCREEN_W + x] = colours[hud_pixel(hud, x + SK_HUD_FETCH_X, y)];
}
