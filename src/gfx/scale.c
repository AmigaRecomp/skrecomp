#include "gfx/scale.h"

void scale_image(const uint32_t *src, int n, uint32_t *dst, int m) {
    for (int y = 0; y < m; y++)
        for (int x = 0; x < m; x++) {
            if (m >= n) { dst[y * m + x] = src[(y * n / m) * n + x * n / m]; continue; }
            int x0 = x * n / m, x1 = (x + 1) * n / m, y0 = y * n / m, y1 = (y + 1) * n / m;   /* the source box */
            unsigned r = 0, g = 0, b = 0, k = 0;
            for (int sy = y0; sy < y1; sy++)
                for (int sx = x0; sx < x1; sx++) {
                    uint32_t c = src[sy * n + sx];
                    r += (c >> 16) & 0xFF; g += (c >> 8) & 0xFF; b += c & 0xFF; k++;
                }
            dst[y * m + x] = k ? (r / k) << 16 | (g / k) << 8 | (b / k) : 0;
        }
}
