/* ProTracker replay of the game (see ptreplay.h). Offsets of the module stand in for the original's pointers; every read
   or write past the module is dropped (port guards). */
#include "audio/ptreplay.h"
#include <string.h>

static const uint8_t pt_funk[16] = {                                                      /* 0x101C90 */
    0, 5, 6, 7, 8, 10, 11, 13, 16, 19, 22, 26, 32, 43, 64, 128 };
static const uint8_t pt_sine[32] = {                                                      /* 0x101CA0 */
    0, 24, 49, 74, 97, 120, 141, 161, 180, 197, 212, 224, 235, 244, 250, 253,
    255, 253, 250, 244, 235, 224, 212, 197, 180, 161, 141, 120, 97, 74, 49, 24,
};
/* 0x101CC0: 16 finetunes x 36 periods, then the 48 words the code that follows holds: the scans can read them */
static const uint16_t pt_periods[16 * 36 + 48] = {
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453, 428, 404, 381, 360, 339, 320,
    302, 285, 269, 254, 240, 226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
    850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450, 425, 401, 379, 357, 337, 318,
    300, 284, 268, 253, 239, 225, 213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113,
    844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447, 422, 398, 376, 355, 335, 316,
    298, 282, 266, 251, 237, 224, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112,
    838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444, 419, 395, 373, 352, 332, 314,
    296, 280, 264, 249, 235, 222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111,
    832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441, 416, 392, 370, 350, 330, 312,
    294, 278, 262, 247, 233, 220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110,
    826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437, 413, 390, 368, 347, 328, 309,
    292, 276, 260, 245, 232, 219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109,
    820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434, 410, 387, 365, 345, 325, 307,
    290, 274, 258, 244, 230, 217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109,
    814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431, 407, 384, 363, 342, 323, 305,
    288, 272, 256, 242, 228, 216, 204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108,
    907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453, 428, 404, 381, 360, 339,
    320, 302, 285, 269, 254, 240, 226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120,
    900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477, 450, 425, 401, 379, 357, 337,
    318, 300, 284, 268, 253, 238, 225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119,
    894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447, 422, 398, 376, 355, 335,
    316, 298, 282, 266, 251, 237, 223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118,
    887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444, 419, 395, 373, 352, 332,
    314, 296, 280, 264, 249, 235, 222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118,
    881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467, 441, 416, 392, 370, 350, 330,
    312, 294, 278, 262, 247, 233, 220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117,
    875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437, 413, 390, 368, 347, 328,
    309, 292, 276, 260, 245, 232, 219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116,
    868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434, 410, 387, 365, 345, 325,
    307, 290, 274, 258, 244, 230, 217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115,
    862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431, 407, 384, 363, 342, 323,
    305, 288, 272, 256, 242, 228, 216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114,
    18663, 65534, 19001, 48, 872, 26172, 29760, 19001, 32, 1252, 26162, 49404, 10, 16889, 32, 1256,
    13872, 6, 14384, 4, 12848, 8, 8240, 0, 45689, 32, 1254, 25872, 13249, 32, 1254, 5057,
    32, 1250, 24832, 32, 19679, 32767, 20085, 16889, 48, 853, 29187, 24576, 90, 16889, 48, 853,
};

static uint8_t mb(const sk_pt *pt, uint32_t off) { return off < pt->n ? pt->mod[off] : 0; }
static uint16_t mw(const sk_pt *pt, uint32_t off) { return (uint16_t)(mb(pt, off) << 8 | mb(pt, off + 1)); }
static sk_amem at(const sk_pt *pt, uint32_t off) {
    if (off >= pt->n) return (sk_amem){ NULL, NULL };
    return (sk_amem){ (const int8_t *)pt->mod + off, (const int8_t *)pt->mod + pt->n };
}
static int ci(const sk_pt *pt, const sk_ptch *c) { return (int)(c - pt->ch); }
static uint16_t faded(const sk_pt *pt, int v) {
    if (pt->master_on) return (uint16_t)(pt->master == 100 ? v : pt->master == 0 ? 0 : v * pt->master / 100);   /* set_channel_volume (anim) */
    v -= pt->fade; return (uint16_t)(v < 0 ? 0 : v);
}
static uint16_t note_word(const sk_ptch *c) { return (uint16_t)(c->note[0] << 8 | c->note[1]); }
static uint16_t cmd_word(const sk_ptch *c) { return (uint16_t)(c->note[2] << 8 | c->note[3]); }
static void set_per(sk_pt *pt, sk_ptch *c, unsigned v) { paula_set_per(pt->paula, ci(pt, c), (uint16_t)v); }
static void set_vol(sk_pt *pt, sk_ptch *c, unsigned v) { paula_set_vol(pt->paula, ci(pt, c), (uint16_t)v); }
static int nch(const sk_pt *pt) { return pt->use4 ? 4 : 3; }

