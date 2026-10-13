#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "check.h"
#include "core/bytes.h"
#include "core/zipfile.h"
#include "gfx/png.h"

void test_png_writes_readable_file(void) {
    uint32_t px[4] = { 0xFF0000, 0x00FF00, 0x0000FF, 0x102030 };
    char path[] = "/tmp/sk_png_XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0);
    sk_err e = {0};
    CHECK(png_write(path, 2, 2, px, &e));
    uint8_t *d = NULL;
    size_t n = 0;
    CHECK(file_read_all(path, &d, &n, &e));
    CHECK(n > 40);
    CHECK(d && n > 33 && memcmp(d, "\x89PNG\r\n\x1a\n", 8) == 0);
    CHECK(d && n > 33 && memcmp(d + 12, "IHDR", 4) == 0);
    CHECK_EQ(be32(d + 16), 2);
    CHECK_EQ(be32(d + 20), 2);
    CHECK_EQ(d[24], 8);                    /* bit depth */
    CHECK_EQ(d[25], 2);                    /* colour type RGB */
    /* IDAT: filter byte 0 then 3 bytes per pixel */
    size_t o = 8;
    bool found = false;
    while (o + 12 <= n) {
        uint32_t len = be32(d + o);
        if (memcmp(d + o + 4, "IDAT", 4) == 0) {
            uint8_t raw[2 * (1 + 2 * 3)];
            uLongf rn = sizeof raw;
            CHECK_EQ(uncompress(raw, &rn, d + o + 8, len), Z_OK);
            CHECK_EQ(rn, sizeof raw);
            CHECK(raw[0] == 0 && raw[1] == 0xFF && raw[2] == 0 && raw[3] == 0);
            CHECK(raw[4] == 0 && raw[5] == 0xFF && raw[6] == 0);
            CHECK(raw[7] == 0 && raw[8] == 0 && raw[9] == 0 && raw[10] == 0xFF);
            CHECK(raw[11] == 0x10 && raw[12] == 0x20 && raw[13] == 0x30);
            CHECK_EQ(be32(d + o + 8 + len), crc32(crc32(0, d + o + 4, 4), d + o + 8, len));
            found = true;
        }
        o += 12 + len;
    }
    CHECK(found);
    CHECK(memcmp(d + n - 8, "IEND", 4) == 0);
    free(d);
    close(fd);
    remove(path);
}

void test_png_unwritable_path(void) {     /* Review Focus 5 */
    uint32_t px[1] = { 0 };
    sk_err e = {0};
    CHECK(!png_write("/nonexistent_dir_sk/x.png", 1, 1, px, &e));
    CHECK_STR(e.msg, "cannot write");
    CHECK_STR(e.msg, "/nonexistent_dir_sk/x.png");
}
