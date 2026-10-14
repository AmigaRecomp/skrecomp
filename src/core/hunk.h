/* AmigaDOS hunk executables loaded into a relocated image with hunk n at (n+1) << 20, the same
   layout the re/ listings use, so every address in the notes applies directly. Port of re/skre/hunk.py. */
#ifndef SK_CORE_HUNK_H
#define SK_CORE_HUNK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

#define SK_MAX_HUNKS 8
#define SK_HUNK_BASE(n) ((uint32_t)((n) + 1) << 20)

typedef struct {
    uint32_t base, size;
    uint8_t *mem;           /* owned, size bytes */
    bool chip;
} sk_segment;

typedef struct {
    sk_segment seg[SK_MAX_HUNKS];
    int count;
} sk_image;

/* Parses the file, allocates every hunk, unpacks ByteKiller-crunched CODE/DATA bodies (the launcher's
   rule for sk.pak) or copies plain ones, and applies RELOC32 / RELOC32SHORT relocations. */
bool hunk_load(const uint8_t *file, size_t len, sk_image *img, sk_err *e);

/* Pointer to n bytes at addr when [addr, addr + n) lies inside one segment, else NULL. */
const uint8_t *image_ptr(const sk_image *img, uint32_t addr, size_t n);
uint8_t  image_u8(const sk_image *img, uint32_t addr);     /* 0 when outside */
uint16_t image_u16(const sk_image *img, uint32_t addr);
uint32_t image_u32(const sk_image *img, uint32_t addr);

/* Writes n bytes at addr when they lie inside one segment (the game's own writes into its data, e.g. the kit). */
bool image_write(sk_image *img, uint32_t addr, const uint8_t *data, size_t n);

void image_free(sk_image *img);

#endif
