/* The HUD panel and the message box, transcribed from the listing (gameplay.md HUD). The glyph cutter hud_glyph
   (0x102A8C) blits with a constant full mask (hud_icon_mask is all ones, BLTALWM 0): the glyph replaces the 16 px
   span it covers, one line below the line it is given. */
#include <string.h>
#include "game/game.h"
#include "game/hud.h"

#define DIGIT_FONT_8 0x204248
#define TIMER_FONT 0x42BA6E
#define HUD_ICONS 0x427DBE
#define HEALTH_ICONS 0x4288BE
#define HUD_PALETTE 0x200E7C
#define BOX_FONT 0x204148
#define BOX_TABLES 0x202484
#define BOX_GLYPH_COLOURS 0x200DE4
#define BOX_FILL_COLOURS 0x200DEC

static const uint8_t health_pairs[16] = { 5, 8, 3, 8, 0, 6, 0, 0, 5, 4, 3, 4, 0, 2, 0, 1 };   /* 0x10292C */

static void glyph(sk_hud *h, const uint8_t *src, int x, int line, int rows) {   /* hud_glyph / hud_glyph_small */
    if (!src) return;
    int col = x >> 4, sh = x & 15;
    for (int r = 0; r < rows; r++) {
        int ln = line + 1 + r;
        if (ln < 0 || ln >= 25) continue;
        for (int p = 0; p < 4; p++) {
            uint16_t v = (uint16_t)(src[(r * 4 + p) * 2] << 8 | src[(r * 4 + p) * 2 + 1]);
            uint32_t data = (uint32_t)v << 16 >> sh, mask = 0xFFFF0000u >> sh;
            uint8_t *row = h->bits + ln * 160 + p * 40;
            for (int b = 0; b < 4; b++) {
                int byte = col * 2 + b;
                if (byte >= 40) break;
                uint8_t m = (uint8_t)(mask >> (24 - 8 * b)), d = (uint8_t)(data >> (24 - 8 * b));
                row[byte] = (uint8_t)((row[byte] & ~m) | (d & m));
            }
        }
    }
}
static void hud_glyph(sk_game *g, int x, int line, int n) { glyph(&g->hud, g->hud.icons ? g->hud.icons + n * 128 : NULL, x, line, 16); }
static void hud_glyph_small(sk_game *g, int x, int line, int n) { glyph(&g->hud, g->hud.health ? g->hud.health + n * 80 : NULL, x, line, 10); }

static void digit8(sk_hud *h, int offset, int d) {               /* 8 rows of one byte, every other font byte */
    if (!h->digits) return;
    for (int r = 0; r < 8; r++) h->bits[offset + r * 160] = h->digits[d * 16 + r * 2];
}

static void hud_lives(sk_game *g) {                              /* 0x1027E8 */
    sk_hud *h = &g->hud;
    for (int r = 0; r < 8; r++) { h->bits[0x846 + r * 160] = 0; h->bits[0x847 + r * 160] = 0; }
    int v = g->player.lives < 0 ? 0 : g->player.lives;
    int off = 0x846;
    if (v / 10) { digit8(h, off, v / 10); off++; }
    digit8(h, off, v % 10);
}

static void hud_score(sk_game *g) {                              /* 0x1028B4: seven digits */
    uint32_t s = g->player.score;
    for (int i = 0; i < 7; i++) digit8(&g->hud, 0xA05 + i, (int)(s >> (4 * (6 - i)) & 15));
}

static void hud_timer(sk_game *g) {                              /* 0x10285E: planes 0, 1 and 3 */
    sk_hud *h = &g->hud;
    if (!h->timer_font) return;
    for (int i = 0; i < 2; i++) {
        int d = (int)(g->level_timer >> (4 * (1 - i)) & 15);
        for (int r = 0; r < 11; r++) {
            uint8_t v = h->timer_font[d * 22 + r * 2];
            int o = 0x1E8 + i + r * 160;
            h->bits[o] = v; h->bits[o + 40] = v; h->bits[o + 120] = v;
        }
    }
}

