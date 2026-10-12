/* Port of re/skre/pak.py, itself a transcription of the game's bk_decrunch routine. */
#include <stdlib.h>
#include "core/bytekiller.h"
#include "core/bytes.h"

typedef struct {
    const uint8_t *p;       /* payload */
    size_t rp;              /* read position, moves down in steps of 4 */
    uint32_t d0;            /* bit buffer with a sentinel bit above the data */
    uint32_t checksum;      /* XOR of every longword read; must end at 0 */
    uint8_t *out;
    size_t wp;              /* write position, moves down */
    size_t total;
    sk_err *e;
    bool ok;
} bk;

static uint32_t next_long(bk *s) {
    if (s->rp < 4) {
        s->ok = sk_fail(s->e, "ByteKiller: packed stream exhausted before the output was complete");
        return 0;
    }
    s->rp -= 4;
    uint32_t v = be32(s->p + s->rp);
    s->checksum ^= v;
    return v;
}

static unsigned next_bit(bk *s) {
    unsigned bit = s->d0 & 1;
    s->d0 >>= 1;
    if (s->d0 == 0) {                   /* only the sentinel was left: that bit was data, refill */
        uint32_t v = next_long(s);
        bit = v & 1;
        s->d0 = (v >> 1) | 0x80000000u;
    }
    return bit;
}

static uint32_t read_bits(bk *s, int n) {
    uint32_t v = 0;
    while (n-- > 0 && s->ok) v = (v << 1) | next_bit(s);
    return v;
}

static void literal(bk *s, uint32_t count) {
    while (count-- > 0 && s->ok) {
        if (s->wp == 0) {
            s->ok = sk_fail(s->e, "ByteKiller: literal run overruns the start of the output");
            return;
        }
        s->out[--s->wp] = (uint8_t)read_bits(s, 8);
    }
}

static void copy(bk *s, uint32_t length, uint32_t offset) {
    while (length-- > 0 && s->ok) {
        if (s->wp == 0 || s->wp - 1 + offset >= s->total) {
            s->ok = sk_fail(s->e, "ByteKiller: match reaches outside the output");
            return;
        }
        s->wp--;
        s->out[s->wp] = s->out[s->wp + offset];
    }
}

bool bk_is_packed(const uint8_t *d, size_t n) {
    return n >= 4 && be32(d) == n - 4;
}

bool bk_decompress(const uint8_t *payload, size_t n, uint8_t **out, size_t *len, sk_err *e) {
    *out = NULL;
    *len = 0;
    if (n < 12 || n % 4)
        return sk_fail(e, "ByteKiller: payload of %zu bytes is too short or not longword aligned", n);
    uint32_t unpacked = be32(payload + n - 4);
    bk s = { payload, n - 8, 0, be32(payload + n - 8), NULL, unpacked, unpacked, e, true };
    s.out = malloc(unpacked ? unpacked : 1);
    if (!s.out) return sk_fail(e, "out of memory");
    s.d0 = next_long(&s);
    while (s.wp > 0 && s.ok) {
        if (next_bit(&s) == 0) {
            if (next_bit(&s) == 0) literal(&s, read_bits(&s, 3) + 1);
            else copy(&s, 2, read_bits(&s, 8));
        } else {
            uint32_t sel = read_bits(&s, 2);
            if (sel == 0) copy(&s, 3, read_bits(&s, 9));
            else if (sel == 1) copy(&s, 4, read_bits(&s, 10));
            else if (sel == 2) { uint32_t l = read_bits(&s, 8) + 1; copy(&s, l, read_bits(&s, 12)); }
            else literal(&s, read_bits(&s, 8) + 9);
        }
    }
    if (s.ok && s.checksum != 0)
        s.ok = sk_fail(e, "ByteKiller: checksum mismatch (residual 0x%08X)", s.checksum);
    if (!s.ok) {
        free(s.out);
        return false;
    }
    *out = s.out;
    *len = unpacked;
    return true;
}

bool bk_unpack(const uint8_t *c, size_t n, uint8_t **out, size_t *len, sk_err *e) {
    if (!bk_is_packed(c, n)) {
        *out = NULL;
        *len = 0;
        return sk_fail(e, "ByteKiller: not a length-prefixed packed file");
    }
    return bk_decompress(c + 4, n - 4, out, len, e);
}
