/* skicon: the app icon from your own game data: the kid on the title picture (disk 1 file078, x 0..127, y 30..157), written
   as the PNG sizes of a macOS .iconset (iconutil then makes the .icns). Build tool, no game logic. */
#include <stdio.h>
#include <stdlib.h>
#include "core/disks.h"
#include "core/zipfile.h"
#include "formats/screens.h"
#include "gfx/png.h"
#include "gfx/scale.h"

#define CROP_X 0
#define CROP_Y 30
#define CROP 128

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "usage: skicon <soccerkid.dat> <out.iconset directory>\n"); return 2; }
    uint8_t *dat = NULL, *pic = NULL; size_t dn = 0, pn = 0;
    sk_disks d; sk_err e = {0};
    sk_screen s;
    if (!file_read_all(argv[1], &dat, &dn, &e) || !disks_open_archive(&d, dat, dn, &e) ||
        !disks_read_file_no(&d, 1, 78, true, &pic, &pn, &e) || !screen_parse_title(pic, pn, &s, &e)) {
        fprintf(stderr, "skicon: %s\n", e.msg); free(dat); free(pic); return 1;
    }
    static uint32_t crop[CROP * CROP], out[1024 * 1024];
    for (int y = 0; y < CROP; y++)
        for (int x = 0; x < CROP; x++) crop[y * CROP + x] = s.palette[screen_pixel(&s, CROP_X + x, CROP_Y + y)];
    static const struct { int size; const char *name; } sizes[] = {
        { 16, "icon_16x16" }, { 32, "icon_16x16@2x" }, { 32, "icon_32x32" }, { 64, "icon_32x32@2x" }, { 128, "icon_128x128" },
        { 256, "icon_128x128@2x" }, { 256, "icon_256x256" }, { 512, "icon_256x256@2x" }, { 512, "icon_512x512" }, { 1024, "icon_512x512@2x" } };
    int rc = 0;
    for (size_t i = 0; i < sizeof sizes / sizeof *sizes && !rc; i++) {
        char path[1024];
        snprintf(path, sizeof path, "%s/%s.png", argv[2], sizes[i].name);
        scale_image(crop, CROP, out, sizes[i].size);
        if (!png_write(path, sizes[i].size, sizes[i].size, out, &e)) { fprintf(stderr, "skicon: %s\n", e.msg); rc = 1; }
    }
    disks_close(&d); free(dat); free(pic);
    return rc;
}
