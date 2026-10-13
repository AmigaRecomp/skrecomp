#include <stdlib.h>
#include <string.h>
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
    if (!game_setup_level(&p->game, &p->scene.level, data_index)) return sk_fail(e, "out of memory");
    if (p->game.sk) kit_palette(&p->scene.palette, p->game.sk, p->game.kit);
    p->level_pos = pos;
    p->game_over_frames = 0;
    return true;
}

bool play_open(sk_play *p, const sk_args *a, sk_err *e) {
    memset(p, 0, sizeof *p);
    if (!scene_open(&p->scene, a, e)) return false;
    p->game.sk = &p->scene.sk;
    p->game.tables = &p->scene.tables;
    p->script = a->script;
    p->script_cursor = a->script;
    int pos = -1;
    for (int i = 0; i < SK_LEVEL_SEQUENCE_LEN; i++) if (p->scene.tables.sequence[i] == a->level) { pos = i; break; }
    game_new_game(&p->game);
    if (pos >= 0) p->game.level_number = pos;
    if (!play_start_level(p, pos, a->level, e)) { play_close(p); return false; }
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

void play_step(sk_play *p, uint8_t keyboard_input) {
    uint8_t in = p->script ? script_input(p) : keyboard_input;
    game_frame_step(&p->game, in, &p->sprites.spr[0]);
    p->sprites.spr[0].in_front = (p->game.layer_flags & 1) != 0;   /* select_layers: bit 0 puts the kid's pair in front of playfield 1 */
    sk_err e = {0};
    switch (game_outcome(&p->game)) {
    case SK_OUTCOME_NEXT_LEVEL: {
        int next = play_level_next(&p->scene.tables, p->level_pos);
        if (next < 0) next = play_level_next(&p->scene.tables, -1);       /* the end: start over (ruling) */
        if (next >= 0) p->game.level_number = next;
        play_start_level(p, next, p->scene.level_index, &e);
        break;
    }
    case SK_OUTCOME_GAME_OVER:
        if (++p->game_over_frames >= GAME_OVER_FRAMES) {
            game_new_game(&p->game);
            if (p->level_pos >= 0) p->game.level_number = p->level_pos;
            play_start_level(p, p->level_pos, p->scene.level_index, &e);
        }
        break;
    default: break;
    }
}

void play_draw(sk_play *p) {
    sk_fb *fb = &p->scene.fb;
    const sk_level *lv = &p->scene.level;
    fb_clear(fb, 0);
    int bg_x, bg_y;
    scene_backdrop_pos(lv, p->game.camera_x, p->game.camera_y, &bg_x, &bg_y);
    p->game.view.bg_x = bg_x;
    p->game.view.bg_y = bg_y;
    if (p->scene.backdrop) scene_draw_backdrop(lv, fb, bg_x, bg_y);
    scene_draw_tiles(lv, fb, p->game.world.tile, p->game.camera_x, p->game.camera_y);
    draw_all_objects(&p->game, fb, p->game.camera_x, p->game.camera_y);
}

void play_present(sk_play *p, uint32_t *out) {
    sk_sky sky;
    scene_sky(&p->scene.level, p->game.view.bg_y, &sky);
    present_screen(&p->scene.fb, &p->scene.palette, &sky, &p->sprites, curtain_rows(&p->game), out);
}

void play_close(sk_play *p) {
    game_close(&p->game);
    scene_close(&p->scene);
}
