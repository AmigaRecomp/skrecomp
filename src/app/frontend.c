#include <stdlib.h>
#include <string.h>
#include "app/frontend.h"
#include "core/bytekiller.h"
#include "core/bytes.h"
#include "formats/screens.h"
#include "game/input.h"
#include "gfx/display_model.h"
#include "gfx/palette.h"

#define LANG_PALETTE 0x200DFCu          /* 32 RGB444 words */
#define LANG_ARROW 40000                /* the 32 x 32 arrow: rows of 4 mask bytes and five 4-byte planes */
#define TITLE_PALETTE 0x14000
#define TITLE_KITS 0x14400              /* 14 x 16 (hi, lo) pairs */
#define TITLE_SPARKLE 0x14780           /* 7 frames, 32 x 32, 8 planes row-interleaved */
#define TITLE_ARROW 0x16380             /* 16 x 12: 24 bytes of mask, then 8 planes of 12 rows x 2 bytes */
#define MENU_IDLE 0x2EE                 /* 750 frames */

static const int16_t menu_arrows[3][4] = { { 0xA8, 0x84, 0x10A, 0x84 }, { 0xA8, 0x94, 0x10A, 0x94 }, { 0xB4, 0xA4, 0xFE, 0xA4 } };   /* 0x11CD4C */
static const uint8_t sparkle_seq[11] = { 0, 1, 3, 5, 6, 6, 6, 5, 3, 1, 0 };                                                     /* 0x11CF86 */
static const uint16_t sparkle_waits[4] = { 100, 150, 20, 150 };                                                                    /* 0x11CF7E */

static uint8_t edges(sk_frontend *f, uint8_t in) {              /* input_edges 0x11CEBC */
    uint8_t n = (uint8_t)(in & ~f->prev_input);
    f->prev_input = in;
    return n;
}


/* ---- the language screen ---- */
bool frontend_language_from(sk_frontend *f, const sk_image *sk, const uint8_t *pak, size_t n, sk_err *e) {
    if (n < sizeof f->lang_pic) return sk_fail(e, "language.pak: %zu bytes, %zu needed", n, sizeof f->lang_pic);
    f->mode = SK_FE_LANGUAGE;
    f->sk = sk;
    memcpy(f->lang_pic, pak, sizeof f->lang_pic);
    memset(f->base, 0, sizeof f->base);
    for (int y = 0; y < 200; y++)                                /* 5 planes one after another, 8000 bytes each */
        for (int x = 0; x < 320; x++) {
            int v = 0;
            for (int p = 0; p < 5; p++) v |= ((pak[p * 8000 + y * 40 + (x >> 3)] >> (7 - (x & 7))) & 1) << p;
            f->base[y * 320 + x] = (uint8_t)v;
        }
    memset(f->pal, 0, sizeof f->pal);
    for (int k = 0; k < 32 && sk; k++) f->pal[k] = rgb12_to_xrgb(image_u16(sk, LANG_PALETTE + 2u * (uint32_t)k));
    f->language = 0;
    f->prev_input = 0xFF;                                        /* a held key is not a press */
    return true;
}

static void draw_language(sk_frontend *f) {
    memcpy(f->px, f->base, sizeof f->px);
    int ox = 0x20 + 0x38 * f->language, oy = 32;                 /* the arrow above the panel */
    for (int r = 0; r < 32; r++)
        for (int c = 0; c < 32; c++) {
            const uint8_t *row = f->lang_pic + LANG_ARROW + r * 24;
            int bit = 7 - (c & 7), b = c >> 3;
            if (!((row[b] >> bit) & 1)) continue;
            int v = 0;
            for (int p = 0; p < 5; p++) v |= ((row[4 + 4 * p + b] >> bit) & 1) << p;
            f->px[(oy + r) * 320 + ox + c] = (uint8_t)v;
        }
}

