#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "gfx/png.h"
#include "core/bytes.h"

static bool chunk(FILE *f, const char *kind, const uint8_t *body, size_t n) {
    uint8_t head[8];
    put_be32(head, (uint32_t)n);
    memcpy(head + 4, kind, 4);
    uint32_t crc = (uint32_t)crc32(crc32(0, head + 4, 4), body, (uInt)n);
    uint8_t tail[4];
    put_be32(tail, crc);
    return fwrite(head, 1, 8, f) == 8 && (n == 0 || fwrite(body, 1, n, f) == n) && fwrite(tail, 1, 4, f) == 4;
}

bool png_write(const char *path, int w, int h, const uint32_t *xrgb, sk_err *e) {
    size_t stride = 1 + (size_t)w * 3, raw_len = stride * (size_t)h;
    uint8_t *raw = malloc(raw_len);
    if (!raw) return sk_fail(e, "out of memory");
    for (int y = 0; y < h; y++) {
        uint8_t *row = raw + (size_t)y * stride;
        row[0] = 0;
        for (int x = 0; x < w; x++) {
            uint32_t c = xrgb[(size_t)y * (size_t)w + (size_t)x];
            row[1 + x * 3] = (uint8_t)(c >> 16);
            row[2 + x * 3] = (uint8_t)(c >> 8);
            row[3 + x * 3] = (uint8_t)c;
        }
    }
    uLongf zlen = compressBound((uLong)raw_len);
    uint8_t *z = malloc(zlen);
    if (!z || compress2(z, &zlen, raw, (uLong)raw_len, 6) != Z_OK) { free(raw); free(z); return sk_fail(e, "png: compression failed"); }
    free(raw);
    FILE *f = fopen(path, "wb");
    if (!f) { free(z); return sk_fail(e, "cannot write %s", path); }
    uint8_t ihdr[13];
    put_be32(ihdr, (uint32_t)w);
    put_be32(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    bool ok = fwrite("\x89PNG\r\n\x1a\n", 1, 8, f) == 8
           && chunk(f, "IHDR", ihdr, sizeof ihdr)
           && chunk(f, "IDAT", z, zlen)
           && chunk(f, "IEND", NULL, 0);
    ok = fclose(f) == 0 && ok;
    free(z);
    return ok ? true : sk_fail(e, "cannot write %s", path);
}
