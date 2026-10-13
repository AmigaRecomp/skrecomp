/* The player: init_player (0x10BC28), player_input_state (0x10A576) and the movement chain it runs.
   Each function is the routine of the same name in re/symbols/sk.sym; see re/docs/notes/player.md. */
#include <stddef.h>
#include <string.h>
#include "game/camera.h"
#include "game/attr_england.h"
#include "game/game.h"
#include "game/obj_england.h"

void play_sfx(sk_game *g, int n) { g->sfx_last = n; g->sfx_count++; }

static int airborne(const sk_player *p) { return p->jumping || p->falling; }   /* tst.w $8(a6) */

void rebuild_player_mask(sk_game *g) { mask_rebuild(&g->mask, &g->world, g->player.x, g->player.y); }

void init_player(sk_game *g) {
    sk_player *p = &g->player;
    g->view.full_redraw = true;                                 /* mark_view_redraw, mark_backdrop_redraw */
    g->layer_flags = g->layer_flags_record & ~2u;
    if (g->layer_flags & 1) g->layer_flags |= 2;
    select_layers(g);
    memset(&p->jumping, 0, sizeof *p - offsetof(sk_player, jumping));   /* +0x08 .. +0x5D */
    p->top_speed = 14;
    replay_bump_restores(g);                                    /* the tile-restore list (0x10BC82) */
    objects_flush_deferred(g);                                  /* the deferred-free list */
    p->x = (int16_t)g->world.start_x;
    p->y = (int16_t)g->world.start_y;
    p->jump_y = p->y;
    g->ball.x = (int32_t)((uint32_t)(uint16_t)g->world.start_x << 16 | ((uint32_t)g->ball.x & 0xFFFF));   /* move.w: high word only */
    g->ball.y = (int32_t)((uint32_t)(uint16_t)g->world.start_y << 16 | ((uint32_t)g->ball.y & 0xFFFF));
    p->facing = 1;
    g->look_x = 0x40;
    if (g->scroll_mode == 1) { p->facing = -1; g->look_x = -0x40; }
    g->ball.inactive = 0;
    p->dying = 0;
    p->falling = 1;
    p->exit_walk = 0;
    g->push_x = 0;
    p->health = 2;                                              /* 3 when byte 0x300366 is set: not in 5a */
    g->hud_redraw |= 1;
    g->timer_flags = 1;
    if (g->level_data_index != 12 && !(g->game_flags & 1)) spawn_jingle(g, 0);
}

void slope_probe_step(sk_game *g) {                             /* slope_probe 0x10B198 */
    sk_player *p = &g->player;
    if (airborne(p)) return;
    if (p->x == p->prev_x && p->y == p->prev_y) return;
    p->top_speed = (int16_t)probe_slope(&g->mask, p->x, p->y, p->facing);
    if (p->powerup) p->top_speed += 8;
}

void floor_probe_step(sk_game *g) {                             /* floor_probe 0x10B0CA */
    sk_player *p = &g->player;
    int gap, push;
    probe_floor(&g->mask, p->x, p->y, &gap, &push);
    if (p->y >= g->world.fg_max_y + 160) gap = 0;               /* fell out: the state code handles it */
    p->gap_below = (int16_t)gap;
    p->push_up = (int16_t)push;
}

void crouch_slide(sk_game *g) {                                 /* 0x10AB88 */
    sk_player *p = &g->player;
    p->crouching = 0;
    if (!p->jumping && (p->input & SK_IN_DOWN)) {
        int dir = p->facing < 0 ? SK_IN_LEFT : SK_IN_RIGHT;
        if ((p->input & dir) && !p->falling) {
            if (p->sliding) return;
            if (p->speed >= 8) { p->sliding = 0xFF; p->speed = 20; return; }
        }
        p->jump_y = (int16_t)(p->y + 50);                       /* crouch */
        p->crouching = 1;
        p->idle_frames = 0;
    }
    p->sliding &= 0x0F;
}

static void accelerate(sk_game *g) {                            /* loc_10ACA8 */
    sk_player *p = &g->player;
    if (p->speed > p->top_speed) {
        if (p->speed < 14 || (g->frame_ticks & 15) == 0) p->speed--;
    } else if (p->speed < p->top_speed) {
        if (p->speed < 14 || (g->frame_ticks & 7) == 0) p->speed++;
    }
}

