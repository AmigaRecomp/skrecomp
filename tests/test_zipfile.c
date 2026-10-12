#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/zipfile.h"

void test_zip_reads_deflated_adf_entry(void) {
    uint8_t *d = NULL;
    size_t n = 0;
    sk_err e = {0};
    CHECK(zip_read_entry(SK_FIXTURES "SKDisk1.zip", ".adf", &d, &n, &e));
    CHECK_EQ(n, 901120);
    CHECK(d != NULL && n >= 3 && memcmp(d, "DOS", 3) == 0);
    free(d);
}

void test_zip_reports_missing_entry_and_bad_file(void) {
    uint8_t *d = NULL;
    size_t n = 0;
    sk_err e = {0};
    CHECK(!zip_read_entry(SK_FIXTURES "SKDisk1.zip", ".xyz", &d, &n, &e));
    CHECK_STR(e.msg, ".xyz");
    CHECK(d == NULL);
    CHECK(!zip_read_entry(SK_FIXTURES "bk_literal.plain", ".adf", &d, &n, &e));
    CHECK_STR(e.msg, "not a zip");
    CHECK(!zip_read_entry(SK_FIXTURES "no-such-file.zip", ".adf", &d, &n, &e));
    CHECK_STR(e.msg, "cannot read");
}

void test_file_read_all_reads_and_reports(void) {
    uint8_t *d = NULL;
    size_t n = 0;
    sk_err e = {0};
    CHECK(file_read_all(SK_FIXTURES "bk_literal.plain", &d, &n, &e));
    CHECK_EQ(n, 303);
    CHECK(d != NULL && n == 303 && memcmp(d + 300, "END", 3) == 0);
    free(d);
    CHECK(!file_read_all(SK_FIXTURES "no-such-file", &d, &n, &e));
    CHECK_STR(e.msg, "no-such-file");
}
