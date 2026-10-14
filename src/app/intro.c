#include <stdlib.h>
#include <string.h>
#include "app/intro.h"
#include "core/bytekiller.h"
#include "core/bytes.h"
#include "gfx/display_model.h"
#include "gfx/palette.h"

#define FONT 0x20012Cu                  /* 41 glyphs of 16 mask words then 16 data words */
#define CHAR_TO_GLYPH 0x100C8Eu         /* 128 bytes */
#define MODULE 0x200ECCu
#define MODULE_SIZE 99308u
static const uint32_t texts[5] = { 0x200B6C, 0x200C02, 0x200C98, 0x200D45, 0x200E1F };

/* main_sequence (0x100256) as a list of operations; each runs over one or more ticks */
enum { OP_MUSIC, OP_RELOAD, OP_TEXT_ON_BLACK, OP_WAIT_TYPE, OP_CUE_PENDING, OP_FADE_BLACK, OP_FADE_IN, OP_HOLD, OP_PENDING,
       OP_PLAY, OP_MUSIC_FADE, OP_CUE, OP_TEXT_OVER_DIM, OP_WAIT, OP_STOP, OP_END };
static const struct { uint8_t op; int16_t a, b, c; } script[] = {
    { OP_MUSIC, 1, 0, 0 }, { OP_RELOAD, 8, 0, 0 }, { OP_TEXT_ON_BLACK, 0, 0, 0 }, { OP_WAIT_TYPE, 0, 0, 0 },     /* 0: text 1 */
    { OP_RELOAD, 3, 0, 0 }, { OP_CUE_PENDING, 3, 0, 0 }, { OP_FADE_BLACK, 0, 0, 0 }, { OP_FADE_IN, 0, 0, 0 },
    { OP_PLAY, 0, 0, 4 },                                                                                         /* 1 */
    { OP_PLAY, 0, 1, 1 },                                                                                         /* 2 */
    { OP_MUSIC_FADE, 0, 0, 0 }, { OP_CUE, 4, 0, 0 }, { OP_FADE_BLACK, 0, 0, 0 }, { OP_FADE_IN, 0, 0, 0 },          /* 3 */
    { OP_HOLD, 100, 0, 0 }, { OP_PENDING, 1, 0, 0 }, { OP_PLAY, 0, 2, 4 }, { OP_HOLD, 0, 0, 0 },                  /* 4: text 2 */
    { OP_TEXT_OVER_DIM, 2, 0, 0 },                                                                                 /* 5: text 3 */
    { OP_PLAY, 1, 0, 1 },                                                                                         /* 6 */
    { OP_MUSIC_FADE, 0, 0, 0 }, { OP_CUE, 5, 0, 0 },                                                              /* 7 */
    { OP_TEXT_ON_BLACK, 3, 0, 0 }, { OP_WAIT_TYPE, 0, 0, 0 }, { OP_WAIT, 100, 0, 0 }, { OP_FADE_BLACK, 0, 0, 0 },
    { OP_FADE_IN, 0, 0, 0 },                                                                                      /* 8: text 4 */
    { OP_PLAY, 1, 1, 2 },                                                                                         /* 9 */
    { OP_PLAY, 1, 2, 1 },                                                                                         /* 10 */
    { OP_FADE_BLACK, 0, 0, 0 }, { OP_FADE_IN, 0, 0, 0 },                                                          /* 11 */
    { OP_PLAY, 1, 3, 4 },                                                                                         /* 12 */
    { OP_CUE, 9, 0, 0 }, { OP_TEXT_OVER_DIM, 4, 0, 0 },                                                           /* 13: text 5 */
    { OP_PLAY, 1, 4, 1 },                                                                                         /* 14 */
    { OP_MUSIC_FADE, 0, 0, 0 }, { OP_STOP, 0, 0, 0 }, { OP_END, 0, 0, 0 } };                                      /* 15 */

uint16_t intro_lerp(uint16_t from, uint16_t to, int step) {     /* lerp_colour 0x10122E: c0 + floor((c1 - c0) * step / 16) */
    uint16_t r = 0;
    for (int sh = 0; sh <= 8; sh += 4) {
        int c0 = (from >> sh) & 15, c1 = (to >> sh) & 15;
        r |= (uint16_t)(((c0 + (((c1 - c0) * step) >> 4)) & 15) << sh);
    }
    return r;
}

