/* Byte-order helpers: the game data is big-endian (68000), zip headers are little-endian. */
#ifndef SK_CORE_BYTES_H
#define SK_CORE_BYTES_H

#include <stdint.h>

static inline uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
}

static inline uint16_t be16(const uint8_t *p) {
    return (uint16_t)((unsigned)p[0] << 8 | p[1]);
}

static inline uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[3] << 24 | (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | (uint32_t)p[0];
}

static inline uint16_t le16(const uint8_t *p) {
    return (uint16_t)((unsigned)p[1] << 8 | p[0]);
}

static inline void put_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

static inline void put_be16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v;
}

#endif
