#include <string.h>
#include "game/attrs.h"
#include "game/camera.h"
#include "game/attr_england.h"
#include "game/game.h"
#include "game/level_flow.h"
#include "game/scroll_modes.h"
#include "game/objects.h"
#include "game/player.h"

#define TIMER_RATE 0x5DC                /* frames per timer unit (0x10066C) */
#define DIW_PLAY_TOP 0x2D               /* the open window's first line (DIWSTRT 0x2D91) */
#define DIW_PLAY_BOTTOM 0xF5            /* the play area's last line + 1 (DIWSTOP 0xF5B1 at the close request) */
#define DIW_CLOSED 0x91                 /* both edges meet here when closed */

static void step(sk_game *g, int n) { if (g->step_probe) g->step_probe(g, n); }

void game_new_game(sk_game *g) {                                /* 0x10402E and the score clears */
    if (!g->rng) g->rng = 0xDC359A7Bu;                          /* the start-up seed (0x1003FC): once per run */
    g->player.lives = 3;
    g->player.score = 0;
    g->level_number = 1;
}

static void level_record_head(const sk_game *g, int data_index, uint32_t *timer, uint8_t *layer, int *kit) {
    *timer = 0x45; *layer = 0; *kit = 0;                        /* England's values when no image is loaded */
    if (!g->sk) return;
    uint32_t a = SK_LEVEL_RECORDS_ADDR + (uint32_t)data_index * SK_LEVEL_RECORD;
    *timer = image_u16(g->sk, a);
    *layer = image_u8(g->sk, a + 3);
    *kit = image_u8(g->sk, a + 4);
}

static void merge_kid_platforms(sk_mask *m, void *ctx) {        /* merge_platform_masks (0x1181AC) after a rebuild */
    merge_platform_masks((sk_game *)ctx, m->bits, m->origin_x, m->origin_y, false);
}

bool game_setup_level(sk_game *g, const sk_level *lv, int data_index) {   /* setup_level + load_level's gameplay part */
    g->level_outcome = 0;
    /* reset_level_state (0x1047F8) */
    memset(&g->ball, 0, sizeof g->ball);
    ball_world_init(&g->bw);
    g->frame_ticks = 0;
    g->push_x = 0; g->shake_amplitude = 0; g->powerup_timer = 0; g->scroll_mode = 0; g->cam_ease = 4;
    g->death_flags = 0;
    g->hoop_count = 0;                                           /* 0x104942 */                                          /* free_level_data clears it (0x10374C) */
    platforms_reset(g);                                          /* sub_118136 */
    g->mask.merge = merge_kid_platforms; g->mask.merge_ctx = g;
    g->flutter_max = 5; g->flutter_count = 0; g->flutter_last_cell = 0;   /* 0x1003DE, 0x10491E */
    memset(g->ball_target, 0, sizeof g->ball_target);
    memset(g->message_shown, 0, sizeof g->message_shown);
    g->message_box_timer = 0; g->message_box_request = 0; g->message_wait = 0;
    g->bump_restores = 0; g->flush_count = 0; g->bump_tile = -1; g->hud_card_flag = 0;   /* 0x104878 */
    g->msg_visible = 0; g->msg_w = 0; g->splash_done = 0;
    memset(g->card_anim, 0, sizeof g->card_anim);
    g->untouchable = false;
    if (g->world.live) world_free(&g->world);
    if (!world_build(&g->world, lv)) return false;
    objects_reset(g);
    attrs_reset(g);
    uint32_t timer; uint8_t layer; int kit;
    level_record_head(g, data_index, &timer, &layer, &kit);
    g->level_data_index = data_index;
    g->layer_flags_record = layer;
    g->kit = g->kit_current = g->kit_extra = kit;
    g->mask.built = false; g->mask.dirty = true;
    g->camera_x = g->camera_y = g->camera_y_raw = 0;
    g->scan_col_left = g->scan_row_left = 0;
    g->scrolled_right = g->scrolled_down = false;
    scroll_modes_setup(g, data_index);                           /* 0x104190: BATTLE SHIP and TRAIN */
    init_player(g);
    g->player.top_speed = 16;                                    /* move.w #$10, player (0x104188) */
    g->timer_tick = 0;
    g->timer_rate = TIMER_RATE;
    g->level_timer = timer;
    g->timeout_armed = false;
    g->curtain_flags = 4;                                        /* open request */
    g->curtain_step = 0x62;
    g->curtain_top = g->curtain_bottom = 100;
    g->level_end = false;
    g->view.full_redraw = true;
    hud_init(g);                                                 /* 0x10372E: clear and draw the panel */
    return true;
}

