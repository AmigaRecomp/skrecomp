/* dispatch_player_state (0x10A672) and the 19 entries of player_state_table (0x10A676); player.md "States". */
#include "game/game.h"

static int left(const sk_player *p) { return p->facing < 0; }

static void player_state_none(sk_game *g) { (void)g; }
static void player_state_stub(sk_game *g) { g->player.state = 0; g->player.timer = 0; }   /* state 12, the rope: 5c */

static void select_death_layers(sk_game *g) { g->layer_flags = 3; select_layers(g); }

static void player_state_death(sk_game *g) {                     /* 0x10BB28 */
    sk_player *p = &g->player;
    select_death_layers(g);
    p->controllable = 0;
    p->dying = 1;
    p->timer = (int16_t)(p->timer - 0x40);
    p->x = (int16_t)(p->x + p->carry_dx);
    int d0 = (int8_t)((uint16_t)p->timer >> 8);                 /* the word's high byte: an 8.8 bounce speed */
    if (d0 < 0 && !(g->death_flags & 1)) {
        g->death_flags |= 1;
        if (!g->game_flags) play_sfx(g, -1);                    /* the death sample (sub_1037C4) */
        g->hud_redraw |= 2;
        p->lives--;
        g->music_requested = true;
        if (p->lives < 0) { g->music_request = 3; g->jingle_requested = true; g->jingle = 1; }
        else g->music_request = 0;
    }
    p->anim = 0x1B;
    p->y = (int16_t)(p->y - d0);
    if (++p->kick_phase < 125) return;
    g->timer_flags = 0;
    if (p->lives < 0) p->state = 64;
    else { init_player(g); g->death_flags &= (uint8_t)~1; }     /* the jingle object clears the latch in the original */
    p->fall_flags = 0;
    p->kick_phase = 0;
    p->timer = 0;
}

static void player_state_fell_out(sk_game *g) {                  /* 0x10BBFC: no code enters it (dead in the original) */
    sk_player *p = &g->player;
    p->anim = 0x1B;
    p->controllable = 0;
    p->dying = 1;
    p->y = (int16_t)(p->y + 2);
    if (g->death_target_y + 128 <= p->y) { init_player(g); p->timer = 0; }
}

static void ladder_turn_anim(sk_game *g) {                       /* 0x10BEE4 */
    sk_player *p = &g->player;
    int lo = p->timer & 0xFF;
    if (p->input & SK_IN_LEFT) { if (lo >= 10) p->facing = -1; }
    else if (p->input & SK_IN_RIGHT) { if (lo >= 10) p->facing = 1; }
    else return;
    p->anim = (int16_t)((left(p) ? 6 : 0) + ((g->frame_ticks >> 4) % 6));
}

static void ladder_common(sk_game *g) {
    sk_player *p = &g->player;
    if (p->jumping) { p->run_chain = 1; p->no_push_up = 0; p->state = 0; return; }
    p->controllable = 0;
    ladder_turn_anim(g);
    int hi = ((p->timer >> 8) + 1) & 3, lo = p->timer & 0xFF;    /* +0x2C counts frames, +0x2D the held direction */
    if (hi == 0) p->y = (int16_t)(p->y + 1);
    p->state = 0;
    if (p->input != p->prev_input) lo = 0xFF;
    lo = (lo + 1) & 0xFF;
    if (lo & 0x80) lo = (lo - 1) & 0xFF;                         /* saturates at 0x7F */
    p->timer = (int16_t)(hi << 8 | lo);
    p->prev_input = p->input;
    rebuild_player_mask(g);
    int d0 = left(p) ? -1 : 1;
    if (p->input & (left(p) ? SK_IN_LEFT : SK_IN_RIGHT)) {
        p->x = (int16_t)(p->x + d0);
        probe_wall(&g->mask, p->x - 8 + (left(p) ? 0 : 16), p->y, p->facing, p->facing, p->sliding != 0, &p->x, &p->speed);   /* sub_10AEE8 */
    }
}

static void player_state_ladder_a(sk_game *g) {                  /* 0x10BE40 */
    sk_player *p = &g->player;
    if ((p->input | 0xC) != (p->prev_input | 0xC)) p->y = (int16_t)(p->y - 2);
    ladder_common(g);
}