void horizontal_move(sk_game *g) {                              /* 0x10ABE6 */
    sk_player *p = &g->player;
    bool clear_idle = true;
    if (p->sliding) {
        if ((g->frame_ticks & 1) == 0 && --p->speed < 0) p->speed = 0;
    } else {
        int dir_bits = p->input & (SK_IN_LEFT | SK_IN_RIGHT);
        if (!dir_bits) {
            p->skidding = 0;
            if (p->speed - 1 < 0) clear_idle = false;           /* standing still: idle frames count on */
            else p->speed--;
        } else {
            bool run_rest = true;
            if (p->skidding) {
                if (--p->speed >= 0) run_rest = false;
                else { p->speed = 0; p->skidding = 0; }
            }
            if (run_rest) {
                p->kick_phase = 0;
                int16_t d1 = p->facing;
                if (dir_bits & SK_IN_LEFT) d1 = -1;
                if (dir_bits & SK_IN_RIGHT) d1 = 1;
                if (p->crouching) {
                    if (--p->speed < 0) { p->facing = d1; p->speed = 0; }
                } else if (airborne(p)) {
                    if (d1 != p->facing) {
                        p->speed -= 2;
                        if (p->speed < 0) { p->facing = d1; p->speed = 0; accelerate(g); }
                    } else accelerate(g);
                } else {
                    if (p->speed == 0) p->facing = d1;
                    if (d1 != p->facing) { p->skidding |= 0x80; p->kick_phase = 0; }
                    accelerate(g);
                }
            }
        }
    }
    if (clear_idle) p->idle_frames = 0;
    int dx = p->speed >> 2;
    if (p->facing < 0) dx = -dx;
    int nx = p->x + p->carry_dx + dx;
    if (nx >= 0) p->x = (int16_t)nx;
}

void step_down(sk_game *g) {                                    /* 0x10B178 */
    sk_player *p = &g->player;
    if (airborne(p)) return;
    if (p->gap_below <= 4) p->y = (int16_t)(p->y + p->gap_below);
    else { p->falling = 1; p->idle_frames = 0; }
}

void push_up_and_walls(sk_game *g) {                            /* 0x10AE5A */
    sk_player *p = &g->player;
    if (!p->no_push_up && !airborne(p) && p->push_up) {
        if (p->sliding) p->speed = 0;
        p->y = (int16_t)(p->y - p->push_up);
        if (p->push_up == 10) { p->x = p->prev_x; p->y = p->prev_y; p->speed = 0; }   /* stuck: revert */
    }
    if (airborne(p) || p->on_platform) {
        probe_wall(&g->mask, p->x - 8, p->y, -1, p->facing, p->sliding != 0, &p->x, &p->speed);
        probe_wall(&g->mask, p->x + 8, p->y, 0, p->facing, p->sliding != 0, &p->x, &p->speed);
        return;
    }
    if (p->x == p->prev_x) return;
    if (p->x > p->prev_x) probe_wall(&g->mask, p->x + 8, p->y, 0, p->facing, p->sliding != 0, &p->x, &p->speed);
    else probe_wall(&g->mask, p->x - 8, p->y, -1, p->facing, p->sliding != 0, &p->x, &p->speed);
}

void slide_turn(sk_game *g) {                                   /* 0x10AB5A */
    sk_player *p = &g->player;
    if (!p->sliding || p->speed != 0) return;
    if (p->facing < 0) { if (p->input & SK_IN_RIGHT) p->facing = (int16_t)-p->facing; }
    else if (p->input & SK_IN_LEFT) p->facing = (int16_t)-p->facing;
}

void jump_control(sk_game *g) {                                 /* 0x10AD0A: player.md "Jump and gravity" */
    sk_player *p = &g->player;
    bool up = (p->input & SK_IN_UP) != 0;
    bool integrate_only = p->falling && (p->vy <= -3 || !up);
    if (!integrate_only) {
        if (!up) {
            if (!p->jumping) return;
            p->jump_hold = 16;
        } else {
            if (!p->jumping) {
                if (p->sliding) return;
                play_sfx(g, 4);
                p->jumping = 1;
                p->jump_hold = 0;
                p->ball_hold = 8;
                p->jump_y = p->y;
            }
            if (p->jump_hold < 16) { p->jump_hold++; p->vy = 10; }
        }
    }
    p->idle_frames = 0;
    p->vy--;
    if (p->vy >= 0) { p->y = (int16_t)(p->y - (p->vy >> 1)); return; }
    int fall = (-p->vy) >> 1;
    if (fall == 0) return;
    if (fall > 8) fall = 8;
    if (fall < p->gap_below) { p->y = (int16_t)(p->y + fall); return; }
    p->y = (int16_t)(p->y + p->gap_below);                     /* landed */
    p->vy = 0;
    p->jumping = 0;
    p->on_rope = 0;
    p->falling = 0;
}