static void hud_health(sk_game *g) {                             /* 0x1028F2 */
    int x = 8, i = g->player.health;
    if (g->extra_heart) { x = 0; i += 4; }
    i = (i & 7) * 2;
    hud_glyph_small(g, x, 8, health_pairs[i]);
    hud_glyph_small(g, x + 16, 8, health_pairs[i + 1]);
}

static void hud_cards(sk_game *g) {                              /* 0x102954 */
    if (g->level_data_index == 12) return;                       /* the scoreboard's own branch: frontend */
    if (g->all_cards) {
        if (g->frame_ticks & 1) return;
        for (int n = 0; n < 11; n++) {
            int d1 = g->card_anim[n]++;
            if (d1 < 0) continue;
            int gl;
            if (d1 == 1) { g->card_anim[n] = -9; gl = n + 5; } else gl = 4 - d1;
            hud_glyph(g, 0x68 + 16 * n, 4, gl);
        }
        return;
    }
    if (!g->hud_card_flag || (g->frame_ticks & 3)) return;
    int x = 0x68 + 16 * g->cards;
    if (g->card_anim[0] != 3) { hud_glyph(g, x, 4, g->card_anim[0] + 2); g->card_anim[0]++; return; }
    hud_glyph(g, x, 4, g->cards + 5);
    g->card_anim[0] = 0;
    g->cards++;
    g->hud_card_flag = 0;
    g->cards_taken++;
    if (g->cards == 11) {
        g->all_cards = 1;
        for (int n = 0; n < 11; n++) g->card_anim[n] = -n;
    }
}

void hud_init(sk_game *g) {                                      /* hud_clear_bitmap + 0x102676 */
    sk_hud *h = &g->hud;
    memset(h->bits, 0, sizeof h->bits);
    if (g->sk) {
        h->digits = image_ptr(g->sk, DIGIT_FONT_8, 16 * 16);
        h->timer_font = image_ptr(g->sk, TIMER_FONT, 22 * 10);
        h->icons = image_ptr(g->sk, HUD_ICONS, 128 * 16);
        h->health = image_ptr(g->sk, HEALTH_ICONS, 80 * 10);
        h->palette12[0] = 0;
        for (int i = 0; i < 15; i++) h->palette12[1 + i] = image_u16(g->sk, HUD_PALETTE + 2u * (uint32_t)i);
    }
    g->hud_redraw |= 1 | 2 | 4 | 8;
    int x = 0x68, n = 0;
    for (; n < g->cards && n < 11; n++, x += 16) hud_glyph(g, x, 4, 5 + n);
    for (; n < 11; n++, x += 16) hud_glyph(g, x, 4, 1);
    hud_glyph(g, 0x120, 4, 0);
}

void hud_update(sk_game *g) {                                    /* 0x1027AA */
    if (g->hud_redraw & 2) { g->hud_redraw &= ~2; hud_lives(g); }
    if (g->hud_redraw & 4) { g->hud_redraw &= ~4; hud_score(g); }
    if (g->hud_redraw & 1) { g->hud_redraw &= ~1; hud_health(g); }
    if (g->timer_flags & 1) { g->timer_flags &= ~1; hud_timer(g); }
    hud_cards(g);
}

int hud_pixel(const sk_hud *h, int x, int y) {                 /* x up to 351: the fetch runs 4 bytes into the next plane row */
    int v = 0, b = x >> 3, bit = 7 - (x & 7);
    for (int p = 0; p < 4; p++) {
        int o = y * 160 + p * 40 + b;
        if (o >= 0 && o < SK_HUD_BYTES) v |= (h->bits[o] >> bit & 1) << p;
    }
    return v;
}

