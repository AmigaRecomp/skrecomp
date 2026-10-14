#include <stdlib.h>
#include <string.h>
#include "core/hunk.h"
#include "core/bytekiller.h"
#include "core/bytes.h"

#define HUNK_CODE 0x3E9
#define HUNK_DATA 0x3EA
#define HUNK_BSS 0x3EB
#define HUNK_RELOC32 0x3EC
#define HUNK_SYMBOL 0x3F0
#define HUNK_DEBUG 0x3F1
#define HUNK_END 0x3F2
#define HUNK_HEADER 0x3F3
#define HUNK_DREL32 0x3F7
#define HUNK_RELOC32SHORT 0x3FC
#define MAX_RELOCS 65536

typedef struct { uint32_t target, offset; } reloc;

typedef struct {
    uint32_t alloc;
    bool chip;
    bool bss;
    const uint8_t *body;
    size_t body_len;
    reloc *relocs;
    size_t n_relocs;
} hunk;

typedef struct {
    const uint8_t *d;
    size_t len;
    size_t o;
    sk_err *e;
    bool ok;
} reader;

static bool need(reader *r, size_t n) {
    if (r->o + n > r->len) {
        r->ok = sk_fail(r->e, "truncated hunk file: needed %zu bytes at offset %zu, have %zu", n, r->o, r->len);
        return false;
    }
    return true;
}

static uint32_t rd32(reader *r) {
    if (!r->ok || !need(r, 4)) return 0;
    uint32_t v = be32(r->d + r->o);
    r->o += 4;
    return v;
}

static uint16_t rd16(reader *r) {
    if (!r->ok || !need(r, 2)) return 0;
    uint16_t v = be16(r->d + r->o);
    r->o += 2;
    return v;
}

static bool add_reloc(hunk *h, uint32_t target, uint32_t offset, sk_err *e) {
    if (h->n_relocs >= MAX_RELOCS) return sk_fail(e, "hunk: more than %d relocations", MAX_RELOCS);
    if (h->n_relocs % 1024 == 0) {
        reloc *grown = realloc(h->relocs, (h->n_relocs + 1024) * sizeof *grown);
        if (!grown) return sk_fail(e, "out of memory");
        h->relocs = grown;
    }
    h->relocs[h->n_relocs++] = (reloc){ target, offset };
    return true;
}

static void free_hunks(hunk *hs, int n) {
    for (int i = 0; i < n; i++) free(hs[i].relocs);
}

static bool parse(reader *r, hunk *hs, int *count) {
    if (r->len < 4 || be32(r->d) != HUNK_HEADER) return sk_fail(r->e, "not a hunk file (missing HUNK_HEADER)");
    r->o = 4;
    for (;;) {                                   /* resident library names */
        uint32_t n = rd32(r);
        if (!r->ok) return false;
        if (n == 0) break;
        r->o += (size_t)n * 4;
    }
    rd32(r);                                     /* table size */
    uint32_t first = rd32(r), last = rd32(r);
    if (!r->ok) return false;
    if (last < first || last - first + 1 > SK_MAX_HUNKS) return sk_fail(r->e, "hunk file declares %u hunks", last - first + 1);
    *count = (int)(last - first + 1);
    for (int i = 0; i < *count; i++) {
        uint32_t v = rd32(r);
        if (!r->ok) return false;
        uint32_t flags = v >> 30;
        hs[i].alloc = (v & 0x3FFFFFFF) * 4;
        hs[i].bss = true;
        if (flags == 3) rd32(r);                 /* extended attribute longword */
        hs[i].chip = flags == 1;
    }
    int cur = -1;
    bool ended = false;
    while (r->ok && r->o < r->len) {
        size_t start = r->o;
        uint32_t t = rd32(r) & 0x3FFFFFFF;
        if (!r->ok) return false;
        ended = t == HUNK_END;
        if (t == HUNK_CODE || t == HUNK_DATA || t == HUNK_BSS) {
            cur++;
            if (cur >= *count) return sk_fail(r->e, "more hunks than the header declares (offset %zu)", start);
            uint32_t n = rd32(r) * 4;
            if (!r->ok) return false;
            if (t != HUNK_BSS) {
                if (!need(r, n)) return false;
                hs[cur].bss = false;
                hs[cur].body = r->d + r->o;
                hs[cur].body_len = n;
                r->o += n;
            }
        } else if (t == HUNK_RELOC32) {
            if (cur < 0) return sk_fail(r->e, "relocation before any hunk (offset %zu)", start);
            for (;;) {
                uint32_t cnt = rd32(r);
                if (!r->ok) return false;
                if (cnt == 0) break;
                uint32_t target = rd32(r);
                if (!need(r, (size_t)cnt * 4)) return false;
                for (uint32_t i = 0; i < cnt; i++)
                    if (!add_reloc(&hs[cur], target, rd32(r), r->e)) return false;
            }
        } else if (t == HUNK_RELOC32SHORT || t == HUNK_DREL32) {
            if (cur < 0) return sk_fail(r->e, "relocation before any hunk (offset %zu)", start);
            for (;;) {
                uint16_t cnt = rd16(r);
                if (!r->ok) return false;
                if (cnt == 0) break;
                uint16_t target = rd16(r);
                if (!need(r, (size_t)cnt * 2)) return false;
                for (uint32_t i = 0; i < cnt; i++)
                    if (!add_reloc(&hs[cur], target, rd16(r), r->e)) return false;
            }
            if (r->o % 4) r->o += 2;
        } else if (t == HUNK_SYMBOL) {
            for (;;) {
                uint32_t n = rd32(r) & 0xFFFFFF;
                if (!r->ok) return false;
                if (n == 0) break;
                r->o += (size_t)n * 4 + 4;
            }
        } else if (t == HUNK_DEBUG) {
            uint32_t n = rd32(r);
            r->o += (size_t)n * 4;
        } else if (t == HUNK_END) {
            /* nothing */
        } else {
            return sk_fail(r->e, "unknown hunk type 0x%X at offset %zu", t, start);
        }
    }
    if (!r->ok) return false;
    if (cur + 1 != *count) return sk_fail(r->e, "file ends after %d of %d hunks", cur + 1, *count);
    if (!ended) return sk_fail(r->e, "file does not end with HUNK_END");
    return true;
}

