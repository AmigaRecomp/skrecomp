/* The scroll modes, transcribed from the listing (objects.md "JAPAN TRAIN scrolling"). */
#include "game/scroll_modes.h"

static const int16_t ship_sway[32] = {                        /* 0x104C38 */
    0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2, 1, 0, 0, 0, -1, -1, -1, -1, -1 };
static const uint16_t train_phases[16] = {                    /* 0x200B7A: (tunnel, open) lengths */
    100, 200, 150, 300, 200, 400, 250, 600, 150, 1200, 350, 700, 400, 1000, 450, 900 };

static void stream_window(sk_game *g, int base, int width) {
    g->stream.base = base;
    g->stream.width = width;
    g->stream.cursor = width;                                    /* dat_3002D2 = dat_3002D4 */
}

void scroll_modes_setup(sk_game *g, int data_index) {
    g->scroll_mode = 0;
    g->bg_speed = 0;
    g->train_fade = 0; g->train_flags = 0; g->train_timer = 0; g->train_level = 15;
    int max_x = g->world.bg_max_x;
    if (data_index == 15) {                                      /* BATTLE SHIP (0x10419A) */
        g->scroll_mode = 2;
        g->bg_speed = 1;
        stream_window(g, max_x - 0x40, 0x130);
    } else if (data_index == 20) {                               /* TRAIN (0x1041E2) */
        g->scroll_mode = 1;
        g->train_timer = 500;
        g->train_flags = 5;                                      /* bit 0 start, bit 2 speeding up */
        g->player.facing = -1;                                   /* dat_302B32 = -1 */
        stream_window(g, max_x - 0x20, 0x130);
    } else return;
    g->view.bg_x = g->stream.base + 32;                          /* sub_1068EE on the full redraw */
    g->stream.last = g->view.bg_x;
    for (int k = 0; k < 32; k++) {                               /* the first view: the map as it is */
        int c = (g->view.bg_x >> 4) - 2 + k;
        g->stream.col[c & 31] = (uint16_t)c;
    }
}

uint16_t train_scale_colour(uint16_t c, int level) {
    int n = level + 1;
    int r = ((c >> 8) & 15) * n >> 4, gr = ((c >> 4) & 15) * n >> 4, b = (c & 15) * n >> 4;
    return (uint16_t)(r << 8 | gr << 4 | b);
}

static void train_driver(sk_game *g) {                          /* sub_1049C8 */
    if (g->train_fade < 0) {                                     /* fading to the tunnel: steps 14 .. 4 */
        g->train_fade++;
        int d = -g->train_fade;
        if (d < 11) g->train_level = 4 + d;                      /* apply_fade_step from 0x300EE2 + 2d */
    } else if (g->train_fade > 0) {                              /* back out: steps 5 .. 15 */
        g->train_fade--;
        int d = g->train_fade;
        if (d < 11) g->train_level = 15 - d;                     /* from 0x300EF8 - 2d */
    }
    if ((g->frame_ticks & 0x1F) == 0) {
        if (g->bg_speed >= 10) g->shake_amplitude = 1;           /* full speed: the train rattles */
        if (g->train_flags & 4) {
            g->bg_speed += (g->train_flags & 8) ? -1 : 1;
            if (g->bg_speed == 0 || g->bg_speed >= 10) g->train_flags &= (uint8_t)~4;
        }
    }
    if (--g->train_timer >= 0) return;
    const uint16_t *pair;
    if (!(g->train_flags & 0x20)) {                              /* into a tunnel */
        g->train_timer = 100;
        pair = train_phases;
        stream_window(g, g->world.bg_max_x - 0x160, 0xA0);
        g->train_fade = (int8_t)0xE6;
    } else {                                                     /* out into the open */
        pair = train_phases + 1;
        int base = 0, width = 0x7E0;
        if (g->train_flags & 0x10) {                             /* the exit: slow to a stop at the station */
            g->train_timer = 0x7D0;
            g->train_flags = (uint8_t)((g->train_flags & 0xF0) | 0xE);
            base = g->world.bg_max_x - 0x20; width = 0x130;
        }
        g->train_flags &= (uint8_t)~3;
        stream_window(g, base, width);
        g->train_fade = 0x14;
    }
    g->train_flags ^= 0x20;
    if (g->train_flags & 0x10) return;
    g->train_timer = pair[2 * (game_random(g) & 7)];
}

static void ship_rock(sk_game *g) {                             /* sub_104BF0 */
    g->shake_amplitude = ship_sway[(g->frame_ticks & 0x3F) >> 1];
    if (g->stream.base != 0x20) {
        g->stream.base = 0x20;                                   /* after the first view: the whole backdrop drifts by */
        g->stream.width = g->world.bg_max_x - 0x70;
    }
}

void scroll_modes_frame(sk_game *g) {
    if (g->scroll_mode == 1) train_driver(g);
    else if (g->scroll_mode == 2) ship_rock(g);
}

void backdrop_stream_step(sk_game *g) {                         /* sub_1068EE */
    if (!g->scroll_mode) return;
    int x = g->view.bg_x - g->bg_speed;
    if (x <= 0x10) { x += 0x5800; g->stream.last += 0x5800; }     /* 0x580 columns: the ring of 32 is unchanged */
    int d3 = (g->stream.last & ~15) - (x & ~15);
    if (d3 > 0) {
        g->stream.cursor -= d3;
        if (g->stream.cursor <= 0x10) g->stream.cursor += g->stream.width;
        int src = (g->stream.cursor + g->stream.base) >> 4, first = x >> 4;
        for (int i = 0; i < d3 >> 4; i++) g->stream.col[(first + i) & 31] = (uint16_t)(src + i);
    }
    g->stream.last = x;
    g->view.bg_x = x;
}

int backdrop_stream_column(const sk_game *g, int buffer_col) { return g->stream.col[buffer_col & 31]; }
