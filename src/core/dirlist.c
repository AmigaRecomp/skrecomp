#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "core/dirlist.h"

bool list_dir(const char *dir, void (*fn)(const char *path, const char *name, void *ctx), void *ctx, sk_err *e) {
    DIR *d = opendir(dir);
    if (!d) return sk_fail(e, "%s: cannot list directory", dir);
    struct dirent *ent;
    char path[2048];
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        snprintf(path, sizeof path, "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        fn(path, ent->d_name, ctx);
    }
    closedir(d);
    return true;
}