/* ---- the title menu ---- */
static void title_colours(sk_frontend *f) {                      /* set_colour(0, 256), apply_kit_preview, COLOR255 */
    for (int k = 0; k < 256; k++) f->pal[k] = rgb24_from_pair(be16(f->title + TITLE_PALETTE + 4 * k), be16(f->title + TITLE_PALETTE + 4 * k + 2));
    const uint8_t *s = f->title + TITLE_KITS + 64 * f->shirt, *t = f->title + TITLE_KITS + 64 * f->shorts;
#define PAIR(p, k) rgb24_from_pair(be16((p) + 4 * (k)), be16((p) + 4 * (k) + 2))
    for (int k = 0; k < 13; k++) f->pal[0x70 + k] = PAIR(s, k);
    for (int k = 0; k < 4; k++) f->pal[0x6C + k] = PAIR(s, 12 + k);
    static const uint8_t shorts_to[12] = { 0x8B, 0x8A, 0x89, 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81, 0x80 };
    for (int k = 0; k < 12; k++) f->pal[shorts_to[k]] = PAIR(t, k);
    f->pal[0x1E] = PAIR(t, 4); f->pal[0x1D] = PAIR(t, 8); f->pal[0x1F] = PAIR(t, 11);
#undef PAIR
    f->pal[255] = rgb12_to_xrgb(0x048C);                         /* the box colour of the panels */
}

bool frontend_menu_from(sk_frontend *f, const sk_image *sk, const uint8_t *title, size_t n, sk_err *e) {
    if (n < SK_TITLE_SIZE) return sk_fail(e, "file078: %zu bytes, %d needed", n, SK_TITLE_SIZE);
    if (!f->title && !(f->title = malloc(SK_TITLE_SIZE))) return sk_fail(e, "out of memory");
    if (f->title != title) memcpy(f->title, title, SK_TITLE_SIZE);
    sk_screen s;
    if (!screen_parse(f->title, SK_TITLE_SIZE, 320, 256, 8, true, 0, true, &s, e)) return false;
    for (int y = 0; y < 256; y++) for (int x = 0; x < 320; x++) f->base[y * 320 + x] = (uint8_t)screen_pixel(&s, x, y);
    f->mode = SK_FE_MENU;
    f->sk = sk;
    if (f->shirt < 0 || f->shirt >= 14) f->shirt = 0;
    if (f->shorts < 0 || f->shorts >= 14) f->shorts = 0;
    if (f->item < 0 || f->item > 2) f->item = 0;
    title_colours(f);
    f->sparkle_wait = 0x32; f->sparkle_on = false; f->sparkle_step = 0;
    f->attract_counter = 0;
    f->tutorial = f->resume = false;
    f->fire_released = false;
    f->prev_input = 0xFF;                                        /* nothing held counts as a press */
    return true;
}

static void title_sparkle_tick(sk_frontend *f) {                 /* 0x11CEFC */
    if (f->sparkle_on && ++f->sparkle_step == 11) { f->sparkle_step = 0; f->sparkle_on = false; }
    if (--f->sparkle_wait == 0) {
        f->sparkle_phase = (f->sparkle_phase + 1) & 3;
        f->sparkle_wait = sparkle_waits[f->sparkle_phase];
        f->sparkle_on = true;
    }
}

static void draw_title(sk_frontend *f) {
    memcpy(f->px, f->base, sizeof f->px);
    const uint8_t *sp = f->title + TITLE_SPARKLE + 0x400 * sparkle_seq[f->sparkle_step];   /* blit_sparkle at (49, 69) */
    for (int r = 0; r < 32; r++)
        for (int c = 0; c < 32; c++) {
            int v = 0;
            for (int p = 0; p < 8; p++) v |= ((sp[r * 32 + p * 4 + (c >> 3)] >> (7 - (c & 7))) & 1) << p;
            f->px[(69 + r) * 320 + 49 + c] = (uint8_t)v;
        }
    const uint8_t *mask = f->title + TITLE_ARROW, *img = mask + 24;                          /* blit_menu_arrow, twice */
    for (int k = 0; k < 2; k++) {
        int ax = menu_arrows[f->item][2 * k], ay = menu_arrows[f->item][2 * k + 1];
        for (int r = 0; r < 12; r++)
            for (int c = 0; c < 16; c++) {
                int bit = 7 - (c & 7), b = c >> 3;
                if (!((mask[r * 2 + b] >> bit) & 1)) continue;
                int v = 0;
                for (int p = 0; p < 8; p++) v |= ((img[p * 24 + r * 2 + b] >> bit) & 1) << p;
                if (ax + c < 320 && ay + r < 256) f->px[(ay + r) * 320 + ax + c] = (uint8_t)v;
            }
    }
}

