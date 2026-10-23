#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "check.h"
#include "formats/custom.h"
#include "game/world.h"

static sk_custom sample(void) {
    sk_custom c = {0}; sk_err e = {0};
    custom_alloc(&c, 4, 22, 13, &e);
    for (int i = 0; i < 22 * 13; i++) { c.cells[i] = (uint16_t)(i * 7); c.states[i] = (uint8_t)(i & 3); }
    return c;
}

void test_custom_round_trip(void) {
    sk_custom c = sample(), r = {0};
    uint8_t *d = NULL; size_t n = 0; sk_err e = {0};
    CHECK(custom_write(&c, &d, &n));
    CHECK_EQ(n, 8 + 8 + 22 * 13 * 3);
    CHECK(memcmp(d, "SKLEVEL1", 8) == 0);
    CHECK(custom_parse(d, n, &r, &e));
    CHECK_EQ(r.setting, 4); CHECK_EQ(r.w, 22); CHECK_EQ(r.h, 13);
    CHECK(memcmp(r.cells, c.cells, 22 * 13 * 2) == 0);
    CHECK(memcmp(r.states, c.states, 22 * 13) == 0);
    free(d); custom_free(&c); custom_free(&r);
}

void test_custom_rejects_bad_files(void) {
    sk_custom c = sample(), r = {0};
    uint8_t *d = NULL; size_t n = 0; sk_err e = {0};
    custom_write(&c, &d, &n);
    CHECK(!custom_parse(d, 7, &r, &e)); CHECK_STR(e.msg, "too short");
    d[0] = 'X'; CHECK(!custom_parse(d, n, &r, &e)); CHECK_STR(e.msg, "not a Soccer Kid level"); d[0] = 'S';
    d[9] = 31; CHECK(!custom_parse(d, n, &r, &e)); CHECK_STR(e.msg, "unknown setting 31");
    d[9] = 5; CHECK(!custom_parse(d, n, &r, &e)); CHECK_STR(e.msg, "setting 5"); d[9] = 4;   /* LONDON2: a boss level */
    d[10] = 0x02; d[11] = 0x58; d[12] = 0; d[13] = 40;                                     /* 600 x 40 */
    CHECK(!custom_parse(d, n, &r, &e)); CHECK_STR(e.msg, "600x40 exceeds 16384 cells");
    d[10] = 0; d[11] = 21; d[13] = 13; CHECK(!custom_parse(d, n, &r, &e)); CHECK_STR(e.msg, "at least 22x13");
    d[11] = 22; d[15] = 1; CHECK(!custom_parse(d, n, &r, &e)); CHECK_STR(e.msg, "marker table"); d[15] = 0;
    CHECK(!custom_parse(d, n - 12, &r, &e)); CHECK_STR(e.msg, "12 bytes short");
    CHECK(!custom_parse(d, n + 1, &r, &e)); CHECK_STR(e.msg, "1 byte too long");
    CHECK(custom_parse(d, n, &r, &e));
    free(d); custom_free(&c); custom_free(&r);
}

void test_custom_settings_table(void) {
    CHECK_EQ(custom_setting_count(), 26);
    CHECK(custom_setting_by_name("London1") && custom_setting_by_name("London1")->index == 4);
    CHECK(custom_setting_by_name("london2") == NULL);
    CHECK(custom_setting_by_index(15) == NULL);              /* BATTLE SHIP */
    CHECK(custom_setting_by_index(40) && custom_setting_by_index(40)->index == 40);
    CHECK(strcmp(custom_setting_at(0)->name, "home1") == 0);
}

void test_custom_save_is_atomic(void) {
    sk_custom c = sample(), r = {0}; sk_err e = {0};
    char path[] = "/tmp/sk_custom_XXXXXX"; int fd = mkstemp(path); close(fd);
    CHECK(custom_save(path, &c, &e));
    CHECK(custom_load(path, &r, &e)); CHECK_EQ(r.w, 22); custom_free(&r);
    CHECK(!custom_save("/nonexistent-dir/x.skl", &c, &e)); CHECK_STR(e.msg, "/nonexistent-dir/x.skl");
    CHECK(custom_load(path, &r, &e));                         /* the earlier file is intact */
    remove(path); custom_free(&r); custom_free(&c);
}

void test_custom_clears_unknown_flags(void) {
    sk_level lv = {0}; uint8_t pals[200] = {0};
    uint16_t m = 40 << 9;                                     /* the setting's map uses flag 40 */
    lv.pals = pals; lv.pals_len = sizeof pals;
    lv.map.width = 1; lv.map.height = 1; lv.map.cells = &m;
    pals[SK_PALS_TYPE + 3] = 58;                               /* flag 3: a rat */
    sk_custom c = {0}; sk_err e = {0}; custom_alloc(&c, 4, 20, 13, &e);
    c.cells[0] = 3 << 9; c.cells[1] = 40 << 9; c.cells[2] = 4 << 9 | 5; c.cells[3] = 32 << 9; c.cells[4] = 41 << 9;
    CHECK_EQ(custom_clear_unknown_flags(&c, &lv), 3);
    CHECK_EQ(c.cells[0], 3 << 9); CHECK_EQ(c.cells[1], 40 << 9); CHECK_EQ(c.cells[2], 5); CHECK_EQ(c.cells[3], 0); CHECK_EQ(c.cells[4], 0);
    custom_free(&c);
}
