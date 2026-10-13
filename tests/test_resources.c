#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "core/tables.h"
#include "game/resources.h"
#include "gfx/display_model.h"
#include "gfx/sprites.h"

static bool open_all(sk_disks *d, sk_image *img, sk_tables *t) {
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) return false;
    sk_err e = {0}; uint8_t *f; size_t n;
    if (!disks_open(d, roms, &e) || !disks_read(d, 0, "sk.pak", &f, &n, &e)) return false;
    bool ok = hunk_load(f, n, img, &e); free(f);
    return ok && tables_load(t, img, &e);
}

void test_resources_real_home1(void) {                          /* ROM-backed */
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0};
    if (!open_all(&d, &img, &t)) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    static sk_resources r;
    sk_err e = {0};
    CHECK(resources_load(&r, &d, &t, 0, &e));
    if (!SK_OK(&e)) printf("resources_load: %s\n", e.msg);
    size_t n = 0;
    CHECK(resource_slot(&r, 4, &n) != NULL);                      /* the bird sheet: 32 x 30 frames, 5 planes */
    CHECK(n > 0 && n % (5 * 30 * 32 / 8) == 0);
    CHECK(resource_slot(&r, 24, &n) != NULL && n % (5 * 20 * 32 / 8) == 0);   /* the crate: 32 x 20 */
    CHECK(resource_slot(&r, 140, &n) == NULL);                    /* HOME1 does not load slot 140 */
    CHECK(resource_slot(&r, 999, &n) == NULL);
    resources_free(&r);
    tables_free(&t); image_free(&img); disks_close(&d);
}

void test_ball_frame_decodes(void) {                             /* ROM-backed */
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0};
    if (!open_all(&d, &img, &t)) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    uint32_t off = image_u32(&img, 0x2008CC);                    /* the roll table's first frame */
    static sk_sprite s;
    memset(&s, 0, sizeof s);
    CHECK(sprite_decode_pair(&img, SK_KID_FRAMES_ADDR + off, SK_BALL_SPR_H, &s));
    int first = -1, last = -1, left = 99, right = -1;
    for (int y = 0; y < SK_SPR_H; y++) for (int x = 0; x < SK_SPR_W; x++)
        if (s.px[y * SK_SPR_W + x]) { if (first < 0) first = y; last = y; if (x < left) left = x; if (x > right) right = x; }
    CHECK(first >= 0 && first <= 2); CHECK(last >= 13 && last <= 15);   /* a 16-line ball */
    CHECK(right - left >= 12 && right - left <= 16);
    tables_free(&t); image_free(&img); disks_close(&d);
}
