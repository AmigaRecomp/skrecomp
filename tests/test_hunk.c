#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/bytes.h"
#include "core/hunk.h"

void test_hunk_loads_crunched_data_hunk_with_reloc(void) {
    uint8_t *f = NULL, *plain = NULL;
    size_t n = 0, pn = 0;
    sk_image img = {0};
    sk_err e = {0};
    CHECK(load_fixture("hunk_crunched.bin", &f, &n));
    CHECK(load_fixture("bk_literal.plain", &plain, &pn));
    CHECK(hunk_load(f, n, &img, &e));
    CHECK_EQ(img.count, 2);
    CHECK_EQ(img.seg[0].base, 0x100000);
    CHECK_EQ(img.seg[1].base, 0x200000);
    CHECK_EQ(img.seg[0].size, 304);                             /* the allocation, not the unpacked 303 */
    CHECK_EQ(img.seg[1].size, 16);
    CHECK(img.seg[1].chip && !img.seg[0].chip);
    const uint8_t *p = image_ptr(&img, 0x100000, 303);
    CHECK(p != NULL);
    if (p && plain) {
        CHECK(memcmp(p + 4, plain + 4, 299) == 0);              /* bytes after the relocated longword */
        CHECK_EQ(be32(p), be32(plain) + 0x200000);              /* RELOC32 at offset 0 targets hunk 1 */
    }
    CHECK(image_ptr(&img, 0x100000 + 300, 8) == NULL);          /* crosses the end of hunk 0 */
    CHECK(image_ptr(&img, 0x300000, 1) == NULL);
    CHECK(image_ptr(&img, 0x200000, 16) != NULL);
    CHECK_EQ(image_u16(&img, 0x100004), be16(plain + 4));
    image_free(&img);
    CHECK(img.count == 0 && img.seg[0].mem == NULL);
    free(f);
    free(plain);
}

void test_hunk_rejects_truncated_file(void) {
    uint8_t *f = NULL;
    size_t n = 0;
    sk_image img = {0};
    sk_err e = {0};
    CHECK(load_fixture("hunk_crunched.bin", &f, &n));
    CHECK(!hunk_load(f, 40, &img, &e));
    CHECK_STR(e.msg, "truncated");
    CHECK(img.count == 0);
    f[3] = 0xF4;                                                 /* break the HUNK_HEADER magic */
    CHECK(!hunk_load(f, n, &img, &e));
    CHECK_STR(e.msg, "HUNK_HEADER");
    free(f);
}
