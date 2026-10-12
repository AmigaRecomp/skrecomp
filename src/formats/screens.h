/* Full-screen pictures: the title (320x256x8 row-interleaved + 256-colour palette tail) and the
   language screen (320x200x5 plane-sequential after a 760-byte prefix). re/docs/notes/screens.md. */
#ifndef SK_FORMATS_SCREENS_H
#define SK_FORMATS_SCREENS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

typedef struct {
    int w, h, planes;
    bool interleaved;
    const uint8_t *bitmap;      /* into the caller's buffer */
    uint32_t palette[256];      /* XRGB */
    bool has_palette;
} sk_screen;

/* Generic decoder: header bytes are skipped; with palette_tail, (1 << planes) (hi, lo) word pairs follow the bitmap. */
bool screen_parse(const uint8_t *data, size_t len, int w, int h, int planes, bool interleaved, size_t header,
                  bool palette_tail, sk_screen *s, sk_err *e);
bool screen_parse_title(const uint8_t *data, size_t len, sk_screen *s, sk_err *e);
bool screen_parse_language(const uint8_t *data, size_t len, sk_screen *s, sk_err *e);

/* Colour index of a pixel. */
int screen_pixel(const sk_screen *s, int x, int y);

/* AGA colour from the (high nibbles, low nibbles) word pair, as XRGB. */
uint32_t rgb24_from_pair(uint16_t hi, uint16_t lo);

#endif
