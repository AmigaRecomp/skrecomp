#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "fixture.h"
#include "game/game.h"
#include "game/player.h"

static uint8_t *g_d; static sk_level g_lv; static sk_game g_g;
static bool make_game(void) {
    size_t n = 0; sk_err e = {0};
    memset(&g_g, 0, sizeof g_g); memset(&g_lv, 0, sizeof g_lv);
    if (!load_fixture("level_play.bin", &g_d, &n) || !level_parse(g_d, n, &g_lv, &e) || !world_build(&g_g.world, &g_lv)) return false;
    g_g.level_bottom_y = g_g.world.fg_max_y + 200;
    init_player(&g_g);
    return true;
}
static void drop_game(void) { world_free(&g_g.world); level_free(&g_lv); free(g_d); }
static void frames(int n, uint8_t input) {
    for (int i = 0; i < n; i++) { g_g.frame_ticks++; g_g.input_state = input; g_g.mask.built = false; player_input_state(&g_g); }
}

void test_init_player_values(void) {
    CHECK(make_game());
    sk_player *p = &g_g.player;
    CHECK_EQ(p->x, 3 * 16); CHECK_EQ(p->y, 10 * 16 - 24); CHECK_EQ(p->jump_y, p->y);
    CHECK_EQ(p->top_speed, 14); CHECK_EQ(p->facing, 1); CHECK_EQ(p->falling, 1); CHECK_EQ(p->health, 2);
    CHECK_EQ(p->state, 0); CHECK_EQ(p->dying, 0); CHECK_EQ(g_g.ball.inactive, 0); CHECK_EQ(g_g.look_x, 0x40);
    CHECK_EQ(g_g.timer_flags, 1); CHECK_EQ(g_g.bplcon2_value, 0x20);
    CHECK(g_g.jingle_requested); CHECK_EQ(g_g.jingle, 0);
    drop_game();
}

void test_player_lands_on_the_floor_from_the_start(void) {
    CHECK(make_game());
    frames(20, 0);
    CHECK_EQ(g_g.player.y, 12 * 16 - 41);                      /* row y + 41 is the floor's top row */
    CHECK_EQ(g_g.player.falling, 0); CHECK_EQ(g_g.player.jumping, 0); CHECK_EQ(g_g.player.vy, 0);
    CHECK_EQ(g_g.player.top_speed, 16);                         /* slope_probe on flat ground */
    drop_game();
}

void test_player_runs_and_skids(void) {
    CHECK(make_game());
    frames(20, 0);
    int x0 = g_g.player.x;
    frames(1, SK_IN_RIGHT);
    CHECK_EQ(g_g.player.speed, 1);                              /* +1 per frame below 14 */
    frames(13, SK_IN_RIGHT);
    CHECK_EQ(g_g.player.speed, 14);
    frames(16, SK_IN_RIGHT);
    CHECK_EQ(g_g.player.speed, 16);                             /* +1 every 8 frames up to top_speed 16 */
    CHECK(g_g.player.x > x0);
    frames(1, 0);
    CHECK_EQ(g_g.player.speed, 15);                             /* skid: -1 per frame */
    frames(30, 0);
    CHECK_EQ(g_g.player.speed, 0);
    frames(1, SK_IN_LEFT);
    CHECK_EQ(g_g.player.facing, -1);                            /* turning from rest is immediate */
    drop_game();
}

void test_player_jump_integrator(void) {
    CHECK(make_game());
    frames(20, 0);
    int ground = g_g.player.y;
    frames(1, SK_IN_UP);
    CHECK_EQ(g_g.player.jumping, 1); CHECK_EQ(g_g.player.jump_hold, 1); CHECK_EQ(g_g.player.vy, 9);
    CHECK_EQ(g_g.player.y, ground - 4);                         /* vy 10 -> 9 after gravity, y -= 9 >> 1 */
    CHECK_EQ(g_g.sfx_last, 4);
    frames(15, SK_IN_UP);                                       /* 16 boost frames in all */
    CHECK_EQ(g_g.player.jump_hold, 16); CHECK_EQ(g_g.player.y, ground - 16 * 4);
    int top = g_g.player.y;
    for (int i = 0; i < 60 && g_g.player.jumping; i++) { frames(1, SK_IN_UP); if (g_g.player.y < top) top = g_g.player.y; }
    CHECK_EQ(ground - top, 16 * 4 + 4 + 3 + 3 + 2 + 2 + 1 + 1);   /* vy 8..1 after the boost, halved */
    CHECK_EQ(g_g.player.jumping, 0); CHECK_EQ(g_g.player.y, ground);  /* landed exactly */
    frames(1, SK_IN_UP); frames(1, 0);                          /* a short press: release ends the boost */
    CHECK_EQ(g_g.player.jump_hold, 16);
    drop_game();
}

