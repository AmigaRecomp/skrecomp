#include <string.h>
#include "game/scoreboard.h"
#include "game/attrs.h"
#include "game/game.h"
#include "game/objects.h"
#include "audio/sound.h"

#define FONT_8 0x204148u                /* two-plane 8 x 8 font: 16 bytes a glyph, (face, shadow) per row */
#define TICKER_HEAD 0x203E41u           /* "...SOCCER KID HAS CLEARED " */
#define TICKER_MID 0x203E67u            /* ". NEXT STOP... " */
#define TICKER_TAIL 0x203E77u           /* ". WELL DONE !..." */
#define BLANK_ROW 0x203E8Fu             /* 14 spaces */
#define VALUE_TEXT 0x203E3Au            /* "00o000" */
#define LEVEL_NAMES 0x203E9Eu           /* level_name_ptrs: 30 longs by data index */
#define TEXT_DX 16                      /* window pixel = text x + 16, line = text y + 24 (pf1 at view +8, +0x20 / +0x18) */
#define TEXT_DY 24

/* scoreboard_rows (0x103BBC): label x y, value x y, the label, the counter, points per unit (BCD), wait, flags, delay after */
static const struct { int16_t lx, ly, vx, vy; uint32_t label; uint32_t points; uint8_t wait, flags, delay; } rows[6] = {
    { 32, 30, 128, 30, 0x203E0C, 0x300, 50, 0, 0 },     /* CARDS       0x300370 */
    { 32, 50, 128, 50, 0x203E12, 0x200, 50, 0, 0 },     /* BALLS       0x300371 */
    { 32, 70, 128, 70, 0x203E18, 0x100, 50, 0, 50 },    /* TIME        0x300372 */
    { 40, 30, 76, 60, 0x203E1D, 0x100, 50, 1, 0 },      /* TRICK KICKS 0x300373 */
    { 72, 30, 76, 60, 0x203E29, 0x400, 50, 1, 0 },      /* HEADERS     0x300374 */
    { 64, 30, 76, 60, 0x203E31, 0x800, 50, 1, 100 },    /* OVERHEAD    0x300375 */
};
static const uint32_t bonus_steps[5] = { 0x100, 0x1000, 0x10000, 0x100000, 0x1000000 };      /* bonus_step_table 0x103ACA */
static const uint32_t bonus_masks[5] = { 0xF00, 0xF000, 0xF0000, 0xF00000, 0xF000000 };     /* bonus_mask_table 0x103ADE */
static const uint8_t juggle_script[6] = { 0x01, 0x04, 0x08, 0x01, 0x08, 0x04 };            /* 0x10C1B6 */

static uint8_t *counter_of(sk_game *g, int row, uint8_t *tmp) {     /* CARDS is the int cards_taken */
    switch (row) {
    case 0: *tmp = (uint8_t)g->cards_taken; return tmp;
    case 1: return &g->stat_balls;
    case 2: return &g->stat_time;
    case 3: return &g->kills[1];
    case 4: return &g->kills[3];
    default: return &g->kills[2];
    }
}

static const char *image_text(const sk_game *g, uint32_t addr, char *buf, size_t n) {   /* a NUL-ended string in sk */
    buf[0] = 0;
    if (!g->sk) return buf;
    size_t i = 0;
    for (; i + 1 < n; i++) {
        const uint8_t *p = image_ptr(g->sk, addr + (uint32_t)i, 1);
        if (!p || !*p) break;
        buf[i] = (char)*p;
    }
    buf[i] = 0;
    return buf;
}

