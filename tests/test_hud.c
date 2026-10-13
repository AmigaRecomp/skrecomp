#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "game/attr_england.h"
#include "game/game.h"
#include "game/hud.h"
#include "game/level_flow.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static uint8_t font8[16 * 16], tfont[22 * 10], icons[128 * 16], health[80 * 10];
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    if (!game_setup_level(&g_g, &g_lv, 0)) return false;
    for (int i = 0; i < 16; i++) for (int r = 0; r < 8; r++) font8[i * 16 + r * 2] = (uint8_t)(0x80 >> (i & 7) | r);   /* glyph i, row r */
    for (int i = 0; i < 10; i++) for (int r = 0; r < 11; r++) tfont[i * 22 + r * 2] = (uint8_t)(i * 16 + r);
    for (int i = 0; i < 16; i++) for (int k = 0; k < 128; k++) icons[i * 128 + k] = (uint8_t)(i + 1);
    for (int i = 0; i < 10; i++) for (int k = 0; k < 80; k++) health[i * 80 + k] = (uint8_t)(0x10 + i);
    g_g.hud.digits = font8; g_g.hud.timer_font = tfont; g_g.hud.icons = icons; g_g.hud.health = health;
    hud_init(&g_g);
    return true;
}
static void drop_game(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }
static uint8_t plane_byte(int line, int plane, int byte) { return g_g.hud.bits[line * 160 + plane * 40 + byte]; }

void test_hud_lives_draw_without_a_leading_zero(void) {
    CHECK(make_game());
    g_g.player.lives = 3; g_g.hud_redraw |= 2;
    hud_update(&g_g);
    for (int r = 0; r < 8; r++) CHECK_EQ(plane_byte(13 + r, 0, 38), font8[3 * 16 + r * 2]);   /* byte 0x846: x = 304 */
    CHECK_EQ(plane_byte(13, 0, 39), 0);
    g_g.player.lives = 12; g_g.hud_redraw |= 2;
    hud_update(&g_g);
    CHECK_EQ(plane_byte(14, 0, 38), font8[1 * 16 + 2]); CHECK_EQ(plane_byte(14, 0, 39), font8[2 * 16 + 2]);
    drop_game();
}

void test_hud_score_draws_seven_digits(void) {
    CHECK(make_game());
    g_g.player.score = 0x1230; g_g.hud_redraw |= 4;
    hud_update(&g_g);
    static const int digits[7] = { 0, 0, 0, 1, 2, 3, 0 };
    for (int i = 0; i < 7; i++) CHECK_EQ(plane_byte(16 + 5, 0, 5 + i), font8[digits[i] * 16 + 10]);   /* byte 0xA05: line 16, x 40 */
    drop_game();
}

void test_hud_digits_wrap(void) {                               /* Review Focus 5 */
    CHECK(make_game());
    g_g.player.score = 0x999990;
    add_score(&g_g, 0x20);
    CHECK_EQ(g_g.player.score, 0x1000010);
    hud_update(&g_g);
    static const int digits[7] = { 1, 0, 0, 0, 0, 1, 0 };
    for (int i = 0; i < 7; i++) CHECK_EQ(plane_byte(16, 0, 5 + i), font8[digits[i] * 16]);
    g_g.player.lives = 99; g_g.player.lives++; g_g.hud_redraw |= 2;   /* 100: the tens glyph is 10 */
    hud_update(&g_g);
    CHECK_EQ(plane_byte(13, 0, 38), font8[10 * 16]); CHECK_EQ(plane_byte(13, 0, 39), font8[0]);
    drop_game();
}

void test_hud_timer_draws_in_three_planes(void) {
    CHECK(make_game());
    g_g.level_timer = 0x45; g_g.timer_flags |= 1;
    hud_update(&g_g);
    for (int r = 0; r < 11; r++) {
        CHECK_EQ(plane_byte(3 + r, 0, 8), tfont[4 * 22 + r * 2]); CHECK_EQ(plane_byte(3 + r, 1, 8), tfont[4 * 22 + r * 2]);
        CHECK_EQ(plane_byte(3 + r, 3, 8), tfont[4 * 22 + r * 2]); CHECK_EQ(plane_byte(3 + r, 2, 8), 0);
        CHECK_EQ(plane_byte(3 + r, 0, 9), tfont[5 * 22 + r * 2]);
    }
    CHECK_EQ(g_g.timer_flags & 1, 0);
    drop_game();
}

