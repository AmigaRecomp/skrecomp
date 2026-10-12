#include <stdarg.h>
#include <stdio.h>
#include "core/err.h"

bool sk_fail(sk_err *e, const char *fmt, ...) {
    if (e) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(e->msg, sizeof e->msg, fmt, ap);
        va_end(ap);
    }
    return false;
}
