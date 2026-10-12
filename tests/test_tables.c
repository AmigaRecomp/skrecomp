#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "core/bytes.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "core/tables.h"

void test_tables_real_sk(void) {                       /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0};
    sk_err e = {0};
    CHECK(disks_open(&d, roms, &e));
    uint8_t *f = NULL;
    size_t n = 0;
    CHECK(disks_read(&d, 0, "sk.pak", &f, &n, &e));
    sk_image img = {0};
    CHECK(hunk_load(f, n, &img, &e));
    CHECK_EQ(img.count, 5);
    sk_tables t = {0};
    CHECK(tables_load(&t, &img, &e));
    CHECK_EQ(t.sequence[0], 30);
    CHECK_EQ(t.sequence[1], 0);
    CHECK_EQ(t.sequence[3], 12);
    CHECK_EQ(t.record[0].disk, 0);
    CHECK_EQ(t.record[17].disk, 2);
    CHECK_EQ(t.record[17].music, 81);
    CHECK_EQ(t.n_res[0], 41);
    CHECK_EQ(t.n_res[3], 66);
    CHECK(t.record[0].n_res > 0);
    const sk_resource *r = tables_resource(&t, 0, t.record[0].res[0]);
    CHECK(r != NULL);
    CHECK(tables_resource(&t, 0, 200) == NULL);
    bool found = false;                                 /* disk 1's table binds file031 section 2 to slot 108 */
    for (int i = 0; i < t.n_res[0]; i++)
        if (t.res[0][i].file == 31 && t.res[0][i].section == 2 && t.res[0][i].slot == 108) found = true;
    CHECK(found);
    tables_free(&t);
    image_free(&img);
    free(f);
    disks_close(&d);
}

static void make_image(sk_image *img, size_t hunk1_size) {
    memset(img, 0, sizeof *img);
    img->count = 2;
    img->seg[0].base = 0x100000; img->seg[0].size = 16; img->seg[0].mem = calloc(16, 1);
    img->seg[1].base = 0x200000; img->seg[1].size = (uint32_t)hunk1_size; img->seg[1].mem = calloc(hunk1_size, 1);
}

void test_tables_rejects_bad_pointers(void) {
    sk_image img;
    sk_tables t = {0};
    sk_err e = {0};
    make_image(&img, 0x2200);
    uint8_t *tab = img.seg[1].mem + 0x1CA4;             /* DISK_TABLES_ADDR - 0x200000 */
    put_be32(tab + 0, 0x201E00);                          /* p_res after p_tab2: negative count */
    put_be32(tab + 4, 0x201D00);
    put_be32(tab + 8, 0x201F00);
    CHECK(!tables_load(&t, &img, &e));
    CHECK_STR(e.msg, "inconsistent");
    image_free(&img);
    make_image(&img, 0x1000);                             /* hunk 1 too short for the sequence */
    CHECK(!tables_load(&t, &img, &e));
    CHECK_STR(e.msg, "outside the image");
    image_free(&img);
}
