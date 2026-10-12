/* Error reporting: every decoder returns false and leaves a message here. */
#ifndef SK_CORE_ERR_H
#define SK_CORE_ERR_H

#include <stdbool.h>

typedef struct sk_err {
    char msg[200];          /* msg[0] == 0 means no error */
} sk_err;

/* Formats the message into e (when e is not NULL) and returns false. */
bool sk_fail(sk_err *e, const char *fmt, ...);

#define SK_OK(e) ((e) == NULL || (e)->msg[0] == 0)

#endif