static void copper_palette(sk_intro *it, const uint16_t *p) { memcpy(it->copper, p, sizeof it->copper); }
static void fade_start(sk_intro *it, const uint16_t *to) { memcpy(it->fade_from, it->pal_current, sizeof it->fade_from); it->fade_to = to; it->fade_step = -1; }
static bool fade_tick(sk_intro *it) {                            /* fade_palette 0x1011CE: 17 ticks, each waits a tick, then writes */
    if (it->fade_step < 0) { it->fade_step = 0; return false; }
    for (int k = 0; k < 32; k++) it->copper[k] = intro_lerp(it->fade_from[k], it->fade_to[k], it->fade_step);
    if (++it->fade_step < 17) return false;
    memcpy(it->pal_current, it->fade_to, sizeof it->pal_current);
    return true;
}

static void tw_start(sk_intro *it, int text) {                  /* typewriter_start 0x100B62 */
    it->tw_text = image_ptr(&it->anim, texts[text], 1);
    it->tw_end = NULL;
    for (uint32_t k = 1; it->tw_text && k <= 512; k++)           /* port guard: the text ends inside its hunk */
        if (image_ptr(&it->anim, texts[text], k)) it->tw_end = it->tw_text + k; else break;
    it->tw_active = it->tw_text != NULL;
    it->tw_cell = 0; it->tw_line = 0; it->tw_countdown = 0;
}

static void draw_glyph(sk_intro *it, int g) {                   /* 0x100BF8: colour 31, plane 6 cleared under the mask */
    const uint8_t *gl = image_ptr(&it->anim, FONT + (uint32_t)(g - 1) * 64u, 64);
    if (!gl || it->tw_cell >= 20 || it->tw_line >= 12) return;
    uint8_t (*s)[8000] = it->screen[it->shown];
    for (int r = 0; r < 16; r++) {
        uint16_t mask = be16(gl + 2 * r), data = be16(gl + 32 + 2 * r);
        int o = (it->tw_line * 16 + r) * 40 + it->tw_cell * 2;
        for (int p = 0; p < 6; p++) {
            uint16_t w = (uint16_t)(s[p][o] << 8 | s[p][o + 1]);
            w = (uint16_t)((w & ~mask) | (p < 5 ? data : 0));
            s[p][o] = (uint8_t)(w >> 8); s[p][o + 1] = (uint8_t)w;
        }
    }
}

static void tw_tick(sk_intro *it) {                             /* typewriter_tick 0x100BB2 */
    if (!it->tw_active) return;
    if (it->tw_countdown) { it->tw_countdown--; return; }
    for (;;) {
        if (it->tw_text >= it->tw_end) { it->tw_active = false; return; }
        uint8_t c = *it->tw_text;
        if (!c) { it->tw_active = false; return; }
        it->tw_text++;
        if (c == 0x0D) { it->tw_line++; it->tw_cell = 0; continue; }   /* a new line, the next byte at once */
        int g = image_u8(&it->anim, CHAR_TO_GLYPH + (c & 0x7F));
        if (g) draw_glyph(it, g);
        it->tw_cell++;
        it->tw_countdown = it->tw_reload;
        return;
    }
}

bool intro_open(sk_intro *it, sk_disks *d, sk_sound *snd, sk_err *e) {
    memset(it, 0, sizeof *it);
    uint8_t *raw = NULL; size_t rn = 0;
    if (!disks_read(d, 0, "anim", &raw, &rn, e)) return false;
    bool ok = hunk_load(raw, rn, &it->anim, e);
    free(raw);
    if (!ok) return false;
    static const char *names[2] = { "animfil1.pak", "animfil2.pak" };
    for (int f = 0; f < 2; f++) {
        if (!disks_read(d, 0, names[f], &raw, &rn, e)) { intro_free(it); return false; }
        ok = bk_unpack(raw, rn, &it->file[f], &it->file_n[f], e);
        free(raw);
        if (!ok) { intro_free(it); return false; }
    }
    it->work = 0; it->shown = 1; it->displayed = 1;
    it->bplcon0 = 0x5200;
    it->snd = snd;
    return true;
}

static void swap(sk_intro *it) { int t = it->work; it->work = it->shown; it->shown = t; }

static void palette_from_cmap(sk_intro *it, const uint8_t *cmap) {   /* anim_player: 8-bit triples to 12 bits */
    for (int k = 0; k < 32; k++) {
        uint16_t c = (uint16_t)(((cmap[3 * k] & 0xF0) << 4) | (cmap[3 * k + 1] & 0xF0) | (cmap[3 * k + 2] >> 4));
        it->pal_half[k] = (uint16_t)((c & 0xEEE) >> 1);
        it->pal_full[k] = c;
    }
    it->pal_full[31] = 0xFFF;                                    /* the text colour */
}

