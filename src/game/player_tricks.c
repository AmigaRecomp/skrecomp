/* The player's ball states and ball handling, transcribed from the listing: player_physics (0x10A80A), kick_charge
   (0x10AA4C, the ball recall), ball_handling (0x10C31E), the tricks (state 16, 0x10C26A), the pick-up hold (40),
   the volleys (44/48), the spring (36) and the ground kick (8) with kick_ball (0x10CD3A). a5 = ball throughout. */
#include "game/game.h"

#define PICKUP_KICK_TABLE 0x200D14
#define TRICK8_VX_TABLE 0x10C628

static const int16_t kick_table[16][2] = {      /* 0x10AB1A: (dx, dy) by fire_held - 8 (+8 facing right) */
    { -16, 32 }, { -16, 16 }, { -16, 0 }, { -16, -16 }, { 0, 32 }, { 0, 16 }, { 0, 0 }, { 0, -16 },
    { 0, 32 }, { 0, 16 }, { 0, 0 }, { 0, -16 }, { 16, 32 }, { 16, 16 }, { 16, 0 }, { 16, -16 },
};
static const int8_t steer_trick0[16] = { -1, 4, 8, -1, 1, -2, -1, -1, 2, -2, -1, -1, -1, -1, -1, -1 };   /* 0x10C4D8 */
static const int8_t steer_trick4[16] = { -1, -2, 5, -1, 6, -2, -2, -1, 7, -2, -2, -1, -1, -1, -1, -1 };  /* 0x10C754 */
static const uint8_t volley_anim_table[16] = { 1, 2, 0x83, 4, 4, 4, 5, 6, 5, 5, 5, 5, 6, 5, 5, 0xFF };  /* 0x200D94 */
static const int32_t pickup_kick_table[16][2] = {   /* 0x200D14: (vx, vy) by input & 0xF */
    { 0, 0 }, { 0, (int32_t)0xFFF60000 }, { 0, 0 }, { 0, 0 }, { (int32_t)0xFFFA0000, (int32_t)0xFFFA0000 },
    { (int32_t)0xFFFE0000, (int32_t)0xFFF60000 }, { (int32_t)0xFFF80000, (int32_t)0xFFFE0000 }, { 0, 0 },
    { 0x60000, (int32_t)0xFFFA0000 }, { 0x20000, (int32_t)0xFFF60000 }, { 0x80000, (int32_t)0xFFFE0000 }, { 0, 0 },
    { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
};
static const int32_t trick8_vx_table[4] = { 0x10000, (int32_t)0xFFFF4B58, (int32_t)0xFFFF4B58, 0x10000 };

static int16_t bx(const sk_game *g) { return (int16_t)(g->ball.x >> 16); }
static int16_t by(const sk_game *g) { return (int16_t)(g->ball.y >> 16); }
static int16_t bvy(const sk_game *g) { return (int16_t)(g->ball.vy >> 16); }
static void set_bx(sk_game *g, int v) { g->ball.x = (int32_t)((uint32_t)(uint16_t)v << 16 | ((uint32_t)g->ball.x & 0xFFFF)); }
static void set_by(sk_game *g, int v) { g->ball.y = (int32_t)((uint32_t)(uint16_t)v << 16 | ((uint32_t)g->ball.y & 0xFFFF)); }

static void ball_hold_offset(const sk_game *g, int *x, int *y) {   /* 0x10CE12 */
    const sk_player *p = &g->player;
    *y = p->y + 11;
    if (p->on_platform) *x = p->x + (p->facing >= 0 ? -6 : -10);
    else *x = p->x + (p->facing >= 0 ? 7 : -23);
}

/* A tile's 32-byte solidity table is all zero (the tests at 0x10A8A2 / 0x10AAD0). */
static bool tile_empty(const sk_game *g, int cx, int cy) {
    if (!world_inside(&g->world, cx, cy)) return true;
    const uint8_t *t = world_tail(&g->world, cx, cy);
    if (!t) return true;
    for (int i = 0; i < 32; i++) if (t[i]) return false;
    return true;
}

void kick_charge(sk_game *g) {                    /* 0x10AA4C: where the recalled ball will appear */
    sk_player *p = &g->player;
    if (p->on_platform) {
        g->throw_x = p->x + (p->facing >= 0 ? 16 : -16);
        g->throw_y = p->y;
        g->throw_ready = 1;
        return;
    }
    if (p->fire_held < 8 || g->throw_ready) return;
    int i = p->fire_held - 8 + (p->facing >= 0 ? 8 : 0);
    if (i > 15) i = 15;
    int x = p->x + kick_table[i][0], y = p->y + kick_table[i][1];
    if (x < 0) x = 0;
    g->throw_x = x;
    y = (y + 7) & ~15;
    g->throw_y = y;
    if (tile_empty(g, (x & ~15) >> 4, y >> 4) && tile_empty(g, ((x & ~15) >> 4) + 1, y >> 4)) g->throw_ready = 1;
}

void player_physics(sk_game *g) {                 /* 0x10A80A */
    sk_player *p = &g->player;
    int in = p->input;
    if (!(in & SK_IN_FIRE) || p->state != 0 || p->sliding) goto clear_down;
    if (in & SK_IN_DOWN) {                        /* drop through a thin floor */
        rebuild_player_mask(g);
        int rows = probe_ground(&g->mask, p->x, p->y);
        if (rows >= 11 || rows == 0) goto clear_down;
        p->falling = 1;
        p->y = (int16_t)(p->y + rows + 1);
        if (p->ball_hold >= 0) goto clear_down;
        int x0 = p->x & ~15, y1 = p->y + 41;      /* the ball goes down too when the two cells below are open */
        int cx = x0 >> 4, cy = y1 >> 4, row = y1 & 15;
        for (int k = 0; k < 16; k++) {            /* 16 rows from (x0, y1), crossing into the next cell */
            if (!world_inside(&g->world, cx, cy)) break;
            const uint8_t *t = world_tail(&g->world, cx, cy);
            if (t && (t[row * 2] | t[row * 2 + 1])) return;
            if (++row == 16) { row = 0; cy++; }
        }
        set_bx(g, x0);
        set_by(g, y1);
        g->ball.vy = (int32_t)((uint32_t)1 << 16 | ((uint32_t)g->ball.vy & 0xFFFF));
        return;
    }
    if (in & (SK_IN_LEFT | SK_IN_RIGHT)) {        /* fire with a direction: the ground kick */
        p->jump_input = (uint8_t)(in & (SK_IN_LEFT | SK_IN_RIGHT));
        p->timer = 0;
        p->state = 8;
        return;
    }
    if (p->ball_hold == 0) goto recall;
    if (p->ball_hold > 0) goto clear_down;
    /* the ball at the feet, fire held still: the tricks after 4 frames */
    if (p->jumping || p->falling || g->trick_block || p->carry_dx) goto clear_down;
    if (++p->down_held < 4) goto clear_fire;
    {
        int hx, hy;
        ball_hold_offset(g, &hx, &hy);
        if (!tile_empty(g, (hx & ~15) >> 4, (p->y + 16) >> 4)) goto clear_down;
    }
    p->jump_input = (uint8_t)p->input;
    p->trick = 0;
    p->trick_phase = 0;
    p->timer = 0;
    p->crouching = 0;
    p->state = 16;
clear_down:
    p->down_held = 0;
clear_fire:
    p->fire_held = 0;
    g->throw_ready = 0;
    return;
recall:                                           /* fire held: the ball spins back to the kid after 16 frames */
    if (p->jumping || p->falling || p->carry_dx) goto clear_down;
    if (++p->fire_held < 16) { kick_charge(g); return; }
    if (!g->throw_ready) goto clear_down;
    p->fire_lock = 1;
    if (g->layer_zone) {
        g->layer_flags |= 2;
        if (g->layer_zone != 1) g->layer_flags &= (uint8_t)~2;
        if (p->facing >= 0) g->layer_flags ^= 2;
    } else {
        g->layer_flags &= (uint8_t)~2;
        if (g->layer_flags & 1) g->layer_flags |= 2;
    }
    select_layers(g);
    p->ball_free = 0;
    g->ball.flag_1a = 0xFF;
    g->ball.kick_anim = 12;
    g->ball.inactive = 0xFF;
    g->ball.vanishing = 0;
    g->recalls++;
    g->ball.kick_timer = 100;
    set_bx(g, g->throw_x);
    set_by(g, g->throw_y);
    g->ball.vx = 0;
    g->ball.vy = 0x10000;
    g->ball.prev_x = (int16_t)g->throw_x;
    g->ball.prev_y = (int16_t)g->throw_y;
    g->kit_extra = g->kit_current;                /* set_kit_extra_colours with the current kit */
}

void ball_handling(sk_game *g) {                  /* 0x10C31E: landing on the ball from above starts trick 8 */
    sk_player *p = &g->player;
    if (!p->jumping || p->vy >= 0 || g->ball.touched || !(p->input & SK_IN_DOWN) || bvy(g) != 0) return;
    int d = p->y + 40 - by(g);
    if ((d < 0 ? -d : d) >= 6) return;
    d = p->x - bx(g) - 8;
    if ((d < 0 ? -d : d) >= 15) return;
    p->jump_input = (uint8_t)(p->input | SK_IN_FIRE);
    p->trick = 8;
    p->trick_flag = 0;
    p->trick_phase = (int16_t)0x8000;
    p->timer = 0;
    p->crouching = 0;
    p->x = (int16_t)(bx(g) + 8);
    p->y = (int16_t)(by(g) - 39);
    p->state = 16;
}

static void trick_steer(sk_game *g, const int8_t *table) {   /* 0x10C80C */
    sk_player *p = &g->player;
    if (p->trick_phase && (p->input & 0xF)) p->trick_flag = table[p->input & 0xF];
}

static void phase_tick(sk_player *p) {            /* addq #1; bpl; subq #1: saturating */
    if (p->trick_phase != 0x7FFF) p->trick_phase++;
}

static void trick_end(sk_game *g, bool free_ball) {
    sk_player *p = &g->player;
    p->jump_input &= (uint8_t)~SK_IN_FIRE;
    if (free_ball) p->ball_free = 0;
    p->timer = 0;
    p->state = 0;
}

static void tricks_dispatch(sk_game *g);

static void trick0_held(sk_game *g) {             /* 0x10C3B2: flick the ball up onto the foot, juggle */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    int hx, hy;
    if (p->trick_phase == 0) {
        p->trick_flag = -1;
        ball_hold_offset(g, &hx, &hy);
        if (by(g) - hy < 0 || bx(g) != hx) { trick_end(g, true); return; }
        set_bx(g, hx); set_by(g, hy);
        play_sfx(g, 3);
        b->vx = 0;
        g->kill_kind = 1;
        p->ball_free = 1;
        b->vy = (int32_t)0xFFFC0000;
        p->ball_hold = 0;
    } else trick_steer(g, steer_trick0);
    ball_hold_offset(g, &hx, &hy);
    int d0 = hx - bx(g) + 4, d1 = hy - by(g);
    if (d1 < 0) {
        if ((unsigned)(uint16_t)d0 >= 8) { trick_end(g, true); return; }
        play_sfx(g, 3);
        b->vx = 0;
        b->vy = (int32_t)0xFFFC0000;
        g->kill_kind = 1;
        p->ball_free = 1;
        p->trick_phase = 1;
        if (p->trick_flag >= 0) {
            if (p->on_platform) { p->fire_lock = 1; trick_end(g, true); return; }
            p->trick = (uint8_t)p->trick_flag;
            p->trick_flag = -1;
            p->trick_phase = 0;
            tricks_dispatch(g);
            return;
        }
    }
    if (d1 > 0x3E) { trick_end(g, true); return; }
    d0 += 0x1C;
    if ((unsigned)(uint16_t)d0 >= 0x40 || b->vanishing) { trick_end(g, true); return; }
    int d3 = p->facing >= 0 ? 0x26 : 0x28;
    int a = (bx(g) - p->x + 7) < 0 ? 0x28 : 0x26;
    if (d3 == a) {
        if (bvy(g) >= 0 || !((unsigned)p->trick_phase < 5 || p->anim == a)) a++;
        p->anim = (int16_t)a;
    }
    phase_tick(p);
    if ((uint16_t)p->trick_phase >= 0x4B) trick_end(g, true);
}

static void trick8_held(sk_game *g) {             /* 0x10C4E8: standing on the ball, rolling it along */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    if (p->trick_phase >= 0) {
        if (p->trick_phase == 0) { b->vx = b->vy = 0; goto tick; }
        if (bvy(g) != 0) goto tick;
        if ((unsigned)(uint16_t)(by(g) - p->y) < 0x17) goto tick;
        if (!ball_near_player(g)) goto tick;
        int16_t sx = p->x, sy = p->y;
        p->x = (int16_t)(bx(g) + 8); p->y = (int16_t)(by(g) - 39);
        g->mask.built = false;
        rebuild_player_mask(g);
        p->probe_flags = (uint8_t)probe_surroundings(&g->mask, p->x, p->y, p->facing, p->sliding != 0);
        p->x = sx; p->y = sy;
        if (p->probe_flags) goto tick;
    }
    {
        int v = bvy(g);
        if ((v < 0 ? -v : v) >= 3 || b->touched || p->carry_dx) goto end;
    }
    g->trick8_bias = 0;
    rebuild_player_mask(g);
    int d0 = 0;
    if (p->input & SK_IN_RIGHT) { g->trick8_bias = 0xC000; d0 = 16; }
    if (p->input & SK_IN_LEFT) { g->trick8_bias = (int32_t)0xFFFF4000; d0 = -16; }
    {                                             /* sub_10AE14: walls on either side of x + d0 */
        int16_t px = (int16_t)(p->x + d0), speed = 0;
        int f = 0;
        if (probe_wall(&g->mask, px - 8, p->y, -1, p->facing, p->sliding != 0, &px, &speed)) f |= 0x40;
        px = (int16_t)(p->x + d0);
        if (probe_wall(&g->mask, px + 8, p->y, 0, p->facing, p->sliding != 0, &px, &speed)) f |= 0x40;
        p->probe_flags = (uint8_t)f;
        if (f) g->trick8_bias = 0;
    }
    p->trick_phase = (int16_t)((uint16_t)p->trick_phase | 0x8000);
    p->trick_phase++;
    p->trick_phase = (int16_t)(((uint16_t)p->trick_phase & 0xFF00) | ((uint16_t)p->trick_phase & 0x1F));
    p->x = (int16_t)(bx(g) + 8); p->y = (int16_t)(by(g) - 39);
    {
        int ph = (p->trick_phase & 0x1F) >> 3;
        p->anim = (int16_t)((p->facing >= 0 ? 0x3C : 0x40) + ph);
        int32_t vx = g->sk ? (int32_t)image_u32(g->sk, TRICK8_VX_TABLE + 4u * (uint32_t)ph) : trick8_vx_table[ph];
        b->vx = vx + g->trick8_bias;
    }
    return;
tick:
    p->trick_phase++;
    if ((uint16_t)p->trick_phase < 0x4B) return;
end:
    trick_end(g, false);
}

static void trick_hop_anim(sk_game *g) {          /* 0x10C764 */
    sk_player *p = &g->player;
    if ((uint16_t)p->trick_phase >= 7) p->anim = (int16_t)(p->facing >= 0 ? 0x35 : 0x37);
    if (bvy(g) >= 0) p->anim = (int16_t)(p->facing >= 0 ? 0x34 : 0x36);
}

static void trick4_held(sk_game *g) {             /* 0x10C65C: the overhead kick */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    if (p->trick_phase == 0) {
        g->kill_kind = 3;
        p->ball_free = 1;
        b->vy = (int32_t)0xFFFA0000; b->vx = 0x7000; p->anim = 0x57;
        if (p->facing >= 0) { b->vy = (int32_t)0xFFFA0000; b->vx = (int32_t)0xFFFF9000; p->anim = 0x55; }
    }
    trick_steer(g, steer_trick4);
    bool d3 = false;
    int d0 = bx(g) - p->x + (p->facing >= 0 ? 11 : 10);
    if ((unsigned)(uint16_t)d0 < 4) { trick_hop_anim(g); d3 = true; }
    if (bvy(g) >= 0) {
        int hx, hy;
        ball_hold_offset(g, &hx, &hy);
        if (hy - 0x18 <= by(g) && d3) {
            if (p->trick_phase == 8) { trick_end(g, true); return; }
            b->vy = (int32_t)0xFFFC0000;
            g->kill_kind = 3;
            p->ball_free = 1;
            b->vx = 0;
            p->trick_phase = 7;
            play_sfx(g, 3);
            if (p->trick_flag >= 0) {
                p->ball_free = 0;
                p->trick = (uint8_t)p->trick_flag;
                p->trick_flag = -1;
                p->trick_phase = 0;
                tricks_dispatch(g);
                return;
            }
        }
    }
    phase_tick(p);
    if ((uint16_t)p->trick_phase >= 0x4B) trick_end(g, true);
}

static void trick3_held(sk_game *g) {             /* 0x10C824 */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    if (p->trick_phase == 0) {
        b->vy = (int32_t)0xFFFA0000; b->vx = 0x5000; p->anim = 0x57;
        if (p->facing >= 0) { b->vy = (int32_t)0xFFFA0000; b->vx = (int32_t)0xFFFFB000; p->anim = 0x55; }
    }
    if (bvy(g) >= 0) {
        int hx, hy;
        ball_hold_offset(g, &hx, &hy);
        hy -= 14;
        int d0 = bx(g) - p->x + (p->facing >= 0 ? 7 : 15);
        if (hy <= by(g) && (unsigned)(uint16_t)d0 < 8) {
            set_by(g, hy);
            b->vx = b->vy = 0;
            p->trick_phase &= 3;
            p->anim = (int16_t)(p->facing >= 0 ? 0x0F : 0x10);
        }
    }
    p->trick_phase++;
}

static void trick1_held(sk_game *g) {             /* 0x10C8B0 */
    sk_player *p = &g->player;
    p->trick = 0;
    if (p->facing >= 0) { p->facing = -1; g->ball.vy = (int32_t)0xFFFA0000; g->ball.vx = (int32_t)0xFFFF4000; p->anim = 0x55; }
    p->trick_phase++;
}

static void trick2_held(sk_game *g) {             /* 0x10C906 */
    sk_player *p = &g->player;
    p->trick = 0;
    if (p->facing < 0) { g->ball.vy = (int32_t)0xFFFA0000; p->facing = 1; p->anim = 0x57; g->ball.vx = 0xC000; }
    p->trick_phase++;
}

static void trick5_held(sk_game *g) {             /* 0x10C8DC */
    sk_player *p = &g->player;
    p->trick_phase++;
    p->trick = 3;
    g->ball.vy = (int32_t)0xFFFE8000;
    g->ball.vx = p->facing >= 0 ? 0x3000 : (int32_t)0xFFFFD000;
}

static void trick7_held(sk_game *g) {             /* 0x10C932 */
    sk_player *p = &g->player;
    p->trick = 4;
    if (p->facing >= 0) {
        p->trick = 7;
        if (p->anim == 0x35) { if (bvy(g) < 0) { p->trick_phase++; return; } p->trick = 0; return; }
        g->ball.vy = (int32_t)0xFFFB0000; g->ball.vx = 0x6000; p->anim = 0x35;
    }
    p->trick_phase++;
}

static void trick6_held(sk_game *g) {             /* 0x10C974 */
    sk_player *p = &g->player;
    p->trick = 4;
    if (p->facing < 0) {
        p->trick = 6;
        if (p->anim == 0x37) { if (bvy(g) < 0) { p->trick_phase++; return; } p->trick = 0; return; }
        g->ball.vy = (int32_t)0xFFFB0000; g->ball.vx = (int32_t)0xFFFFA000; p->anim = 0x37;
    }
    p->trick_phase++;
}

static void tricks_dispatch(sk_game *g) {         /* loc_10C2EC: trick_table_held (0x10C2FA) */
    switch (g->player.trick) {
    case 0: trick0_held(g); break;
    case 1: trick1_held(g); break;
    case 2: trick2_held(g); break;
    case 3: trick3_held(g); break;
    case 4: trick4_held(g); break;
    case 5: trick5_held(g); break;
    case 6: trick6_held(g); break;
    case 7: trick7_held(g); break;
    case 8: trick8_held(g); break;
    default: g->player.timer = 0; g->player.state = 0; break;
    }
}

static void trick0_released(sk_game *g) {         /* 0x10C9B6: the pick-up kick */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    p->anim = (int16_t)(p->facing >= 0 ? 0x54 : 0x56);
    int hx, hy;
    ball_hold_offset(g, &hx, &hy);
    int d0 = hx - bx(g) + 4, d1 = hy - by(g);
    if (d1 < 0 && (unsigned)(uint16_t)d0 < 8) {
        int i = p->input & 0xF;
        if (g->sk) { b->vy = (int32_t)image_u32(g->sk, PICKUP_KICK_TABLE + 8u * (uint32_t)i + 4); b->vx = (int32_t)image_u32(g->sk, PICKUP_KICK_TABLE + 8u * (uint32_t)i); }
        else { b->vx = pickup_kick_table[i][0]; b->vy = pickup_kick_table[i][1]; }
        if (b->vx == 0 && b->vy == 0) { p->timer = 0; p->state = 0; return; }
        if (b->vx != 0) {
            int32_t mask = p->facing >= 0 ? 0 : -1;
            if ((b->vx ^ mask) < 0) { b->vx = b->vy = 0; p->timer = 0; p->state = 0; return; }
        }
        p->anim++;
        play_sfx(g, 3);
        p->timer = 0;
        p->state = 40;
        g->kill_kind = 1;
        p->ball_free = 1;
        return;
    }
    if (d1 > 0x1E) { trick_end(g, false); return; }
    d0 += 0x1C;
    if ((unsigned)(uint16_t)d0 >= 0x40) { trick_end(g, false); return; }
    phase_tick(p);
    if ((uint16_t)p->trick_phase >= 0x4B) trick_end(g, false);
}

static void trick4_released(sk_game *g) {         /* 0x10C790: a direction turns the overhead into a volley */
    sk_player *p = &g->player;
    if (!(p->jump_input & (SK_IN_LEFT | SK_IN_RIGHT))) { p->timer = 0; p->state = 0; return; }
    p->anim = (int16_t)(p->facing >= 0 ? 0x46 : 0x4D);
    int hx, hy;
    ball_hold_offset(g, &hx, &hy);
    if (hy - 0x28 < by(g) && bvy(g) >= 0) {
        int bit = SK_IN_LEFT, st = 48, d0 = bx(g) - p->x;
        if (p->facing < 0) { d0 += 12; bit = SK_IN_RIGHT; st = 44; } else d0 += 13;
        if ((unsigned)(uint16_t)d0 < 8) {
            if (!(p->jump_input & bit)) { p->timer = 0; p->state = 0; return; }
            p->timer = 0;
            p->state = (int16_t)st;
            return;
        }
    }
    phase_tick(p);
    if ((uint16_t)p->trick_phase >= 0x4B) { p->timer = 0; p->state = 0; }
}

static void trick8_released(sk_game *g) {         /* 0x10C638: up on top of the ball = the spring */
    sk_player *p = &g->player;
    int st = 0;
    if ((p->jump_input & SK_IN_UP) && (unsigned)(uint16_t)(p->x - bx(g)) < 11) st = 36;
    p->timer = 0;
    p->state = (int16_t)st;
}

void player_state_tricks(sk_game *g) {            /* 0x10C26A, state 16 */
    sk_player *p = &g->player;
    g->ball.at_rest = 0;
    p->controllable = 0;
    p->tricking = 1;
    if (!(p->jump_input & SK_IN_FIRE)) {          /* fire released: trick_table_released */
        switch (p->trick) {
        case 0: trick0_released(g); return;
        case 4: trick4_released(g); return;
        case 8: trick8_released(g); return;
        default: p->timer = 0; p->state = 0; return;
        }
    }
    int in = p->input & 0xFF;
    bool fire = in & SK_IN_FIRE;
    in &= ~SK_IN_FIRE;
    if (!fire && in && (p->jump_input & ~SK_IN_FIRE) != in) {
        p->jump_input = (uint8_t)in;              /* a new direction without fire: ends at the next frame */
        p->trick_phase = 0;
        return;
    }
    p->jump_input = (uint8_t)(in | SK_IN_FIRE);
    tricks_dispatch(g);
}

void player_state_trick_hold(sk_game *g) {        /* 0x10CA8A, state 40 */
    sk_player *p = &g->player;
    p->controllable = 0;
    p->tricking = 1;
    if (++p->timer >= 20) { p->timer = 0; p->state = 0; }
}

static int volley_anim(sk_game *g) {              /* 0x10CB0C: 1 on the kick frame */
    sk_player *p = &g->player;
    p->tricking = 1;
    p->controllable = 0;
    p->timer++;
    int i = (uint16_t)p->timer >> 2;
    int v = i < 16 ? volley_anim_table[i] : 0xFF;
    if (v == p->anim) return 0;
    if (v == 0xFF) { p->timer = 0; p->state = 0; return 0; }
    int kick = v & 0x80;
    v = (v & 0x7F) + 0x46 + (p->facing < 0 ? 7 : 0);
    p->anim = (int16_t)v;
    return kick ? 1 : 0;
}

static bool ball_at_feet_height(const sk_game *g) {   /* 0x10CB68 */
    int d = g->player.y - by(g) - 4;
    return d >= 0 && d < 18;
}

static void volley(sk_game *g, int32_t vx) {
    if (!volley_anim(g) || !ball_at_feet_height(g)) return;
    g->kill_kind = 2;
    g->player.ball_free = 1;
    g->ball.vx = vx;
    g->ball.vy = (int32_t)0xFFFC0000;
    play_sfx(g, 3);
}

void player_state_volley_right(sk_game *g) { volley(g, 0x80000); }             /* 0x10CAA8, state 44 */
void player_state_volley_left(sk_game *g) { volley(g, (int32_t)0xFFF80000); }  /* 0x10CADA, state 48 */

void player_state_ball_spring(sk_game *g) {       /* 0x10CB82, state 36 */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    p->controllable = 0;
    if (p->timer == 0) {
        play_sfx(g, 4);
        b->at_rest = 1;
        b->vx = b->vy = 0;
        p->anim = (int16_t)(p->facing >= 0 ? 0x1C : 0x1F);
        p->fixed_vy = (int16_t)0xFF00;
        p->timer += 4;
    }
    switch (p->timer) {
    case 4: {                                     /* sink onto the ball */
        int d = (int8_t)((uint16_t)p->fixed_vy >> 8);
        p->fixed_vy = (int16_t)(p->fixed_vy + 0x40);
        p->y = (int16_t)(p->y + d);
        if (p->fixed_vy >= 0x1C0) { p->timer += 4; p->fixed_vy = 8; }
        return;
    }
    case 8:                                       /* crouch, then spring */
        p->anim = (int16_t)(p->facing >= 0 ? 0x0F : 0x10);
        if (--p->fixed_vy >= 0) return;
        b->at_rest = 0;
        p->jumping = 1;
        p->jump_y = p->y;
        p->timer += 4;
        p->fixed_vy = (int16_t)0xF800;
        b->vy = (int32_t)0xFFFC0000;
        play_sfx(g, 18);
        return;
    case 12: {                                    /* in the air (the code after the phase table, 0x10CBD2) */
        p->speed = 0;
        int d0 = (uint16_t)p->top_speed >> 2;
        if (p->input & SK_IN_RIGHT) { p->x = (int16_t)(p->x + d0); p->facing = (int16_t)d0; p->speed = p->top_speed; }
        else if (p->input & SK_IN_LEFT) { d0 = -d0; p->x = (int16_t)(p->x + d0); p->facing = (int16_t)d0; p->speed = p->top_speed; }
        p->anim = (int16_t)(p->facing >= 0 ? 0x44 : 0x45);
        int d = (int8_t)((uint16_t)p->fixed_vy >> 8);
        p->fixed_vy = (int16_t)(p->fixed_vy + 0x40);
        p->y = (int16_t)(p->y + d);
        if (d >= 0) {
            p->jump_hold = 16; p->vy = 0; p->jumping = 1; p->skidding = 0; p->timer = 0; p->state = 0;
        } else {
            g->mask.built = false;
            rebuild_player_mask(g);
            p->probe_flags = (uint8_t)probe_surroundings(&g->mask, p->x, p->y, p->facing, p->sliding != 0);
            if (p->probe_flags & 0x80) { p->timer = 0; p->state = 0; }
        }
        return;
    }
    default: p->timer = 0; p->state = 0; return;  /* offset 0 loops forever in the original; never reached */
    }
}

int kick_ball(sk_game *g) {                       /* 0x10CD3A: 1 kicked, -1 overhead from a jump, 0 missed */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    if (b->inactive) return 0;                    /* the port's guard: an inactive ball cannot be kicked */
    bool left = p->jump_input & SK_IN_LEFT;
    int d3 = left ? -32 : -4, d4 = left ? -12 : 18;
    int32_t vx = left ? (int32_t)0xFFFA0000 : 0x60000;
    int result;
    if (!p->jumping && p->ball_hold < 0) result = 1;
    else {
        int d1 = bx(g) - p->x, d2 = by(g) - p->y;
        if (d1 <= d3 || d1 >= d4) return 0;
        if (d2 > 16) { if (d2 > 0x1A) return 0; result = 1; }
        else if (d2 <= -16 || !p->jumping) return 0;
        else result = -1;
    }
    b->vy = (int32_t)0xFFFE0000;
    b->vx = vx;
    play_sfx(g, 3);
    g->kill_kind = result < 0 ? 3 : 0;
    p->ball_free = 1;
    p->ball_hold = 8;
    g->kick_cooldown = 200;
    return result;
}

void player_state_kick(sk_game *g) {              /* 0x10CCB2, state 8 */
    sk_player *p = &g->player;
    int d0 = 0x55, d1 = 0x39;
    if (p->facing < 0 || (p->facing == 0 && (p->jump_input & SK_IN_LEFT))) { d0 = 0x57; d1 = 0x3B; }
    if (p->timer == 0) g->kick_result = (int8_t)kick_ball(g);
    p->timer++;
    if (g->kick_result < 0) {
        int t = d0; d0 = d1; d1 = t;
        if (p->timer >= 18) { p->timer = 0; p->state = 0; return; }
    } else if (!(p->input & SK_IN_FIRE) && p->timer >= 12) { p->timer = 0; p->state = 0; return; }
    p->anim = (int16_t)d0;
    if (p->timer < 6) p->anim--;
    p->input &= SK_IN_FIRE;
    p->controllable = 0;
    p->run_chain = 1;
}
