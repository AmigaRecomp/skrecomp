#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/disks.h"
#include "core/bytekiller.h"
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
    for (int k = 0; k < SK_DISKS; k++) {
        free(d->image[k]);
        d->image[k] = NULL;
        d->len[k] = 0;
    }
}