static uint8_t to_bcd(uint8_t v) { return (uint8_t)(((v / 10) % 10) << 4 | v % 10); }   /* binary_to_bcd 0x103F7A */
static uint8_t bcd_dec8(uint8_t v) { return (v & 15) ? (uint8_t)(v - 1) : (uint8_t)(((v >> 4) - 1) << 4 | 9); }
static uint32_t bcd_sub32(uint32_t a, uint32_t b) {
    uint32_t r = 0; int borrow = 0;
    for (int i = 0; i < 8; i++) {
        int d = (int)(a >> (4 * i) & 15) - (int)(b >> (4 * i) & 15) - borrow;
        borrow = d < 0;
        if (borrow) d += 10;
        r |= (uint32_t)d << (4 * i);
    }
    return r;
}
static void two_digits(char *s, uint8_t bcd) {                   /* bcd_to_two_digits 0x103C8A: a leading zero is a space */
    s[0] = (bcd >> 4) ? (char)('0' + (bcd >> 4)) : ' ';
    s[1] = (char)('0' + (bcd & 15));
}

void scoreboard_text(sk_game *g, const char *s, int x, int y) {  /* 0x103CFC */
    for (int i = 0; i < 15 && s[i]; i++) {
        const uint8_t *glyph = g->sk ? image_ptr(g->sk, FONT_8 + (uint32_t)((((uint8_t)s[i] - 0x20) & 0x7F) * 16), 16) : NULL;
        for (int r = 0; r < 8; r++) {
            int line = (y + TEXT_DY + 2 * r) & 0xFF;             /* every other line: the modulo 0x15C */
            uint8_t row = glyph ? glyph[2 * r] : 0;
            for (int c = 0; c < 16; c++) {
                int px = x + TEXT_DX + 16 * i + c;
                if (px < 0 || px >= 320) continue;
                uint8_t bit = (uint8_t)(0x80 >> (px & 7));
                g->board.text_set[line][px >> 3] |= bit;
                bool on = !(c & 1) && ((row >> (7 - c / 2)) & 1);   /* source pixel k lights pixel 2k */
                if (on) g->board.text[line][px >> 3] |= bit; else g->board.text[line][px >> 3] &= (uint8_t)~bit;
            }
        }
    }
}

static void clear_rows(sk_game *g) {                             /* scoreboard_clear_rows 0x103C50 */
    char blank[32];
    if (g->sk) image_text(g, BLANK_ROW, blank, sizeof blank); else strcpy(blank, "              ");
    static const int ys[4] = { 0x1E, 0x2E, 0x3E, 0x48 };
    for (int k = 0; k < 4; k++) scoreboard_text(g, blank, 0, ys[k]);
}

static void append(char *dst, size_t n, const char *s) { size_t l = strlen(dst); if (l + 1 < n) strncat(dst, s, n - l - 1); }

static void level_name(const sk_game *g, int data_index, char *buf, size_t n) {
    buf[0] = 0;
    if (!g->sk || data_index < 0 || data_index >= 30) return;
    image_text(g, image_u32(g->sk, LEVEL_NAMES + 4u * (uint32_t)data_index), buf, n);
}

void scoreboard_setup(sk_game *g) {                              /* 0x103DAC */
    if (g->level_data_index != 12) return;
    if (!g->piece_collected && g->cards_taken == 11) g->bonus_pending = 0xFF;
    memset(&g->board, 0, sizeof g->board);
    g->board.active = true;
    char a[64], b[64], t[64];
    int ln = g->level_number;
    int prev = ln >= 1 && ln - 1 < SK_LEVEL_SEQUENCE_LEN && g->tables ? g->tables->sequence[ln - 1] : -1;
    int next = -1;
    if (g->tables && ln + 1 < SK_LEVEL_SEQUENCE_LEN) {
        next = g->tables->sequence[ln + 1];
        if (next >= 0x1E && ln + 2 < SK_LEVEL_SEQUENCE_LEN) next = g->tables->sequence[ln + 2];   /* a map or ALIEN: the one after */
    }
    level_name(g, prev, a, sizeof a);
    level_name(g, next, b, sizeof b);
    char *k = g->board.ticker; size_t kn = sizeof g->board.ticker;
    append(k, kn, image_text(g, TICKER_HEAD, t, sizeof t)); append(k, kn, a);
    append(k, kn, image_text(g, TICKER_MID, t, sizeof t)); append(k, kn, b);
    append(k, kn, image_text(g, TICKER_TAIL, t, sizeof t));
    g->board.ticker_len = (int)(strlen(k) + 1) * 16;
    if (g->sk) image_text(g, VALUE_TEXT, g->board.value, sizeof g->board.value); else strcpy(g->board.value, "00 000");
    g->board.draw = 2;
    g->stat_balls = to_bcd(g->stat_balls);
    g->cards_taken = to_bcd((uint8_t)g->cards_taken);
    g->kills[1] = to_bcd(g->kills[1]); g->kills[3] = to_bcd(g->kills[3]); g->kills[2] = to_bcd(g->kills[2]);
    g->card_anim[0] = 0; g->cards = 0; g->hud_card_flag = 0;
}

