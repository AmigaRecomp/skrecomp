#include <stdio.h>
#include "app/audio_out.h"
#include <stdlib.h>
#include <string.h>
#include "game/player.h"
#include "game/world.h"
#include "app/play.h"
#include "game/scroll_modes.h"
#include "game/camera.h"
#include "game/level_flow.h"
#include "game/objects.h"
#include "game/savegame.h"
#include "game/attract.h"
#include "game/front.h"
#include "game/scoreboard.h"
#include "gfx/display_model.h"

static void play_advance(sk_play *p);
static void save_rows(sk_play *p, char rows[5][24]);
static void session_start(sk_play *p);

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
    p->cup_arg = a->cup;
    if (a->save_path) snprintf(p->save_path, sizeof p->save_path, "%s", a->save_path);
    if (a->cup >= 0) p->game.cup_pieces = (uint8_t)a->cup;
    if (a->level < 0 && !a->new_game) {                             /* the frontend: the language screen first */
        p->with_frontend = true;
        if (!a->no_intro) {                                         /* s/startup-sequence: anim first */
            p->intro = calloc(1, sizeof *p->intro);
            sk_err ie = {0};
            if (p->intro && intro_open(p->intro, &p->scene.disks, &p->snd, &ie)) return true;
            fprintf(stderr, "soccerkid: no intro: %s\n", p->intro ? ie.msg : "out of memory");
            free(p->intro); p->intro = NULL;
        }
        if (!frontend_language(&p->fe, &p->scene.sk, &p->scene.disks, e)) { play_close(p); return false; }
        return true;
    }
    if (a->level < 0) { play_advance(p); if (p->load_failed) { *e = p->load_err; play_close(p); return false; } return true; }   /* the whole game */
    if (pos >= 0) p->game.level_number = pos;
    if (a->level >= 36 && a->level <= 40) {                         /* a bonus level started directly: as bonus_check would */
        static const int after[5] = { 10, 20, 29, 38, 48 };        /* the position each world's last scoreboard leads to */
        p->game.timeout_armed = true; p->game.timer_rate = 0x32; p->game.fulltime_level = after[a->level - 36];
    }
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

static void fail(sk_play *p, const sk_err *e) { p->load_failed = true; p->load_err = *e; }

/* play_level (0x104060): what level_number points at, behind the loading strip */
static void play_advance(sk_play *p) {
    sk_err e = {0};
    bonus_check(&p->game);
    sk_run r = play_level_entry(&p->scene.tables, &p->game.level_number);
    if (r.kind == SK_RUN_GAME_END) {                                /* 42: play_level returns to the outer loop */
        if (p->with_frontend) { session_start(p); return; }
        game_new_game(&p->game);                                    /* --new-game: round again (ruling) */
        r = play_level_entry(&p->scene.tables, &p->game.level_number);
    }
    p->next = r;
    if (r.kind == SK_RUN_MAP && p->with_frontend && p->game.level_number >= 4 && p->game.level_number < 0x30) {   /* save_game_screen */
        char rows[5][24];
        save_rows(p, rows);
        if (savescreen_open(&p->save, SK_SAVE_ASK, &p->scene.sk, p->fe.language, NULL, 0, rows, &e)) return;
        fprintf(stderr, "soccerkid: %s\n", e.msg);
    }
    if (!pictures_loading(&p->pic, &p->scene.sk, &e)) fail(p, &e);
}

static void start_next(sk_play *p) {                                /* after loading_screen's 25 frames */
    sk_err e = {0};
    switch (p->next.kind) {
    case SK_RUN_MAP:                                                /* world_map: free_level_data, no music */
        sound_stop(&p->snd);
        if (!pictures_map(&p->pic, &p->scene.disks, p->next.map_index, p->game.cup_pieces, &e)) fail(p, &e);
        break;
    case SK_RUN_ENDING: {                                           /* end_screens 0x11EFAC */
        bool good = p->game.cup_pieces == 0x1F;
        sound_stop(&p->snd);
        if (!pictures_ending(&p->pic, &p->scene.disks, good, &e)) { fail(p, &e); break; }
        uint8_t *mod = NULL; size_t n = 0;                          /* disk3 file090 "skend", four channels, position 1 / 0 */
        if (disks_read_file_no(&p->scene.disks, 3, 90, true, &mod, &n, &e)) {
            sound_set_module(&p->snd, 0, mod, n);
            p->snd.music_use4 = true;
            sound_music(&p->snd, SK_MUSIC_LEVEL, good ? 1 : 0);
        } else fprintf(stderr, "soccerkid: no ending music: %s\n", e.msg);
        break;
    }
    default:
        pictures_free(&p->pic);
        if (!play_start_level(p, p->game.level_number, 0, &e)) fail(p, &e);
        break;
    }
}

