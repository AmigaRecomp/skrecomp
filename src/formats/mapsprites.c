#include "formats/mapsprites.h"
#include "core/bytes.h"

bool mapsprites_parse(const uint8_t *data, size_t len, sk_mapsprites *m, sk_err *e) {   /* world_map 0x11E50C */
    if (len < 10) return sk_fail(e, "file253: %zu bytes, the header needs 10", len);
    size_t at = 10;
    for (int k = 0; k < 5; k++) {
        m->len[k] = be16(data + 2 * k);
        if (m->len[k] > len - at) return sk_fail(e, "file253: chunk %d (%zu bytes) runs past the pack", k, m->len[k]);
        m->chunk[k] = data + at;
        at += m->len[k];
    }
    return true;
}

static uint8_t chunk_byte(const uint8_t *chunk, size_t len, size_t at) { return at < len ? chunk[at] : 0; }

void masked8_draw(const uint8_t *chunk, size_t len, size_t off, int w, int h, uint8_t *px, int pw, int ph, int x, int y) {
    int bw = w / 8;                                              /* sub_11E984: minterm 0xFCA per plane, A = mask */
    for (int r = 0; r < h; r++) {
        int sy = y + r;
        if (sy < 0 || sy >= ph) continue;
        size_t row = off + (size_t)r * 9 * (size_t)bw;
        for (int c = 0; c < w; c++) {
            int sx = x + c;
            if (sx < 0 || sx >= pw) continue;
            int bit = 7 - (c & 7);
            if (!((chunk_byte(chunk, len, row + (size_t)(c >> 3)) >> bit) & 1)) continue;
            uint8_t v = 0;
            for (int p = 0; p < 8; p++)
                v |= (uint8_t)(((chunk_byte(chunk, len, row + (size_t)((p + 1) * bw + (c >> 3))) >> bit) & 1) << p);
            px[sy * pw + sx] = v;
        }
    }
}
