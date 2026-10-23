#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor/skld_args.h"

static const char *world_of(int index) {
    return index <= 5 ? "England" : index <= 11 ? "Italy" : index <= 17 ? "Russia" : index <= 22 ? "Japan" : index <= 28 ? "USA" : "Bonus";
}

const char *skld_usage(void) {                              /* built once from the settings table */
    static char u[4096];
    if (u[0]) return u;
    size_t n = (size_t)snprintf(u, sizeof u,
        "usage: skld new SETTING FILE [--size WxH]   make a new level on SETTING, from a copy of its map\n"
        "                                            (or, with --size, an empty map with a floor, a start and an exit)\n"
        "       skld FILE                            open a level made earlier\n"
        "       skld --settings                      list the settings\n"
        "options: --roms DIR (your disk images) or --data FILE (a soccerkid.dat); default: the data built in\n"
        "\n"
        "SETTING is one of the original levels; the new level uses its tiles, enemies, objects, backdrop and music:\n");
    const char *world = "";
    int col = 0;
    for (int i = 0; i < custom_setting_count() && n < sizeof u; i++) {
        const sk_setting *st = custom_setting_at(i);
        const char *w = world_of(st->index);
        if (strcmp(w, world) != 0) { n += (size_t)snprintf(u + n, sizeof u - n, "%s  %-9s", col ? "\n" : "", w); world = w; col = 0; }
        else if (col == 2) { n += (size_t)snprintf(u + n, sizeof u - n, "\n  %-9s", ""); col = 0; }
        if (n < sizeof u) n += (size_t)snprintf(u + n, sizeof u - n, " %-11s %-17s", st->name, st->title);
        col++;
    }
    if (n < sizeof u)
        snprintf(u + n, sizeof u - n, "\n\n"
            "Not available: the boss levels (LONDON2, RIVIERA 2, RED SQUARE 2, FACTORY2, CITY2), BATTLE SHIP and\n"
            "JAPAN TRAIN (their own scrolling), the scoreboard and the ALIEN.\n"
            "\n"
            "examples: skld new london1 mylevel.skl\n"
            "          skld new bonus3 small.skl --size 60x17\n"
            "          skld mylevel.skl                then play it with: soccerkid --custom mylevel.skl\n");
    for (char *r = u, *w = u; ; r++) {                       /* no spaces at the ends of the lines */
        if (*r == '\n' || !*r) while (w > u && w[-1] == ' ') w--;
        if (!(*w++ = *r)) break;
    }
    return u;
}

bool skld_args_parse(int argc, char **argv, skld_args *a, sk_err *e) {
    memset(a, 0, sizeof *a);
    a->copy = true;
    const char *pos[3];
    int npos = 0;
    bool settings = false, help = false;
    for (int i = 1; i < argc; i++) {
        const char *o = argv[i], *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (strcmp(o, "--settings") == 0) { settings = true; continue; }
        if (strcmp(o, "--help") == 0 || strcmp(o, "-h") == 0) { help = true; continue; }
        if (strcmp(o, "--roms") == 0 || strcmp(o, "--data") == 0 || strcmp(o, "--size") == 0 || strcmp(o, "--exit-after") == 0 || strcmp(o, "--screenshot") == 0) {
            if (!v) return sk_fail(e, "%s needs a value", o);
            i++;
            if (o[2] == 'r') a->roms = v;
            else if (o[2] == 'd') a->data_path = v;
            else if (o[2] == 's' && o[3] == 'c') a->screenshot = v;
            else if (o[2] == 'e') { a->exit_after = atoi(v); if (a->exit_after <= 0) return sk_fail(e, "--exit-after: a positive count, got %s", v); }
            else {
                char *x = NULL;
                long w = strtol(v, &x, 10), h = x && *x == 'x' ? strtol(x + 1, &x, 10) : -1;
                if (!x || *x || w <= 0 || h <= 0) return sk_fail(e, "--size: expected WxH, got %s", v);
                if (!custom_check_size((int)w, (int)h, e)) return false;
                a->w = (int)w; a->h = (int)h; a->copy = false;
            }
            continue;
        }
        if (o[0] == '-' && o[1] == '-') return sk_fail(e, "unknown option %s", o);
        if (npos == 3) return sk_fail(e, "too many arguments");
        pos[npos++] = o;
    }
    if (help) { a->mode = SKLD_HELP; return true; }
    if (settings) { a->mode = SKLD_SETTINGS; return true; }
    if (npos == 0) { a->mode = SKLD_HELP; return true; }
    if (strcmp(pos[0], "new") == 0) {
        if (npos != 3) return sk_fail(e, "expected: skld new SETTING FILE");
        a->setting = custom_setting_by_name(pos[1]);
        if (!a->setting) return sk_fail(e, "unknown setting \"%s\" (skld --settings lists them)", pos[1]);
        a->mode = SKLD_NEW; a->file = pos[2];
        return true;
    }
    if (npos != 1) return sk_fail(e, "expected one level file, or: skld new SETTING FILE");
    if (!a->copy) return sk_fail(e, "--size is for skld new");
    a->mode = SKLD_OPEN; a->file = pos[0];
    return true;
}