/* ---- the frontend: level_session (0x104E9E) and the outer loop (0x103FC0) ---- */
static void title_music(sk_play *p, bool on) {                      /* frontend_load: file027, tune 0, four channels; frontend_teardown */
    sound_stop(&p->snd);
    sound_free_level_sfx(&p->snd);                                  /* port: channel 4 lets go before a module is freed */
    sound_set_module(&p->snd, 0, NULL, 0);
    p->snd.music_use4 = false;
    if (!on) return;
    sk_err e = {0};
    uint8_t *mod = NULL; size_t n = 0;
    if (!disks_read_file_no(&p->scene.disks, 1, 27, true, &mod, &n, &e)) { fprintf(stderr, "soccerkid: no title music: %s\n", e.msg); return; }
    sound_set_module(&p->snd, 0, mod, n);
    p->snd.music_use4 = true;
    sound_music(&p->snd, SK_MUSIC_LEVEL, 0);
}

static void attract_start(sk_play *p, sk_attract kind);

static void session_menu(sk_play *p) {                              /* loop_b: title_menu */
    sk_err e = {0};
    p->attract = SK_ATTRACT_NONE;
    p->game.game_flags = 0xFF;
    p->game.hud.shift = 0;
    if (!frontend_menu_from(&p->fe, &p->scene.sk, p->fe.title, SK_TITLE_SIZE, &e)) fail(p, &e);
}

static void session_start(sk_play *p) {                             /* level_session */
    sk_err e = {0};
    pictures_free(&p->pic);
    p->fade_wait = 0; p->game_over_frames = 0;
    p->game.game_flags = 0xFF;
    title_music(p, true);
    p->fe.music_off = &p->snd.music_off; p->fe.sfx_muted = &p->snd.sfx_muted;
    if (!frontend_menu(&p->fe, &p->scene.sk, &p->scene.disks, &e)) { fail(p, &e); return; }
    p->session_round = 1;                                           /* loop_a */
    int rank = p->game.player.score ? hiscore_insert(&p->scene.sk, p->game.player.score, p->fe.shirt, p->fe.shorts) : -1;
    if (rank >= 0) { p->name.rank = rank; attract_start(p, SK_ATTRACT_NAME); }   /* attract_hiscores */
}

/* ---- the frontend's levels: the hi-score pan and the name entry (file077), the demo (level 0), the tutorial (file076) ---- */
static bool attract_load(sk_play *p, int data_index, int disk, int file) {
    sk_err e = {0};
    bool ok = data_index >= 0 ? scene_load_level(&p->scene, data_index, &e) : scene_load_level_file(&p->scene, disk, file, 1000 + file, &e);
    resources_free(&p->res);
    if (ok && data_index >= 0) ok = resources_load(&p->res, &p->scene.disks, &p->scene.tables, data_index, &e);   /* ruling: the record's sets */
    p->game.res = &p->res;
    if (ok && !game_setup_level(&p->game, &p->scene.level, data_index >= 0 ? data_index : 0)) ok = sk_fail(&e, "out of memory");
    if (!ok) { fail(p, &e); return false; }
    if (p->game.sk) kit_palette(&p->scene.palette, p->game.sk, p->game.kit, p->game.kit);
    memset(&p->sprites, 0, sizeof p->sprites);
    p->level_pos = -1;
    return true;
}

