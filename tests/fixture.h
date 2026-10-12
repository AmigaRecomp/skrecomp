/* Test helpers: fixture files under SK_FIXTURES. */
#ifndef SK_TESTS_FIXTURE_H
#define SK_TESTS_FIXTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Reads tests/fixtures/<name>; returns false (and prints) when missing. Caller frees *out. */
bool load_fixture(const char *name, uint8_t **out, size_t *len);

/* Writes bytes to path; returns false on failure. */
bool write_file(const char *path, const uint8_t *data, size_t len);

#endif
