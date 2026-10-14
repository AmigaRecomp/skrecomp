/* Square image scaling for the app icon: nearest neighbour up (crisp pixels), box average down. */
#ifndef SK_GFX_SCALE_H
#define SK_GFX_SCALE_H

#include <stdint.h>

void scale_image(const uint32_t *src, int n, uint32_t *dst, int m);   /* n x n XRGB to m x m */

#endif
