#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "core/bytekiller.h"
#include "core/disks.h"
#include "formats/anim.h"

static uint8_t planes[6][8000];

void test_anim_byterun1_body_fills_five_planes_row_by_row(void) {   /* decode_ilbm_body 0x100D52 */
    uint8_t body[64]; size_t n = 0;
    body[n++] = 1; body[n++] = 0xAA; body[n++] = 0xBB;           /* literal 2: plane 0 row 0 bytes 0, 1 */
    body[n++] = (uint8_t)-37; body[n++] = 0x11;                  /* repeat 38: bytes 2..39 */
    body[n++] = (uint8_t)-39; body[n++] = 0x22;                  /* 40 bytes of plane 1 */
    memset(planes, 0, sizeof planes);
    anim_decode_body(body, n, planes);
    CHECK_EQ(planes[0][0], 0xAA); CHECK_EQ(planes[0][1], 0xBB); CHECK_EQ(planes[0][39], 0x11);
    CHECK_EQ(planes[1][0], 0x22); CHECK_EQ(planes[1][39], 0x22); CHECK_EQ(planes[2][0], 0);
}

void test_anim_op5_delta_writes_down_columns(void) {           /* apply_delta_frame 0x100E4E, decode_op5_plane 0x100E7C */
    uint8_t d[64 + 64];
    memset(d, 0, sizeof d);
    d[3] = 64;                                                   /* plane 0's column data at +64 */
    uint8_t *c = d + 64; size_t k = 0;
    c[k++] = 3;                                                  /* column 0: three ops */
    c[k++] = 2;                                                  /* skip two rows */
    c[k++] = 0x82; c[k++] = 0x5A; c[k++] = 0x5B;                 /* two literal bytes: rows 2, 3 */
    c[k++] = 0; c[k++] = 2; c[k++] = 0x77;                       /* run of 2: rows 4, 5 */
    for (int col = 1; col < 40; col++) c[k++] = 0;               /* the other columns unchanged */
    memset(planes, 0, sizeof planes);
    anim_apply_delta(d, 64 + k, planes);
    CHECK_EQ(planes[0][0], 0); CHECK_EQ(planes[0][2 * 40], 0x5A); CHECK_EQ(planes[0][3 * 40], 0x5B);
    CHECK_EQ(planes[0][4 * 40], 0x77); CHECK_EQ(planes[0][5 * 40], 0x77); CHECK_EQ(planes[0][6 * 40], 0);
    anim_apply_delta(d, 70, planes);                             /* a short chunk: no read past it */
}

void test_anim_real_files_hold_eight_animations(void) {         /* ROM-backed: 35 32 36 / 32 69 32 32 32 frames */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks dk = {0}; sk_err e = {0};
    CHECK(disks_open(&dk, roms, &e));
    static const int want[2][5] = { { 35, 32, 36, 0, 0 }, { 32, 69, 32, 32, 32 } };
    const char *names[2] = { "animfil1.pak", "animfil2.pak" };
    for (int f = 0; f < 2; f++) {
        uint8_t *raw = NULL, *data = NULL; size_t rn = 0, n = 0;
        CHECK(disks_read(&dk, 0, names[f], &raw, &rn, &e) && bk_unpack(raw, rn, &data, &n, &e));
        for (int k = 0; k < (f ? 5 : 3); k++) {
            sk_anim a;
            CHECK(anim_open(data, n, k, &a, &e));
            CHECK_EQ(a.count - 2, want[f][k]);                   /* the key frame, the deltas, two more that close the loop */
            CHECK(a.frame[0].key && a.frame[0].cmap != NULL);
        }
        free(raw); free(data);
    }
    disks_close(&dk);
}

void test_anim_open_bounds_the_offset_table(void) {
    uint8_t f[8] = { 0x7F, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0 };        /* a huge first offset, 8 bytes in all */
    sk_anim a; sk_err e = {0};
    CHECK(!anim_open(f, sizeof f, 4, &a, &e));
}
