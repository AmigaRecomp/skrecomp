/* The game's ProTracker replay (music_start 0x100FCC, the tick 0x101180..0x101CC0), transcribed. It drives an sk_paula
   the way the original writes the audio registers. The tick runs in three phases each frame (0x100F6E): rows and
   effects, DMA on, the loop registers and the row advance. */
#ifndef SK_AUDIO_PTREPLAY_H
#define SK_AUDIO_PTREPLAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "audio/paula.h"

typedef struct {                     /* a channel record (0x200322 + 0x2C n) */
    uint8_t note[4];                 /* +0x00 the pattern cell */
    uint32_t start;                  /* +0x04 n_start (offset into the module) */
    uint16_t length;                 /* +0x08 */
    uint32_t loopstart;              /* +0x0A */
    uint16_t replen;                 /* +0x0E */
    uint16_t period;                 /* +0x10 */
    uint8_t finetune, volume;        /* +0x12, +0x13 */
    uint16_t dmabit;                 /* +0x14 */
    uint8_t tp_dir, tp_speed;        /* +0x16, +0x17 */
    uint16_t wanted;                 /* +0x18 */
    uint8_t vib_cmd, vib_pos, trem_cmd, trem_pos, wavecontrol, glissfunk;   /* +0x1A..+0x1F */
    uint8_t sampleoffset, pattpos, loopcount, funkoffset;                   /* +0x20..+0x23 */
    uint32_t wavestart;              /* +0x24 */
    uint16_t reallength;             /* +0x28 */
} sk_ptch;

typedef struct {
    sk_paula *paula;
    uint8_t *mod; size_t n;          /* the module, writable (EFx inverts sample bytes) */
    bool playing;                    /* music_playing 0x200308 */
    bool use4;                       /* 0x200314: the music has channel 4 too */
    int fade;                        /* 0x200316: subtracted from every volume */
    uint8_t speed, counter, songpos, pbreakpos, posjump, pbreak, lowmask, pattdelay, pattdelay2;   /* 0x200452.. */
    uint16_t patternpos, dmacontemp; /* 0x20045C, 0x20045E */
    uint8_t phases;                  /* 0x200309: bit 0 phase 2 pending, bit 1 phase 3 */
    uint32_t samplestarts[31];       /* 0x2003D2 */
    sk_ptch ch[4];
} sk_pt;

void pt_init(sk_pt *pt, sk_paula *paula);   /* the static channel records (DMA bits 1 2 4 8) */
/* music_start: tune = the start position. False (nothing plays) when the buffer is too short for a module. */
bool pt_start(sk_pt *pt, sk_paula *paula, uint8_t *mod, size_t n, int tune, bool use4);
void pt_stop(sk_pt *pt);                    /* music_stop 0x10113C */
void pt_phase1(sk_pt *pt);                  /* 0x101180 */
void pt_phase2(sk_pt *pt);                  /* 0x1013EE */
void pt_phase3(sk_pt *pt);                  /* 0x10140C */

#endif