static sk_fe_result menu_step(sk_frontend *f, uint8_t input, unsigned keys) {   /* the loop of title_menu */
    title_sparkle_tick(f);                                       /* menu_wait_frame (game_flags set in the frontend) */
    uint8_t n = edges(f, input);
    if (n & SK_IN_UP) { f->attract_counter = 0; if (--f->item < 0) f->item = 2; }
    if (n & SK_IN_DOWN) { f->attract_counter = 0; if (++f->item >= 3) f->item = 0; }
    if (n & SK_IN_FIRE) {
        if (f->item == 1) { f->attract_counter = 0; f->mode = SK_FE_KIT; f->panel_step = 0; return SK_FE_STAY; }
        if (f->item == 2) { f->attract_counter = 0; f->mode = SK_FE_OPTIONS; f->panel_step = 0; f->option = 4; return SK_FE_STAY; }
    }
    if (keys & SK_FE_KEY_QUIT) return SK_FE_QUIT;                /* CTRL + ESC: jmp 0x1001C6 */
    if (f->resume) { f->attract_counter = -1; return SK_FE_RESUME; }
    if (f->tutorial) { f->attract_counter = -1; return SK_FE_TUTORIAL; }
    if (++f->attract_counter >= MENU_IDLE) return SK_FE_IDLE;
    if (!(input & SK_IN_FIRE)) f->fire_released = true;
    if ((input & SK_IN_FIRE) && f->fire_released && f->item == 0) { f->attract_counter = -1; return SK_FE_START; }
    return SK_FE_STAY;
}

sk_fe_result frontend_step(sk_frontend *f, uint8_t input, unsigned keys) {
    switch (f->mode) {
    case SK_FE_LANGUAGE: {
        uint8_t n = edges(f, input);
        if ((n & SK_IN_LEFT) && f->language > 0) f->language--;
        if ((n & SK_IN_RIGHT) && f->language < 4) f->language++;
        if (n & SK_IN_FIRE) return SK_FE_LANGUAGE_CHOSEN;
        return SK_FE_STAY;
    }
    case SK_FE_MENU: return menu_step(f, input, keys);
    default: return SK_FE_STAY;
    }
}

void frontend_present(const sk_frontend *f, uint32_t *out) {
    sk_frontend *m = (sk_frontend *)f;                           /* the frame is composed on the way out */
    if (f->mode == SK_FE_LANGUAGE) draw_language(m);
    else if (f->mode != SK_FE_OFF) draw_title(m);
    for (int i = 0; i < SK_SCREEN_W * SK_SCREEN_H; i++) out[i] = f->pal[f->px[i]];
}

bool frontend_language(sk_frontend *f, const sk_image *sk, sk_disks *d, sk_err *e) {
    uint8_t *raw = NULL, *pak = NULL; size_t rn = 0, pn = 0;
    bool ok = disks_read(d, 0, "language.pak", &raw, &rn, e) && bk_unpack(raw, rn, &pak, &pn, e) && frontend_language_from(f, sk, pak, pn, e);
    free(raw); free(pak);
    return ok;
}

bool frontend_menu(sk_frontend *f, const sk_image *sk, sk_disks *d, sk_err *e) {
    uint8_t *t = NULL; size_t n = 0;
    bool ok = disks_read_file_no(d, 1, 78, true, &t, &n, e) && frontend_menu_from(f, sk, t, n, e);
    free(t);
    return ok;
}

void frontend_free(sk_frontend *f) { free(f->title); f->title = NULL; f->mode = SK_FE_OFF; }
