#include <stdio.h>
#include <string.h>
#include "check.h"
#include "core/err.h"

void test_sk_fail_formats_and_returns_false(void) {
    sk_err e = {0};
    bool r = sk_fail(&e, "disk %d: %s not found", 1, "FILE031");
    CHECK(!r);
    CHECK(strcmp(e.msg, "disk 1: FILE031 not found") == 0);
    CHECK(!SK_OK(&e));
    CHECK(!sk_fail(NULL, "ignored %d", 1));      /* NULL err is allowed */
    sk_err ok = {0};
    sk_err *none = NULL;
    CHECK(SK_OK(&ok) && SK_OK(none));
}
