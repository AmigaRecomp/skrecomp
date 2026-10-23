/* Command line of soccerkid. */
#ifndef SK_APP_ARGS_H
#define SK_APP_ARGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

typedef struct {
    const char *roms;           /* directory with the disk images; NULL: --data or the embedded data */
    const char *data_path;      /* --data FILE: a soccerkid.dat made by skextract */
    const char *custom_path;    /* --custom FILE: a level made with skld */
    const struct sk_custom_level *custom;   /* the loaded custom level (main, skld) */
    const uint8_t *data; size_t data_len;   /* the archive in memory (main: the embedded copy or --data's file) */
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
    bool viewer;                /* the sub-project 3 level viewer instead of playing */
    int frames;                 /* headless: run N game frames, then the screenshot */
    const char *script;         /* scripted input: "R150,RU20,.30" */
    bool has_start; int start_x, start_y;   /* --start X,Y: the kid's start in world pixels */
    int near_type, near_n;      /* --near TYPE[,N]: start beside the N-th marker of TYPE (<= 0: off) */
    bool markers;               /* --markers: list the level's object markers and exit */
    const char *save_path;
    const char *controls_path;  /* --controls FILE: the key and pad bindings (default: the user's data folder) */      /* --save FILE: the SaveGame file (default: the user's data folder) */
    bool no_intro;              /* --no-intro: start at the language screen */
    bool new_game;              /* --new-game: straight into a new game at world map 0, no frontend */
    int cup;                    /* --cup MASK: the cup pieces held at the start (0..31), -1 = none given */
    bool help, version;
} sk_args;

bool args_parse(int argc, char **argv, sk_args *a, sk_err *e);
const char *args_usage(void);

#endif
