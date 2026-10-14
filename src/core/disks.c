#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/disks.h"
#include "core/bytekiller.h"
#include "core/bytes.h"
#include "core/dirlist.h"
#include "core/zipfile.h"

typedef struct {
    sk_disks *d;
    int looked;
} scan;

static bool ends_with_nocase(const char *s, const char *suffix) {
    size_t n = strlen(s), m = strlen(suffix);
    if (m > n) return false;
    for (size_t i = 0; i < m; i++)
        if (tolower((unsigned char)s[n - m + i]) != tolower((unsigned char)suffix[i])) return false;
    return true;
}

static void consider(const char *path, const char *name, void *ctx) {
    scan *s = ctx;
    bool zip = ends_with_nocase(name, ".zip"), adf = ends_with_nocase(name, ".adf");
    if (!zip && !adf) return;
    s->looked++;
    uint8_t *img;
    size_t n;
    sk_err e = {0};
    bool ok = zip ? zip_read_entry(path, ".adf", &img, &n, &e) : file_read_all(path, &img, &n, &e);
    if (!ok) return;
    sk_adf v;
    if (adf_open(&v, img, n, &e) && strncmp(v.name, "SKDisk", 6) == 0 && isdigit((unsigned char)v.name[6]) && v.name[7] == 0) {
        int k = v.name[6] - '0';
        if (k < SK_DISKS && s->d->image[k] == NULL) {
            s->d->image[k] = img;
            s->d->len[k] = n;
            s->d->vol[k] = v;
            snprintf(s->d->path[k], sizeof s->d->path[k], "%s", path);
            return;
        }
    }
    free(img);
}

static bool archive_read(const sk_disks *d, int disk, const char *name, uint8_t **out, size_t *len, sk_err *e);

bool disks_open(sk_disks *d, const char *dir, sk_err *e) {
    memset(d, 0, sizeof *d);
    scan s = { d, 0 };
    if (!list_dir(dir, consider, &s, e)) return false;
    for (int k = 0; k < SK_DISKS; k++)
        if (d->image[k] == NULL)
            return sk_fail(e, "%s: SKDisk%d not found (looked at %d zip/adf files)", dir, k, s.looked);
    return true;
}

bool disks_read(sk_disks *d, int disk, const char *name, uint8_t **out, size_t *len, sk_err *e) {
    *out = NULL;
    *len = 0;
    if (disk < 0 || disk >= SK_DISKS) return sk_fail(e, "disk %d: no such disk", disk);
    if (d->archive) return archive_read(d, disk, name, out, len, e);
    if (d->image[disk] == NULL) return sk_fail(e, "disk %d: SKDisk%d was not found", disk, disk);
    return adf_read_file(&d->vol[disk], name, out, len, e);
}

bool disks_read_file_no(sk_disks *d, int disk, int file_no, bool unpack, uint8_t **out, size_t *len, sk_err *e) {
    char name[16];
    snprintf(name, sizeof name, "file%03d", file_no);
    uint8_t *raw;
    size_t n;
    if (!disks_read(d, disk, name, &raw, &n, e)) return false;
    if (!unpack) {
        *out = raw;
        *len = n;
        return true;
    }
    sk_err inner = {0};
    bool ok = bk_unpack(raw, n, out, len, &inner);
    free(raw);
    if (!ok) return sk_fail(e, "disk %d %s: %s", disk, name, inner.msg);
    return true;
}

void disks_close(sk_disks *d) {
    d->archive = NULL; d->archive_len = 0; d->archive_count = 0;
    for (int k = 0; k < SK_DISKS; k++) {
        free(d->image[k]);
        d->image[k] = NULL;
        d->len[k] = 0;
    }
}
#define ARCHIVE_MAGIC "SKDATA01"
#define ARCHIVE_ENTRY 32

