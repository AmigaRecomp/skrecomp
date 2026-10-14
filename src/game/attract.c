#include <string.h>
#include "game/attract.h"
#include "game/attrs.h"
#include "game/front.h"
#include "game/game.h"
#include "core/bytes.h"

#define BIG_FONT 0x207A00u              /* 0x68 bytes a glyph: 13 rows x (mask word, three plane words) */
#define STRIP_ROW 392                   /* 4 planes x 98 bytes */

static sk_image *table_image(sk_game *g) { return (sk_image *)g->sk; }   /* default_hiscores is the game's own memory */

static int name_tile(uint8_t c) { return c >= 0x41 ? c - 0x41 + 2 : c == 0x40 ? 1 : 0; }   /* 'e' (the cursor) -> 38 */

void hiscore_tiles(sk_game *g, int c0, int only_rank) {         /* draw_hiscore_rows 0x10583E / draw_hiscore_row 0x10586C */
    if (!g->sk) return;
    for (int k = 0; k < 10; k++) {
        if (only_rank >= 0 && k != only_rank) continue;
        uint32_t e = SK_HISCORES_ADDR + 16u * (uint32_t)k;
        int cy = k + 2;
        for (int i = 0; i < 8; i++) set_cell_tile(g, c0 + i, cy, name_tile(image_u8(g->sk, e + (uint32_t)i)), 0x20);
        uint32_t score = image_u32(g->sk, e + 8);
        for (int i = 0; i < 7; i++) set_cell_tile(g, c0 + 9 + i, cy, 0x1C + (int)((score >> (4 * (6 - i))) & 15), 0x20);
    }
}

void name_entry_start(sk_name_entry *ne, int rank) {            /* attract_hiscores 0x10543E.. (the leftovers cleared: port) */
    memset(ne, 0, sizeof *ne);
    ne->rank = rank;
}

int name_entry_step(sk_game *g, sk_name_entry *ne, int input) {  /* hiscore_name_input 0x1055E4 */
    if (ne->dir == 1) {                                          /* right: the next letter */
        if (ne->ae == 0 && ++ne->ac >= 29) ne->ac = 0;
        ne->ae = (ne->ae - 4) & 15;
        if (ne->ae) goto edit;
        ne->dir = 0;
    } else if (ne->dir == 0xFF) {                                /* left: the previous letter, at the end of the step */
        ne->ae = (ne->ae + 4) & 15;
        if (ne->ae) goto edit;
        ne->dir = 0;
        if (--ne->ac < 0) ne->ac = 28;
    }
    if (input & SK_IN_LEFT) ne->dir = 0xFF;
    if (input & SK_IN_RIGHT) ne->dir = 1;
edit:;
    sk_image *img = table_image(g);
    uint32_t name = SK_HISCORES_ADDR + 16u * (uint32_t)ne->rank;
    uint8_t c = ' ';
    if (img && ne->cursor != 8) {
        if (ne->blink) c = 0x65;                                 /* the cursor tile */
        image_write(img, name + (uint32_t)ne->cursor, &c, 1);
    }
    if (ne->rep) { ne->rep--; return 0; }
    if (!(input & SK_IN_FIRE)) return 0;
    ne->rep = 10;
    int sel = (ne->ac + 10) % 29;
    if (sel == 28) {                                             /* END */
        c = ' ';
        if (img && ne->cursor != 8) image_write(img, name + (uint32_t)ne->cursor, &c, 1);
        return -1;
    }
    c = ' ';
    if (sel == 27) {                                             /* DEL */
        if (img && ne->cursor != 8) image_write(img, name + (uint32_t)ne->cursor, &c, 1);
        if (img && ne->cursor > 0) image_write(img, name + (uint32_t)ne->cursor - 1, &c, 1);   /* port: not before the entry */
        if (--ne->cursor < 0) ne->cursor = 0;
    } else if (ne->cursor != 8) {
        c = (uint8_t)(0x40 + sel);
        if (img) image_write(img, name + (uint32_t)ne->cursor, &c, 1);
        ne->cursor++;
    }
    return 0;
}

static int strip_pixel(const sk_game *g, int X, int row) {     /* build_letter_strip: glyph k at word column k (k 29.. = 0..) */
    int k = X >> 4, bit = 15 - (X & 15);
    if (k < 0 || k > 47 || !g->sk) return 0;
    int glyph = k < 29 ? k : k - 29;
    uint32_t a = BIG_FONT + (uint32_t)glyph * 0x68u + (uint32_t)row * 8u;
    int v = 0;
    for (int p = 0; p < 3; p++) v |= ((image_u16(g->sk, a + 2u + 2u * (uint32_t)p) >> bit) & 1) << p;
    return v;
}

void name_entry_hud(sk_game *g, const sk_name_entry *ne) {      /* hud_clear_bitmap, draw_letter_strip, the marker */
    uint8_t *h = g->hud.bits;
    memset(h, 0, sizeof g->hud.bits);
    for (int y = 1; y <= 13; y++)
        for (int x = 0; x < 304; x++) {
            int v = strip_pixel(g, 16 * ne->ac + 16 - ne->ae + x, y - 1);
            for (int p = 0; p < 3; p++) if ((v >> p) & 1) h[y * 160 + p * 40 + (x >> 3)] |= (uint8_t)(0x80 >> (x & 7));
        }
    for (int p = 0; p < 3; p++) { h[15 * 160 + p * 40 + 18] = 0xFF; h[15 * 160 + p * 40 + 19] = 0xFF; }   /* +0x972 .. */
}
