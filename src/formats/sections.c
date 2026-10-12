#include <stdlib.h>
#include <string.h>
#include "formats/sections.h"
#include "core/bytekiller.h"
#include "core/bytes.h"

bool sections_parse(const uint8_t *data, size_t len, sk_sections *s, sk_err *e) {
    memset(s, 0, sizeof *s);
    if (len < 4) return sk_fail(e, "sections file too short (%zu bytes)", len);
    unsigned hdr = be16(data);
    if (hdr < 2 || hdr % 2 || hdr > len) return sk_fail(e, "bad section header length %u", hdr);
    int count = (int)(hdr / 2);                     /* hdr/2 - 1 offsets + the implicit first section */
    size_t *bounds = malloc((size_t)(count + 1) * sizeof *bounds);
    if (!bounds) return sk_fail(e, "out of memory");
    bounds[0] = hdr;
    for (int i = 1; i < count; i++) bounds[i] = be16(data + 2 + (size_t)(i - 1) * 2);
    bounds[count] = len;
    for (int i = 0; i < count; i++) {
        if (bounds[i + 1] < bounds[i] || bounds[i + 1] > len) {
            free(bounds);
            return sk_fail(e, "section offset %zu is not increasing or runs past the end", bounds[i + 1]);
        }
    }
    s->sec = calloc((size_t)count, sizeof *s->sec);
    if (!s->sec) { free(bounds); return sk_fail(e, "out of memory"); }
    s->count = count;
    for (int i = 0; i < count; i++) {
        const uint8_t *p = data + bounds[i];
        size_t n = bounds[i + 1] - bounds[i];
        if (n < 4) { free(bounds); sections_free(s); return sk_fail(e, "section %d is shorter than its length field", i); }
        uint32_t field = be32(p);
        if (bk_is_packed(p, n)) {
            sk_err inner = {0};
            if (!bk_decompress(p + 4, n - 4, &s->sec[i].data, &s->sec[i].len, &inner)) {
                free(bounds);
                sections_free(s);
                return sk_fail(e, "section %d: %s", i, inner.msg);
            }
            s->sec[i].packed = true;
        } else if (field == n) {
            s->sec[i].data = malloc(n - 4 ? n - 4 : 1);
            if (!s->sec[i].data) { free(bounds); sections_free(s); return sk_fail(e, "out of memory"); }
            memcpy(s->sec[i].data, p + 4, n - 4);
            s->sec[i].len = n - 4;
            s->sec[i].packed = false;
        } else {
            free(bounds);
            sections_free(s);
            return sk_fail(e, "section %d: length field %u does not match its %zu bytes", i, field, n);
        }
    }
    free(bounds);
    return true;
}

void sections_free(sk_sections *s) {
    for (int i = 0; i < s->count; i++) free(s->sec[i].data);
    free(s->sec);
    memset(s, 0, sizeof *s);
}