void test_hud_health_icons(void) {
    CHECK(make_game());
    g_g.player.health = 2; g_g.hud_redraw |= 1;
    hud_update(&g_g);
    CHECK_EQ(plane_byte(9, 0, 1), health[0 * 80]);               /* pair (0, 6) at x 8 and 24, line 8 + 1 */
    CHECK_EQ(plane_byte(9, 0, 3), health[6 * 80]);
    drop_game();
}

void test_hud_card_slots_and_collect(void) {
    CHECK(make_game());
    for (int n = 0; n < 11; n++) CHECK_EQ(plane_byte(5, 0, (0x68 + 16 * n) / 8), icons[1 * 128]);   /* empty slots: glyph 1 */
    CHECK_EQ(plane_byte(5, 0, 0x120 / 8), icons[0]);
    g_g.hud_card_flag = 0xFF;
    for (int i = 0; i < 20 && g_g.hud_card_flag; i++) { g_g.frame_ticks++; hud_update(&g_g); }
    CHECK_EQ(g_g.cards, 1); CHECK_EQ(plane_byte(5, 0, 0x68 / 8), icons[5 * 128]);
    drop_game();
}

void test_message_box_draws_the_text(void) {
    CHECK(make_game());
    g_g.message_box_request = 1; g_g.message_box_kind = 0;
    message_box_tick(&g_g);
    CHECK_EQ(g_g.message_wait, 1);
    CHECK_EQ(g_g.msg_visible, 1); CHECK_EQ(g_g.message_shown[0], 1);
    g_g.input_state = 0x10;
    static sk_sprite kid;
    game_frame_step(&g_g, 0x10, &kid);
    CHECK_EQ(g_g.message_wait, 0); CHECK_EQ(g_g.msg_visible, 0); CHECK_EQ(g_g.message_shown[0], 0);   /* fire dismisses it */
    drop_game();
}

void test_hud_palette_is_the_game_palette(void) {               /* ROM-backed */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_image img = {0}; sk_err e = {0};
    uint8_t *f = NULL; size_t n = 0;
    CHECK(disks_open(&d, roms, &e) && disks_read(&d, 0, "sk.pak", &f, &n, &e));
    CHECK(hunk_load(f, n, &img, &e)); free(f);
    static sk_game g;
    memset(&g, 0, sizeof g);
    g.sk = &img;
    hud_init(&g);
    for (int i = 0; i < 15; i++) CHECK_EQ(g.hud.palette12[1 + i], image_u16(&img, 0x200E7C + 2u * (uint32_t)i));
    CHECK(g.hud.digits != NULL && g.hud.icons != NULL);
    image_free(&img); disks_close(&d);
}

void test_message_box_rejects_a_tiny_box(void) {                /* a box under two character rows is refused */
    CHECK(make_game());
    static sk_image img;
    static uint8_t mem[0x3000];
    memset(&img, 0, sizeof img);
    img.seg[0].base = 0x202000; img.seg[0].size = sizeof mem; img.seg[0].mem = mem; img.count = 1;
    memset(mem, 0, sizeof mem);
    mem[0x484 + 1] = 0x10;                                          /* level 0, message 0 at 0x202494 */
    mem[0x494 + 1] = 32; mem[0x494 + 3] = 4;                        /* 32 x 4 */
    mem[0x494 + 4] = 'A';
    for (int i = 0x2148; i < 0x2148 + 0x600; i++) mem[i] = 0xFF;      /* the font */
    g_g.sk = &img; g_g.level_data_index = 0;
    message_box_build(&g_g, 0);
    CHECK_EQ(g_g.msg_w, 0);
    g_g.sk = NULL;
    drop_game();
}
