/* The world map's sprite pack, file253 (re/docs/notes/between-levels.md 1.3): five big-endian chunk sizes, then the chunks
   (medallions 32x44, unused, aircraft 32x19, shadows 16x13, trophy pieces 64x40), each image h rows of w/8 mask bytes then
   eight planes of w/8 bytes. */
#ifndef SK_FORMATS_MAPSPRITES_H
#define SK_FORMATS_MAPSPRITES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

typedef struct { const uint8_t *chunk[5]; size_t len[5]; } sk_mapsprites;

bool mapsprites_parse(const uint8_t *data, size_t len, sk_mapsprites *m, sk_err *e);   /* points into data */
/* blit_masked_image (0x11E984): the w x h image at off in the chunk, cookie-cut into the pw x ph index buffer at (x, y);
   clipped; bytes past the chunk read as zero. */
void masked8_draw(const uint8_t *chunk, size_t len, size_t off, int w, int h, uint8_t *px, int pw, int ph, int x, int y);

#endif
