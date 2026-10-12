/* Whole-file reading and extraction of one entry from a .zip (stored or deflate via zlib). */
#ifndef SK_CORE_ZIPFILE_H
#define SK_CORE_ZIPFILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

/* Reads a whole file into a malloc'ed buffer. */
bool file_read_all(const char *path, uint8_t **out, size_t *len, sk_err *e);

/* Extracts the first entry whose name ends with name_suffix (case-insensitive) into a malloc'ed buffer. */
bool zip_read_entry(const char *path, const char *name_suffix, uint8_t **out, size_t *len, sk_err *e);

#endif