bool disks_open_archive(sk_disks *d, const uint8_t *data, size_t len, sk_err *e) {
    memset(d, 0, sizeof *d);
    if (len < 12 || memcmp(data, ARCHIVE_MAGIC, 8) != 0) return sk_fail(e, "soccerkid.dat: not a Soccer Kid data file (run skextract)");
    uint32_t count = be32(data + 8);
    if (count > (len - 12) / ARCHIVE_ENTRY) return sk_fail(e, "soccerkid.dat: the table of %u files runs past the end", count);
    for (uint32_t i = 0; i < count; i++) {
        const uint8_t *en = data + 12 + (size_t)i * ARCHIVE_ENTRY;
        uint32_t off = be32(en + 24), size = be32(en + 28);
        if (en[0] >= SK_DISKS || off > len || size > len - off) return sk_fail(e, "soccerkid.dat: entry %u runs past the end", i);
    }
    d->archive = data; d->archive_len = len; d->archive_count = count;
    return true;
}

static bool archive_read(const sk_disks *d, int disk, const char *name, uint8_t **out, size_t *len, sk_err *e) {
    for (uint32_t i = 0; i < d->archive_count; i++) {
        const uint8_t *en = d->archive + 12 + (size_t)i * ARCHIVE_ENTRY;
        char stored[24];
        memcpy(stored, en + 1, 23); stored[23] = 0;
        if (en[0] != disk) continue;
        size_t k = 0;
        while (stored[k] && name[k] && tolower((unsigned char)stored[k]) == tolower((unsigned char)name[k])) k++;
        if (stored[k] || name[k]) continue;
        uint32_t off = be32(en + 24), size = be32(en + 28);
        *out = malloc(size ? size : 1);
        if (!*out) return sk_fail(e, "out of memory");
        memcpy(*out, d->archive + off, size);
        *len = size;
        return true;
    }
    return sk_fail(e, "SKDisk%d: %s is not in soccerkid.dat", disk, name);
}

bool disks_extract(sk_disks *d, uint8_t **out, size_t *len, sk_err *e) {
    static const char *named[] = { "sk.pak", "anim", "animfil1.pak", "animfil2.pak", "language.pak",
                                   "english.bin", "french.bin", "german.bin", "italian.bin", "spanish.bin" };
    struct { int disk; char name[24]; uint8_t *data; size_t n; } *f = NULL;
    size_t count = 0, cap = 0, total = 0;
    bool ok = true;
    for (int disk = 0; disk < SK_DISKS && ok; disk++)
        for (int k = 0; k < (int)(sizeof named / sizeof *named) + 256; k++) {
            char name[24];
            if (k < (int)(sizeof named / sizeof *named)) snprintf(name, sizeof name, "%s", named[k]);
            else snprintf(name, sizeof name, "file%03d", k - (int)(sizeof named / sizeof *named));
            uint8_t *data = NULL; size_t n = 0; sk_err ignored = {0};
            if (!disks_read(d, disk, name, &data, &n, &ignored)) continue;   /* not on this disk */
            if (count == cap) {
                cap = cap ? cap * 2 : 256;
                void *g = realloc(f, cap * sizeof *f);
                if (!g) { free(data); ok = sk_fail(e, "out of memory"); break; }
                f = g;
            }
            f[count].disk = disk; snprintf(f[count].name, sizeof f[count].name, "%s", name);
            f[count].data = data; f[count].n = n;
            count++; total += n;
        }
    uint8_t *a = ok ? malloc(12 + count * ARCHIVE_ENTRY + total) : NULL;
    if (ok && !a) ok = sk_fail(e, "out of memory");
    if (ok) {
        memcpy(a, ARCHIVE_MAGIC, 8);
        put_be32(a + 8, (uint32_t)count);
        size_t at = 12 + count * ARCHIVE_ENTRY;
        for (size_t i = 0; i < count; i++) {
            uint8_t *en = a + 12 + i * ARCHIVE_ENTRY;
            memset(en, 0, ARCHIVE_ENTRY);
            en[0] = (uint8_t)f[i].disk;
            memcpy(en + 1, f[i].name, strlen(f[i].name));
            put_be32(en + 24, (uint32_t)at); put_be32(en + 28, (uint32_t)f[i].n);
            memcpy(a + at, f[i].data, f[i].n);
            at += f[i].n;
        }
        *out = a; *len = at;
    }
    for (size_t i = 0; i < count; i++) free(f[i].data);
    free(f);
    return ok;
}
