#include <stdlib.h>
#include <string.h>
#include "gfx/framebuffer.h"

bool fb_init(sk_fb *fb, int w, int h) {
    fb->w = w;
    fb->h = h;
    fb->px = calloc((size_t)w * (size_t)h, 1);
    return fb->px != NULL;
}

void fb_free(sk_fb *fb) {
    free(fb->px);
    fb->px = NULL;
    fb->w = fb->h = 0;
}

void fb_clear(sk_fb *fb, uint8_t value) {
    memset(fb->px, value, (size_t)fb->w * (size_t)fb->h);
}

static inline void put_nibble(uint8_t *px, int group, int v) {
    int shift = group ? 4 : 0;
    *px = (uint8_t)((*px & ~(0xF << shift)) | (v << shift));
}

void fb_blit_tile(sk_fb *fb, const sk_tileset *ts, int index, int x, int y, int group) {
    if (index < 0 || index >= ts->count) return;
    const uint8_t *tile = ts->tiles + (size_t)index * SK_TILE_BYTES;
    int x0 = x < 0 ? -x : 0, y0 = y < 0 ? -y : 0;
    int x1 = x + 16 > fb->w ? fb->w - x : 16, y1 = y + 16 > fb->h ? fb->h - y : 16;
    for (int ty = y0; ty < y1; ty++) {
        uint8_t *row = fb->px + (size_t)(y + ty) * (size_t)fb->w + x;
        for (int tx = x0; tx < x1; tx++) put_nibble(row + tx, group, tile_pixel(tile, tx, ty));
    }
}

void fb_blit_bob_flash(sk_fb *fb, const sk_frame *fr, int x, int y, int group) {   /* the silhouette in colour 1 */
    int x0 = x < 0 ? -x : 0, y0 = y < 0 ? -y : 0;
    int x1 = x + fr->width > fb->w ? fb->w - x : fr->width;
    int y1 = y + fr->height > fb->h ? fb->h - y : fr->height;
    for (int fy = y0; fy < y1; fy++) {
        uint8_t *row = fb->px + (size_t)(y + fy) * (size_t)fb->w + x;
        for (int fx = x0; fx < x1; fx++)
            if (bobs_pixel(fr, fx, fy) >= 0) put_nibble(row + fx, group, 1);
    }
}

void fb_blit_bob(sk_fb *fb, const sk_frame *fr, int x, int y, int group) {
    int x0 = x < 0 ? -x : 0, y0 = y < 0 ? -y : 0;
    int x1 = x + fr->width > fb->w ? fb->w - x : fr->width;
    int y1 = y + fr->height > fb->h ? fb->h - y : fr->height;
    for (int fy = y0; fy < y1; fy++) {
        uint8_t *row = fb->px + (size_t)(y + fy) * (size_t)fb->w + x;
        for (int fx = x0; fx < x1; fx++) {
            int v = bobs_pixel(fr, fx, fy);
            if (v >= 0) put_nibble(row + fx, group, v);
        }
    }
}
