#include <string.h>
#include "check.h"
#include "audio/paula.h"

static int8_t g_sq[64];                                              /* a square wave: 32 x +100, 32 x -100 */
static void fill(void) { for (int i = 0; i < 64; i++) g_sq[i] = (int8_t)(i < 32 ? 100 : -100); }
static sk_amem mem(const int8_t *p, size_t n) { return (sk_amem){ p, p + n }; }

void test_paula_plays_at_the_period_rate(void) {
    fill();
    static sk_paula p; paula_init(&p, 48000);
    paula_set_ptr(&p, 0, mem(g_sq, 64)); paula_set_len(&p, 0, 32); paula_set_per(&p, 0, 428); paula_set_vol(&p, 0, 64);
    paula_dma(&p, 0x8001);
    CHECK(p.ch[0].on);
    int16_t out[2 * 4800];
    paula_render(&p, out, 4800);                                     /* 0.1 s */
    int flips = 0;
    for (int i = 1; i < 4800; i++) flips += (out[2 * i] > 0) != (out[2 * i - 2] > 0);
    int cycles = flips / 2;                                          /* 3546895 / 428 / 64 = 129.5 Hz: about 13 cycles */
    CHECK(cycles >= 12 && cycles <= 14);
    CHECK(out[0] != 0); CHECK(out[1] != 0);                          /* channel 1 is left, softly in the right too */
    CHECK(out[0] > out[1] || out[0] < out[1]);
}

void test_paula_latches_the_loop_at_the_block_end(void) {
    fill();
    static int8_t one[4] = { 50, 50, -50, -50 };
    static sk_paula p; paula_init(&p, 48000);
    paula_set_ptr(&p, 1, mem(g_sq, 64)); paula_set_len(&p, 1, 32); paula_set_per(&p, 1, 200); paula_set_vol(&p, 1, 64);
    paula_dma(&p, 0x8002);
    paula_set_ptr(&p, 1, mem(one, 4)); paula_set_len(&p, 1, 2);    /* written after the start: the next block */
    CHECK(p.ch[1].cur == g_sq);
    int16_t out[2 * 2000];
    paula_render(&p, out, 2000);
    CHECK(p.ch[1].cur == one);                                       /* the first block ended: the loop took over */
    paula_dma(&p, 0x0002);
    CHECK(!p.ch[1].on);
    paula_render(&p, out, 10);
    CHECK_EQ(out[0], 0); CHECK_EQ(out[1], 0);
}

void test_paula_short_and_bad_loops(void) {                          /* Review Focus 2 */
    fill();
    static sk_paula p; paula_init(&p, 48000);
    paula_set_ptr(&p, 2, mem(g_sq, 64)); paula_set_len(&p, 2, 1000);  /* runs past its buffer */
    paula_set_per(&p, 2, 0); paula_set_vol(&p, 2, 200);                 /* period 0, volume over 64 */
    paula_dma(&p, 0x8004);
    int16_t out[2 * 48000];
    paula_render(&p, out, 48000);                                    /* no crash, no read past g_sq (ASan) */
    CHECK_EQ(p.ch[2].vol, 64);
    paula_set_ptr(&p, 3, mem(g_sq, 64)); paula_set_len(&p, 3, 0); paula_set_per(&p, 3, 300); paula_set_vol(&p, 3, 64);
    paula_dma(&p, 0x8008);
    paula_render(&p, out, 48000);
    paula_set_ptr(&p, 3, (sk_amem){ NULL, NULL }); paula_set_len(&p, 3, 1);
    paula_render(&p, out, 48000);
    CHECK(1);
}

void test_paula_without_a_rate_is_silent(void) {                     /* a zeroed model (never initialised) */
    static sk_paula p;
    memset(&p, 0, sizeof p);
    p.ch[0].on = true; p.ch[0].vol = 64;
    int16_t out[2 * 16];
    memset(out, 0x55, sizeof out);
    paula_render(&p, out, 16);
    for (int i = 0; i < 32; i++) CHECK_EQ(out[i], 0);
}
