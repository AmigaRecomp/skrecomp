/* soccerkid: entry point. For now only usage and version. */
#include <stdio.h>
#include <string.h>

static const char *USAGE = "usage: soccerkid --roms <dir> [--level N] [--screenshot out.png]\n";

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            puts("soccerkid 0.1");
            return 0;
        }
    }
    fputs(USAGE, stderr);
    return 2;
}
