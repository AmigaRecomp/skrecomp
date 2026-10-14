#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/scoreboard.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_board(void) {                                   /* a game set up as data index 12 */
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e)) return false;
    game_new_game(&g_g);
    return game_setup_level(&g_g, &g_lv, 12);
}
static void drop(void) { game_close(&g_g); level_free(&g_lv); free(g_d); }

void test_scoreboard_setup_turns_the_counters_to_bcd(void) {
    CHECK(make_board());
    g_g.stat_balls = 12; g_g.cards_taken = 11; g_g.kills[1] = 25; g_g.kills[3] = 3; g_g.kills[2] = 0; g_g.stat_time = 0x47;
    g_g.level_number = 3;
    scoreboard_setup(&g_g);
    CHECK(g_g.board.active);
    CHECK_EQ(g_g.stat_balls, 0x12); CHECK_EQ(g_g.cards_taken, 0x11); CHECK_EQ(g_g.kills[1], 0x25); CHECK_EQ(g_g.kills[3], 3);
    CHECK_EQ(g_g.stat_time, 0x47);                               /* already BCD */
    CHECK_EQ(g_g.bonus_pending, 0xFF);                           /* 11 cards, no piece since the map */
    CHECK_EQ(g_g.board.phase, 0); CHECK_EQ(g_g.board.draw, 2); CHECK_EQ(g_g.board.bonus, 0);
    CHECK_EQ(g_g.board.ticker_len, (int)(strlen(g_g.board.ticker) + 1) * 16);
    drop();
}

void test_scoreboard_setup_needs_all_cards_and_no_piece(void) {
    CHECK(make_board());
    g_g.cards_taken = 11; g_g.piece_collected = 0xFF;
    scoreboard_setup(&g_g);
    CHECK_EQ(g_g.bonus_pending, 0);
    g_g.piece_collected = 0; g_g.cards_taken = 10;
    scoreboard_setup(&g_g);
    CHECK_EQ(g_g.bonus_pending, 0);
    drop();
}

void test_scoreboard_kid_walks_in_then_fetches_the_ball(void) {
    CHECK(make_board());
    g_g.player.x = 100; g_g.camera_x = 0;
    CHECK_EQ(scoreboard_player_input(&g_g, SK_IN_FIRE), SK_IN_RIGHT);   /* the keyboard is ignored */
    CHECK_EQ(g_g.board.state, 0);
    g_g.player.x = 0xE7;
    CHECK_EQ(scoreboard_player_input(&g_g, 0), SK_IN_RIGHT);
    CHECK_EQ(g_g.board.state, 0xFE);
    g_g.player.facing = 1;
    CHECK_EQ(scoreboard_player_input(&g_g, 0), SK_IN_LEFT);      /* turn round for the ball */
    g_g.player.facing = -1;
    g_g.ball.x = (int32_t)((uint32_t)(g_g.player.x - 23) << 16); g_g.ball.y = (int32_t)((uint32_t)(g_g.player.y + 11) << 16);
    CHECK_EQ(scoreboard_player_input(&g_g, 0), SK_IN_FIRE);      /* the ball at the hold offset: juggle */
    CHECK_EQ(g_g.board.state, 0xFF);
    int in = 0;
    for (int i = 0; i < 0x4F; i++) in = scoreboard_player_input(&g_g, 0);
    CHECK_EQ(in, SK_IN_FIRE | SK_IN_UP);                         /* every 0x50 frames a direction from 01 04 08 01 08 04 */
    drop();
}

static void run_board(int frames) { for (int i = 0; i < frames; i++) scoreboard_update(&g_g); }

void test_scoreboard_counts_the_cards_into_the_bonus(void) {
    CHECK(make_board());
    g_g.cards_taken = 2;
    scoreboard_setup(&g_g);
    g_g.board.state = 0xFF;
    run_board(51);                                               /* the label, then 50 frames' wait */
    CHECK_EQ(g_g.cards_taken, 2); CHECK_EQ(g_g.board.bonus, 0);
    int n = 0;
    while (g_g.cards_taken && n < 100) {                         /* every fourth frame, once the HUD card has settled */
        scoreboard_update(&g_g); n++;
        if (g_g.hud_card_flag) { g_g.hud_card_flag = 0; }
    }
    CHECK_EQ(g_g.cards_taken, 0); CHECK_EQ(g_g.board.bonus, 0x600);   /* 300 a card, BCD */
    run_board(2);
    CHECK_EQ(g_g.board.phase, 1);
    drop();
}

void test_scoreboard_pays_the_bonus_into_the_score(void) {
    CHECK(make_board());
    scoreboard_setup(&g_g);
    g_g.board.state = 0xFF; g_g.board.phase = -1; g_g.board.bonus = 0x12300;   /* past 10000: the ruling pays it all */
    uint32_t before = g_g.player.score;
    run_board(4 * 200);
    CHECK_EQ(g_g.board.phase, 0x7F);
    CHECK_EQ(g_g.board.bonus, 0);
    CHECK_EQ(g_g.player.score - before, 0x12300u);               /* add_score in BCD */
    drop();
}

void test_scoreboard_ends_with_the_exit_walk(void) {
    CHECK(make_board());
    scoreboard_setup(&g_g);
    g_g.board.state = 0xFF; g_g.board.phase = 0x7F; g_g.board.counter = 0;
    run_board(0xC7);
    CHECK(!(g_g.level_outcome & 2));
    run_board(1);
    CHECK(g_g.level_outcome & 2);                                /* 200 frames after the pay-out */
    for (int i = 0; i < 2000 && g_g.player.state != 60; i++) scoreboard_update(&g_g);
    CHECK_EQ(g_g.player.state, 60);                              /* when the ticker has run out */
    drop();
}

void test_level_end_stats_add_time_and_balls(void) {
    CHECK(make_board());
    g_g.stat_time = 0x38; g_g.level_timer = 0x25; g_g.stat_balls = 3; g_g.balls_allotted = 10; g_g.recalls = 4;
    level_end_stats(&g_g);
    CHECK_EQ(g_g.stat_time, 0x63); CHECK_EQ(g_g.stat_balls, 9); CHECK_EQ(g_g.recalls, 0);
    g_g.balls_allotted = 2; g_g.recalls = 5;
    level_end_stats(&g_g);
    CHECK_EQ(g_g.stat_balls, 9);                                 /* more balls lost than allotted: nothing */
    drop();
}

void test_scoreboard_free_clears_the_cards(void) {
    CHECK(make_board());
    scoreboard_setup(&g_g);
    g_g.cards = 11; g_g.stat_balls = 5; g_g.stat_time = 7;
    scoreboard_free(&g_g);
    CHECK(!g_g.board.active); CHECK_EQ(g_g.cards, 0); CHECK_EQ(g_g.stat_balls, 0); CHECK_EQ(g_g.stat_time, 0);
    drop();
}