void scoreboard_free(sk_game *g) {                               /* 0x103F9A, from reset_level_state */
    if (!g->board.active) return;
    g->board.active = false;
    g->cards = 0; g->hud_card_flag = 0;                          /* reset_card_count */
    g->stat_balls = 0; g->stat_time = 0;
}

static void count_step(sk_game *g, uint8_t *c, uint32_t points) {   /* scoreboard_count_step 0x103CAC */
    g->board.counter = (uint16_t)((g->board.counter + 1) & 3);
    if (g->board.counter) return;
    if (g->board.phase == 0) {                                   /* CARDS: one HUD card at a time */
        if (g->hud_card_flag) return;
        g->hud_card_flag = 0xFF;
    }
    *c = bcd_dec8(*c);
    g->board.bonus = bcd_add32(g->board.bonus, points);
    play_sfx(g, 0xD);
}

static void board_row(sk_game *g) {                              /* scoreboard_row 0x103AF2 */
    if (--g->board.delay >= 0) return;
    g->board.delay = 0;
    if (g->board.phase >= 6) { g->board.phase = -1; return; }    /* the terminator row */
    int r = g->board.phase;
    char buf[32];
    if (g->board.draw) {
        g->board.draw--;
        two_digits(g->board.value + 3, (uint8_t)(rows[r].points >> 4));
        g->board.wait = (int8_t)rows[r].wait;
        if (rows[r].flags & 1) clear_rows(g);
        scoreboard_text(g, image_text(g, rows[r].label, buf, sizeof buf), rows[r].lx, rows[r].ly);
    }
    uint8_t tmp, *c = counter_of(g, r, &tmp);
    if (--g->board.wait < 0) {
        g->board.wait = 0;
        if (*c) { count_step(g, c, rows[r].points); if (r == 0) g->cards_taken = *c; }
        else { g->board.phase++; g->board.draw = 2; g->board.delay = (int8_t)rows[r].delay; }
    }
    two_digits(g->board.value, *c);
    scoreboard_text(g, g->board.value, rows[r].vx, rows[r].vy);
}

static void pay_bonus(sk_game *g) {                              /* scoreboard_pay_bonus 0x103A32 */
    clear_rows(g);
    if (!g->board.bonus) {
        g->board.phase = 0x7F; g->board.counter = 0;
        if (g->snd) g->snd->request = false;                     /* music_request_flag */
        return;
    }
    g->board.counter = (uint16_t)((g->board.counter + 1) & 3);
    if (g->board.counter) return;
    int i = -g->board.phase - 1;
    while (i < 5 && !(g->board.bonus & bonus_masks[i])) { g->board.phase--; i++; }
    if (i >= 5) { g->board.bonus = 0; return; }                  /* port guard: no digit of 100 or more left */
    add_score(g, bonus_steps[i]);
    g->board.bonus = bcd_sub32(g->board.bonus, bonus_steps[i]);  /* ruling: the whole step (the original subtracts its low word) */
    play_sfx(g, 0xD);
}

static void patch_music(sk_game *g) {                            /* 0x103920: B05 into the playing pattern, the tune moves to its end */
    if (!g->snd || !g->snd->pt.mod) return;
    sk_pt *pt = &g->snd->pt;
    int pos = pt->songpos;
    uint32_t row = 0x200;                                        /* row 32 */
    if (pt->patternpos >= 0x1E0) { row = 0; pos++; }
    if (0x3B8u + (uint32_t)pos >= pt->n) return;
    uint32_t at = 0x43Cu + (uint32_t)pt->mod[0x3B8 + pos] * 1024u + row;
    if (at + 4 > pt->n) return;
    pt->mod[at + 2] = 0x0B; pt->mod[at + 3] = 0x05;
}

