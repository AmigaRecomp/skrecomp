/* skld's command line (pure, for the tests). */
#ifndef SK_EDITOR_SKLD_ARGS_H
#define SK_EDITOR_SKLD_ARGS_H

#include <stdbool.h>
#include "core/err.h"
#include "formats/custom.h"

typedef enum { SKLD_HELP, SKLD_SETTINGS, SKLD_NEW, SKLD_OPEN } skld_mode;
typedef struct {
    skld_mode mode;
    const char *file;               /* the level */
    const sk_setting *setting;      /* new: the setting */
    bool copy;                      /* new: start from the setting's own map (else empty, w x h) */
    int w, h;
    const char *roms, *data_path;
    const char *screenshot;         /* tests: the first frame as a PNG, then exit */
    int exit_after;                 /* tests: stop after this many redraws (0: run until quit) */
} skld_args;

bool skld_args_parse(int argc, char **argv, skld_args *a, sk_err *e);
const char *skld_usage(void);

#endif
