/* Graphics-set files: a u16 header length, (header/2 - 1) u16 section start offsets, then sections
   that each start with a u32: equal to the section length -> raw, equal to length - 4 -> ByteKiller.
   Port of re/skre/formats/containers.py read_sections. */
#ifndef SK_FORMATS_SECTIONS_H
#define SK_FORMATS_SECTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

typedef struct {
    bool packed;
    uint8_t *data;          /* owned: decompressed payload, or the raw bytes after the length field */
    size_t len;
} sk_section;

typedef struct {
    sk_section *sec;        /* owned */
    int count;
} sk_sections;

bool sections_parse(const uint8_t *data, size_t len, sk_sections *s, sk_err *e);
void sections_free(sk_sections *s);

#endif