void scoreboard_update(sk_game *g) {                             /* 0x1038E8 */
    if (!g->board.active || !g->board.state) return;
    if (g->board.phase < 0) pay_bonus(g);
    else if (g->board.phase == 0x7F) {
        clear_rows(g);
        g->board.counter++;
        if (g->board.counter == 0x1E) patch_music(g);
        if (g->board.counter >= 0xC8) g->level_outcome |= 2;
    } else board_row(g);
    char digits[8];
    uint32_t b = g->board.bonus;
    for (int k = 0; k < 7; k++) digits[k] = (char)('0' + (b >> (4 * (6 - k)) & 15));   /* 10^6 .. 10^0 */
    digits[7] = 0;
    scoreboard_text(g, digits, 0x48, 0x5A);
    g->board.ticker_pos += 2;
    if (g->board.ticker_pos > g->board.ticker_len) {
        g->board.ticker_pos -= 2;
        if (g->level_outcome & 2) g->player.state = 60;          /* the exit (state 60 does not walk here) */
    }
    int p = g->board.ticker_pos;
    if ((p >> 4) < (int)sizeof g->board.ticker) scoreboard_text(g, g->board.ticker + (p >> 4), (~p) & 0xE, 2);
    g->board.border_x[0] = (g->camera_x + 0x20) & 0xFFF0;       /* place_tile_column: tile 9 and 8, seven rows */
    g->board.border_x[1] = (g->camera_x + 0x20 + 0xF0) & 0xFFF0;
    g->board.border_y = (g->camera_y + 0x20) & 0xFFF0;
    fill_cells(g, g->camera_x + 0x20, g->camera_y + 0x20, 9, 0x20, 6);
    fill_cells(g, g->camera_x + 0x20 + 0xF0, g->camera_y + 0x20, 8, 0x20, 6);
}

int scoreboard_player_input(sk_game *g, int input) {             /* 0x10C116 */
    (void)input;
    sk_player *p = &g->player;
    if (g->board.state == 0) {
        if (p->x - g->camera_x > 0xE6) g->board.state = 0xFE;
        return SK_IN_RIGHT;
    }
    if (g->board.state != 0xFF) {
        if (p->facing >= 0) { g->board.juggle_wait = 0; g->board.juggle = 0; return SK_IN_LEFT; }
        int hy = p->y + 11;                                      /* ball_hold_offset 0x10CE12 */
        int hx = p->x + (p->on_platform ? (p->facing >= 0 ? -6 : -10) : (p->facing >= 0 ? 7 : -23));
        if ((int16_t)(g->ball.y >> 16) - hy < 0) return 0;
        if ((int16_t)(g->ball.x >> 16) != hx) return 0;
        g->board.state = 0xFF;
    }
    p->fire_lock = 0;                                            /* the juggle: fire held, a direction every 0x50 frames */
    int d = SK_IN_FIRE;
    if (++g->board.juggle_wait >= 0x50) {
        d |= juggle_script[g->board.juggle];
        g->board.juggle_wait = 0;
        if (++g->board.juggle == 6) g->board.juggle = 0;
    }
    return d;
}

void level_end_stats(sk_game *g) {                               /* 0x104558 */
    uint8_t t = (uint8_t)g->level_timer, s = g->stat_time;      /* abcd: the carry out is dropped */
    int lo = (t & 15) + (s & 15), hi = (t >> 4) + (s >> 4);
    if (lo > 9) { lo -= 10; hi++; }
    g->stat_time = (uint8_t)((hi % 10) << 4 | lo);
    int d = (int8_t)(uint8_t)(g->balls_allotted - g->recalls);
    if (d >= 0) g->stat_balls = (uint8_t)(g->stat_balls + d);
    g->recalls = 0;
}
