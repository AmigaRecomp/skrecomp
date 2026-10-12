#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/bytekiller.h"

void test_bk_unpacks_literal_fixture(void) {
    uint8_t *packed = NULL, *plain = NULL, *out = NULL;
    size_t np = 0, npl = 0, no = 0;
    sk_err e = {0};
    CHECK(load_fixture("bk_literal.bin", &packed, &np) && load_fixture("bk_literal.plain", &plain, &npl));
    CHECK(bk_is_packed(packed, np));
    CHECK(bk_unpack(packed, np, &out, &no, &e));
    CHECK_EQ(no, npl);
    CHECK(out != NULL && plain != NULL && no == npl && memcmp(out, plain, npl) == 0);
    free(packed); free(plain); free(out);
}

void test_bk_rejects_bad_checksum(void) {
    uint8_t *packed = NULL, *out = NULL;
    size_t np = 0, no = 0;
    sk_err e = {0};
    CHECK(load_fixture("bk_literal.bin", &packed, &np));
    packed[np - 8] ^= 0x40;                               /* the stored checksum word */
    CHECK(!bk_unpack(packed, np, &out, &no, &e));
    CHECK_STR(e.msg, "checksum");
    CHECK(out == NULL);
    free(packed);
    /* not a container at all */
    uint8_t junk[8] = {9, 9, 9, 9, 0, 0, 0, 0};
    CHECK(!bk_is_packed(junk, sizeof junk));
    CHECK(!bk_unpack(junk, sizeof junk, &out, &no, &e));
    CHECK_STR(e.msg, "length-prefixed");
}

void test_bk_rejects_overrun(void) {
    /* One longword of bits asking for a match of 2 at offset 200 into a 4-byte output:
       token "01" then 8 bits of offset (200 = 11001000), consumed LSB-first from the longword. */
    uint8_t payload[12] = {0};
    uint32_t bits = 0;
    int n = 0;
    const int seq[] = {0, 1, 1, 1, 0, 0, 1, 0, 0, 0};
    for (int i = 0; i < 10; i++) bits |= (uint32_t)seq[i] << n++;
    bits |= 1u << n;                                      /* the sentinel bit above the data */
    payload[0] = (uint8_t)(bits >> 24); payload[1] = (uint8_t)(bits >> 16);
    payload[2] = (uint8_t)(bits >> 8);  payload[3] = (uint8_t)bits;
    memcpy(payload + 4, payload, 4);                      /* checksum = the single longword */
    payload[11] = 4;                                      /* unpacked length */
    uint8_t *out = NULL;
    size_t no = 0;
    sk_err e = {0};
    CHECK(!bk_decompress(payload, sizeof payload, &out, &no, &e));
    CHECK_STR(e.msg, "outside");
    CHECK(out == NULL);
    /* too short / unaligned payloads */
    CHECK(!bk_decompress(payload, 11, &out, &no, &e));
    CHECK_STR(e.msg, "longword");
}
