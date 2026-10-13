#include <stdio.h>
#include "app/audio_out.h"
#include <stdlib.h>
#include <string.h>
#include "game/player.h"
#include "game/world.h"
#include "app/play.h"
#include "game/camera.h"
#include "game/level_flow.h"
#include "game/objects.h"
#include "gfx/display_model.h"

#define GAME_OVER_FRAMES 150            /* ruling: the original's exit from state 64 is unknown */

bool script_parse_step(const char **s, uint8_t *input, int *frames, sk_err *e) {
    const char *c = *s;
    uint8_t in = 0;
    const char *start = c;
    for (; *c && (*c < '0' || *c > '9'); c++) {
        switch (*c) {
        case 'U': in |= SK_IN_UP; break;
        case 'D': in |= SK_IN_DOWN; break;
        case 'L': in |= SK_IN_LEFT; break;
        case 'R': in |= SK_IN_RIGHT; break;
        case 'F': in |= SK_IN_FIRE; break;
        case '.': break;
        default: return sk_fail(e, "--script: unknown key '%c' in \"%s\"", *c, start);
        }
    }
    if (*c < '0' || *c > '9') return sk_fail(e, "--script: a frame count must follow \"%s\"", start);
    int n = 0;
    for (; *c >= '0' && *c <= '9'; c++) n = n * 10 + (*c - '0');
    if (n <= 0) return sk_fail(e, "--script: a frame count must be positive in \"%s\"", start);
    if (*c == ',') c++;
    else if (*c) return sk_fail(e, "--script: expected a comma in \"%s\"", start);
    *s = c;
    *input = in;
    *frames = n;
    return true;
}

bool play_start_level(sk_play *p, int pos, int data_index, sk_err *e) {
    if (pos >= 0) data_index = p->scene.tables.sequence[pos];
    if (!(p->scene.has_level && p->scene.level_index == data_index) && !scene_load_level(&p->scene, data_index, e)) return false;   /* the parsed level is pristine: a restart rebuilds the world from it */
    if (p->game.tables) {                                           /* the level's graphics sets by slot */
        resources_free(&p->res);
        if (!resources_load(&p->res, &p->scene.disks, p->game.tables, data_index, e)) return false;
        p->game.res = &p->res;
    }
    if (!game_setup_level(&p->game, &p->scene.level, data_index)) return sk_fail(e, "out of memory");
    p->game.snd = &p->snd;
    sk_err se = {0};
    if (!sound_load_level(&p->snd, &p->scene.disks, &p->scene.tables, data_index, &se))   /* load_level_resources: the music and effect samples */
        fprintf(stderr, "soccerkid: no sound for level %d: %s\n", data_index, se.msg);
    if (data_index == 12 || data_index == 0x29) sound_music(&p->snd, SK_MUSIC_LEVEL, 0);   /* 0x103708: no KICK OFF, the tune at once */
    if (p->has_start && p->start_level == data_index) {                             /* a test start: kept for every restart of this level */
        p->game.world.start_x = p->start_x;
        p->game.world.start_y = p->start_y;
        const sk_world *w = &p->game.world;                         /* the layer switches (attributes 53/54) he would have walked past */
        for (int cx = 0; cx < w->w && cx * 16 + 8 <= p->start_x; cx++)
            for (int cy = 0; cy < w->h; cy++) {
                int at = world_attr(w, cx, cy);
                if (at == 53) { p->game.layer_flags_record &= (uint8_t)~1; break; }   /* past a left-edge switch: behind */
                if (at == 54) { p->game.layer_flags_record |= 1; break; }             /* past a right-edge switch: in front */
            }
        init_player(&p->game);
    }
    if (p->game.sk) kit_palette(&p->scene.palette, p->game.sk, p->game.kit, p->game.kit);
    p->level_pos = pos;
    p->game_over_frames = 0;
    return true;
}

