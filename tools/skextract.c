/* skextract: the game's files from the five disk images into one soccerkid.dat, which the build then puts inside the
   soccerkid program. The data never leaves your machine: the repository holds no game bytes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/disks.h"
#ifdef _WIN32
#include <direct.h>
#define make_dir(p) _mkdir(p)
#else
#include <sys/stat.h>
#define make_dir(p) mkdir(p, 0755)
#endif

static void make_parent(const char *path) {                    /* data/ is created on the first run */
    char dir[1024];
    snprintf(dir, sizeof dir, "%s", path);
    char *slash = strrchr(dir, '/');
#ifdef _WIN32
    char *back = strrchr(dir, '\\');
    if (back > slash) slash = back;
#endif
    if (!slash || slash == dir) return;
    *slash = 0;
    make_dir(dir);
}

#ifndef SK_DEFAULT_DATA
#define SK_DEFAULT_DATA "data/soccerkid.dat"
#endif

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: skextract <directory with the five disk images (.adf or .zip)> [output, default %s]\n", SK_DEFAULT_DATA);
        return 2;
    }
    const char *out_path = argc == 3 ? argv[2] : SK_DEFAULT_DATA;
    sk_disks d; sk_err e = {0};
    if (!disks_open(&d, argv[1], &e)) { fprintf(stderr, "skextract: %s\n", e.msg); return 1; }
    uint8_t *data = NULL; size_t n = 0;
    if (!disks_extract(&d, &data, &n, &e)) { fprintf(stderr, "skextract: %s\n", e.msg); disks_close(&d); return 1; }
    disks_close(&d);
    sk_disks check;
    if (!disks_open_archive(&check, data, n, &e)) { fprintf(stderr, "skextract: %s\n", e.msg); free(data); return 1; }
    make_parent(out_path);
    FILE *f = fopen(out_path, "wb");
    bool ok = f && fwrite(data, 1, n, f) == n;
    if (f && fclose(f) != 0) ok = false;
    free(data);
    if (!ok) { fprintf(stderr, "skextract: could not write %s\n", out_path); return 1; }
    printf("skextract: %u files, %zu bytes -> %s\nNow build again (cmake --build build): soccerkid will carry the game inside.\n",
           check.archive_count, n, out_path);
    return 0;
}
