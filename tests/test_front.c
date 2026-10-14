#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "core/bytes.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "game/front.h"

static sk_image img;
static void make_image(void) {                                   /* code hunk 0 and data hunk 1, zeroed */
    image_free(&img); memset(&img, 0, sizeof img);
    img.count = 2;
    img.seg[0] = (sk_segment){ SK_HUNK_BASE(0), 0x20000, calloc(1, 0x20000), false };
    img.seg[1] = (sk_segment){ SK_HUNK_BASE(1), 0x10000, calloc(1, 0x10000), false };
}

void test_image_write_stays_inside_a_segment(void) {
    make_image();
    uint8_t b[2] = { 0x12, 0x34 };
    CHECK(image_write(&img, 0x200010, b, 2));
    CHECK_EQ(image_u16(&img, 0x200010), 0x1234);
    CHECK(!image_write(&img, 0x20FFFF, b, 2));                   /* runs past hunk 1 */
    CHECK(!image_write(&img, 0x300000, b, 1));
    image_free(&img); memset(&img, 0, sizeof img);
}

void test_kit_apply_writes_the_rows_into_the_kid_palette(void) {   /* 0x11CCFE */
    make_image();
    uint8_t *tab = img.seg[0].mem + (0x11D2B2 - 0x100000);
    put_be16(tab + 3 * 8, 0x0838); put_be16(tab + 3 * 8 + 2, 0x0515); put_be16(tab + 3 * 8 + 4, 0x0B6B);
    put_be16(tab + 9 * 8, 0x0334); put_be16(tab + 9 * 8 + 2, 0x0112); put_be16(tab + 9 * 8 + 4, 0x0556);
    kit_apply(&img, 3, 9);
    CHECK_EQ(image_u16(&img, 0x200EB8 + 0x10), 0x0838); CHECK_EQ(image_u16(&img, 0x200EB8 + 0x12), 0x0515);
    CHECK_EQ(image_u16(&img, 0x200EB8 + 0x02), 0x0B6B);
    CHECK_EQ(image_u16(&img, 0x200EB8 + 0x16), 0x0334); CHECK_EQ(image_u16(&img, 0x200EB8 + 0x18), 0x0112);
    CHECK_EQ(image_u16(&img, 0x200EB8 + 0x14), 0x0556);
    kit_apply(&img, 14, -1);                                      /* out of range: nothing */
    CHECK_EQ(image_u16(&img, 0x200EB8 + 0x10), 0x0838);
    image_free(&img); memset(&img, 0, sizeof img);
}

void test_language_texts_load_into_the_message_tables(void) {    /* language_select: <language>.bin -> 0x202484 */
    make_image();
    uint8_t bin[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    CHECK(language_apply(&img, bin, sizeof bin));
    CHECK_EQ(image_u8(&img, 0x202484), 1); CHECK_EQ(image_u8(&img, 0x20248B), 8);
    static uint8_t big[0x10000];
    CHECK(!language_apply(&img, big, sizeof big));               /* would run past the hunk */
    CHECK_STR(language_file(2), "german.bin"); CHECK(language_file(5) == NULL);
    image_free(&img); memset(&img, 0, sizeof img);
}