void test_player_falls_off_a_ledge_and_may_late_jump(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.x = 26 * 16 + 2;                                 /* feet centre over the pit */
    frames(1, 0);
    CHECK_EQ(g_g.player.falling, 1); CHECK_EQ(g_g.player.vy, -1);
    frames(1, SK_IN_UP);                                        /* within the first three frames: a new jump */
    CHECK_EQ(g_g.player.jumping, 1);
    drop_game();
}

void test_player_is_stopped_by_a_wall(void) {
    CHECK(make_game());
    frames(20, 0);
    g_g.player.x = 20 * 16 - 20;
    int max_x = 0;
    for (int i = 0; i < 40; i++) { frames(1, SK_IN_RIGHT); if (g_g.player.x > max_x) max_x = g_g.player.x; }
    CHECK(max_x <= 20 * 16 - 8);                                /* the probe column x + 8 never enters the wall */
    CHECK(g_g.player.speed <= 4);                               /* cleared on every push */
    drop_game();
}

void test_hurt_player_knockback_and_death(void) {
    CHECK(make_game());
    frames(20, 0);
    CHECK_EQ(hurt_player(&g_g), -1);
    CHECK_EQ(g_g.player.health, 1); CHECK_EQ(g_g.player.state, 52); CHECK_EQ(g_g.player.vy, 14); CHECK_EQ(g_g.sfx_last, 7);
    CHECK_EQ(g_g.player.invulnerable, -1); CHECK_EQ(g_g.player.speed, 2); CHECK_EQ(g_g.player.knock_dir, 1);
    CHECK_EQ(hurt_player(&g_g), -3);                            /* invulnerable (-1) */
    g_g.player.invulnerable = 0; g_g.player.state = 0;
    CHECK_EQ(hurt_player(&g_g), -1); CHECK_EQ(g_g.player.health, 0);
    g_g.player.invulnerable = 0; g_g.player.state = 0;
    CHECK_EQ(hurt_player(&g_g), -2);
    CHECK_EQ(g_g.player.state, 20); CHECK_EQ(g_g.player.dying, 1); CHECK_EQ(g_g.ball.inactive, 1); CHECK_EQ(g_g.player.timer, 0x600);
    CHECK(g_g.music_stopped);
    CHECK_EQ(hurt_player(&g_g), -3);                            /* dying: ignored */
    drop_game();
}

void test_player_logic_picks_walk_and_jump_frames(void) {
    CHECK(make_game());
    frames(20, 0);
    player_logic(&g_g);
    CHECK_EQ(g_g.player.anim, 0x11);                             /* standing, facing right */
    frames(10, SK_IN_RIGHT); player_logic(&g_g);
    CHECK(g_g.player.anim <= 5);                                 /* the walk cycle 0..5 (6..11 facing left) */
    frames(1, SK_IN_UP); player_logic(&g_g);
    CHECK(g_g.player.anim >= 0x1C && g_g.player.anim <= 0x1E);  /* jumping frames by vy, facing right */
    frames(30, 0); frames(1, SK_IN_DOWN); player_logic(&g_g);
    CHECK_EQ(g_g.player.anim, 0x0F);                             /* crouch, facing right */
    static sk_sprite kid;
    g_g.player.x = 100; g_g.player.y = 120; g_g.view.cam_x = 40; g_g.view.cam_y = 10;
    update_player(&g_g, &kid);                                   /* no sk image: the frame table reads as zeros */
    CHECK_EQ(kid.x, 100 - 40 - 1); CHECK_EQ(kid.y, 120 - 6 - 10); CHECK(kid.visible);
    g_g.player.invulnerable = 101; update_player(&g_g, &kid); CHECK(!kid.visible);   /* odd: flicker frame */
    g_g.player.invulnerable = 100; update_player(&g_g, &kid); CHECK(kid.visible);
    g_g.player.exit_walk = 1; update_player(&g_g, &kid); CHECK(!kid.visible);
    drop_game();
}
