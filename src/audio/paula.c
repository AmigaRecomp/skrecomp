#include "audio/paula.h"
#include <string.h>

void paula_init(sk_paula *p, int rate) { memset(p, 0, sizeof *p); p->rate = rate > 0 ? rate : 48000; }
void paula_set_ptr(sk_paula *p, int c, sk_amem m) { p->ch[c & 3].reg = m; }
void paula_set_len(sk_paula *p, int c, uint16_t words) { p->ch[c & 3].reg_len = words; }
void paula_set_per(sk_paula *p, int c, uint16_t period) { p->ch[c & 3].per = period; }
void paula_set_vol(sk_paula *p, int c, uint16_t vol) { p->ch[c & 3].vol = vol > 64 ? 64 : vol; }

static void latch(sk_paula_ch *c) {                  /* a new block from the registers */
    uint32_t bytes = (c->reg_len ? c->reg_len : 65536u) * 2u;
    c->cur = c->reg.p;
    c->cur_end = c->reg.end;
    c->cur_len = bytes;
    c->pos = 0;
}

void paula_dma(sk_paula *p, uint16_t dmacon) {
    for (int i = 0; i < 4; i++) {
        if (!(dmacon & (1u << i))) continue;
        sk_paula_ch *c = &p->ch[i];
        if (dmacon & 0x8000) { if (!c->on) { c->on = true; latch(c); } }
        else c->on = false;
    }
}

static int sample(sk_paula_ch *c, uint64_t step) {
    if (!c->on || !c->cur) return 0;
    uint32_t i = (uint32_t)(c->pos >> 32);
    int v = (c->cur + i < c->cur_end) ? c->cur[i] : 0;   /* past its buffer: silence (port guard) */
    c->pos += step;
    while ((uint32_t)(c->pos >> 32) >= c->cur_len) {
        uint64_t over = c->pos - ((uint64_t)c->cur_len << 32);
        latch(c);
        if (!c->cur) break;
        c->pos = over % ((uint64_t)c->cur_len << 32);
    }
    return v * c->vol;
}

void paula_render(sk_paula *p, int16_t *out, int frames) {
    uint64_t step[4];
    for (int i = 0; i < 4; i++) {
        unsigned per = p->ch[i].per < 113 ? 113 : p->ch[i].per;   /* the DMA's fastest fetch */
        step[i] = ((uint64_t)SK_PAULA_CLOCK << 32) / per / (unsigned)p->rate;
    }
    for (int f = 0; f < frames; f++) {
        int a = sample(&p->ch[0], step[0]) + sample(&p->ch[3], step[3]);   /* left */
        int b = sample(&p->ch[1], step[1]) + sample(&p->ch[2], step[2]);   /* right */
        int l = (a * 3 + b) / 2, r = (b * 3 + a) / 2;                       /* 75/25 blend, x2 */
        out[2 * f] = (int16_t)(l > 32767 ? 32767 : l < -32768 ? -32768 : l);
        out[2 * f + 1] = (int16_t)(r > 32767 ? 32767 : r < -32768 ? -32768 : r);
    }
}