static void attract_start(sk_play *p, sk_attract kind) {
    p->fe.mode = SK_FE_OFF;
    p->attract = kind;
    p->attract_frames = 0;
    sk_game *g = &p->game;
    if (kind == SK_ATTRACT_PAN || kind == SK_ATTRACT_NAME) {        /* demo_level_load(0 / 0x12) */
        int d0 = kind == SK_ATTRACT_PAN ? 0 : 0x12;
        g->game_flags = 0xFF;
        if (!attract_load(p, -1, 1, 77)) return;
        g->world.start_x = (d0 + 6) * 16; g->world.start_y = 0;
        init_player(g);
        g->camera_x = g->world.start_x - 94; g->camera_y = 0;       /* update_camera on the static kid */
        hiscore_tiles(g, 2 + d0, -1);
        memset(g->hud.bits, 0, sizeof g->hud.bits);                 /* hud_clear_bitmap */
        g->curtain_flags = 0; g->curtain_top = g->curtain_bottom = 0;   /* DIW 0x2D91 / 0x06B1: the window open, no curtain */
        p->attract_x = 0x20; p->attract_dx = 1;
        if (kind == SK_ATTRACT_NAME) {                              /* hud_palette_attract, BPLCON1 0x88 */
            int rank = p->name.rank;
            name_entry_start(&p->name, rank);
            for (int i = 0; i < 15 && g->sk; i++) g->hud.palette12[1 + i] = image_u16(g->sk, 0x200E9Au + 2u * (uint32_t)i);
            g->hud.shift = 8;
            g->frame_ticks = 0;
        }
        return;
    }
    g->game_flags = 0xFE;                                           /* run_demo_level / run_demo_level_b */
    g->demo_fire_released = false;
    g->player.score = 0; g->cards = 0;                              /* 0x10511C / 0x104FB2: score, reset_card_count */
    p->saved_muted = p->snd.sfx_muted;
    p->snd.sfx_muted = true;
    if (kind == SK_ATTRACT_DEMO) { if (!attract_load(p, 0, 0, 0)) return; spawn_object(g, 0, 0, 102); }
    else { if (!attract_load(p, -1, 1, 76)) return; spawn_object(g, 0, 0, 86); }
}

static void attract_end(sk_play *p, bool fired) {                  /* back into level_session's loop */
    sk_attract kind = p->attract;
    if (kind == SK_ATTRACT_DEMO || kind == SK_ATTRACT_TUTORIAL) p->snd.sfx_muted = p->saved_muted;
    if (kind == SK_ATTRACT_PAN && !fired && --p->session_round < 0) { attract_start(p, SK_ATTRACT_DEMO); return; }
    if (kind != SK_ATTRACT_PAN || fired) p->session_round = 1;
    session_menu(p);
}

static void attract_frame(sk_play *p, uint8_t in) {
    sk_game *g = &p->game;
    if (p->attract == SK_ATTRACT_DEMO || p->attract == SK_ATTRACT_TUTORIAL) {
        p->game.ball_sprite = &p->sprites.spr[1];
        game_frame_step(g, in, &p->sprites.spr[0]);
        uint8_t lf = g->layer_flags;
        p->sprites.spr[0].in_front = (lf & 1) != 0; p->sprites.spr[1].in_front = (lf & 2) != 0;
        p->sprites.front = lf == 1 ? 0 : 1;
        if (game_outcome(g) != SK_OUTCOME_RUNNING) attract_end(p, false);
        return;
    }
    g->frame_ticks++;                                               /* attract_loop / the name entry loop */
    p->attract_x += p->attract_dx;
    int bg_max = p->scene.level.backdrop.width * 16 - 0x160 + 0x20;     /* bg_scroll_max_x */
    if ((unsigned)p->attract_x >= (unsigned)bg_max) p->attract_dx = -p->attract_dx;
    if (p->attract_x < 0x10) p->attract_dx = -p->attract_dx;
    if (p->attract == SK_ATTRACT_PAN) {
        if (in & SK_IN_FIRE) { attract_end(p, true); return; }
        if (++p->attract_frames >= 0x1F4) attract_end(p, false);   /* 500 frames */
        return;
    }
    if (!(g->frame_ticks & 0x1F)) p->name.blink ^= 0xFF;
    if (name_entry_step(g, &p->name, in) < 0) { g->hud.shift = 0; attract_end(p, true); return; }
    hiscore_tiles(g, 20, p->name.rank);
    name_entry_hud(g, &p->name);
}

