#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/adf.h"
#include "core/bytes.h"
#include "core/zipfile.h"

/* The synthetic SKDisk1 image comes out of the committed zip (the .adf itself is not in git). */
static bool load_image(uint8_t **img, size_t *n) {
    sk_err e = {0};
    if (!zip_read_entry(SK_FIXTURES "SKDisk1.zip", ".adf", img, n, &e)) {
        fprintf(stderr, "fixture: %s\n", e.msg);
        return false;
    }
    return true;
}

void test_adf_opens_volume_and_reads_file(void) {
    uint8_t *img = NULL;
    size_t n = 0;
    sk_adf v;
    sk_err e = {0};
    CHECK(load_image(&img, &n));
    CHECK(adf_open(&v, img, n, &e));
    CHECK(strcmp(v.name, "SKDisk1") == 0);
    CHECK(v.ffs);
    uint8_t *f = NULL, *ref = NULL;
    size_t fn = 0, rn = 0;
    CHECK(adf_read_file(&v, "FILE000", &f, &fn, &e));               /* case-insensitive */
    CHECK(load_fixture("level_tiny.bin", &ref, &rn));
    CHECK_EQ(fn, rn);
    CHECK(f != NULL && ref != NULL && fn == rn && memcmp(f, ref, rn) == 0);
    free(f);
    f = NULL;
    CHECK(!adf_read_file(&v, "nope", &f, &fn, &e));
    CHECK_STR(e.msg, "nope");
    CHECK(f == NULL);
    free(ref);
    free(img);
}

void test_adf_rejects_damaged_images(void) {
    uint8_t *img = NULL;
    size_t n = 0;
    sk_adf v;
    sk_err e = {0};
    CHECK(load_image(&img, &n));
    CHECK(!adf_open(&v, img, 1000, &e));
    CHECK_STR(e.msg, "too small");
    img[0] = 'X';
    CHECK(!adf_open(&v, img, n, &e));
    CHECK_STR(e.msg, "AmigaDOS");
    img[0] = 'D';
    /* a hash-chain loop: the first used hash slot's block points back to itself */
    uint8_t *root = img + 880 * 512;
    uint32_t b = 0;
    for (int i = 0; i < 72 && b == 0; i++) b = be32(root + 24 + 4 * i);
    CHECK(b != 0);
    put_be32(img + b * 512 + 512 - 16, b);
    CHECK(adf_open(&v, img, n, &e));
    uint8_t *f = NULL;
    size_t fn = 0;
    CHECK(!adf_read_file(&v, "nope", &f, &fn, &e));
    CHECK_STR(e.msg, "cycle");
    free(img);
}

void test_adf_rejects_out_of_range_pointers(void) {   /* hostile images must fail, never index past the bitmap */
    uint8_t *img = NULL;
    size_t n = 0;
    sk_adf v;
    sk_err e = {0};
    uint8_t *f = NULL;
    size_t fn = 0;
    CHECK(load_image(&img, &n));
    uint8_t *root = img + 880 * 512;
    uint32_t saved = be32(root + 24);
    put_be32(root + 24, 0xFFFFFFF0u);                  /* first hash slot points far outside the image */
    CHECK(adf_open(&v, img, n, &e));
    CHECK(!adf_read_file(&v, "nope", &f, &fn, &e));
    CHECK_STR(e.msg, "out of range");
    put_be32(root + 24, saved);
    /* the first file's extension pointer points outside the image */
    uint32_t b = 0;
    for (int i = 0; i < 72 && b == 0; i++) b = be32(root + 24 + 4 * i);
    CHECK(b != 0);
    put_be32(img + b * 512 + 512 - 8, 0x7FFFFFFFu);
    CHECK(!adf_read_file(&v, "file000", &f, &fn, &e));
    CHECK_STR(e.msg, "out of range");
    CHECK(f == NULL);
    free(img);
}
