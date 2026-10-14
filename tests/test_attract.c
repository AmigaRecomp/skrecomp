#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/bytes.h"
#include "game/attract.h"
#include "game/attrs.h"
#include "game/front.h"
#include "game/game.h"
#include "game/level_flow.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g; static sk_image img;
static bool make(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    image_free(&img); memset(&img, 0, sizeof img);
    img.count = 2;
    img.seg[0] = (sk_segment){ SK_HUNK_BASE(0), 0x20000, calloc(1, 0x20000), false };
    img.seg[1] = (sk_segment){ SK_HUNK_BASE(1), 0x10000, calloc(1, 0x10000), false };
    uint8_t *d = img.seg[1].mem + 0x934;                         /* the table: "AB" 0x0123456, kits 0 */
    for (int k = 0; k < 10; k++) { memset(d + 16 * k, ' ', 8); put_be32(d + 16 * k + 8, 0x0123456u - (uint32_t)k); }
    d[0] = 'A'; d[1] = 'B'; d[2] = '@';
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    if (!game_setup_level(&g_g, &g_lv, 0)) return false;
    g_g.sk = &img;
    return true;
}
static void drop(void) { game_close(&g_g); level_free(&g_lv); free(g_d); image_free(&img); memset(&img, 0, sizeof img); }
static int tile(int cx, int cy) { return g_g.world.tile[cy * g_g.world.w + cx] & 0x1FF; }

void test_hiscore_rows_are_written_as_tiles(void) {             /* draw_hiscore_rows 0x10583E / draw_hiscore_row 0x10586C */
    CHECK(make());
    hiscore_tiles(&g_g, 2, -1);
    CHECK_EQ(tile(2, 2), 2); CHECK_EQ(tile(3, 2), 3); CHECK_EQ(tile(4, 2), 1); CHECK_EQ(tile(5, 2), 0);   /* A B @ space */
    CHECK_EQ(tile(11, 2), 0x1C + 0); CHECK_EQ(tile(12, 2), 0x1C + 1); CHECK_EQ(tile(17, 2), 0x1C + 6);   /* "0123456" */
    CHECK_EQ(tile(17, 3), 0x1C + 5);                             /* rank 1: 0x0123455 */
    drop();
}

void test_name_entry_scrolls_four_pixels_a_frame_and_types(void) {   /* hiscore_name_input 0x1055E4 */
    CHECK(make());
    sk_name_entry ne;
    name_entry_start(&ne, 0);
    CHECK_EQ((ne.ac + 10) % 29, 10);                             /* 'J' under the marker */
    name_entry_step(&g_g, &ne, SK_IN_RIGHT);                     /* seen: the scroll starts next frame */
    for (int i = 0; i < 4; i++) name_entry_step(&g_g, &ne, 0);
    CHECK_EQ(ne.ac, 1); CHECK_EQ(ne.ae, 0); CHECK_EQ(ne.dir, 0);
    CHECK_EQ(name_entry_step(&g_g, &ne, SK_IN_FIRE), 0);         /* 'K' typed at the cursor */
    CHECK_EQ(image_u8(&img, 0x200934), 'K'); CHECK_EQ(ne.cursor, 1);
    for (int i = 0; i < 10; i++) name_entry_step(&g_g, &ne, SK_IN_FIRE);   /* the repeat delay */
    CHECK_EQ(ne.cursor, 1);
    name_entry_step(&g_g, &ne, SK_IN_FIRE);
    CHECK_EQ(ne.cursor, 2);
    ne.ac = 17; ne.rep = 0;                                      /* (17 + 10) = 27: DEL */
    name_entry_step(&g_g, &ne, SK_IN_FIRE);
    CHECK_EQ(ne.cursor, 1); CHECK_EQ(image_u8(&img, 0x200935), ' ');
    ne.cursor = 0; ne.rep = 0;                                   /* DEL at the first letter: no write before the entry */
    uint8_t before = image_u8(&img, 0x200933);
    name_entry_step(&g_g, &ne, SK_IN_FIRE);
    CHECK_EQ(image_u8(&img, 0x200933), before); CHECK_EQ(ne.cursor, 0);
    ne.ac = 18; ne.rep = 0;                                      /* 28: END */
    CHECK_EQ(name_entry_step(&g_g, &ne, SK_IN_FIRE), -1);
    drop();
}

void test_name_entry_scrolls_left_and_wraps(void) {
    CHECK(make());
    sk_name_entry ne;
    name_entry_start(&ne, 0);
    name_entry_step(&g_g, &ne, SK_IN_LEFT);
    for (int i = 0; i < 4; i++) name_entry_step(&g_g, &ne, 0);
    CHECK_EQ(ne.ac, 28); CHECK_EQ(ne.ae, 0);                     /* 0 - 1 wraps to 28 at the end of the step */
    drop();
}

void test_letter_strip_lands_in_the_hud(void) {                 /* draw_letter_strip 0x10597A */
    CHECK(make());
    uint8_t *font = img.seg[1].mem + 0x7A00;                     /* glyph 10 ('J'): row 0, plane 0 word = 0x8000 */
    put_be16(font + 10 * 0x68 + 2, 0x8000);
    sk_name_entry ne;
    name_entry_start(&ne, 0);
    name_entry_hud(&g_g, &ne);
    CHECK_EQ(g_g.hud.bits[1 * 160 + 0 * 40 + 18], 0x80);         /* glyph AC + 10 at x 144, HUD line 1, plane 0 */
    CHECK_EQ(g_g.hud.bits[15 * 160 + 2 * 40 + 18], 0xFF);        /* the centre marker, line 15 */
    CHECK_EQ(g_g.hud.bits[15 * 160 + 3 * 40 + 18], 0x00);
    drop();
}