/* ---- the SaveGame file: save_game_slot 0x1034B0, load_game_slot 0x103566 ---- */
static size_t read_save(const sk_play *p, uint8_t file[SK_SAVE_SIZE]) {
    memset(file, 0, SK_SAVE_SIZE);
    FILE *f = p->save_path[0] ? fopen(p->save_path, "rb") : NULL;
    if (!f) return 0;
    size_t n = fread(file, 1, SK_SAVE_SIZE, f);
    fclose(f);
    return n;
}

static void save_rows(sk_play *p, char rows[5][24]) {            /* load_save_names */
    uint8_t file[SK_SAVE_SIZE];
    size_t n = read_save(p, file);
    savegame_names(file, n, rows);
}

static void write_save(sk_play *p, int slot, const char *name) {
    uint8_t file[SK_SAVE_SIZE];
    if (read_save(p, file) < SK_SAVE_SIZE) memset(file, 0, sizeof file);
    sk_save_slot s = { "", p->game.player.score, p->game.level_number, p->game.player.lives, p->game.cup_pieces,
                       (uint8_t)p->fe.shirt, (uint8_t)p->fe.shorts, p->game.extra_heart };
    snprintf(s.name, sizeof s.name, "%s", name);
    savegame_put(file, slot, &s);
    FILE *f = p->save_path[0] ? fopen(p->save_path, "wb") : NULL;
    if (!f || fwrite(file, 1, sizeof file, f) != sizeof file) fprintf(stderr, "soccerkid: could not write %s\n", p->save_path);
    if (f) fclose(f);
}

static void start_game(sk_play *p);
static void save_frame(sk_play *p, uint8_t in) {
    sk_err e = {0};
    sk_save_result r = savescreen_step(&p->save, in, p->typed_key, p->esc_key);
    p->typed_key = 0; p->esc_key = false;
    if (r == SK_SAVE_RESULT_STAY) return;
    bool resume = p->save.resume;
    int slot = p->save.slot;
    p->save.mode = SK_SAVE_OFF;
    if (r == SK_SAVE_RESULT_SAVE) write_save(p, slot, p->save.name);
    if (!resume) { if (!pictures_loading(&p->pic, &p->scene.sk, &e)) fail(p, &e); return; }   /* on to the map */
    uint8_t file[SK_SAVE_SIZE];
    size_t n = read_save(p, file);
    sk_save_slot s;
    if (r != SK_SAVE_RESULT_LOAD || !savegame_get(file, n, slot, &s)) { session_start(p); return; }   /* back to the session */
    p->fe.shirt = s.shirt % 14; p->fe.shorts = s.shorts % 14;
    p->resume_slot = s; p->resuming = true;
    start_game(p);
}

static void start_game(sk_play *p) {                                /* START GAME: frontend_teardown, then the outer loop */
    kit_apply(&p->scene.sk, p->fe.shirt, p->fe.shorts);              /* 0x11CCFE: the kit reaches the game */
    p->fe.mode = SK_FE_OFF;
    title_music(p, false);
    p->game.game_flags = 0;
    play_sfx(&p->game, 0);
    game_new_game(&p->game);                                        /* 0x103FC0..0x104022 */
    if (p->cup_arg >= 0) p->game.cup_pieces = (uint8_t)p->cup_arg;
    if (p->resuming) {                                              /* load_game_slot over the new game */
        p->resuming = false;
        p->game.player.score = p->resume_slot.score; p->game.level_number = p->resume_slot.level_number;
        p->game.player.lives = (int8_t)p->resume_slot.lives; p->game.cup_pieces = p->resume_slot.cup_pieces;
        p->game.extra_heart = p->resume_slot.extra_heart;
    }
    play_advance(p);
}

