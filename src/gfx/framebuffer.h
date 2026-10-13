/* Chunky planar framebuffer: one byte per pixel, bit k = bitplane k. Group 0 = bits 0-3 (playfield 1),
   group 1 = bits 4-7 (playfield 2). Blits write a nibble exactly as the blitter writes four planes. */
#ifndef SK_GFX_FRAMEBUFFER_H
#define SK_GFX_FRAMEBUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include "formats/bobs.h"
#include "formats/level.h"

typedef struct {
    int w, h;
    uint8_t *px;            /* w * h, row-major */
} sk_fb;

bool fb_init(sk_fb *fb, int w, int h);
void fb_free(sk_fb *fb);
void fb_clear(sk_fb *fb, uint8_t value);

/* Writes all 16x16 values of tile `index` (zeros too) into the group's nibble; clipped; no-op when the index is out of range. */
void fb_blit_tile(sk_fb *fb, const sk_tileset *ts, int index, int x, int y, int group);

/* Where the frame's mask is set, replaces the group's nibble with the frame's colour; clipped. */
void fb_blit_bob(sk_fb *fb, const sk_frame *fr, int x, int y, int group);
void fb_blit_bob_flash(sk_fb *fb, const sk_frame *fr, int x, int y, int group);   /* draw flags bit 1 (display.md Flash) */

#endif
