#include <stdlib.h>
#include <string.h>
#include "formats/bobs.h"

bool bobs_split(const sk_section *s, int width_px, int height, sk_frames *f, sk_err *e) {
    memset(f, 0, sizeof *f);
    if (width_px <= 0 || width_px % 16 || height <= 0)
        return sk_fail(e, "frame size %dx%d: width must be a positive multiple of 16", width_px, height);
    size_t plane = (size_t)height * (size_t)(width_px / 8);
    size_t frame_bytes = plane * 5;
    if (s->len == 0 || s->len % frame_bytes)
        return sk_fail(e, "section of %zu bytes is not a whole number of frames of %dx%d (%zu bytes)", s->len, width_px, height, frame_bytes);
    int count = (int)(s->len / frame_bytes);
    f->frame = calloc((size_t)count, sizeof *f->frame);
    if (!f->frame) return sk_fail(e, "out of memory");
    for (int i = 0; i < count; i++) {
        const uint8_t *base = s->data + (size_t)i * frame_bytes;
        sk_frame *fr = &f->frame[i];
        fr->width = width_px;
        fr->height = height;
        fr->row_bytes = width_px / 8;
        for (int p = 0; p < 4; p++) fr->planes[p] = base + (size_t)p * plane;
        fr->mask = base + 4 * plane;
    }
    f->count = count;
    f->width = width_px;
    f->height = height;
    return true;
}

void bobs_free(sk_frames *f) {
    free(f->frame);
    memset(f, 0, sizeof *f);
}

int bobs_pixel(const sk_frame *fr, int x, int y) {
    size_t o = (size_t)y * (size_t)fr->row_bytes + (size_t)(x >> 3);
    int shift = 7 - (x & 7);
    if (!(fr->mask[o] >> shift & 1)) return -1;
    int v = 0;
    for (int p = 0; p < 4; p++)
        if (fr->planes[p][o] >> shift & 1) v |= 1 << p;
    return v;
}