/* cmp.w (a0)+,d; dbcc: the index of the last entry compared and whether d >= it (bcc) */
static int scan(int base, uint16_t d, int iterations, bool *found) {
    int k = 0;
    *found = false;
    for (; k < iterations; k++) {
        int i = base + k;
        uint16_t e = (i >= 0 && i < (int)(sizeof pt_periods / sizeof *pt_periods)) ? pt_periods[i] : 0;
        if (d >= e) { *found = true; return k; }
    }
    return k - 1;
}
static uint16_t period_at(int i) { return (i >= 0 && i < (int)(sizeof pt_periods / sizeof *pt_periods)) ? pt_periods[i] : 0; }

void pt_init(sk_pt *pt, sk_paula *paula) {
    memset(pt, 0, sizeof *pt);
    pt->paula = paula;
    for (int i = 0; i < 4; i++) pt->ch[i].dmabit = (uint16_t)(1u << i);
    pt->speed = 6;
}

void pt_stop(sk_pt *pt) {                                           /* music_stop 0x10113C */
    pt->fade = 0;
    if (!pt->paula) return;
    for (int i = 0; i < nch(pt); i++) paula_set_vol(pt->paula, i, 0);
    paula_dma(pt->paula, 0x0007);
    pt->playing = false;
}

bool pt_start(sk_pt *pt, sk_paula *paula, uint8_t *mod, size_t n, int tune, bool use4) {   /* 0x100FCC */
    pt->paula = paula;
    pt->use4 = use4;
    for (int i = 0; i < 4; i++) pt->ch[i].dmabit = (uint16_t)(1u << i);   /* static in the original's channel records */
    pt_stop(pt);
    if (!mod || n < 0x43C) return false;                            /* port guard: no module */
    pt->mod = mod; pt->n = n;
    pt->playing = true;
    int hi = 0;
    for (int i = 0; i < 128; i++) if (mb(pt, 0x3B8 + (uint32_t)i) > hi) hi = mb(pt, 0x3B8 + (uint32_t)i);
    uint32_t s = 0x43C + (uint32_t)(hi + 1) * 1024;
    for (int i = 0; i < 31; i++) {
        for (uint32_t k = 0; k < 4; k++) if (s + k < pt->n) pt->mod[s + k] = 0;   /* clr.l (a2) */
        pt->samplestarts[i] = s;
        s += (uint32_t)mw(pt, 0x2A + 30 * (uint32_t)i) * 2;
    }
    pt->speed = 6;
    pt->counter = pt->speed;
    for (int i = 0; i < nch(pt); i++) paula_set_vol(paula, i, 0);   /* AUD3 only when the music has it (0x101072) */
    pt->patternpos = 0;
    pt->pattdelay2 = 0;
    pt->songpos = (uint8_t)tune;
    return true;
}

static void checkmorefx(sk_pt *pt, sk_ptch *c);

