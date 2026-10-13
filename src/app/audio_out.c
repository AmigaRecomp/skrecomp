#include "app/audio_out.h"
#include <SDL3/SDL.h>
#include <string.h>

bool audio_out_open(sk_audio_out *o, int rate) {
    memset(o, 0, sizeof *o);
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, rate };
    SDL_AudioStream *s = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!s) { SDL_QuitSubSystem(SDL_INIT_AUDIO); return false; }
    o->stream = s; o->rate = rate;
    static const int16_t silence[2 * 960 * 2];
    SDL_PutAudioStreamData(s, silence, sizeof silence);         /* two frames ahead: the steps are not exactly even */
    SDL_ResumeAudioStreamDevice(s);
    return true;
}

int audio_out_wants(int queued_bytes, int frame_bytes) {
    if (queued_bytes == 0) return 2;                                /* ran dry (a stall): one frame of slack back */
    if (queued_bytes >= frame_bytes * 4) return 0;                  /* ahead: drop this frame, latency stays near 3 frames */
    return 1;
}

void audio_out_push(sk_audio_out *o, const int16_t *frames, int n) {
    if (!o || !o->stream) return;
    SDL_AudioStream *s = o->stream;
    int bytes = n * 4;
    int want = audio_out_wants(SDL_GetAudioStreamQueued(s), bytes);
    static const int16_t silence[2 * 960];
    if (want == 2 && bytes <= (int)sizeof silence) SDL_PutAudioStreamData(s, silence, bytes);
    if (want >= 1) SDL_PutAudioStreamData(s, frames, bytes);
}

void audio_out_mute(sk_audio_out *o, bool muted) {
    if (!o || !o->stream) return;
    o->muted = muted;
    SDL_SetAudioStreamGain(o->stream, muted ? 0.0f : 1.0f);
}

void audio_out_close(sk_audio_out *o) {
    if (!o || !o->stream) return;
    SDL_DestroyAudioStream(o->stream);
    o->stream = NULL;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
