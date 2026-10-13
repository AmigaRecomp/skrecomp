/* A software model of the Amiga's four audio channels as the game drives them (no emulation of the chip: the
   registers it writes, the latch at DMA start and at each block end, and a 48 kHz stereo mix). */
#ifndef SK_AUDIO_PAULA_H
#define SK_AUDIO_PAULA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SK_PAULA_CLOCK 3546895u      /* PAL */

typedef struct { const int8_t *p, *end; } sk_amem;   /* sample memory: a pointer and the end of its buffer (reads stop there) */

typedef struct {
    sk_amem reg;                     /* AUDxLC: what the next block starts from */
    uint16_t reg_len;                /* AUDxLEN, words (0 = 65536) */
    uint16_t per, vol;               /* AUDxPER, AUDxVOL (0..64) */
    bool on;                         /* DMA running */
    const int8_t *cur, *cur_end;     /* the block playing, latched */
    uint32_t cur_len;                /* bytes */
    uint64_t pos;                    /* 32.32 bytes into the block */
} sk_paula_ch;

typedef struct {
    sk_paula_ch ch[4];
    int rate;                        /* output frames per second */
} sk_paula;

void paula_init(sk_paula *p, int rate);
void paula_set_ptr(sk_paula *p, int c, sk_amem m);
void paula_set_len(sk_paula *p, int c, uint16_t words);
void paula_set_per(sk_paula *p, int c, uint16_t period);
void paula_set_vol(sk_paula *p, int c, uint16_t vol);
void paula_dma(sk_paula *p, uint16_t dmacon);   /* DMACON write: bit 15 set/clear, bits 0..3 the channels */
void paula_render(sk_paula *p, int16_t *out, int frames);   /* interleaved stereo; channels 1 and 4 left, 2 and 3 right */

#endif