/* end_text_overlay 0x100718: sub 0 half palette, 1 swap_and_clean then the fade back; true when done */
static bool end_overlay(sk_intro *it, int *sub) {
    if (*sub == 0) { it->bplcon0 = 0x5200; copper_palette(it, it->pal_half); memcpy(it->pal_current, it->pal_half, sizeof it->pal_current); *sub = 1; return false; }
    if (*sub == 1) {                                             /* swap_and_clean 0x100734 */
        swap(it); it->displayed = it->shown;
        memcpy(it->screen[it->work], it->screen[it->shown], sizeof it->screen[0]);
        memset(it->screen[it->work][5], 0, 8000);
        fade_start(it, it->pal_full); *sub = 2;
    }
    return fade_tick(it);
}

/* one tick of an operation; true when it is finished (the next one starts in the same tick) */
static bool run_op(sk_intro *it, bool skip) {
    int op = script[it->op].op, a = script[it->op].a, b = script[it->op].b, c = script[it->op].c;
    switch (op) {
    case OP_MUSIC:                                               /* music_start(module, 1), four channels, master 100 */
        if (it->snd) {
            const uint8_t *m = image_ptr(&it->anim, MODULE, MODULE_SIZE);
            uint8_t *copy = m ? malloc(MODULE_SIZE) : NULL;
            if (copy) {
                memcpy(copy, m, MODULE_SIZE);
                sound_set_module(it->snd, 0, copy, MODULE_SIZE);
                it->snd->music_use4 = true;
                pt_master(&it->snd->pt, true);
                sound_music(it->snd, SK_MUSIC_LEVEL, a);
            }
        }
        return true;
    case OP_RELOAD: it->tw_reload = a; return true;
    case OP_TEXT_ON_BLACK:                                       /* text_on_black 0x100ABA */
        memset(it->screen[it->shown], 0, sizeof it->screen[0]);
        copper_palette(it, it->pal_full); it->copper[31] = 0xEEE;
        memcpy(it->pal_current, it->copper, sizeof it->pal_current);   /* 0x100ABA writes palette_current too */
        it->bplcon0 = 0x5200;
        tw_start(it, a);
        return true;
    case OP_WAIT_TYPE:                                           /* typewriter_wait 0x100B7A: polls the key */
        if (skip) { it->quit = true; return true; }
        return !it->tw_active;
    case OP_CUE_PENDING: it->cue_pending = a; return true;
    case OP_FADE_BLACK: {                                        /* fade_to_black 0x1006F0 */
        static const uint16_t black[32];
        if (it->sub == 0) { fade_start(it, black); it->sub = 1; }
        if (!fade_tick(it)) return false;
        it->sub = 0;
        return true;
    }
    case OP_FADE_IN: it->fade_in_flag = true; return true;
    case OP_HOLD: it->hold_ticks = a; return true;
    case OP_PENDING: it->pending_text = (const char *)(intptr_t)(a + 1); return true;
    case OP_WAIT:
        if (it->sub == 0) { it->wait = a; it->sub = 1; }
        if (--it->wait > 0) return false;
        it->sub = 0;
        return true;
    case OP_MUSIC_FADE:                                          /* wait_music_faded 0x1006E0 */
        if (!it->snd) return true;
        it->snd->pt.master_target = 0;
        return it->snd->pt.master <= 0 || !it->snd->pt.playing;
    case OP_CUE: if (it->snd) pt_cue(&it->snd->pt, a); return true;
    case OP_STOP: if (it->snd) sound_stop(it->snd); return true;
    case OP_END: it->done = true; return false;
    case OP_TEXT_OVER_DIM:                                       /* text_over_dim 0x100B1A */
        switch (it->sub) {
        case 0: memset(it->screen[it->shown][5], 0xFF, 8000); it->bplcon0 = 0x5200; fade_start(it, it->pal_half); it->sub = 1; /* fall through */
        case 1: if (!fade_tick(it)) return false; it->sub = 2; return false;
        case 2: it->bplcon0 = 0x6200; copper_palette(it, it->pal_full); memcpy(it->pal_current, it->pal_full, sizeof it->pal_current); it->sub = 3; return false;
        case 3: tw_start(it, a); it->sub = 4; return false;
        case 4: if (skip) { it->quit = true; return true; } if (it->tw_active) return false; it->wait = 100; it->sub = 5; return false;
        case 5: if (--it->wait > 0) return false; it->sub = 10; it->fade_step = 0; it->node = 0; /* fall through */
        default: { int s = it->sub - 10; bool done = end_overlay(it, &s); it->sub = s + 10; if (!done) return false; it->sub = 0; return true; }
        }
    case OP_PLAY:
        for (;;) switch (it->sub) {
        case 0:                                                  /* play_anim: parse, the list */
            if (!anim_open(it->file[a], it->file_n[a], b, &it->cur, &(sk_err){0})) { it->sub = 0; return true; }
            it->node = 0; it->delay = c; it->sub = 1;
            break;
        case 1: {                                                /* the next node into the work screen */
            if (it->node >= it->cur.count - 2) { it->sub = 0; return true; }
            const sk_anim_frame *f = &it->cur.frame[it->node];
            if (f->key) {
                if (skip || it->quit) { it->quit = true; return true; }   /* decode_ilbm_body polls the key */
                memset(it->screen[it->work], 0, sizeof it->screen[0]);
                anim_decode_body(f->data, f->n, it->screen[it->work]);
                memcpy(it->screen[it->shown], it->screen[it->work], sizeof it->screen[0]);
                it->bplcon0 = 0x5200;
                if (it->hold_ticks) { memset(it->screen[it->work][5], 0xFF, 8000); it->bplcon0 = 0x6200; }   /* dim_screen_ehb */
            } else anim_apply_delta(f->data, f->n, it->screen[it->work]);
            swap(it);
            it->sub = 2;
            return false;                                        /* sf vbl_flag: at least one fresh tick */
        }
        case 2:                                                  /* wait until tick_counter > D, then show */
            if (it->tick_counter <= it->delay) return false;
            it->displayed = it->shown;
            it->sub = 3;
            break;
        case 3: {
            const sk_anim_frame *f = &it->cur.frame[it->node];
            it->sub = 6;
            if (f->cmap) {
                palette_from_cmap(it, f->cmap);
                if (it->fade_in_flag) { it->fade_in_flag = false; fade_start(it, it->pal_full); it->sub = 4; }
                else { copper_palette(it, it->pal_full); memcpy(it->pal_current, it->pal_full, sizeof it->pal_current); }
            }
            break;
        }
        case 4:
            if (!fade_tick(it)) return false;
            if (it->cue_pending && it->snd) pt_cue(&it->snd->pt, it->cue_pending);
            it->cue_pending = 0;
            it->sub = 6;
            break;
        case 6:
            if (it->hold_ticks && it->cur.frame[it->node].key && it->pending_text) {
                tw_start(it, (int)(intptr_t)it->pending_text - 1); it->sub = 7; return false;
            }
            it->sub = 9;
            break;
        case 7:
            if (skip) { it->quit = true; return true; }
            if (it->tw_active) return false;
            it->pending_text = NULL; it->wait = it->hold_ticks; it->sub = 8;
            return false;
        case 8:
            if (--it->wait > 0) return false;
            it->sub = 20;
            /* fall through */
        default:
            if (it->sub >= 20) {
                int s = it->sub - 20; bool done = end_overlay(it, &s); it->sub = s + 20;
                if (!done) return false;
            }
            it->sub = 9;
            /* fall through */
        case 9:
            it->tick_counter = 0;
            it->node++;
            it->sub = 1;
            break;
        }
    }
    return true;
}

