/* Directory listing. The one place that touches a platform directory API. */
#ifndef SK_CORE_DIRLIST_H
#define SK_CORE_DIRLIST_H

#include <stdbool.h>
#include "core/err.h"

/* Calls fn(path, name, ctx) for every regular file in dir (path = dir/name). Order is unspecified. */
bool list_dir(const char *dir, void (*fn)(const char *path, const char *name, void *ctx), void *ctx, sk_err *e);

#endif
