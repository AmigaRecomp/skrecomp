/* Bob frames: 4 image planes then a mask plane, plane-sequential, each plane height * width/8 bytes.
   Width is a multiple of 16. Port of re/skre/formats/bobs.py; format in re/docs/notes/sprites.md. */
#ifndef SK_FORMATS_BOBS_H
#define SK_FORMATS_BOBS_H

#include <stdbool.h>
#include <stdint.h>
#include "core/err.h"
#include "formats/sections.h"

typedef struct {
    int width, height, row_bytes;
    const uint8_t *planes[4];   /* into the section's data */
    const uint8_t *mask;
} sk_frame;

typedef struct {
    sk_frame *frame;            /* owned array; the pixel data belongs to the section */
    int count;
    int width, height;
} sk_frames;

/* Splits a packed section into frames of width_px x height; fails when the size is not a whole number of frames. */
bool bobs_split(const sk_section *s, int width_px, int height, sk_frames *f, sk_err *e);
void bobs_free(sk_frames *f);

/* -1 where the mask is clear, else the 0..15 colour index. */
int bobs_pixel(const sk_frame *fr, int x, int y);

/* Estimates (width, height, count) from the data: image bits inside the mask, row coherence, no
   wrap-around, then squareness (port of skre.formats.bobs.estimate_geometry). False when nothing fits. */
bool bobs_estimate(const sk_section *s, int *width_px, int *height, int *count);

#endif
