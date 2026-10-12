#include <string.h>
#include "gfx/palette.h"
#include "gfx/display_model.h"

uint32_t rgb12_to_xrgb(uint16_t w) {
    uint32_t r = (uint32_t)(w >> 8 & 15) * 17, g = (uint32_t)(w >> 4 & 15) * 17, b = (uint32_t)(w & 15) * 17;
    return r << 16 | g << 8 | b;
}

void palette_from_tilesets(sk_palette *p, const sk_tileset *a, const sk_tileset *b) {
    memset(p, 0, sizeof *p);
    for (int i = 1; i < 16; i++) {
        p->xrgb[i] = rgb12_to_xrgb(a->colours[i]);
        p->xrgb[SK_PF2_COLOUR_BASE + i] = rgb12_to_xrgb(b->colours[i]);
    }
}