static void frontend_frame(sk_play *p, uint8_t in) {
    sk_err e = {0};
    sk_fe_result r = frontend_step(&p->fe, in, p->fe_keys);
    if (p->fe.music_changed) {                                      /* OPTIONS: music_stop when off, then music_request(module, 0) */
        p->fe.music_changed = false;
        if (p->snd.music_off) sound_stop(&p->snd);
        sound_music(&p->snd, SK_MUSIC_LEVEL, 0);
    }
    switch (r) {
    case SK_FE_LANGUAGE_CHOSEN: {                                   /* the language's message texts, then the session */
        uint8_t *bin = NULL; size_t n = 0;
        if (disks_read(&p->scene.disks, 0, language_file(p->fe.language), &bin, &n, &e)) language_apply(&p->scene.sk, bin, n);
        else fprintf(stderr, "soccerkid: %s\n", e.msg);
        free(bin);
        session_start(p);
        break;
    }
    case SK_FE_START: start_game(p); break;
    case SK_FE_QUIT: p->quit = true; break;
    case SK_FE_IDLE: attract_start(p, SK_ATTRACT_PAN); break;       /* attract_loop */
    case SK_FE_TUTORIAL: attract_start(p, SK_ATTRACT_TUTORIAL); break;
    case SK_FE_RESUME: {                                            /* the session ends; the outer loop's resume_game_screen */
        game_new_game(&p->game);                                    /* 0x103FD4: the new-game clears come first */
        char rows[5][24];
        save_rows(p, rows);
        p->fe.mode = SK_FE_OFF;
        title_music(p, false);
        p->game.game_flags = 0;
        if (!savescreen_open(&p->save, SK_SAVE_RESUME, &p->scene.sk, p->fe.language, NULL, 0, rows, &e)) fail(p, &e);
        break;
    }
    default: break;
    }
}

static void picture_step(sk_play *p, uint8_t in) {
    sk_pic_kind kind = p->pic.kind;
    bool was_fading = p->pic.fading;
    bool done = pictures_step(&p->pic, in);
    if (p->pic.fading && !was_fading) sound_fade_out(&p->snd);     /* the ending: fade_music_out */
    if (!done) return;
    if (kind == SK_PIC_LOADING) { start_next(p); return; }
    if (kind == SK_PIC_ENDING) {                                    /* music_stop, 0x200314 = 0, the module freed */
        sound_stop(&p->snd);
        sound_free_level_sfx(&p->snd);                              /* port: channel 4 lets go of the module before it is freed */
        p->snd.music_use4 = false;
        sound_set_module(&p->snd, 0, NULL, 0);
    }
    play_advance(p);
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
    if (!p->typed_key) p->typed_key = play_next_typed(p);           /* one queued key a frame */
    bool esc = p->esc_key && p->save.mode == SK_SAVE_OFF, pause = p->pause_key;
    p->pause_key = false;
    if (esc) {
        p->esc_key = false;
        if (p->intro || p->fe.mode != SK_FE_OFF || p->attract != SK_ATTRACT_NONE) { p->quit = true; return; }   /* ruling: ESC leaves from the frontend */
        if (p->pic.kind == SK_PIC_OFF && !p->fade_wait) {           /* frame step 32: the curtain closes */
            if (p->with_frontend) game_request_escape(&p->game);
            else { p->quit = true; return; }
        }
    }
    if (pause && p->pic.kind == SK_PIC_OFF && p->fe.mode == SK_FE_OFF && p->attract == SK_ATTRACT_NONE && !p->intro && !p->fade_wait &&
        p->save.mode == SK_SAVE_OFF && game_outcome(&p->game) == SK_OUTCOME_RUNNING)
        game_request_pause(&p->game);
    if (p->intro) {                                                 /* the intro: any key or fire ends it at its polling points */
        uint8_t in = p->script ? script_input(p) : keyboard_input;
        intro_step(p->intro, in != 0 || p->typed_key != 0 || p->fe_keys != 0);
        p->typed_key = 0;
        if (p->intro->done) {                                       /* exit_intro: music_stop, then soccerkid's language screen */
            title_music(p, false);
            p->snd.pt.master_on = false;
            intro_free(p->intro); free(p->intro); p->intro = NULL;
            p->typed_n = 0;                                         /* the taps that skipped it are spent */
            sk_err e = {0};
            if (!frontend_language(&p->fe, &p->scene.sk, &p->scene.disks, &e)) fail(p, &e);
            p->fe.prev_input = 0xFF;
        }
        sound_step(p);
        return;
    }
    if (p->fe.mode != SK_FE_OFF) {
        frontend_frame(p, p->script ? script_input(p) : keyboard_input);
        sound_step(p);
        return;
    }
    if (p->save.mode != SK_SAVE_OFF) {
        save_frame(p, p->script ? script_input(p) : keyboard_input);
        sound_step(p);
        return;
    }
    if (p->attract != SK_ATTRACT_NONE) {
        attract_frame(p, p->script ? script_input(p) : keyboard_input);
        sound_step(p);
        return;
    }
    if (p->pic.kind != SK_PIC_OFF) {
        picture_step(p, p->script ? script_input(p) : keyboard_input);
        sound_step(p);
        return;
    }
    if (p->fade_wait > 0) {                                         /* fade_music_out blocks the game for 64 frames */
        sound_step(p);
        if (--p->fade_wait > 0) return;
        if (p->leaving) { p->leaving = false; session_start(p); return; }   /* play_level returned: the outer loop, the title */
        play_advance(p);                                            /* play_level loops: level_number moved on by the exit */
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
        level_end_stats(&p->game);                                  /* the frame loop's exit (0x104558) */
        sound_fade_out(&p->snd);
        p->fade_wait = 64;
        break;
    case SK_OUTCOME_LEFT:                                           /* ESC: fade_music_out, then the title */
        sound_fade_out(&p->snd);
        p->fade_wait = 64; p->leaving = true;
        break;
    case SK_OUTCOME_GAME_OVER:
        if (p->game_over_frames == 0) sound_fade_out(&p->snd);       /* fade_music_out 0x1040F2 (the statistics go: a new game follows) */
        if (++p->game_over_frames >= GAME_OVER_FRAMES && p->with_frontend) { session_start(p); break; }   /* back to the title */
        if (p->game_over_frames >= GAME_OVER_FRAMES) {
            int idx = p->scene.level_index, back = p->game.fulltime_level;
            game_new_game(&p->game);
            if (idx >= 36 && idx <= 40) {                           /* a bonus level is not replayed untimed: on from its board */
                if (p->cup_arg >= 0) p->game.cup_pieces = (uint8_t)p->cup_arg;
                p->game.level_number = back;
                p->game_over_frames = 0;
                play_advance(p);
                break;
            }
            if (p->cup_arg >= 0) p->game.cup_pieces = (uint8_t)p->cup_arg;   /* --cup holds through a game over */
            if (p->level_pos >= 0) p->game.level_number = p->level_pos;
            if (!play_start_level(p, p->level_pos, p->scene.level_index, &e)) { p->load_failed = true; p->load_err = e; }
        }
        break;
    default: break;
    }
    sound_step(p);
}

