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

static int popcount8(uint8_t v) { return __builtin_popcount(v); }

static long scaled(double v) { return (long)(v * 1000.0 + 0.5); }     /* Python rounds to 3 decimals before ranking */

bool bobs_estimate(const sk_section *s, int *width_px, int *height, int *count) {
    size_t size = s->len;
    if (size == 0 || size % 5) return false;
    size_t plane_total = size / 5;
    bool found = false;
    long best[5] = {0};
    int bw = 0, bh = 0, bn = 0;
    for (int w = 16; w <= 128; w += 16) {
        size_t rb = (size_t)w / 8;
        if (plane_total % rb) continue;
        size_t rows_total = plane_total / rb;
        long mismatch_bits = 0;
        for (size_t i = 0; i + rb < size; i++) mismatch_bits += popcount8((uint8_t)(s->data[i] ^ s->data[i + rb]));
        double row_mismatch = (double)mismatch_bits / (double)((size - rb) * 8);
        for (int h = 4; h <= 128; h++) {
            if (rows_total % (size_t)h) continue;
            int n = (int)(rows_total / (size_t)h);
            size_t plane = rb * (size_t)h, frame = plane * 5;
            long violations = 0, set_bits = 0, wrap_rows = 0, mask_rows = 0;
            for (int f = 0; f < n; f++) {
                const uint8_t *base = s->data + (size_t)f * frame;
                const uint8_t *mask = base + 4 * plane;
                for (size_t i = 0; i < plane; i++) {
                    uint8_t u = (uint8_t)(base[i] | base[plane + i] | base[2 * plane + i] | base[3 * plane + i]);
                    set_bits += popcount8(u);
                    violations += popcount8((uint8_t)(u & ~mask[i]));
                }
                for (int y = 0; y < h; y++) {
                    const uint8_t *row = mask + (size_t)y * rb;
                    bool any = false;
                    for (size_t i = 0; i < rb; i++) if (row[i]) { any = true; break; }
                    if (!any) continue;
                    mask_rows++;
                    if ((row[0] & 0x80) && (row[rb - 1] & 1)) wrap_rows++;
                }
            }
            if (set_bits == 0 || mask_rows == 0) continue;
            long score[5] = { scaled((double)violations / (double)set_bits), scaled(row_mismatch),
                              scaled((double)wrap_rows / (double)mask_rows), labs((long)w - h), -(long)n };
            bool better = !found;
            for (int k = 0; k < 5 && !better && found; k++) {
                if (score[k] < best[k]) better = true;
                if (score[k] != best[k]) break;
            }
            if (better) { memcpy(best, score, sizeof best); bw = w; bh = h; bn = n; found = true; }
        }
    }
    if (!found) return false;
    *width_px = bw;
    *height = bh;
    *count = bn;
    return true;
}
