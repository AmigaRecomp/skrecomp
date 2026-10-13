/* The sound device: an SDL3 audio stream fed 1/50 s per game step (48 kHz stereo, 16-bit). */
#ifndef SK_APP_AUDIO_OUT_H
#define SK_APP_AUDIO_OUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct sk_audio_out {
    void *stream;                    /* SDL_AudioStream */
    int rate;
    bool muted;
} sk_audio_out;

bool audio_out_open(sk_audio_out *o, int rate);   /* false: no device (the game runs silent) */
void audio_out_push(sk_audio_out *o, const int16_t *frames, int n);
void audio_out_mute(sk_audio_out *o, bool muted);
void audio_out_close(sk_audio_out *o);

#endif