void game_close(sk_game *g) { if (g->world.live) world_free(&g->world); }

static uint16_t bcd_sub1(uint16_t w) {                           /* sbcd over the two low bytes */
    for (int i = 0; i < 4; i++) {
        unsigned d = w >> (4 * i) & 15;
        if (d) { w = (uint16_t)(w - (1u << (4 * i))); return w; }
        w = (uint16_t)(w | 9u << (4 * i));                       /* borrow: this digit becomes 9 */
    }
    return w;
}

void timer_tick(sk_game *g) {                                   /* vbl_interrupt 0x100D60 */
    if (!(g->timer_flags & 0x80)) return;
    if (++g->timer_tick < g->timer_rate) return;
    g->timer_tick = 0;
    if (g->level_timer == 0) {                                   /* the clock stops; only the scoreboard arms the time-out */
        g->timer_flags &= (uint8_t)~0x80;
        return;
    }
    g->level_timer = (g->level_timer & 0xFFFF0000u) | bcd_sub1((uint16_t)g->level_timer);
    g->timer_flags |= 1;
}

void timeout_check(sk_game *g) {                                /* sub_11EF7C */
    if (g->level_timer != 0) return;
    g->player.state = 68;
    g->jingle_requested = true; g->jingle = 2;
    g->timer_rate = TIMER_RATE;
    g->timeout_armed = false;
}

void run_curtain(sk_game *g) {                                  /* 0x10471A */
    if (!g->curtain_flags) return;
    if (g->curtain_flags & 4) {                                  /* open request: start closed, then open */
        g->curtain_flags &= (uint8_t)~4;
        g->curtain_step = 0x62;
        g->curtain_top = g->curtain_bottom = DIW_CLOSED - DIW_PLAY_TOP;   /* 100 */
        g->curtain_flags |= 2;
    }
    if (g->curtain_flags & 8) { g->curtain_flags &= (uint8_t)~8; g->curtain_top = g->curtain_bottom = 0; }
    if (g->curtain_flags & 1) {                                  /* closing */
        if (g->curtain_step >= 0x64) g->level_end = true;
        g->curtain_step += 2;
        g->curtain_top += 2; g->curtain_bottom += 2;
        return;
    }
    if (g->curtain_flags & 0x10) {                               /* close request */
        g->curtain_flags &= (uint8_t)~0x10;
        g->curtain_flags |= 1;
        g->curtain_step = 2;
        g->curtain_bottom = 0;
    }
    if (g->curtain_flags & 2) {                                  /* opening */
        if (g->curtain_step <= 2) { g->curtain_flags &= (uint8_t)~2; g->curtain_flags |= 8; }
        g->curtain_step -= 2;
        g->curtain_top -= 2; g->curtain_bottom -= 2;
    }
}

int curtain_rows(const sk_game *g) { return g->curtain_top > 0 ? g->curtain_top : 0; }

static void anim_group(sk_game *g, int grp) {                    /* sub_1095CE */
    uint16_t *t = g->tile_anim;
    t[1 + 16 * grp] ^= 0x80;                                     /* bchg #7 on the low bytes: two-frame toggles */
    t[129 + 16 * grp] ^= 0x80;
    uint16_t cap = 0x100;
    for (int k = 0; k < 14; k++) {                               /* entry k: k + 3 frames, its mirror runs backwards */
        uint16_t *e = &t[2 + 16 * grp + k];
        *e = (uint16_t)(*e + 0x80);
        if ((int16_t)*e > (int16_t)cap) *e = 0;
        t[130 + 16 * grp + k] = (uint16_t)(cap - *e);
        cap = (uint16_t)(cap + 0x80);
    }
}

void tile_anim_step(sk_game *g) {                                /* sub_1094EA */
    g->anim_phase += 4;
    int ph = g->anim_phase & 0xC;
    for (int k = 0; k < 2; k++) {
        int grp = (ph >> 1) + k;                                 /* phase 0: groups 0 1, 4: 2 3, 8: 4 5, 12: 6 7 */
        if (grp == 0) { anim_group(g, 0); continue; }
        if (--g->anim_wait[grp] >= 0) continue;
        g->anim_wait[grp] = grp;
        anim_group(g, grp);
    }
}

