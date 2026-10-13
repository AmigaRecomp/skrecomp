/* Minimal PNG writer (8-bit RGB, filter 0, zlib) for screenshots. */
#ifndef SK_GFX_PNG_H
#define SK_GFX_PNG_H

#include <stdbool.h>
#include <stdint.h>
#include "core/err.h"

bool png_write(const char *path, int w, int h, const uint32_t *xrgb, sk_err *e);

#endif