static void player_state_ladder_b(sk_game *g) {                  /* 0x10BE5A */
    if (g->player.input & SK_IN_UP) g->player.run_chain = 1;
    ladder_common(g);
}

static void knock_integrate(sk_game *g, bool knockback) {        /* shared tail of states 52 and 72 */
    sk_player *p = &g->player;
    rebuild_player_mask(g);
    floor_probe_step(g);
    push_up_and_walls(g);
    if (knockback) p->kick_phase++;
    p->vy--;
    if (p->vy >= 0) p->y = (int16_t)(p->y - (p->vy >> 1));
    else {
        int fall = (-p->vy) >> 1;
        if (fall > 8) fall = 8;
        if (fall < p->gap_below) p->y = (int16_t)(p->y + fall);
        else {
            p->y = (int16_t)(p->y + p->gap_below);
            if (!knockback || !((p->fall_flags & 2) && p->kick_phase < 60)) {
                p->vy = 0;
                p->invulnerable = (int16_t)(knockback ? 100 : 0);
                p->kick_phase = 0;
                p->fall_flags = 0;
                p->crushed = 0;
                p->state = 0;
                p->speed = 0;
                if (knockback) p->controllable = 0;
                p->anim = (int16_t)(left(p) ? 0x12 : 0x11);
            }
        }
    }
    ceiling_probe_at(g, p->x - 8 + (left(p) ? 0 : 16), p->y);
}

static void player_state_knockback(sk_game *g) {                 /* 0x10B4A8 */
    sk_player *p = &g->player;
    if (!p->crushed) {
        p->controllable = 0;
        int anim = left(p) ? 0x14 : 0x13;
        int dx = p->speed;
        if (p->knock_dir >= 0) dx = -dx;
        if (p->fall_flags & 2) { anim = 12; dx = p->carry_dx; }
        p->x = (int16_t)(p->x + dx);
        p->anim = (int16_t)anim;
    }
    knock_integrate(g, true);
}

static void player_state_bounce(sk_game *g) {                    /* 0x10C1BC */
    sk_player *p = &g->player;
    p->controllable = 0;
    int dx = p->speed;
    if (p->knock_dir >= 0) dx = -dx;
    p->x = (int16_t)(p->x + dx);
    p->anim = (int16_t)(left(p) ? 0x14 : 0x13);
    knock_integrate(g, false);
}

static void player_state_carried(sk_game *g) { g->player.x = (int16_t)(g->player.x + g->player.carry_dx); }   /* 0x10BB1E */

static void player_state_exit_walk(sk_game *g) {                 /* 0x10C04C, scroll mode 0 */
    sk_player *p = &g->player;
    p->run_chain = 1;
    p->input = 0;
    g->level_outcome |= 1;
    if (g->level_data_index != 12) {
        p->input = SK_IN_RIGHT;
        if (p->x - g->camera_x <= 320) return;
        p->exit_walk = 1;
    }
    if (!(g->level_outcome & 0x80)) {
        g->level_outcome = 0x81;
        g->level_number++;
        g->curtain_flags |= 0x10;
    }
}

static void player_state_game_over(sk_game *g) { g->player.controllable = 0; }               /* 0x10C106 */
static void player_state_timeout(sk_game *g) { g->player.input = 0; g->player.run_chain = 1; }   /* 0x10C10C */

static void (*const player_state_table[19])(sk_game *) = {
    player_state_none, player_state_none, player_state_kick, player_state_stub, player_state_tricks,
    player_state_death, player_state_ladder_a, player_state_ladder_b, player_state_fell_out, player_state_ball_spring,
    player_state_trick_hold, player_state_volley_right, player_state_volley_left, player_state_knockback, player_state_carried,
    player_state_exit_walk, player_state_game_over, player_state_timeout, player_state_bounce,
};

void dispatch_player_state(sk_game *g) {
    int i = g->player.state / 4;
    if (i < 0 || i >= 19 || (g->player.state & 3)) { g->player.state = 0; return; }
    player_state_table[i](g);
}