/* ---- the message box: a w x h five-plane bob (four planes and the mask) ---- */
static void box_glyph(sk_game *g, int byte, int row, int c) {   /* sub_11CABC: an 8 x 8 two-colour character */
    int bpr = g->msg_w >> 3, plane = bpr * g->msg_h;
    const uint8_t *font = image_ptr(g->sk, BOX_FONT + (uint32_t)(((c - 0x20) & 0xFF) * 16), 16);
    uint32_t ct = g->level_data_index == 0x29 ? BOX_GLYPH_COLOURS + 12 : BOX_GLYPH_COLOURS;
    if (!font) return;
    static const int b_index[4] = { 4, 5, 6, 5 };                /* plane 3 uses +5, as the original */
    for (int r = 0; r < 8; r++) {
        uint8_t a = font[r * 2], b = font[r * 2 + 1], keep = (uint8_t)~(a | b);
        int o = (row + r) * bpr + byte;
        if (row + r >= g->msg_h || byte >= bpr) continue;
        for (int p = 0; p < 4; p++) {
            uint8_t *d = &g->msg_bits[p * plane + o];
            *d = (uint8_t)((*d & keep) | (a & image_u8(g->sk, ct + (uint32_t)p)) | (b & image_u8(g->sk, ct + (uint32_t)b_index[p])));
        }
        g->msg_bits[4 * plane + o] |= (uint8_t)(a | b);
    }
}

void message_box_build(sk_game *g, int index) {                 /* show_message_box + draw_box_frame */
    g->msg_w = g->msg_h = 0;
    g->msg_visible = 1;
    if (!g->sk) return;
    uint32_t rec = BOX_TABLES + image_u16(g->sk, BOX_TABLES + (uint32_t)g->level_data_index * 16 + 2u * (uint32_t)index);
    int w = image_u16(g->sk, rec), h = image_u16(g->sk, rec + 2);
    int bpr = (w + 7) >> 3;
    if (w <= 0 || h <= 0 || (size_t)bpr * (size_t)h * 5 > sizeof g->msg_bits) return;
    g->msg_w = bpr * 8; g->msg_h = h;
    int plane = bpr * h;
    memset(g->msg_bits, 0, (size_t)plane * 5);
    uint32_t fill = g->level_data_index == 0x29 ? BOX_FILL_COLOURS + 12 : BOX_FILL_COLOURS;   /* sub_11C998 */
    for (int r = 4; r < h - 4; r++)
        for (int b = 0; b < bpr; b++) {
            uint8_t m = b == 0 ? 0x0F : b == bpr - 1 ? 0xF0 : 0xFF;
            for (int p = 0; p < 4; p++) g->msg_bits[p * plane + r * bpr + b] = (uint8_t)(m & image_u8(g->sk, fill + (uint32_t)p));
            g->msg_bits[4 * plane + r * bpr + b] = m;
        }
    int rows = h >> 3;                                            /* the frame from characters 0x61..0x68 */
    box_glyph(g, 0, 0, 0x61);
    for (int b = 1; b < bpr - 1; b++) box_glyph(g, b, 0, 0x62);
    box_glyph(g, bpr - 1, 0, 0x63);
    for (int r = 1; r < rows - 1; r++) { box_glyph(g, 0, r * 8, 0x68); box_glyph(g, bpr - 1, r * 8, 0x64); }
    box_glyph(g, 0, (rows - 1) * 8, 0x67);
    for (int b = 1; b < bpr - 1; b++) box_glyph(g, b, (rows - 1) * 8, 0x66);
    box_glyph(g, bpr - 1, (rows - 1) * 8, 0x65);
    int line = 1, col = 1, n = 1;                                /* sub_11C95E: the text from (byte 1, row 8) */
    for (uint32_t a = rec + 4;; a++) {
        int c = image_u8(g->sk, a);
        if (!c) break;
        if (c == 0x0D) { n = 1; line++; col = 1; continue; }
        if (++n >= bpr) { n = 2; line++; col = 1; }
        box_glyph(g, col, line * 8, c);
        col++;
    }
}
