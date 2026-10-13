/* The game's sound driver: play_sfx (0x102140) and sound_tick (0x102296) on channel 4, the music module slots
   (music_module_ptr 0x3000FA, sub_103770), the jingle module (0x30002C, sub_1037C4), music requests started on the
   next frame (music_request 0x100F80 / music_start_if_enabled 0x100F9C), and the per-frame order of the VBL. */
#ifndef SK_AUDIO_SOUND_H
#define SK_AUDIO_SOUND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "audio/paula.h"
#include "audio/ptreplay.h"
#include "core/disks.h"
#include "core/err.h"
#include "core/hunk.h"
#include "core/tables.h"

#define SK_SFX_COUNT 26
enum { SK_MUSIC_LEVEL = 0, SK_MUSIC_JINGLE = 1 };

typedef struct { sk_amem mem; uint16_t len, per, dur; } sk_sfx;   /* sfx_table 0x2004E8: pointer, words, period, duration */

typedef struct sk_sound {
    sk_paula paula;
    sk_pt pt;
    sk_sfx sfx[SK_SFX_COUNT];
    uint8_t *sfx_owned[SK_SFX_COUNT];                                /* the per-level samples (15..25) */
    int8_t frames_left;                                             /* sfx_frames_left 0x2004E2 */
    uint16_t priority;                                              /* sfx_priority 0x2004E6 */
    bool sfx_muted, music_off;                                      /* 0x300368, 0x300367 */
    uint8_t stop_request;                                           /* sfx_stop_request 0x2004E4 */
    uint16_t hs_258, hs_25a, hs_25e, hs_260; bool hs_25c;           /* the DMA handshake words 0x300258.. */
    uint8_t *slot[4]; size_t slot_n[4];                             /* music_module_ptr: 0 = the playing module */
    uint8_t *jingle; size_t jingle_n;                               /* 0x30002C */
    const uint8_t *jingle_src;                                      /* 0x2046C8 in sk */
    bool request; int req_module, req_tune;                         /* music_request_flag, 0x20030C (as a kind), 0x200310 */
    int fading;                                                     /* fade_music_out: frames into the fade, 0 = none */
} sk_sound;

void sound_init(sk_sound *s, const sk_image *sk, int rate);         /* effects 0..14 and the jingle from sk (NULL: none) */
void sound_free(sk_sound *s);
/* A level's music (record +43, +44..46) and effect samples (+47 through table 2), replacing the last level's. */
bool sound_load_level(sk_sound *s, sk_disks *d, const sk_tables *t, int data_index, sk_err *e);
void sound_free_level_sfx(sk_sound *s);                            /* sub_1031D2: the per-level effect samples */
void sound_set_module(sk_sound *s, int slot, uint8_t *mod, size_t n);   /* takes ownership */

void sound_play_sfx(sk_sound *s, int n);                            /* play_sfx */
void sound_tick(sk_sound *s);                                       /* sound_tick (the VBL's effect step) */
void sound_music(sk_sound *s, int module, int tune);                /* music_request */
void sound_music_slot(sk_sound *s, int slot);                       /* sub_103770: slot 1..3 becomes the level module, tune 0 */
void sound_jingle_load(sk_sound *s);                                /* sub_1037C4 */
void sound_jingle_free(sk_sound *s);                                /* 0x111C90 */
void sound_stop(sk_sound *s);                                       /* music_stop */
void sound_fade_out(sk_sound *s);                                   /* fade_music_out: 64 frames */
void sound_frame(sk_sound *s, int16_t *out, int frames);            /* one VBL: the effects, a pending start, the replay, the mix */

#endif