/* The scoreboard's dots live in playfield 1's plane 0 (scoreboard_text); the frame columns it places are drawn over them. */
static void draw_board(sk_play *p) {
    const sk_game *g = &p->game;
    sk_fb *fb = &p->scene.fb;
    for (int y = 0; y < SK_VIEW_H && y < fb->h; y++)
        for (int x = 0; x < SK_VIEW_W; x++) {
            uint8_t bit = (uint8_t)(0x80 >> (x & 7));
            if (!(g->board.text_set[y][x >> 3] & bit)) continue;
            uint8_t *px = &fb->px[y * fb->w + SK_VIEW_X + x];
            *px = (uint8_t)((*px & ~1) | ((g->board.text[y][x >> 3] & bit) ? 1 : 0));
        }
    const sk_level *lv = &p->scene.level;
    for (int k = 0; k < 2; k++)
        for (int r = 0; r < 7; r++) {
            int cx = g->board.border_x[k] >> 4, cy = (g->board.border_y >> 4) + r;
            if (!world_inside(&g->world, cx, cy)) continue;
            int i = cy * g->world.w + cx, idx = g->world.tile[i] & SK_INDEX_MASK, a = idx + (g->tile_anim[g->world.state[i]] >> 7);
            fb_blit_tile(fb, &lv->tiles_a, a < lv->tiles_a.count ? a : idx, cx * 16 - g->camera_x - SK_BUFFER_WORLD_X, cy * 16 - g->camera_y, SK_GROUP_TILES_A);
        }
}