void intro_step(sk_intro *it, bool skip) {
    if (it->done) return;
    it->quit_req |= skip;                                        /* frame_interrupt sets quit_flag; it stays set */
    skip = it->quit_req;
    it->tick_counter++;                                          /* frame_interrupt */
    tw_tick(it);
    for (int guard = 0; guard < 64; guard++) {
        if (!run_op(it, skip)) break;
        if (it->quit) { if (it->snd) sound_stop(it->snd); it->done = true; return; }   /* jmp exit_intro */
        if (++it->op >= (int)(sizeof script / sizeof *script)) { it->done = true; return; }
        it->sub = 0;
    }
}

void intro_present(const sk_intro *it, uint32_t *out) {
    for (int i = 0; i < SK_SCREEN_W * SK_SCREEN_H; i++) out[i] = 0;
    const uint8_t (*s)[8000] = it->screen[it->displayed];
    for (int y = 0; y < 200; y++)
        for (int x = 0; x < 320; x++) {
            int o = y * 40 + (x >> 3), bit = 7 - (x & 7), idx = 0;
            for (int p = 0; p < 5; p++) idx |= ((s[p][o] >> bit) & 1) << p;
            if (!idx) continue;                                  /* COLOR00 is black */
            uint16_t c = it->copper[idx];
            if (it->bplcon0 == 0x6200 && ((s[5][o] >> bit) & 1)) c = (uint16_t)((c >> 1) & 0x777);   /* Extra Half-Brite */
            out[y * SK_SCREEN_W + x] = rgb12_to_xrgb(c);
        }
}

void intro_free(sk_intro *it) {
    image_free(&it->anim);
    for (int f = 0; f < 2; f++) { free(it->file[f]); it->file[f] = NULL; }
}
