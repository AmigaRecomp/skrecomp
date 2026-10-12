#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "core/zipfile.h"
#include "core/bytes.h"

bool file_read_all(const char *path, uint8_t **out, size_t *len, sk_err *e) {
    *out = NULL;
    *len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return sk_fail(e, "cannot read %s", path);
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return sk_fail(e, "cannot read %s", path); }
    long n = ftell(f);
    if (n < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return sk_fail(e, "cannot read %s", path); }
    uint8_t *buf = malloc(n > 0 ? (size_t)n : 1);
    if (!buf) { fclose(f); return sk_fail(e, "out of memory reading %s", path); }
    if (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        return sk_fail(e, "cannot read %s", path);
    }
    fclose(f);
    *out = buf;
    *len = (size_t)n;
    return true;
}

static bool ends_with_nocase(const uint8_t *name, size_t n, const char *suffix) {
    size_t m = strlen(suffix);
    if (m > n) return false;
    for (size_t i = 0; i < m; i++)
        if (tolower(name[n - m + i]) != tolower((unsigned char)suffix[i])) return false;
    return true;
}

static bool inflate_raw(const uint8_t *src, size_t n, uint8_t *dst, size_t dn, const char *path, sk_err *e) {
    z_stream z;
    memset(&z, 0, sizeof z);
    if (inflateInit2(&z, -MAX_WBITS) != Z_OK) return sk_fail(e, "%s: inflate failed (init)", path);
    z.next_in = (Bytef *)src;
    z.avail_in = (uInt)n;
    z.next_out = dst;
    z.avail_out = (uInt)dn;
    int r = inflate(&z, Z_FINISH);
    bool ok = r == Z_STREAM_END && z.total_out == dn;
    const char *msg = z.msg ? z.msg : (r == Z_STREAM_END ? "short output" : "corrupt data");
    inflateEnd(&z);
    return ok ? true : sk_fail(e, "%s: inflate failed (%s)", path, msg);
}

bool zip_read_entry(const char *path, const char *name_suffix, uint8_t **out, size_t *len, sk_err *e) {
    *out = NULL;
    *len = 0;
    uint8_t *d;
    size_t n;
    if (!file_read_all(path, &d, &n, e)) return false;
    /* end of central directory: signature 50 4B 05 06, within the last 65557 bytes */
    size_t eocd = 0;
    bool found = false;
    if (n >= 22) {
        size_t lo = n > 65557 ? n - 65557 : 0;
        for (size_t i = n - 22; ; i--) {
            if (d[i] == 0x50 && d[i + 1] == 0x4B && d[i + 2] == 0x05 && d[i + 3] == 0x06) { eocd = i; found = true; break; }
            if (i == lo) break;
        }
    }
    if (!found) { free(d); return sk_fail(e, "%s: not a zip file", path); }
    uint32_t entries = le16(d + eocd + 10);
    size_t cd = le32(d + eocd + 16);
    size_t o = cd;
    for (uint32_t k = 0; k < entries; k++) {
        if (o + 46 > n || le32(d + o) != 0x02014B50) { free(d); return sk_fail(e, "%s: damaged central directory", path); }
        uint32_t method = le16(d + o + 10);
        size_t csize = le32(d + o + 20), usize = le32(d + o + 24);
        size_t name_len = le16(d + o + 28), extra_len = le16(d + o + 30), comment_len = le16(d + o + 32);
        size_t local = le32(d + o + 42);
        const uint8_t *name = d + o + 46;
        if (o + 46 + name_len > n) { free(d); return sk_fail(e, "%s: damaged central directory", path); }
        if (ends_with_nocase(name, name_len, name_suffix)) {
            if (local + 30 > n || le32(d + local) != 0x04034B50) { free(d); return sk_fail(e, "%s: damaged local header", path); }
            size_t data = local + 30 + le16(d + local + 26) + le16(d + local + 28);
            if (data + csize > n) { free(d); return sk_fail(e, "%s: entry data runs past the end of the file", path); }
            uint8_t *buf = malloc(usize ? usize : 1);
            if (!buf) { free(d); return sk_fail(e, "out of memory reading %s", path); }
            bool ok;
            if (method == 0) {
                ok = csize == usize;
                if (ok) memcpy(buf, d + data, usize);
                else sk_fail(e, "%s: stored entry sizes disagree", path);
            } else if (method == 8) {
                ok = inflate_raw(d + data, csize, buf, usize, path, e);
            } else {
                ok = sk_fail(e, "%s: unsupported zip method %u", path, method);
            }
            free(d);
            if (!ok) { free(buf); return false; }
            *out = buf;
            *len = usize;
            return true;
        }
        o += 46 + name_len + extra_len + comment_len;
    }
    free(d);
    return sk_fail(e, "%s: no entry ending in %s", path, name_suffix);
}
