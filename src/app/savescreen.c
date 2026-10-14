#include <stdlib.h>
#include <string.h>
#include "app/savescreen.h"
#include "core/bytekiller.h"
#include "core/bytes.h"
#include "game/input.h"
#include "gfx/display_model.h"
#include "gfx/palette.h"

#define SAVE_PICTURE 0x20A20Au          /* ByteKiller: the length word 0x718 and its payload */
#define SAVE_PALETTE 0x200E5Cu          /* 16 RGB444 words */
#define SAVE_STRINGS 0x203B2Cu          /* 5 languages x 0x93 bytes: seven 21-byte lines */
#define SELECT_HEADING 0x11E148u        /* "SELECT SAVE GAME", English only */
#define BIG_FONT 0x207A00u
#define FONT_8 0x204148u
#define TOP 58                          /* DIW lines 102..191, the frame starts at line 44 */

bool savescreen_open(sk_savescreen *s, sk_save_mode mode, const sk_image *sk, int language, const uint8_t *picture, size_t n,
                     char rows[5][24], sk_err *e) {
    memset(s, 0, sizeof *s);
    uint8_t *own = NULL;
    if (!picture && sk) {
        const uint8_t *c = image_ptr(sk, SAVE_PICTURE, 4);
        size_t len = c ? 4 + be32(c) : 0;
        if (!c || !image_ptr(sk, SAVE_PICTURE, len) || !bk_unpack(c, len, &own, &n, e)) return sk_fail(e, "sk: the save picture");
        picture = own;
    }
    if (!picture || n < SK_SAVE_PICTURE_SIZE) { free(own); return sk_fail(e, "save picture: %zu bytes, %d needed", n, SK_SAVE_PICTURE_SIZE); }
    memcpy(s->pic, picture, SK_SAVE_PICTURE_SIZE);
    free(own);
    s->sk = sk;
    s->language = language >= 0 && language < 5 ? language : 0;
    for (int k = 0; k < 16 && sk; k++) s->pal[k] = rgb12_to_xrgb(image_u16(sk, SAVE_PALETTE + 2u * (uint32_t)k));
    memcpy(s->rows, rows, sizeof s->rows);
    s->resume = mode == SK_SAVE_RESUME;
    s->mode = s->resume ? SK_SAVE_LIST : SK_SAVE_ASK;
    s->wait_release = s->resume;                                 /* level_session 0x104F2E: fire let go before resume_game_screen */
    s->prev_input = 0xFF;
    return true;
}

static void set_px(sk_savescreen *s, int x, int y, int v) { if (x >= 0 && x < 320 && y >= 0 && y < 90) s->px[y * 320 + x] = (uint8_t)v; }

static void draw_big(sk_savescreen *s, const char *t, int x, int y) {   /* draw_text_big 0x11E400 */
    for (int i = 0; t[i]; i++, x += 16) {
        int c = (uint8_t)t[i];
        if (c < 0x40 || !s->sk) continue;                        /* a blank cell */
        uint32_t a = BIG_FONT + (uint32_t)(c - 0x40) * 0x68u;
        for (int r = 0; r < 13; r++) {
            uint16_t mask = image_u16(s->sk, a + (uint32_t)r * 8), p0 = image_u16(s->sk, a + (uint32_t)r * 8 + 2);
            for (int b = 0; b < 16; b++) {
                int px = x + b, py = y + r;
                if (px >= 320 || py >= 90) continue;
                int v = s->px[py * 320 + px];
                if (!((mask >> (15 - b)) & 1)) v = 0;            /* planes &= mask */
                if ((p0 >> (15 - b)) & 1) v |= 3;                /* plane0 |= p0, plane1 |= p0 */
                s->px[py * 320 + px] = (uint8_t)v;
            }
        }
    }
}

static void draw_small(sk_savescreen *s, const char *t, int x, int y, int colour) {   /* draw_text_4bit 0x11E15A */
    for (int i = 0; t[i]; i++)
        for (int r = 0; r < 8 && s->sk; r++) {
            uint8_t c = (uint8_t)t[i], g = c >= 0x20 ? image_u8(s->sk, FONT_8 + (uint32_t)(c - 0x20) * 16u + 2u * (uint32_t)r) : 0;
            for (int b = 0; b < 8; b++) if ((g >> (7 - b)) & 1) set_px(s, x + 8 * i + b, y + r, colour);
        }
}

static void draw_button(sk_savescreen *s, int x, int y, int k) {  /* draw_save_button 0x11E2B0 */
    const uint8_t *b = s->pic + 0x3840 + 0x280 * k;
    for (int r = 0; r < 32; r++)
        for (int c = 0; c < 32; c++) {
            int bit = 7 - (c & 7), o = r * 20 + (c >> 3);
            if (!((b[o] >> bit) & 1)) continue;
            int v = 0;
            for (int p = 0; p < 4; p++) v |= ((b[o + 4 + 4 * p] >> bit) & 1) << p;
            set_px(s, x + c, y + r, v);
        }
}

