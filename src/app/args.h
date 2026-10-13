/* Command line of soccerkid. */
#ifndef SK_APP_ARGS_H
#define SK_APP_ARGS_H

#include <stdbool.h>
#include "core/err.h"

typedef struct {
    const char *roms;           /* directory with the disk images (required unless --help/--version) */
    int level;                  /* data index 0..41 (0 = England 1, 12 = bonus) */
    const char *section_file;   /* --section fileNNN:S[:WxH]: graphics-set file name on the level's disk */
    int section;                /* -1 = the level record's first graphics set */
    int frame_w, frame_h;       /* 0 = estimate the frame size from the data */
    const char *screenshot;     /* write this PNG headless and exit */
    int cam_x, cam_y;
    bool no_backdrop;
    bool no_frames;             /* do not draw any graphics-set frames (cross-check renders) */
    int scale;                  /* window scale factor */
    int exit_after;             /* stop the window loop after N frames (tests) */
    bool help, version;
} sk_args;

bool args_parse(int argc, char **argv, sk_args *a, sk_err *e);
const char *args_usage(void);

#endif