bool play_open(sk_play *p, const sk_args *a, sk_err *e) {
    memset(p, 0, sizeof *p);
    if (!scene_open(&p->scene, a, e)) return false;
    sound_init(&p->snd, &p->scene.sk, 48000);
    p->game.sk = &p->scene.sk;
    p->game.tables = &p->scene.tables;
    p->script = a->script;
    p->script_cursor = a->script;
    int pos = -1;
    for (int i = 0; i < SK_LEVEL_SEQUENCE_LEN; i++) if (p->scene.tables.sequence[i] == a->level) { pos = i; break; }
    game_new_game(&p->game);
    if (pos >= 0) p->game.level_number = pos;
    if (a->has_start) { p->has_start = true; p->start_level = a->level; p->start_x = a->start_x; p->start_y = a->start_y; }
    if (!play_start_level(p, pos, a->level, e)) { play_close(p); return false; }
    if (a->near_type > 0 && !a->has_start) {                        /* --near: find the marker, then restart there */
        int seen = 0, mx = -1, my = -1;
        for (int cx = 0; cx < p->game.world.w && mx < 0; cx++)       /* left to right, as the level is played */
            for (int cy = 0; cy < p->game.world.h; cy++) {
                int param = 0;
                if (world_marker_type(&p->game.world, cx, cy, &param) == a->near_type && ++seen == a->near_n) { mx = cx; my = cy; break; }
            }
        if (mx < 0) { sk_fail(e, "--near: level %d has %d marker(s) of type %d", a->level, seen, a->near_type); play_close(p); return false; }
        p->has_start = true;
        p->start_level = a->level;
        p->start_x = mx * 16 - 128 < 16 ? 16 : mx * 16 - 128;
        p->start_y = my * 16 - 24;
        if (!play_start_level(p, pos, a->level, e)) { play_close(p); return false; }
    }
    return true;
}

static uint8_t script_input(sk_play *p) {
    if (!p->script || p->script_done) return 0;
    if (p->script_left == 0) {
        sk_err e = {0};
        if (!*p->script_cursor || !script_parse_step(&p->script_cursor, &p->script_input, &p->script_left, &e)) {
            p->script_done = true;
            return 0;
        }
    }
    p->script_left--;
    return p->script_input;
}

static void sound_step(sk_play *p) {                                /* the VBL's sound part, then the device */
    sound_frame(&p->snd, p->audio_buf, 960);
    if (p->audio) audio_out_push(p->audio, p->audio_buf, 960);
}

void play_step(sk_play *p, uint8_t keyboard_input) {
    if (p->load_failed) return;
    p->game.snd = &p->snd;
    if (p->fade_wait > 0) {                                         /* fade_music_out blocks the game for 64 frames */
        sound_step(p);
        if (--p->fade_wait > 0) return;
        sk_err e = {0};
        int next = play_level_next(&p->scene.tables, p->level_pos);
        if (next < 0) next = play_level_next(&p->scene.tables, -1);       /* the end: start over (ruling) */
        if (next >= 0) p->game.level_number = next;
        if (!play_start_level(p, next, p->scene.level_index, &e)) { p->load_failed = true; p->load_err = e; }
        return;
    }
    uint8_t in = p->script ? script_input(p) : keyboard_input;
    p->game.ball_sprite = &p->sprites.spr[1];
    game_frame_step(&p->game, in, &p->sprites.spr[0]);
    uint8_t lf = p->game.layer_flags;                            /* select_layers: bit 0 the kid's pair, bit 1 the ball's in front of playfield 1 */
    p->sprites.spr[0].in_front = (lf & 1) != 0;
    p->sprites.spr[1].in_front = (lf & 2) != 0;
    p->sprites.front = lf == 1 ? 0 : 1;                          /* block 0 holds the ball unless layer_flags is 1 */
    sk_err e = {0};
    switch (game_outcome(&p->game)) {
    case SK_OUTCOME_NEXT_LEVEL:
        sound_fade_out(&p->snd);
        p->fade_wait = 64;
        break;
    case SK_OUTCOME_GAME_OVER:
        if (p->game_over_frames == 0) sound_fade_out(&p->snd);       /* fade_music_out 0x1040F2: every level exit */
        if (++p->game_over_frames >= GAME_OVER_FRAMES) {
            game_new_game(&p->game);
            if (p->level_pos >= 0) p->game.level_number = p->level_pos;
            if (!play_start_level(p, p->level_pos, p->scene.level_index, &e)) { p->load_failed = true; p->load_err = e; }
        }
        break;
    default: break;
    }
    sound_step(p);
}

