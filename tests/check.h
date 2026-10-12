/* Minimal check macros for the sk_tests executable. */
#ifndef SK_TESTS_CHECK_H
#define SK_TESTS_CHECK_H

#include <stdio.h>
#include <string.h>

extern int sk_failures;

#define CHECK(c) do { if (!(c)) { sk_failures++; fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECK_EQ(a, b) do { long long _a = (long long)(a), _b = (long long)(b); if (_a != _b) { sk_failures++; \
    fprintf(stderr, "%s:%d: %s == %lld, expected %lld\n", __FILE__, __LINE__, #a, _a, _b); } } while (0)
#define CHECK_STR(s, sub) do { const char *_s = (s), *_sub = (sub); if (_s == NULL || strstr(_s, _sub) == NULL) { sk_failures++; \
    fprintf(stderr, "%s:%d: \"%s\" does not contain \"%s\"\n", __FILE__, __LINE__, _s ? _s : "(null)", _sub); } } while (0)

#endif
