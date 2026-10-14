#include "check.h"
#include "gfx/scale.h"

void test_scale_up_repeats_pixels_and_down_averages_them(void) {
    uint32_t src[4] = { 0x000000, 0xFFFFFF, 0xFF0000, 0x0000FF };   /* 2 x 2 */
    uint32_t up[16];
    scale_image(src, 2, up, 4);
    CHECK_EQ(up[0], 0x000000u); CHECK_EQ(up[1], 0x000000u); CHECK_EQ(up[2], 0xFFFFFFu); CHECK_EQ(up[15], 0x0000FFu);
    uint32_t down[1];
    scale_image(src, 2, down, 1);
    CHECK_EQ(down[0], 0x7F3F7Fu);                                /* the four averaged per channel */
}
