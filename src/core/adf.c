#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "core/adf.h"
#include "core/bytes.h"

#define BLOCK 512
#define ROOT_BLOCK 880
#define HT_SIZE 72
#define T_HEADER 2
#define ST_ROOT 1
#define ST_USERDIR 2
#define ST_FILE (-3)
#define MAX_BLOCKS 4096        /* a double-density ADF has 1760; the visited bitmap covers larger images too */

typedef struct {
    const sk_adf *v;
    uint8_t seen[MAX_BLOCKS / 8];
    const char *name;
    uint32_t header;        /* found file header block, 0 if none */
    sk_err *e;
    bool ok;
} walk;

static const uint8_t *block(const sk_adf *v, uint32_t n, sk_err *e) {
    if (n == 0 || n >= MAX_BLOCKS || (size_t)(n + 1) * BLOCK > v->len) {
        sk_fail(e, "block %u out of range", n);
        return NULL;
    }
    return v->data + (size_t)n * BLOCK;
}

static bool mark_seen(uint8_t *seen, uint32_t n) {       /* false when already seen; n < MAX_BLOCKS (block() ran first) */
    if (n >= MAX_BLOCKS) return false;
    uint8_t bit = (uint8_t)(1u << (n & 7));
    if (seen[n >> 3] & bit) return false;
    seen[n >> 3] |= bit;
    return true;
}

static void bstr(const uint8_t *p, char *out, size_t cap) {
    size_t n = p[0];
    if (n >= cap) n = cap - 1;
    memcpy(out, p + 1, n);
    out[n] = 0;
}

static bool name_equals(const uint8_t *bcpl, const char *name) {
    size_t n = bcpl[0], m = strlen(name);
    if (n != m) return false;
    for (size_t i = 0; i < n; i++)
        if (tolower(bcpl[1 + i]) != tolower((unsigned char)name[i])) return false;
    return true;
}

bool adf_open(sk_adf *v, const uint8_t *data, size_t len, sk_err *e) {
    memset(v, 0, sizeof *v);
    if (len < (size_t)(ROOT_BLOCK + 1) * BLOCK) return sk_fail(e, "image too small (%zu bytes)", len);
    if (memcmp(data, "DOS", 3) != 0) return sk_fail(e, "not an AmigaDOS image");
    const uint8_t *root = data + ROOT_BLOCK * BLOCK;
    if (be32(root) != T_HEADER || (int32_t)be32(root + BLOCK - 4) != ST_ROOT)
        return sk_fail(e, "root block missing or damaged");
    v->data = data;
    v->len = len;
    v->ffs = (data[3] & 1) != 0;
    bstr(root + BLOCK - 80, v->name, sizeof v->name);
    return true;
}

/* Visits every entry below a directory block, stopping at the named file. */
static void walk_dir(walk *w, const uint8_t *dir) {
    for (int i = 0; i < HT_SIZE && w->ok && !w->header; i++) {
        uint32_t ptr = be32(dir + 24 + 4 * i);
        while (ptr && w->ok && !w->header) {
            const uint8_t *b = block(w->v, ptr, w->e);          /* range check before the bitmap is touched */
            if (!b) { w->ok = false; return; }
            if (!mark_seen(w->seen, ptr)) { w->ok = sk_fail(w->e, "block %u: cycle in the directory structure", ptr); return; }
            if (be32(b) != T_HEADER) { w->ok = sk_fail(w->e, "block %u: expected a header block", ptr); return; }
            int32_t sec = (int32_t)be32(b + BLOCK - 4);
            if (sec == ST_USERDIR) {
                walk_dir(w, b);
            } else if (sec == ST_FILE) {
                if (name_equals(b + BLOCK - 80, w->name)) w->header = ptr;
            } else {
                w->ok = sk_fail(w->e, "block %u: unknown secondary type %d", ptr, sec);
                return;
            }
            ptr = be32(b + BLOCK - 16);
        }
    }
}

static bool read_data(const sk_adf *v, uint32_t header, uint8_t **out, size_t *len, sk_err *e) {
    const uint8_t *h = block(v, header, e);
    if (!h) return false;
    size_t size = be32(h + BLOCK - 188);
    uint8_t *buf = malloc(size ? size : 1);
    if (!buf) return sk_fail(e, "out of memory");
    size_t got = 0;
    uint8_t seen[MAX_BLOCKS / 8];
    memset(seen, 0, sizeof seen);
    uint32_t blk = header;
    while (blk) {
        const uint8_t *b = block(v, blk, e);
        if (!b) { free(buf); return false; }
        if (!mark_seen(seen, blk)) { free(buf); return sk_fail(e, "block %u: cycle in the file's extension chain", blk); }
        uint32_t count = be32(b + 8);
        if (count > HT_SIZE) { free(buf); return sk_fail(e, "block %u: data block count %u exceeds %d", blk, count, HT_SIZE); }
        for (uint32_t i = 0; i < count; i++) {
            const uint8_t *db = block(v, be32(b + BLOCK - 204 - 4 * i), e);
            if (!db) { free(buf); return false; }
            const uint8_t *src = v->ffs ? db : db + 24;
            size_t n = v->ffs ? BLOCK : be32(db + 12);
            if (n > BLOCK - 24 && !v->ffs) n = BLOCK - 24;
            if (got < size) {
                size_t take = n < size - got ? n : size - got;
                memcpy(buf + got, src, take);
            }
            got += n;
        }
        blk = be32(b + BLOCK - 8);
    }
    if (got < size) { free(buf); return sk_fail(e, "file header %u: only %zu bytes of data for size %zu", header, got, size); }
    *out = buf;
    *len = size;
    return true;
}

bool adf_read_file(const sk_adf *v, const char *name, uint8_t **out, size_t *len, sk_err *e) {
    *out = NULL;
    *len = 0;
    walk w;
    memset(&w, 0, sizeof w);
    w.v = v;
    w.name = name;
    w.e = e;
    w.ok = true;
    mark_seen(w.seen, ROOT_BLOCK);
    walk_dir(&w, v->data + ROOT_BLOCK * BLOCK);
    if (!w.ok) return false;
    if (!w.header) return sk_fail(e, "%s: no such file on %s", name, v->name);
    return read_data(v, w.header, out, len, e);
}