static void playvoice(sk_pt *pt, sk_ptch *c, uint32_t cell) {      /* sub_10127C */
    uint32_t prev = (uint32_t)c->note[0] << 24 | (uint32_t)c->note[1] << 16 | (uint32_t)c->note[2] << 8 | c->note[3];
    if (prev == 0) set_per(pt, c, c->period);
    for (int i = 0; i < 4; i++) c->note[i] = mb(pt, cell + (uint32_t)i);
    int smp = (c->note[2] >> 4) | (c->note[0] & 0xF0);
    if (smp) {
        uint32_t h = 12 + 30 * (uint32_t)smp;                        /* the sample's length word */
        c->start = pt->samplestarts[smp - 1 < 31 ? smp - 1 : 30];   /* port guard: samples 32..255 read sample 31 */
        c->length = mw(pt, h);
        c->reallength = mw(pt, h);
        c->finetune = mb(pt, h + 2);
        c->volume = mb(pt, h + 3);
        uint16_t rep = mw(pt, h + 4);
        if (rep) {
            c->loopstart = c->start + (uint32_t)(uint16_t)(rep * 2);   /* asl.w */
            c->wavestart = c->loopstart;
            c->length = (uint16_t)(rep + mw(pt, h + 6));
            c->replen = mw(pt, h + 6);
        } else {
            c->loopstart = c->start;
            c->wavestart = c->loopstart;
            c->replen = mw(pt, h + 6);
        }
        set_vol(pt, c, faded(pt, c->volume));
    }
    if ((note_word(c) & 0xFFF) == 0) { checkmorefx(pt, c); return; }
    uint16_t cw = cmd_word(c);
    int cmd = c->note[2] & 0xF;
    if ((cw & 0xFF0) == 0xE50) c->finetune = c->note[3] & 0xF;      /* sub_101AB6 */
    else if (cmd == 3 || cmd == 5) {                                /* sub_101696 */
        uint16_t d2 = note_word(c) & 0xFFF;
        bool f;
        int base = c->finetune * 37;                                /* mulu #$4a */
        int k = scan(base, d2, 37, &f);
        int idx = f ? base + k : base + k - 1;                      /* not found: subq.w #2,a0 */
        /* the finetune-8 test only decrements a loop counter (subq.w #2,d0): no effect */
        c->wanted = period_at(idx);
        c->tp_dir = 0;
        if (c->wanted == c->period) c->wanted = 0;
        else if (!((int16_t)c->wanted >= (int16_t)c->period)) c->tp_dir = 1;
        checkmorefx(pt, c);
        return;
    } else if (cmd == 9) checkmorefx(pt, c);
    /* setperiod 0x101376 */
    bool f;
    int k = scan(0, note_word(c) & 0xFFF, 37, &f);
    c->period = period_at(k + c->finetune * 36);
    if ((cw & 0xFF0) == 0xED0) { checkmorefx(pt, c); return; }
    paula_dma(pt->paula, c->dmabit);
    if (!(c->wavecontrol & 4)) c->vib_pos = 0;
    if (!(c->wavecontrol & 0x40)) c->trem_pos = 0;
    paula_set_ptr(pt->paula, ci(pt, c), at(pt, c->start));
    paula_set_len(pt->paula, ci(pt, c), c->length);
    set_per(pt, c, c->period);
    pt->dmacontemp |= c->dmabit;
    checkmorefx(pt, c);
}

static void getnew(sk_pt *pt) {                                     /* 0x101202 */
    uint32_t d1 = (uint32_t)mb(pt, 0x3B8 + pt->songpos) * 1024 + pt->patternpos;
    pt->dmacontemp = 0;
    for (int i = 0; i < nch(pt); i++) { playvoice(pt, &pt->ch[i], 0x43C + d1); d1 += 4; }
    pt->phases = 0xFF;
}

static void updatefunk(sk_pt *pt, sk_ptch *c) {                     /* sub_101C3A */
    int f = c->glissfunk >> 4;
    if (!f) return;
    c->funkoffset = (uint8_t)(c->funkoffset + pt_funk[f]);
    if (!(c->funkoffset & 0x80)) return;
    c->funkoffset = 0;
    uint32_t end = c->loopstart + (uint32_t)c->replen * 2u;
    uint32_t a = c->wavestart + 1;
    if (!(a < end)) a = c->loopstart;
    c->wavestart = a;
    if (a < pt->n) pt->mod[a] = (uint8_t)~pt->mod[a];
}

static void portaup(sk_pt *pt, sk_ptch *c) {                        /* 0x101604 */
    int d0 = c->note[3] & pt->lowmask;
    pt->lowmask = 0xFF;
    c->period = (uint16_t)(c->period - d0);
    if ((int16_t)((c->period & 0xFFF) - 0x71) < 0) c->period = (uint16_t)((c->period & 0xF000) | 0x71);
    set_per(pt, c, c->period & 0xFFF);
}
static void portadown(sk_pt *pt, sk_ptch *c) {                      /* 0x101656 */
    int d0 = c->note[3] & pt->lowmask;
    pt->lowmask = 0xFF;
    c->period = (uint16_t)(c->period + d0);
    if (!((int16_t)((c->period & 0xFFF) - 0x358) < 0)) c->period = (uint16_t)((c->period & 0xF000) | 0x358);
    set_per(pt, c, c->period & 0xFFF);
}

