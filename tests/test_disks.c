#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "check.h"
#include "fixture.h"
#include "core/disks.h"
#include "core/zipfile.h"

/* A temp dir holding the synthetic SKDisk1 image twice: as a zip under a lying name and as a plain .adf. */
static bool make_dir_with_disk1(char *dir, size_t cap) {
    snprintf(dir, cap, "/tmp/sk_disks_XXXXXX");
    if (!mkdtemp(dir)) return false;
    uint8_t *zip = NULL, *adf = NULL;
    size_t zn = 0, an = 0;
    sk_err e = {0};
    if (!load_fixture("SKDisk1.zip", &zip, &zn)) return false;
    if (!zip_read_entry(SK_FIXTURES "SKDisk1.zip", ".adf", &adf, &an, &e)) { free(zip); return false; }
    char path[1200];
    snprintf(path, sizeof path, "%s/a.zip", dir);
    bool ok = write_file(path, zip, zn);
    snprintf(path, sizeof path, "%s/b.adf", dir);
    ok = ok && write_file(path, adf, an);
    snprintf(path, sizeof path, "%s/notes.txt", dir);
    ok = ok && write_file(path, (const uint8_t *)"not an image", 12);
    free(zip);
    free(adf);
    return ok;
}

void test_disks_opens_zip_or_adf_by_volume_name(void) {
    char dir[64];
    CHECK(make_dir_with_disk1(dir, sizeof dir));
    sk_disks d = {0};
    sk_err e = {0};
    CHECK(!disks_open(&d, dir, &e));                     /* only disk 1 is there */
    CHECK_STR(e.msg, "SKDisk0");
    CHECK_STR(e.msg, dir);
    CHECK(d.image[1] != NULL && d.vol[1].ffs && strcmp(d.vol[1].name, "SKDisk1") == 0);
    CHECK(d.image[0] == NULL && d.image[2] == NULL);
    uint8_t *f = NULL, *ref = NULL;
    size_t n = 0, rn = 0;
    CHECK(disks_read_file_no(&d, 1, 0, false, &f, &n, &e));
    CHECK(load_fixture("level_tiny.bin", &ref, &rn));
    CHECK_EQ(n, rn);
    CHECK(f && ref && n == rn && memcmp(f, ref, rn) == 0);
    free(f);
    free(ref);
    f = NULL;
    CHECK(!disks_read_file_no(&d, 1, 999, false, &f, &n, &e));
    CHECK_STR(e.msg, "file999");
    CHECK(!disks_read_file_no(&d, 0, 0, false, &f, &n, &e));
    CHECK_STR(e.msg, "disk 0");
    CHECK(!disks_read_file_no(&d, 7, 0, false, &f, &n, &e));
    CHECK_STR(e.msg, "no such disk");
    /* unpacking: file031 is a sections file, not a ByteKiller container */
    CHECK(!disks_read_file_no(&d, 1, 31, true, &f, &n, &e));
    CHECK_STR(e.msg, "file031");
    disks_close(&d);
    CHECK(d.image[1] == NULL);
}

void test_disks_reports_missing_volume(void) {         /* Review Focus 1 */
    char dir[64];
    snprintf(dir, sizeof dir, "/tmp/sk_disks_XXXXXX");
    CHECK(mkdtemp(dir) != NULL);
    sk_disks d = {0};
    sk_err e = {0};
    CHECK(!disks_open(&d, dir, &e));
    CHECK_STR(e.msg, dir);
    CHECK_STR(e.msg, "SKDisk0");
    disks_close(&d);
    CHECK(!disks_open(&d, "/nonexistent/sk/dir", &e));
    CHECK_STR(e.msg, "/nonexistent/sk/dir");
}

void test_disks_real_images(void) {                    /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0};
    sk_err e = {0};
    CHECK(disks_open(&d, roms, &e));
    for (int i = 0; i < 5; i++) CHECK(d.image[i] != NULL);
    uint8_t *f = NULL;
    size_t n = 0;
    CHECK(disks_read(&d, 0, "language.pak", &f, &n, &e));
    CHECK_EQ(n, 7664);
    free(f);
    f = NULL;
    CHECK(disks_read_file_no(&d, 1, 78, true, &f, &n, &e));   /* the title picture, unpacked */
    CHECK_EQ(n, 91224);
    free(f);
    disks_close(&d);
}

#include "core/bytes.h"
static size_t make_archive(uint8_t *a, const char *name, int disk, const char *body) {   /* one entry */
    memset(a, 0, 256);
    memcpy(a, "SKDATA01", 8);
    put_be32(a + 8, 1);
    uint8_t *e = a + 12;
    e[0] = (uint8_t)disk; snprintf((char *)e + 1, 23, "%s", name);
    put_be32(e + 24, 12 + 32); put_be32(e + 28, (uint32_t)strlen(body));
    memcpy(a + 44, body, strlen(body));
    return 44 + strlen(body);
}

void test_disks_read_from_an_archive(void) {
    uint8_t a[256]; size_t n = make_archive(a, "file078", 1, "hello");
    sk_disks d; sk_err e = {0};
    CHECK(disks_open_archive(&d, a, n, &e));
    uint8_t *f = NULL; size_t fn = 0;
    CHECK(disks_read(&d, 1, "FILE078", &f, &fn, &e));            /* names as AmigaDOS: any case */
    CHECK_EQ(fn, 5); CHECK(f && memcmp(f, "hello", 5) == 0);
    free(f);
    CHECK(!disks_read(&d, 2, "file078", &f, &fn, &e));           /* another disk */
    CHECK_STR(e.msg, "file078");
    disks_close(&d);
    a[0] = 'X';
    CHECK(!disks_open_archive(&d, a, n, &e)); CHECK_STR(e.msg, "soccerkid.dat");
    a[0] = 'S';
    CHECK(!disks_open_archive(&d, a, 30, &e));                   /* the table runs past the end */
    put_be32(a + 12 + 28, 9999);
    CHECK(!disks_open_archive(&d, a, n, &e));                    /* an entry runs past the end */
}

void test_disks_extract_matches_the_disks(void) {                   /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}, ar = {0}; sk_err e = {0};
    CHECK(disks_open(&d, roms, &e));
    uint8_t *data = NULL; size_t n = 0;
    CHECK(disks_extract(&d, &data, &n, &e));
    CHECK(disks_open_archive(&ar, data, n, &e));
    static const struct { int disk; const char *name; } files[] = {
        { 0, "sk.pak" }, { 0, "anim" }, { 0, "animfil2.pak" }, { 0, "german.bin" }, { 1, "file078" }, { 3, "file250" }, { 4, "file141" } };
    for (size_t i = 0; i < sizeof files / sizeof *files; i++) {
        uint8_t *x = NULL, *y = NULL; size_t xn = 0, yn = 0;
        CHECK(disks_read(&d, files[i].disk, files[i].name, &x, &xn, &e));
        CHECK(disks_read(&ar, files[i].disk, files[i].name, &y, &yn, &e));
        CHECK(xn == yn && x && y && memcmp(x, y, xn) == 0);
        free(x); free(y);
    }
    disks_close(&ar); free(data); disks_close(&d);
}