void play_draw(sk_play *p) {
    if (p->pic.kind != SK_PIC_OFF || p->fe.mode != SK_FE_OFF || p->save.mode != SK_SAVE_OFF || p->intro) return;   /* pictures_present / frontend_present draw the mode */
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
            p->scene.palette.xrgb[k] = rgb12_to_xrgb(fade_colour(p->game.flash_table ? train_scale_colour(lv->tiles_a.colours[k], 4) : lv->tiles_a.colours[k], 0xFFF, p->game.flash_timer));
        for (int k = 0; k < 15 && p->game.sk; k++)
            p->scene.palette.xrgb[49 + k] = rgb12_to_xrgb(fade_colour(image_u16(p->game.sk, SK_KIT_BASE_ADDR + 2u * (uint32_t)k), 0xFFF, p->game.flash_timer));
    }
    if (p->game.train_level < 15 && !p->game.flash_timer) {                              /* apply_fade_step: TRAIN's tunnel, playfield and sprite colours */
        for (int k = 1; k < 16; k++)
            p->scene.palette.xrgb[k] = rgb12_to_xrgb(train_scale_colour(lv->tiles_a.colours[k], p->game.train_level));
        for (int k = 0; k < 15 && p->game.sk; k++)
            p->scene.palette.xrgb[49 + k] = rgb12_to_xrgb(train_scale_colour(image_u16(p->game.sk, SK_KIT_BASE_ADDR + 2u * (uint32_t)k), p->game.train_level));
    }
    int bg_x, bg_y;
    scene_backdrop_pos(lv, p->game.camera_x, p->game.camera_y, &bg_x, &bg_y);
    if (p->game.scroll_mode) bg_x = p->game.view.bg_x;          /* the backdrop streams by itself (backdrop_stream_step) */
    if (p->attract == SK_ATTRACT_PAN || p->attract == SK_ATTRACT_NAME) { bg_x = p->attract_x; bg_y = 0x28; }   /* backdrop_scroll 0x106662 */
    p->game.view.bg_x = bg_x;
    p->game.view.bg_y = bg_y;
    if (p->scene.backdrop) {
        if (p->game.scroll_mode) scene_draw_backdrop_stream(lv, p->game.world.bg, p->game.stream.col, fb, bg_x, bg_y);
        else scene_draw_backdrop_cells(lv, p->game.world.bg, fb, bg_x, bg_y);
    }
    scene_draw_tiles_anim(lv, fb, p->game.world.tile, p->game.world.state, p->game.tile_anim, p->game.camera_x, p->game.camera_y);
    if (p->game.board.active) draw_board(p);
    draw_all_objects(&p->game, fb, p->game.camera_x + SK_BUFFER_WORLD_X, p->game.camera_y);
    const sk_game *g = &p->game;
    if (g->msg_visible && g->msg_w > 0) {                        /* the message box: centred on the view (show_message_box) */
        int plane = g->msg_w / 8 * g->msg_h;
        sk_frame fr = { g->msg_w, g->msg_h, g->msg_w / 8, { g->msg_bits, g->msg_bits + plane, g->msg_bits + 2 * plane, g->msg_bits + 3 * plane }, g->msg_bits + 4 * plane };
        fb_blit_bob(fb, &fr, 160 - g->msg_w / 2 - SK_BUFFER_WORLD_X, 100 - g->msg_h / 2 + g->msg_dy, SK_GROUP_TILES_A);
    }
}

void play_present(sk_play *p, uint32_t *out) {
    if (p->intro) { intro_present(p->intro, out); return; }
    if (p->save.mode != SK_SAVE_OFF) { savescreen_present(&p->save, out); return; }
    if (p->fe.mode != SK_FE_OFF) { frontend_present(&p->fe, out); return; }
    if (p->pic.kind != SK_PIC_OFF) { pictures_present(&p->pic, out); return; }
    sk_sky sky;
    scene_sky(&p->scene.level, p->game.view.bg_y, &sky);
    present_screen(&p->scene.fb, &p->scene.palette, &sky, &p->sprites, curtain_rows(&p->game), &p->game.hud, out);
}

void play_close(sk_play *p) {
    if (p->intro) { intro_free(p->intro); free(p->intro); p->intro = NULL; }
    pictures_free(&p->pic);
    frontend_free(&p->fe);
    sound_free(&p->snd);
    resources_free(&p->res);
    game_close(&p->game);
    scene_close(&p->scene);
}
void play_type(sk_play *p, int key) { if (p->typed_n < 8) p->typed_queue[p->typed_n++] = key; }

int play_next_typed(sk_play *p) {
    if (!p->typed_n) return 0;
    int k = p->typed_queue[0];
    memmove(p->typed_queue, p->typed_queue + 1, sizeof p->typed_queue[0] * (size_t)--p->typed_n);
    return k;
}