static void toneslide(sk_pt *pt, sk_ptch *c) {                      /* sub_1016FE */
    if (!c->wanted) return;
    int d0 = c->tp_speed;
    if (c->tp_dir) {
        c->period = (uint16_t)(c->period - d0);
        if (!((int16_t)c->wanted < (int16_t)c->period)) { c->period = c->wanted; c->wanted = 0; }
    } else {
        c->period = (uint16_t)(c->period + d0);
        if (!((int16_t)c->wanted > (int16_t)c->period)) { c->period = c->wanted; c->wanted = 0; }
    }
    uint16_t d2 = c->period;
    if (c->glissfunk & 0xF) {
        bool f;
        int k = scan(c->finetune * 36, d2, 36, &f);
        d2 = period_at(c->finetune * 36 + k);
    }
    set_per(pt, c, d2);
}

static int wave(uint8_t pos, int form) {                            /* the vibrato/tremolo shape lookup */
    int d0 = (pos >> 2) & 0x1F;
    if (!form) return pt_sine[d0];
    d0 = (uint8_t)(d0 << 3);
    if (form == 1) return (int8_t)pos < 0 ? 255 - d0 : d0;          /* ramp: by the position's sign */
    return 255;
}

static void vibrato(sk_pt *pt, sk_ptch *c) {                        /* sub_10179C */
    int d2 = wave(c->vib_pos, c->wavecontrol & 3);
    d2 = (uint16_t)(d2 * (c->vib_cmd & 0xF)) >> 7;
    int d0 = (int8_t)c->vib_pos < 0 ? c->period - d2 : c->period + d2;
    set_per(pt, c, (uint16_t)d0);
    c->vib_pos = (uint8_t)(c->vib_pos + ((c->vib_cmd >> 2) & 0x3C));
}

static void tremolo(sk_pt *pt, sk_ptch *c) {                        /* 0x101840 */
    int form = (c->wavecontrol >> 4) & 3;
    int d0 = (c->trem_pos >> 2) & 0x1F, d2;
    if (!form) d2 = pt_sine[d0];
    else {
        d0 = (uint8_t)(d0 << 3);
        if (form == 1) d2 = (int8_t)c->vib_pos < 0 ? 255 - d0 : d0;   /* tst.b $1b(a6): the vibrato position (as the original) */
        else d2 = 255;
    }
    d2 = (uint16_t)(d2 * (c->trem_cmd & 0xF)) >> 6;
    int v = (int8_t)c->trem_pos < 0 ? c->volume - d2 : c->volume + d2;
    if (v < 0) v = 0;
    if ((unsigned)v > 0x40) v = 0x40;
    set_vol(pt, c, faded(pt, v));
    c->trem_pos = (uint8_t)(c->trem_pos + ((c->trem_cmd >> 2) & 0x3C));
}

static void volup(sk_pt *pt, sk_ptch *c, int d0) {                  /* 0x101902 */
    c->volume = (uint8_t)(c->volume + d0);
    if (!((int8_t)(c->volume - 0x40) < 0)) c->volume = 0x40;
    set_vol(pt, c, faded(pt, c->volume));
}
static void voldown(sk_pt *pt, sk_ptch *c, int d0) {                /* 0x101934 */
    c->volume = (uint8_t)(c->volume - d0);
    if ((int8_t)c->volume < 0) c->volume = 0;
    set_vol(pt, c, faded(pt, c->volume));
}
static void volslide(sk_pt *pt, sk_ptch *c) {                       /* 0x1018F6 */
    int d0 = c->note[3] >> 4;
    if (d0) volup(pt, c, d0); else voldown(pt, c, c->note[3] & 0xF);
}

static void doretrig(sk_pt *pt, sk_ptch *c) {                       /* 0x101B50 */
    int i = ci(pt, c);
    paula_dma(pt->paula, c->dmabit);
    paula_set_ptr(pt->paula, i, at(pt, c->start));
    paula_set_len(pt->paula, i, c->length);
    paula_dma(pt->paula, (uint16_t)(c->dmabit | 0x8000));
    paula_set_ptr(pt->paula, i, at(pt, c->loopstart));
    paula_set_len(pt->paula, i, c->replen);                         /* move.l $e(a6),$4(a5): LEN and PER */
    set_per(pt, c, c->period);
}