static void line(const sk_savescreen *s, int k, char *out) {    /* save_strings: 20 characters */
    out[0] = 0;
    if (!s->sk) return;
    for (int i = 0; i < 20; i++) out[i] = (char)image_u8(s->sk, SAVE_STRINGS + 0x93u * (uint32_t)s->language + 21u * (uint32_t)k + (uint32_t)i);
    out[20] = 0;
}

static void compose(sk_savescreen *s) {                         /* save_screen_clear, then the screen's drawing */
    for (int y = 0; y < 90; y++)
        for (int x = 0; x < 320; x++) {
            int v = 0;
            for (int p = 0; p < 4; p++) v |= ((s->pic[y * 160 + p * 40 + (x >> 3)] >> (7 - (x & 7))) & 1) << p;
            s->px[y * 320 + x] = (uint8_t)v;
        }
    char t[24];
    switch (s->mode) {
    case SK_SAVE_ASK:
        line(s, 0, t); draw_big(s, t, 0, 10);
        draw_button(s, 104, 40, s->tick ? 1 : 3);
        draw_button(s, 200, 40, s->tick ? 2 : 0);
        break;
    case SK_SAVE_LIST: case SK_SAVE_TYPE: {                   /* slot_list_draw 0x11E104 */
        char h[17] = "SELECT SAVE GAME";
        if (s->sk) { for (int i = 0; i < 16; i++) h[i] = (char)image_u8(s->sk, SELECT_HEADING + (uint32_t)i); h[16] = 0; }
        draw_big(s, h, 32, 10);
        for (int k = 0; k < 5; k++) draw_small(s, s->rows[k], 88, 40 + 8 * k, k == s->slot ? 3 : 8);
        break;
    }
    case SK_SAVE_SAVING: line(s, 1, t); draw_big(s, t, 0, 32); break;
    default: break;
    }
}

sk_save_result savescreen_step(sk_savescreen *s, uint8_t input, int key, bool esc) {
    uint8_t edge = (uint8_t)(input & ~s->prev_input);
    s->prev_input = input;
    switch (s->mode) {
    case SK_SAVE_ASK:
        if (s->wait_release) {
            if (input & SK_IN_FIRE) return SK_SAVE_RESULT_STAY;
            s->wait_release = false; s->mode = SK_SAVE_LIST; s->slot = 0; s->prev_input = input;
            return SK_SAVE_RESULT_STAY;
        }
        if (input & SK_IN_LEFT) s->tick = 1;
        if (input & SK_IN_RIGHT) s->tick = 0;
        if (input & SK_IN_FIRE) {
            if (!s->tick) return SK_SAVE_RESULT_NONE;            /* the cross: at once, nothing saved */
            s->wait_release = true;
        }
        return SK_SAVE_RESULT_STAY;
    case SK_SAVE_LIST:                                          /* save_slot_select 0x11E014 */
        if (s->wait_release) { if (input & SK_IN_FIRE) return SK_SAVE_RESULT_STAY; s->wait_release = false; }
        if (edge & SK_IN_UP) s->slot = s->slot == 0 ? 4 : s->slot - 1;
        if (edge & SK_IN_DOWN) s->slot = s->slot == 4 ? 0 : s->slot + 1;
        if (esc) return SK_SAVE_RESULT_NONE;
        if (!(input & SK_IN_FIRE)) return SK_SAVE_RESULT_STAY;
        if (s->resume) return SK_SAVE_RESULT_LOAD;
        memset(s->rows[s->slot] + 3, ' ', 16);
        s->typed = 0;
        s->mode = SK_SAVE_TYPE;                                  /* port: a stale Return is not taken (last_ascii_key cleared) */
        return SK_SAVE_RESULT_STAY;
    case SK_SAVE_TYPE:
        if (key >= 0x20 && key <= 0x64 && s->typed < 16) s->rows[s->slot][3 + s->typed++] = (char)key;
        else if (key == 8) { if (s->typed > 0) s->rows[s->slot][3 + --s->typed] = ' '; }
        else if (key == 0x0D) {
            memcpy(s->name, s->rows[s->slot] + 3, 16); s->name[16] = 0;
            for (int i = 15; i >= 0 && s->name[i] == ' '; i--) s->name[i] = 0;
            s->mode = SK_SAVE_SAVING; s->saving = 50;
        }
        return SK_SAVE_RESULT_STAY;
    case SK_SAVE_SAVING:
        return --s->saving > 0 ? SK_SAVE_RESULT_STAY : SK_SAVE_RESULT_SAVE;
    default: return SK_SAVE_RESULT_NONE;
    }
}

void savescreen_present(const sk_savescreen *s, uint32_t *out) {
    compose((sk_savescreen *)s);
    for (int i = 0; i < SK_SCREEN_W * SK_SCREEN_H; i++) out[i] = 0;
    for (int y = 0; y < 90; y++) for (int x = 0; x < 320; x++) out[(TOP + y) * SK_SCREEN_W + x] = s->pal[s->px[y * 320 + x]];
}
