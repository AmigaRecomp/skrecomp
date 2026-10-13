#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "audio/sound.h"
#include "fixture.h"
#include "game/game.h"
#include "game/player.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e) || !world_build(&g_g.world, &g_lv)) return false;
    init_player(&g_g);
    return true;
}
static void drop_game(void) { world_free(&g_g.world); level_free(&g_lv); free(g_d); }
static void frames(int n, uint8_t input) {
    for (int i = 0; i < n; i++) { g_g.frame_ticks++; g_g.input_state = input; g_g.mask.built = false; player_input_state(&g_g); }
}

void test_knockback_lands_and_grants_invulnerability(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.x = 200;                                          /* away from the map's left edge */
    int x0 = g_g.player.x;
    hurt_player(&g_g);                                           /* state 52, vy 14, facing right -> moves left */
    frames(1, 0);
    CHECK(g_g.player.x < x0); CHECK_EQ(g_g.player.state, 52); CHECK_EQ(g_g.player.anim, 0x13);
    for (int i = 0; i < 80 && g_g.player.state == 52; i++) frames(1, 0);
    CHECK_EQ(g_g.player.state, 0); CHECK_EQ(g_g.player.invulnerable, 99); CHECK_EQ(g_g.player.anim, 0x11);   /* 100, less this frame's tick */
    CHECK_EQ(g_g.player.y, 12 * 16 - 41);                       /* back on the floor */
    drop_game();
}

void test_death_counts_a_life_and_respawns(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.lives = 3; g_g.player.health = 0; g_g.player.x = 200;
    CHECK_EQ(hurt_player(&g_g), -2);
    int rise = 0;
    for (int i = 0; i < 40 && g_g.player.lives == 3; i++) { frames(1, 0); rise++; }
    CHECK_EQ(g_g.player.lives, 2); CHECK_EQ(g_g.player.state, 20); CHECK_EQ(g_g.player.anim, 0x1B);
    CHECK(rise > 10 && rise < 40);                               /* the bounce rises before the life is taken */
    CHECK(g_g.player.y < 12 * 16 - 41);
    CHECK(g_g.music_requested); CHECK_EQ(g_g.music_module, SK_MUSIC_JINGLE); CHECK_EQ(g_g.music_request, 0);   /* the life-lost jingle */
    CHECK_EQ(g_g.jingle_loads, 1);                                  /* sub_1037C4 copied the jingle module out */
    for (int i = 0; i < 130 && g_g.player.state == 20; i++) frames(1, 0);
    CHECK_EQ(g_g.player.state, 0); CHECK_EQ(g_g.player.x, 3 * 16); CHECK_EQ(g_g.player.health, 2);   /* init_player ran */
    CHECK_EQ(g_g.timer_flags & 0x80, 0);
    drop_game();
}

void test_death_with_no_lives_left_is_game_over(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.lives = 0; g_g.player.health = 0;
    hurt_player(&g_g);
    for (int i = 0; i < 130 && g_g.player.state == 20; i++) frames(1, 0);
    CHECK_EQ(g_g.player.lives, -1); CHECK_EQ(g_g.player.state, 64); CHECK_EQ(g_g.player.controllable, 0);
    CHECK(g_g.jingle_requested); CHECK_EQ(g_g.jingle, 1); CHECK_EQ(g_g.music_module, SK_MUSIC_JINGLE); CHECK_EQ(g_g.music_request, 3);
    frames(1, 0); CHECK_EQ(g_g.player.state, 64); CHECK_EQ(g_g.player.controllable, 0);
    drop_game();
}

void test_player_falls_out_of_an_empty_level_and_respawns(void) {   /* Review Focus 1 */
    CHECK(make_game());
    g_g.player.lives = 3;
    g_g.player.x = 27 * 16 + 8;                                  /* over the pit: nothing below down to the map's end */
    int bottom = g_g.world.fg_max_y + 160;
    for (int i = 0; i < 400 && (g_g.player.falling || g_g.player.y < bottom); i++) frames(1, 0);
    CHECK(g_g.player.y >= bottom && g_g.player.y < bottom + 8);  /* floor_probe reports no gap there: he "lands" */
    CHECK_EQ(g_g.player.falling, 0); CHECK_EQ(g_g.player.state, 0);
    kill_player(&g_g);                                           /* a deadly tile would do this (sub_10A780) */
    CHECK_EQ(g_g.player.state, 20); CHECK_EQ(g_g.player.dying, 1); CHECK_EQ(g_g.player.health, 0);
    for (int i = 0; i < 160 && g_g.player.state == 20; i++) frames(1, 0);
    CHECK_EQ(g_g.player.state, 0); CHECK_EQ(g_g.player.x, 3 * 16); CHECK_EQ(g_g.player.y, 10 * 16 - 24);
    drop_game();
}

void test_exit_walk_sets_the_outcome(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.x = 36 * 16; g_g.camera_x = g_g.world.fg_max_x; g_g.level_number = 1;
    g_g.player.state = 60; g_g.player.timer = 0;
    for (int i = 0; i < 200 && g_g.level_outcome < 0x80; i++) frames(1, 0);
    CHECK_EQ(g_g.level_outcome, 0x81); CHECK_EQ(g_g.level_number, 2); CHECK(g_g.curtain_flags & 0x10);
    CHECK_EQ(g_g.player.exit_walk, 1); CHECK(g_g.player.x > 36 * 16 + 32);
    drop_game();
}

void test_ball_states_are_stubs_that_return_to_normal(void) {
    CHECK(make_game());
    frames(20, 0);
    int stubs[] = { 12 };                                        /* the rope: 5c */
    for (size_t i = 0; i < sizeof stubs / sizeof *stubs; i++) { g_g.player.state = (int16_t)stubs[i]; frames(1, 0); CHECK_EQ(g_g.player.state, 0); }
    g_g.player.state = 56; g_g.player.carry_dx = 3; int x0 = g_g.player.x; frames(1, 0);
    CHECK_EQ(g_g.player.state, 56); CHECK_EQ(g_g.player.x, x0 + 3);         /* carried: x += carry_dx only */
    g_g.player.state = 68; frames(1, SK_IN_RIGHT); CHECK_EQ(g_g.player.input, 0);   /* time out: input cleared */
    drop_game();
}