static void ecommands(sk_pt *pt, sk_ptch *c) {                      /* 0x101A24 */
    int lo = c->note[3] & 0xF;
    switch (c->note[3] >> 4) {
    case 0x0: break;                                                /* the LED filter: not modelled */
    case 0x1: if (pt->counter) break; pt->lowmask = 0x0F; portaup(pt, c); break;
    case 0x2: if (pt->counter) break; pt->lowmask = 0x0F; portadown(pt, c); break;
    case 0x3: c->glissfunk = (uint8_t)((c->glissfunk & 0xF0) | lo); break;
    case 0x4: c->wavecontrol = (uint8_t)((c->wavecontrol & 0xF0) | lo); break;
    case 0x5: c->finetune = (uint8_t)lo; break;
    case 0x6:                                                       /* jump loop */
        if (pt->counter) break;
        if (!lo) { c->pattpos = (uint8_t)(pt->patternpos >> 4); break; }
        if (c->loopcount) { if (--c->loopcount == 0) break; }
        else c->loopcount = (uint8_t)lo;
        pt->pbreakpos = c->pattpos; pt->pbreak = 0xFF;
        break;
    case 0x7: c->wavecontrol = (uint8_t)((c->wavecontrol & 0x0F) | lo << 4); break;
    case 0x8: break;
    case 0x9: {                                                     /* retrigger */
        if (!lo) break;
        if (!pt->counter && (note_word(c) & 0xFFF)) break;
        if (pt->counter % lo) break;
        doretrig(pt, c);
        break;
    }
    case 0xA: if (pt->counter) break; volup(pt, c, lo); break;
    case 0xB: if (pt->counter) break; voldown(pt, c, lo); break;
    case 0xC: if (lo != pt->counter) break; c->volume = 0; set_vol(pt, c, 0); break;
    case 0xD: if (lo != pt->counter || !note_word(c)) break; doretrig(pt, c); break;
    case 0xE: if (pt->counter || pt->pattdelay2) break; pt->pattdelay = (uint8_t)(lo + 1); break;
    case 0xF:
        if (pt->counter) break;
        c->glissfunk = (uint8_t)((c->glissfunk & 0x0F) | lo << 4);
        if (lo) updatefunk(pt, c);
        break;
    }
}

static void checkmorefx(sk_pt *pt, sk_ptch *c) {                    /* sub_1019CE */
    updatefunk(pt, c);
    int p = c->note[3];
    switch (c->note[2] & 0xF) {
    case 0xB: pt->songpos = (uint8_t)(p - 1); pt->pbreakpos = 0; pt->posjump = 0xFF; break;
    case 0xC: { int v = p > 0x40 ? 0x40 : p; c->volume = (uint8_t)v; set_vol(pt, c, faded(pt, v)); break; }
    case 0xD: {
        int d0 = (p >> 4) * 10 + (p & 0xF);
        if (d0 > 0x3F) pt->pbreakpos = 0; else pt->pbreakpos = (uint8_t)d0;
        pt->posjump = 0xFF;
        break;
    }
    case 0xE: ecommands(pt, c); break;
    case 0xF: if (p) { pt->counter = 0; pt->speed = (uint8_t)p; } break;
    default: {                                                      /* 0..A: the 9xx code (0x1019E2: the table sends them all here) */
        if (p) c->sampleoffset = (uint8_t)p;
        int d0 = c->sampleoffset << 7;
        if (d0 < (int16_t)c->length) { c->length = (uint16_t)(c->length - d0); c->start += (uint32_t)d0 * 2; }
        else c->length = 1;
        break;
    }
    }
}

static void checkfx(sk_pt *pt, sk_ptch *c) {                        /* sub_10151C */
    updatefunk(pt, c);
    if (!(cmd_word(c) & 0xFFF)) { set_per(pt, c, c->period); return; }
    switch (c->note[2] & 0xF) {
    case 0x0: {                                                     /* arpeggio */
        int r = pt->counter % 3;
        if (r == 0) { set_per(pt, c, c->period); return; }
        int nib = r == 2 ? c->note[3] & 0xF : c->note[3] >> 4;
        bool f;
        int base = c->finetune * 36, k = scan(base, c->period, 37, &f);
        if (f) set_per(pt, c, period_at(base + k + nib));
        return;
    }
    case 0x1: portaup(pt, c); return;
    case 0x2: portadown(pt, c); return;
    case 0x3: if (c->note[3]) { c->tp_speed = c->note[3]; c->note[3] = 0; } toneslide(pt, c); return;
    case 0x4:
        if (c->note[3]) {
            uint8_t d2 = c->vib_cmd;
            if (c->note[3] & 0xF) d2 = (uint8_t)((d2 & 0xF0) | (c->note[3] & 0xF));
            if (c->note[3] & 0xF0) d2 = (uint8_t)((d2 & 0x0F) | (c->note[3] & 0xF0));
            c->vib_cmd = d2;
        }
        vibrato(pt, c);
        return;
    case 0x5: toneslide(pt, c); volslide(pt, c); return;
    case 0x6: vibrato(pt, c); volslide(pt, c); return;
    case 0xE: ecommands(pt, c); return;
    default: break;
    }
    set_per(pt, c, c->period);
    if ((c->note[2] & 0xF) == 0x7) {
        if (c->note[3]) {
            uint8_t d2 = c->trem_cmd;
            if (c->note[3] & 0xF) d2 = (uint8_t)((d2 & 0xF0) | (c->note[3] & 0xF));
            if (c->note[3] & 0xF0) d2 = (uint8_t)((d2 & 0x0F) | (c->note[3] & 0xF0));
            c->trem_cmd = d2;
        }
        tremolo(pt, c);
    } else if ((c->note[2] & 0xF) == 0xA) volslide(pt, c);
}