uint16_t fade_colour(uint16_t from, uint16_t to, int step) {     /* sub_104E40: one of the 16 steps */
    uint16_t d0 = from & 0xF00, d1 = from & 0xF0, d2 = (uint16_t)((from & 0xF) << 4);
    uint16_t d3 = (uint16_t)((uint16_t)((to & 0xF00) - d0) >> 4), d4 = (uint16_t)((uint16_t)((to & 0xF0) - d1) >> 4);
    uint16_t d5 = (uint16_t)((to & 0xF) - (from & 0xF));
    for (int k = 0; k < step; k++) { d0 = (uint16_t)(d0 + d3); d1 = (uint16_t)(d1 + d4); d2 = (uint16_t)(d2 + d5); }
    return (uint16_t)((d0 & 0xF00) | (d1 & 0xF0) | ((d2 & 0xF0) >> 4));
}

void game_frame_step(sk_game *g, uint8_t input, sk_sprite *kid) {
    if (g->message_wait) {                                       /* step 31 (sub_11C816): the game waits for fire */
        g->input_state = input;
        if (!(input & SK_IN_FIRE)) return;
        g->message_wait = 0;                                     /* sub_11C89E: the box goes, its flag clears */
        g->msg_visible = 0;
        g->message_shown[g->message_box_shown_kind & 7] = 0;
    }
    g->frame_counter++;                                          /* the VBL's work */
    timer_tick(g);
    g->input_state = input;
    step(g, 1); g->frame_ticks++;
    step(g, 2);
    if (g->powerup_timer && --g->powerup_timer == 0) g->player.powerup = 0;
    if (g->flash_timer) g->flash_timer--;                       /* step 6 (sub_104C78): the app shows the fade step */
    step(g, 3);
    if (g->shake_amplitude && g->scroll_mode != 2 && (g->frame_ticks & 3) == 0) {
        g->shake_amplitude = -g->shake_amplitude;
        if (g->shake_amplitude > 0) g->shake_amplitude--;
    }
    scroll_modes_frame(g);                                       /* 0x104338: sub_1049C8 / sub_104BF0 */
    step(g, 8); if (g->timeout_armed) timeout_check(g);
    step(g, 10); update_camera(g);
    step(g, 11); update_player(g, kid);
    step(g, 13); message_box_tick(g);
    step(g, 14); process_tile_queue(g);
    step(g, 15); edge_scan_rows(g);
    step(g, 16); edge_scan_columns(g);
    step(g, 18); backdrop_stream_step(g);                        /* backdrop_scroll's sub_1068EE; the app places the rest */
    step(g, 19); tile_anim_step(g);                              /* blit_fg_strips' first call */
    step(g, 21);                                                 /* draw_all_objects: the app draws after the step */
    step(g, 23); if (g->ball_sprite) ball_sprites(g, g->ball_sprite);
    step(g, 24); hud_update(g);
    step(g, 25); update_ball(g);
    step(g, 26); player_input_state(g);
    step(g, 27); update_objects(g);
    step(g, 29); run_curtain(g);
    step(g, 32);                                                 /* request flags: the window handles Escape */
}

int play_level_next(const sk_tables *t, int level_number) {
    for (int i = level_number + 1; i < SK_LEVEL_SEQUENCE_LEN; i++) {
        int d = t->sequence[i];
        if (d == 42) return -1;
        if (d == 12 || d == 29 || (d >= 30 && d <= 35)) continue;
        return i;
    }
    return -1;
}

sk_outcome game_outcome(const sk_game *g) {
    if (g->level_end && (g->level_outcome & 0x80)) return SK_OUTCOME_NEXT_LEVEL;
    if (g->player.state == 64 && g->player.lives < 0) return SK_OUTCOME_GAME_OVER;   /* state 64 alone is "frozen": the lift tube uses it */
    return SK_OUTCOME_RUNNING;
}

uint32_t game_random(sk_game *g) {                              /* sub_102B42 */
    uint32_t c = g->rng >> 31;
    g->rng <<= 1;
    if (!c) g->rng ^= 0x1D872B41u;
    return g->rng;
}
