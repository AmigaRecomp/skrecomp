#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/args.h"
#include "app/play.h"

static const char *USAGE =
    "usage: soccerkid [--roms <dir> | --data FILE] [options]\n"
    "  --roms <dir>            directory holding the five disk images (.adf or .zip)\n"
    "  --data FILE             a soccerkid.dat made by skextract (default: the data built into this program)\n"
    "  --custom FILE           play a level made with skld (.skl)\n"
    "  --save FILE             the SaveGame file (default: the user's data folder)\n"
    "  --no-intro              start at the language screen\n"
    "  --controls FILE         the key and gamepad bindings (default: the user's data folder; F12 remaps in the game)\n"
    "  --new-game              skip the language screen and the title: a new game at world map 0\n"
    "  --cup MASK              the cup pieces already held (31 = all five: the ALIEN fights, the good ending)\n"
    "  --level N               play one level, data index 0..41 (default: the whole game from world map 0)\n"
    "  --section fileNNN:S[:WxH]  show the frames of graphics-set file NNN section S (size estimated unless given)\n"
    "  --cam X,Y               camera position in pixels (default 0,0)\n"
    "  --start X,Y             start the kid at world pixel X,Y (respawns come back there too)\n"
    "  --near TYPE[,N]         start the kid 128 px left of the N-th (default 1st) object marker of TYPE\n"
    "  --markers               list the level's object markers (type, param, cell, pixel) and exit\n"
    "  --no-backdrop           draw the foreground only\n"
    "  --no-frames             draw no graphics-set frames\n"
    "  --screenshot out.png    render one frame to a PNG and exit (no window)\n"
    "  --scale K               window scale factor (default 3)\n"
    "  --exit-after N          close the window after N frames\n"
    "  --viewer                the level viewer (arrows scroll) instead of playing\n"
    "  --frames N              headless: run N game frames (50 per second of play), then --screenshot\n"
    "  --script S              scripted input: tokens <keys><frames> joined by commas, keys U D L R F or . (none), e.g. R150,RU20,.30\n"
    "  --help, --version\n";

const char *args_usage(void) { return USAGE; }

static bool parse_int(const char *s, int *out) {
    char *end;
    long v = strtol(s, &end, 10);
    if (*s == 0 || *end != 0) return false;
    *out = (int)v;
    return true;
}

static char section_name[64];

