/* AmigaDOS floppy images (ADF, OFS or FFS): volume name and file reading. Port of re/skre/adf.py. */
#ifndef SK_CORE_ADF_H
#define SK_CORE_ADF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

typedef struct {
    const uint8_t *data;    /* the image, not owned */
    size_t len;
    bool ffs;
    char name[32];          /* volume name */
} sk_adf;

/* Validates the boot and root blocks and reads the volume name. */
bool adf_open(sk_adf *v, const uint8_t *data, size_t len, sk_err *e);

/* Finds a file by name (case-insensitive, root directory and subdirectories) and reads it. */
bool adf_read_file(const sk_adf *v, const char *name, uint8_t **out, size_t *len, sk_err *e);

#endif