int ceiling_probe_at(sk_game *g, int x, int y) {                /* ceiling_probe 0x10B000 with its side effects */
    sk_player *p = &g->player;
    int r = probe_ceiling(&g->mask, x, y, p->facing, p->sliding != 0);
    if (p->sliding) {
        if (r & SK_CEIL_SLIDE_BLOCKED) { if (p->speed < 5) p->speed = 7; }
        else if (!(p->sliding & 0x80)) p->sliding = 0;
    }
    if (!(r & SK_CEIL_BUMP)) return 0;
    play_sfx(g, 6);
    p->y = p->prev_y;
    p->vy = -1;
    p->jump_hold = 16;
    return -1;
}

void ceiling_check(sk_game *g) {                                /* 0x10AF0E */
    sk_player *p = &g->player;
    if (!p->sliding) {
        if (!p->jumping) return;
        if (p->y >= p->prev_y) return;                          /* only while rising */
    }
    int x = p->x - 8;
    if (p->facing >= 0) x += 16;                                /* ceiling_probe takes 16 off again for facing right */
    ceiling_probe_at(g, x, p->y);
}

void edge_probe_step(sk_game *g) {                              /* edge_probe 0x10B24E */
    sk_player *p = &g->player;
    if (p->idle_frames < 25) { g->edge_cache = 0; p->teeter = 0; return; }
    if (p->x == g->edge_cache) return;
    g->edge_cache = p->x;
    if (!probe_edge(&g->mask, p->x, p->y, p->facing)) p->teeter = 0xFF;
}

int hurt_player(sk_game *g) {                                   /* 0x10A6C4 */
    sk_player *p = &g->player;
    if (g->level_outcome || p->dying) return -3;
    if (p->health == 0) {
        if (p->invulnerable) return -3;
        freeze_death_target(g);
        p->dying = 1;
        p->kick_phase = 0;
        p->state = 20;
        g->ball.inactive = 1;
        p->timer = 0x600;
        play_sfx(g, 7);
        g->music_stopped = true;
        return -2;
    }
    if (p->invulnerable) return -3;
    p->health--;
    g->hud_redraw |= 1;
    p->invulnerable = -1;
    p->timer = 0;
    p->kick_phase = 0;
    p->on_rope &= 0x7F;
    p->knock_dir = p->facing;
    p->state = 52;
    p->speed = 2;
    play_sfx(g, 7);
    p->vy = 14;
    if (p->fall_flags & 1) { p->fall_flags |= 2; p->vy = 0; }
    return -1;
}

void bounce_player(sk_game *g) {                                /* 0x10A7C4 */
    sk_player *p = &g->player;
    if (p->dying || p->state == 60 || p->invulnerable) return;
    p->state = 72;
    p->timer = 0;
    p->kick_phase = 0;
    p->on_rope &= 0x7F;
    p->knock_dir = p->facing;
    p->speed = 2;
    p->vy = 8;
}

void player_input_state(sk_game *g) {                           /* 0x10A576 */
    sk_player *p = &g->player;
    g->mask.built = false;
    p->controllable = 1;
    p->run_chain = 0;
    p->no_push_up = 0;
    p->anim_hold = 0;
    p->tricking = 0;
    int in = g->input_state;                                    /* sub_100BD0 only records/replays demos: identity */
    if (!(in & SK_IN_FIRE)) p->fire_lock = 0;
    if (p->fire_lock) in &= ~SK_IN_FIRE;
    p->input = (int16_t)in;
    if (g->push_x && g->push_x >= p->x) p->input = (int16_t)((p->input & SK_IN_UP) | SK_IN_RIGHT);
    slope_probe_step(g);
    p->prev_x = p->x;
    p->prev_y = p->y;
    player_physics(g);
    if (p->state) dispatch_player_state(g);
    if (p->state == 0 || p->run_chain) {                       /* tst.b $45(a6); beq skip: a handler must ask for the chain */
        crouch_slide(g);
        horizontal_move(g);
        rebuild_player_mask(g);
        floor_probe_step(g);
        step_down(g);
        push_up_and_walls(g);
        slide_turn(g);
        jump_control(g);
        ceiling_check(g);
        edge_probe_step(g);
        ball_handling(g);
    }
    if (p->invulnerable) p->invulnerable--;
    g->ball.touched = 0;
    g->layer_zone = 0;
    player_tile_attrs(g);
    /* rope_catch: 5b */
    p->idle_frames++;
    p->on_platform = 0;
}

void freeze_death_target(sk_game *g) {                          /* sub_106C22 */
    camera_target(g, &g->death_target_x, &g->death_target_y);
}

void kill_player(sk_game *g) {                                  /* sub_10A780 */
    sk_player *p = &g->player;
    if (p->dying) return;
    freeze_death_target(g);
    p->dying = 1;
    p->kick_phase = 0;
    p->state = 20;
    g->ball.inactive = 1;
    p->timer = 0x600;
    p->health = 0;
    g->hud_redraw |= 1;
    play_sfx(g, 7);
    g->music_stopped = true;
}
