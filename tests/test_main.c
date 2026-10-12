/* sk_tests [name]: runs one named test or all of them; exit code = number of failed checks. */
#include <stdio.h>
#include <string.h>
#include "check.h"

int sk_failures = 0;

void test_sk_fail_formats_and_returns_false(void);

static const struct { const char *name; void (*fn)(void); } tests[] = {
    { "sk_fail_formats_and_returns_false", test_sk_fail_formats_and_returns_false },
};

int main(int argc, char **argv) {
    size_t n = sizeof tests / sizeof tests[0];
    int ran = 0;
    for (size_t i = 0; i < n; i++) {
        if (argc < 2 || strcmp(argv[1], tests[i].name) == 0) {
            tests[i].fn();
            ran++;
        }
    }
    if (ran == 0) {
        fprintf(stderr, "no test named %s\n", argv[1]);
        return 2;
    }
    if (sk_failures) fprintf(stderr, "%d check(s) failed\n", sk_failures);
    return sk_failures ? 1 : 0;
}
