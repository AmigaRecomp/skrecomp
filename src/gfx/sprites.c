#include <string.h>
#include "gfx/sprites.h"

bool sprite_decode_pair(const sk_image *sk, uint32_t frame_addr, int height, sk_sprite *out) {
    if (height < 1 || height > SK_SPR_H) return false;
    uint32_t frame = (uint32_t)height * 16 + 32;                 /* control words, rows, terminator */
    const uint8_t *a = image_ptr(sk, frame_addr, 2 * frame);
    if (!a) return false;
    const uint8_t *b = a + frame;
    memset(out->px, 0, sizeof out->px);
    for (int r = 0; r < height; r++) {
        const uint8_t *ra = a + 16 + r * 16, *rb = b + 16 + r * 16;
        for (int x = 0; x < SK_SPR_W; x++) {
            int byte = x >> 3, shift = 7 - (x & 7);
            unsigned v = (ra[byte] >> shift & 1) | (ra[8 + byte] >> shift & 1) << 1
                       | (rb[byte] >> shift & 1) << 2 | (rb[8 + byte] >> shift & 1) << 3;
            out->px[r * SK_SPR_W + x] = (uint8_t)v;
        }
    }
    return true;
}

static uint16_t scale_colour(uint16_t w, int s) {                /* scale_palette 0x11B0FA: per nibble, floor */
    unsigned r = (w >> 8 & 15) * (unsigned)s >> 4, g = (w >> 4 & 15) * (unsigned)s >> 4, b = (w & 15) * (unsigned)s >> 4;
    return (uint16_t)((r & 15) << 8 | (g & 15) << 4 | (b & 15));
}

void kit_palette(sk_palette *p, const sk_image *sk, int kit, int extra) {
    if (kit < 0) kit = 0;
    if (kit > 7) kit = 7;
    if (extra < 0) extra = 0;
    if (extra > 7) extra = 7;
    int s = 16 - kit;
    for (int i = 0; i < 13; i++) {
        if (i == 0 && kit >= 4) continue;                        /* those tables start with -1: colour 49 kept */
        p->xrgb[49 + i] = rgb12_to_xrgb(scale_colour(image_u16(sk, SK_KIT_BASE_ADDR + 2 * (uint32_t)i), s));
    }
    for (int i = 0; i < 2; i++)
        p->xrgb[62 + i] = rgb12_to_xrgb(scale_colour(image_u16(sk, SK_KIT_EXTRA_ADDR + 2 * (uint32_t)i), 16 - extra));
}