void play_draw(sk_play *p) {
    sk_fb *fb = &p->scene.fb;
    const sk_level *lv = &p->scene.level;
    fb_clear(fb, 0);
    uint32_t sprite_colours[16];                                  /* the copper keeps colours 48..63 between writes: kits 4..7 skip 49 */
    memcpy(sprite_colours, &p->scene.palette.xrgb[48], sizeof sprite_colours);
    palette_from_tilesets(&p->scene.palette, &lv->tiles_a, &lv->tiles_b);   /* the playfield colours, restored after a flash */
    memcpy(&p->scene.palette.xrgb[48], sprite_colours, sizeof sprite_colours);
    if (p->game.sk) kit_palette(&p->scene.palette, p->game.sk, p->game.kit_current, p->game.kit_extra);   /* the shade attributes */
    if (p->game.flash_timer && p->game.flash_timer < 16) {       /* sub_104C78: the fade table step (toward white, 0x200ED6) */
        for (int k = 1; k < 16; k++)
            p->scene.palette.xrgb[k] = rgb12_to_xrgb(fade_colour(lv->tiles_a.colours[k], 0xFFF, p->game.flash_timer));
        for (int k = 0; k < 15 && p->game.sk; k++)
            p->scene.palette.xrgb[49 + k] = rgb12_to_xrgb(fade_colour(image_u16(p->game.sk, SK_KIT_BASE_ADDR + 2u * (uint32_t)k), 0xFFF, p->game.flash_timer));
    }
    int bg_x, bg_y;
    scene_backdrop_pos(lv, p->game.camera_x, p->game.camera_y, &bg_x, &bg_y);
    p->game.view.bg_x = bg_x;
    p->game.view.bg_y = bg_y;
    if (p->scene.backdrop) scene_draw_backdrop_cells(lv, p->game.world.bg, fb, bg_x, bg_y);
    scene_draw_tiles_anim(lv, fb, p->game.world.tile, p->game.world.state, p->game.tile_anim, p->game.camera_x, p->game.camera_y);
    draw_all_objects(&p->game, fb, p->game.camera_x + SK_BUFFER_WORLD_X, p->game.camera_y);
    const sk_game *g = &p->game;
    if (g->msg_visible && g->msg_w > 0) {                        /* the message box: centred on the view (show_message_box) */
        int plane = g->msg_w / 8 * g->msg_h;
        sk_frame fr = { g->msg_w, g->msg_h, g->msg_w / 8, { g->msg_bits, g->msg_bits + plane, g->msg_bits + 2 * plane, g->msg_bits + 3 * plane }, g->msg_bits + 4 * plane };
        fb_blit_bob(fb, &fr, 160 - g->msg_w / 2 - SK_BUFFER_WORLD_X, 100 - g->msg_h / 2, SK_GROUP_TILES_A);
    }
}

void play_present(sk_play *p, uint32_t *out) {
    sk_sky sky;
    scene_sky(&p->scene.level, p->game.view.bg_y, &sky);
    present_screen(&p->scene.fb, &p->scene.palette, &sky, &p->sprites, curtain_rows(&p->game), &p->game.hud, out);
}

void play_close(sk_play *p) {
    sound_free(&p->snd);
    resources_free(&p->res);
    game_close(&p->game);
    scene_close(&p->scene);
}
