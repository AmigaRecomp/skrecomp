/* ByteKiller decompression, the format every packed Soccer Kid file uses (re/docs/notes/compression.md).
   A "container" is a u32 big-endian payload length followed by the payload; the payload ends with
   [bit buffer][checksum][unpacked length] and is read backwards. */
#ifndef SK_CORE_BYTEKILLER_H
#define SK_CORE_BYTEKILLER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

/* True when d looks like a container: n >= 4 and the length word equals n - 4. */
bool bk_is_packed(const uint8_t *d, size_t n);

/* Decompresses a payload (without the length word). *out is malloc'ed; on failure *out is NULL. */
bool bk_decompress(const uint8_t *payload, size_t n, uint8_t **out, size_t *len, sk_err *e);

/* Decompresses a container (with the length word). */
bool bk_unpack(const uint8_t *container, size_t n, uint8_t **out, size_t *len, sk_err *e);

#endif