static void nextposition(sk_pt *pt) {                               /* 0x1014D0 */
    pt->patternpos = (uint16_t)(pt->pbreakpos << 4);
    pt->pbreakpos = 0;
    pt->posjump = 0;
    pt->songpos = (uint8_t)((pt->songpos + 1) & 0x7F);
    if (!(pt->songpos < mb(pt, 0x3B6))) pt->songpos = 0;
}
static void posjump_check(sk_pt *pt) { while (pt->posjump) nextposition(pt); }   /* 0x101512 */

static void dskip(sk_pt *pt) {                                      /* 0x10146E */
    pt->patternpos = (uint16_t)(pt->patternpos + 16);
    if (pt->pattdelay) { pt->pattdelay2 = pt->pattdelay; pt->pattdelay = 0; }
    if (pt->pattdelay2 && --pt->pattdelay2) pt->patternpos = (uint16_t)(pt->patternpos - 16);
    if (pt->pbreak) { pt->pbreak = 0; pt->patternpos = (uint16_t)(pt->pbreakpos << 4); pt->pbreakpos = 0; }
    if (pt->patternpos >= 0x400) nextposition(pt);
    posjump_check(pt);
}

static void all_checkfx(sk_pt *pt) { for (int i = 0; i < nch(pt); i++) checkfx(pt, &pt->ch[i]); }   /* sub_1011B8 */

void pt_phase1(sk_pt *pt) {                                         /* 0x101180 */
    if (!pt->playing || !pt->paula) return;
    if (pt->master_on && pt->master != pt->master_target) {          /* anim's master_volume_ramp: 2 a tick, then the four volumes */
        int d = pt->master_target - pt->master;
        pt->master += d > 2 ? 2 : d < -2 ? -2 : d;
        for (int i = 0; i < 4; i++) paula_set_vol(pt->paula, i, faded(pt, pt->ch[i].volume));
    }
    pt->counter++;
    if (pt->counter < pt->speed) { all_checkfx(pt); posjump_check(pt); return; }
    pt->counter = 0;
    if (pt->pattdelay2) { all_checkfx(pt); dskip(pt); return; }
    getnew(pt);
}

void pt_phase2(sk_pt *pt) {                                         /* 0x1013EE */
    if (!(pt->phases & 1)) return;
    pt->phases &= (uint8_t)~1;
    if (pt->paula) paula_dma(pt->paula, (uint16_t)(pt->dmacontemp | 0x8000));
}

void pt_phase3(sk_pt *pt) {                                         /* 0x10140C */
    if (!(pt->phases & 2)) return;
    pt->phases &= (uint8_t)~2;
    if (!pt->paula) return;
    for (int i = nch(pt) - 1; i >= 0; i--) {
        paula_set_ptr(pt->paula, i, at(pt, pt->ch[i].loopstart));
        paula_set_len(pt->paula, i, pt->ch[i].replen);
    }
    dskip(pt);
}
void pt_master(sk_pt *pt, bool on) { pt->master_on = on; pt->master = pt->master_target = 100; }
void pt_cue(sk_pt *pt, int position) {                              /* anim: music_cue 0x101706 */
    pt->playing = false;
    pt->songpos = (uint8_t)position;
    pt->speed = 6; pt->counter = pt->speed;
    pt->patternpos = 0; pt->pattdelay2 = 0; pt->phases = 0;
    pt->master_target = 100;
    pt->playing = pt->mod != NULL;
}
