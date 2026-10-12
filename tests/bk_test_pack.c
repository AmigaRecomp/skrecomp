#include <stdlib.h>
#include <string.h>
#include "bk_test_pack.h"
#include "core/bytes.h"

/* Bits in consumption order; the decoder reads them LSB-first from longwords taken from the end. */
typedef struct { uint8_t *bits; size_t n, cap; } bitvec;

static void push(bitvec *v, int bit) {
    if (v->n == v->cap) { v->cap = v->cap ? v->cap * 2 : 1024; v->bits = realloc(v->bits, v->cap); }
    v->bits[v->n++] = (uint8_t)bit;
}

static void push_value(bitvec *v, uint32_t value, int nbits) {      /* MSB first */
    for (int i = nbits - 1; i >= 0; i--) push(v, (value >> i) & 1);
}

/* A literal run producing data[0..n) at the current end of the output: the decoder stores the
   first byte it reads at the highest address, so the bytes go out back to front. */
static void literal(bitvec *v, const uint8_t *data, size_t n) {
    if (n <= 8) { push(v, 0); push(v, 0); push_value(v, (uint32_t)(n - 1), 3); }
    else { push(v, 1); push(v, 1); push(v, 1); push_value(v, (uint32_t)(n - 9), 8); }
    for (size_t i = n; i-- > 0;) push_value(v, data[i], 8);
}

void bk_test_pack(const uint8_t *data, size_t n, uint8_t **out, size_t *len) {
    bitvec v = {0};
    size_t end = n;
    while (end > 0) {                                   /* runs are emitted back to front */
        size_t k = end < 264 ? end : 264;
        literal(&v, data + end - k, k);
        end -= k;
    }
    size_t n_first = v.n % 32;
    size_t n_longs = 1 + (v.n - n_first) / 32;
    uint32_t *longs = calloc(n_longs, sizeof *longs);
    size_t li = 0;
    uint32_t first = 0;
    for (size_t i = 0; i < n_first; i++) first |= (uint32_t)v.bits[i] << i;
    first |= 1u << n_first;                             /* sentinel */
    longs[li++] = first;
    for (size_t j = n_first; j < v.n; j += 32) {
        uint32_t w = 0;
        for (size_t i = 0; i < 32; i++) w |= (uint32_t)v.bits[j + i] << i;
        longs[li++] = w;
    }
    uint32_t checksum = 0;
    for (size_t i = 0; i < n_longs; i++) checksum ^= longs[i];
    size_t payload = n_longs * 4 + 8;
    uint8_t *buf = malloc(4 + payload);
    put_be32(buf, (uint32_t)payload);
    for (size_t i = 0; i < n_longs; i++) put_be32(buf + 4 + i * 4, longs[n_longs - 1 - i]);   /* reversed */
    put_be32(buf + 4 + n_longs * 4, checksum);
    put_be32(buf + 4 + n_longs * 4 + 4, (uint32_t)n);
    free(longs);
    free(v.bits);
    *out = buf;
    *len = 4 + payload;
}
