#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "gfx/display_model.h"
#include "gfx/palette.h"
#include "gfx/sprites.h"

static bool load_sk(sk_disks *d, sk_image *img) {
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) return false;
    sk_err e = {0}; uint8_t *f; size_t n;
    if (!disks_open(d, roms, &e) || !disks_read(d, 0, "sk.pak", &f, &n, &e)) return false;
    bool ok = hunk_load(f, n, img, &e); free(f); return ok;
}

void test_kid_frame_decodes_from_the_chip_hunk(void) {          /* ROM-backed */
    sk_disks d = {0}; sk_image img = {0};
    if (!load_sk(&d, &img)) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    uint32_t off = image_u32(&img, SK_PLAYER_ANIM_TABLE + 8 * 0x11);
    static sk_sprite s;
    memset(&s, 0, sizeof s);
    CHECK(sprite_decode_pair(&img, SK_KID_FRAMES_ADDR + off, &s));
    int first = -1, last = -1;
    for (int y = 0; y < SK_SPR_H; y++) for (int x = 0; x < SK_SPR_W; x++) if (s.px[y * SK_SPR_W + x]) { if (first < 0) first = y; last = y; }
    CHECK_EQ(first, 10); CHECK_EQ(last, 47);                     /* the standing frame: hair at row 10, boots at 47 */
    CHECK_EQ(s.px[11 * SK_SPR_W + 28], 4);                        /* "4455555545" at row 11 starts at x 28 */
    CHECK(!sprite_decode_pair(&img, 0x9FFFFFF0, &s));
    image_free(&img); disks_close(&d);
}

void test_kit_palette_scales_the_base_colours(void) {            /* ROM-backed */
    sk_disks d = {0}; sk_image img = {0};
    if (!load_sk(&d, &img)) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_palette p = {0};
    kit_palette(&p, &img, 0);
    CHECK_EQ(p.xrgb[49], rgb12_to_xrgb(0xEEE)); CHECK_EQ(p.xrgb[50], rgb12_to_xrgb(0xE66)); CHECK_EQ(p.xrgb[62], rgb12_to_xrgb(0xEEE)); CHECK_EQ(p.xrgb[63], rgb12_to_xrgb(0xBAA));
    kit_palette(&p, &img, 1);                                     /* scale 15/16 per nibble, floor */
    CHECK_EQ(p.xrgb[50], rgb12_to_xrgb(0xD55));
    p.xrgb[49] = 0x12345678; kit_palette(&p, &img, 4);           /* kits 4..7 leave colour 49 alone */
    CHECK_EQ(p.xrgb[49], 0x12345678); CHECK_EQ(p.xrgb[50], rgb12_to_xrgb(0xA44));
    image_free(&img); disks_close(&d);
}