bool args_parse(int argc, char **argv, sk_args *a, sk_err *e) {
    memset(a, 0, sizeof *a);
    a->section = -1;
    a->level = -1;
    a->cup = -1;
    a->scale = 3;
    a->near_type = -1;
    for (int i = 1; i < argc; i++) {
        const char *opt = argv[i];
        const char *val = i + 1 < argc ? argv[i + 1] : NULL;
        if (strcmp(opt, "--help") == 0) { a->help = true; continue; }
        if (strcmp(opt, "--version") == 0) { a->version = true; continue; }
        if (strcmp(opt, "--no-backdrop") == 0) { a->no_backdrop = true; continue; }
        if (strcmp(opt, "--no-frames") == 0) { a->no_frames = true; continue; }
        if (strcmp(opt, "--viewer") == 0) { a->viewer = true; continue; }
        if (strcmp(opt, "--markers") == 0) { a->markers = true; continue; }
        if (strcmp(opt, "--new-game") == 0) { a->new_game = true; continue; }
        if (strcmp(opt, "--no-intro") == 0) { a->no_intro = true; continue; }
        bool takes_value = strcmp(opt, "--roms") == 0 || strcmp(opt, "--level") == 0 || strcmp(opt, "--section") == 0 ||
                           strcmp(opt, "--cam") == 0 || strcmp(opt, "--screenshot") == 0 || strcmp(opt, "--scale") == 0 ||
                           strcmp(opt, "--exit-after") == 0 || strcmp(opt, "--frames") == 0 || strcmp(opt, "--script") == 0 ||
                           strcmp(opt, "--start") == 0 || strcmp(opt, "--near") == 0 || strcmp(opt, "--cup") == 0 ||
                           strcmp(opt, "--save") == 0 || strcmp(opt, "--controls") == 0 || strcmp(opt, "--data") == 0 || strcmp(opt, "--custom") == 0;
        if (!takes_value) return sk_fail(e, "unknown option %s", opt);
        if (!val) return sk_fail(e, "%s needs a value", opt);
        i++;
        if (strcmp(opt, "--roms") == 0) a->roms = val;
        else if (strcmp(opt, "--screenshot") == 0) a->screenshot = val;
        else if (strcmp(opt, "--save") == 0) a->save_path = val;
        else if (strcmp(opt, "--data") == 0) a->data_path = val;
        else if (strcmp(opt, "--custom") == 0) a->custom_path = val;
        else if (strcmp(opt, "--controls") == 0) a->controls_path = val;
        else if (strcmp(opt, "--cup") == 0) { if (!parse_int(val, &a->cup) || a->cup < 0 || a->cup > 31) return sk_fail(e, "--cup: a mask 0..31, got %s", val); }
        else if (strcmp(opt, "--level") == 0) { if (!parse_int(val, &a->level)) return sk_fail(e, "--level: not a number: %s", val); }
        else if (strcmp(opt, "--scale") == 0) { if (!parse_int(val, &a->scale) || a->scale < 1) return sk_fail(e, "--scale: bad value %s", val); }
        else if (strcmp(opt, "--exit-after") == 0) { if (!parse_int(val, &a->exit_after)) return sk_fail(e, "--exit-after: not a number: %s", val); }
        else if (strcmp(opt, "--frames") == 0) { if (!parse_int(val, &a->frames) || a->frames < 0) return sk_fail(e, "--frames: bad value %s", val); }
        else if (strcmp(opt, "--script") == 0) {
            const char *cur = val; uint8_t in; int n;
            while (*cur) if (!script_parse_step(&cur, &in, &n, e)) return false;
            a->script = val;
        }
        else if (strcmp(opt, "--start") == 0) {
            char buf[64];
            snprintf(buf, sizeof buf, "%s", val);
            char *comma = strchr(buf, ',');
            if (!comma) return sk_fail(e, "--start: expected X,Y, got %s", val);
            *comma = 0;
            if (!parse_int(buf, &a->start_x) || !parse_int(comma + 1, &a->start_y)) return sk_fail(e, "--start: expected X,Y, got %s", val);
            a->has_start = true;
        }
        else if (strcmp(opt, "--near") == 0) {
            char buf[64];
            snprintf(buf, sizeof buf, "%s", val);
            char *comma = strchr(buf, ',');
            a->near_n = 1;
            if (comma) { *comma = 0; if (!parse_int(comma + 1, &a->near_n) || a->near_n < 1) return sk_fail(e, "--near: bad count in %s", val); }
            if (!parse_int(buf, &a->near_type) || a->near_type < 1 || a->near_type > 127) return sk_fail(e, "--near: expected a type 1..127, got %s", val);
        }
        else if (strcmp(opt, "--cam") == 0) {
            char buf[64];
            snprintf(buf, sizeof buf, "%s", val);
            char *comma = strchr(buf, ',');
            if (!comma) return sk_fail(e, "--cam: expected X,Y, got %s", val);
            *comma = 0;
            if (!parse_int(buf, &a->cam_x) || !parse_int(comma + 1, &a->cam_y)) return sk_fail(e, "--cam: expected X,Y, got %s", val);
        } else if (strcmp(opt, "--section") == 0) {
            snprintf(section_name, sizeof section_name, "%s", val);
            char *colon = strchr(section_name, ':');
            if (!colon) return sk_fail(e, "--section: expected fileNNN:S[:WxH], got %s", val);
            *colon = 0;
            char *size = strchr(colon + 1, ':');
            if (size) *size++ = 0;
            if (!parse_int(colon + 1, &a->section) || a->section < 0) return sk_fail(e, "--section: bad section number in %s", val);
            if (size) {
                char *x = strchr(size, 'x');
                if (!x) return sk_fail(e, "--section: expected WxH after the second colon in %s", val);
                *x = 0;
                if (!parse_int(size, &a->frame_w) || !parse_int(x + 1, &a->frame_h) || a->frame_w <= 0 || a->frame_h <= 0)
                    return sk_fail(e, "--section: bad frame size in %s", val);
            }
            a->section_file = section_name;
        }
    }
    if (a->custom_path && (a->level >= 0 || a->viewer || a->new_game || a->markers || a->section_file))
        return sk_fail(e, "--custom cannot be combined with --level, --viewer, --new-game, --markers or --section");
    if (a->level < 0 && (a->viewer || a->markers || a->near_type > 0 || a->has_start || a->section_file)) a->level = 0;   /* these need a level */
    return true;
}