bool hunk_load(const uint8_t *file, size_t len, sk_image *img, sk_err *e) {
    memset(img, 0, sizeof *img);
    hunk hs[SK_MAX_HUNKS];
    memset(hs, 0, sizeof hs);
    int count = 0;
    reader r = { file, len, 0, e, true };
    if (!parse(&r, hs, &count)) { free_hunks(hs, SK_MAX_HUNKS); return false; }
    for (int i = 0; i < count; i++) {
        sk_segment *s = &img->seg[i];
        s->base = SK_HUNK_BASE(i);
        s->size = hs[i].alloc;
        s->chip = hs[i].chip;
        s->mem = calloc(hs[i].alloc ? hs[i].alloc : 1, 1);
        if (!s->mem) { sk_fail(e, "out of memory"); goto fail; }
        img->count = i + 1;
        if (hs[i].bss || hs[i].body_len == 0) continue;
        if (bk_is_packed(hs[i].body, hs[i].body_len)) {
            uint8_t *plain;
            size_t n;
            sk_err inner = {0};
            if (!bk_unpack(hs[i].body, hs[i].body_len, &plain, &n, &inner)) { sk_fail(e, "hunk %d: %s", i, inner.msg); goto fail; }
            if (n > hs[i].alloc) { free(plain); sk_fail(e, "hunk %d: unpacked %zu bytes exceed allocation %u", i, n, hs[i].alloc); goto fail; }
            memcpy(s->mem, plain, n);
            free(plain);
        } else {
            if (hs[i].body_len > hs[i].alloc) { sk_fail(e, "hunk %d: body of %zu bytes exceeds allocation %u", i, hs[i].body_len, hs[i].alloc); goto fail; }
            memcpy(s->mem, hs[i].body, hs[i].body_len);
        }
    }
    for (int i = 0; i < count; i++) {
        for (size_t k = 0; k < hs[i].n_relocs; k++) {
            reloc rl = hs[i].relocs[k];
            if (rl.target >= (uint32_t)count) { sk_fail(e, "hunk %d: relocation to missing hunk %u", i, rl.target); goto fail; }
            if ((size_t)rl.offset + 4 > img->seg[i].size) { sk_fail(e, "hunk %d: relocation offset %u outside the hunk", i, rl.offset); goto fail; }
            uint8_t *p = img->seg[i].mem + rl.offset;
            put_be32(p, be32(p) + SK_HUNK_BASE(rl.target));
        }
    }
    free_hunks(hs, SK_MAX_HUNKS);
    return true;
fail:
    free_hunks(hs, SK_MAX_HUNKS);
    image_free(img);
    return false;
}

const uint8_t *image_ptr(const sk_image *img, uint32_t addr, size_t n) {
    for (int i = 0; i < img->count; i++) {
        const sk_segment *s = &img->seg[i];
        if (addr >= s->base && addr < s->base + s->size && n <= (size_t)(s->base + s->size - addr))
            return s->mem + (addr - s->base);
    }
    return NULL;
}

bool image_write(sk_image *img, uint32_t addr, const uint8_t *data, size_t n) {
    uint8_t *p = (uint8_t *)image_ptr(img, addr, n);
    if (!p) return false;
    memcpy(p, data, n);
    return true;
}

uint8_t image_u8(const sk_image *img, uint32_t addr) { const uint8_t *p = image_ptr(img, addr, 1); return p ? p[0] : 0; }
uint16_t image_u16(const sk_image *img, uint32_t addr) { const uint8_t *p = image_ptr(img, addr, 2); return p ? be16(p) : 0; }
uint32_t image_u32(const sk_image *img, uint32_t addr) { const uint8_t *p = image_ptr(img, addr, 4); return p ? be32(p) : 0; }

void image_free(sk_image *img) {
    for (int i = 0; i < img->count; i++) free(img->seg[i].mem);
    memset(img, 0, sizeof *img);
}
