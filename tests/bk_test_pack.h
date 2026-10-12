/* Test-only ByteKiller encoder (literal runs only), a port of re/tests/helpers.py lit/encode/bk_pack. */
#ifndef SK_TESTS_BK_TEST_PACK_H
#define SK_TESTS_BK_TEST_PACK_H

#include <stddef.h>
#include <stdint.h>

/* Produces a container (u32 length + payload) that bk_unpack turns back into data. Caller frees *out. */
void bk_test_pack(const uint8_t *data, size_t n, uint8_t **out, size_t *len);

#endif
