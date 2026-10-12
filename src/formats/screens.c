#include <string.h>
#include "formats/screens.h"
#include "core/bytes.h"

uint32_t rgb24_from_pair(uint16_t hi, uint16_t lo) {
    uint32_t r = (uint32_t)((hi >> 8 & 15) << 4 | (lo >> 8 & 15));
    uint32_t g = (uint32_t)((hi >> 4 & 15) << 4 | (lo >> 4 & 15));
    uint32_t b = (uint32_t)((hi & 15) << 4 | (lo & 15));
    return r << 16 | g << 8 | b;
}

bool screen_parse(const uint8_t *data, size_t len, int w, int h, int planes, bool interleaved, size_t header,
                  bool palette_tail, sk_screen *s, sk_err *e) {
    memset(s, 0, sizeof *s);
    if (w <= 0 || h <= 0 || planes <= 0 || planes > 8) return sk_fail(e, "screen %dx%dx%d: bad geometry", w, h, planes);
    size_t row_bytes = (size_t)((w + 15) / 16) * 2;
    size_t need = header + row_bytes * (size_t)h * (size_t)planes;
    if (len < need) return sk_fail(e, "screen: %zu bytes but %dx%dx%d needs %zu", len, w, h, planes, need);
    s->w = w;
    s->h = h;
    s->planes = planes;
    s->interleaved = interleaved;
    s->bitmap = data + header;
    if (palette_tail) {
        int n = 1 << planes;
        if (len < need + (size_t)n * 4) return sk_fail(e, "screen: palette tail needs %d bytes after the bitmap, have %zu", n * 4, len - need);
        const uint8_t *p = data + need;
        for (int i = 0; i < n; i++) s->palette[i] = rgb24_from_pair(be16(p + i * 4), be16(p + i * 4 + 2));
        s->has_palette = true;
    }
    return true;
}

bool screen_parse_title(const uint8_t *data, size_t len, sk_screen *s, sk_err *e) {
    return screen_parse(data, len, 320, 256, 8, true, 0, true, s, e);
}

bool screen_parse_language(const uint8_t *data, size_t len, sk_screen *s, sk_err *e) {
    return screen_parse(data, len, 320, 200, 5, false, 760, false, s, e);
}

int screen_pixel(const sk_screen *s, int x, int y) {
    size_t row_bytes = (size_t)((s->w + 15) / 16) * 2;
    size_t plane_size = row_bytes * (size_t)s->h;
    int v = 0, shift = 7 - (x & 7);
    for (int p = 0; p < s->planes; p++) {
        size_t off = s->interleaved ? ((size_t)y * (size_t)s->planes + (size_t)p) * row_bytes
                                    : (size_t)p * plane_size + (size_t)y * row_bytes;
        if (s->bitmap[off + (size_t)(x >> 3)] >> shift & 1) v |= 1 << p;
    }
    return v;
}
