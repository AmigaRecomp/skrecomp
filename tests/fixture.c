#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fixture.h"

bool load_fixture(const char *name, uint8_t **out, size_t *len) {
    char path[1024];
    snprintf(path, sizeof path, "%s%s", SK_FIXTURES, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "fixture %s: cannot open\n", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(n > 0 ? (size_t)n : 1);
    if (!buf || (n > 0 && fread(buf, 1, (size_t)n, f) != (size_t)n)) {
        fprintf(stderr, "fixture %s: cannot read\n", path);
        free(buf);
        fclose(f);
        return false;
    }
    fclose(f);
    *out = buf;
    *len = (size_t)n;
    return true;
}

bool write_file(const char *path, const uint8_t *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    bool ok = fwrite(data, 1, len, f) == len;
    fclose(f);
    return ok;
}
