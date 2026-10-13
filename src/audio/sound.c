/* The game's sound driver (see sound.h), transcribed. */
#include "audio/sound.h"
#include <stdlib.h>
#include <string.h>
#include "core/bytekiller.h"
#include "formats/sections.h"

#define SFX_TABLE 0x2004E8
#define JINGLE_ADDR 0x2046C8
#define JINGLE_LEN 0x3338

static const int8_t silent[2];                                      /* dat_553170: a one-word silent sample */

static uint32_t be32p(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static uint16_t be16p(const uint8_t *p) { return (uint16_t)(p[0] << 8 | p[1]); }

void sound_init(sk_sound *s, const sk_image *sk, int rate) {
    memset(s, 0, sizeof *s);
    paula_init(&s->paula, rate);
    pt_init(&s->pt, &s->paula);
    if (!sk) return;
    const uint8_t *t = image_ptr(sk, SFX_TABLE, SK_SFX_COUNT * 10);
    for (int i = 0; t && i < SK_SFX_COUNT; i++) {
        const uint8_t *q = t + 10 * i;
        s->sfx[i].len = be16p(q + 4); s->sfx[i].per = be16p(q + 6); s->sfx[i].dur = be16p(q + 8);
        uint32_t p = be32p(q);
        const uint8_t *m = p ? image_ptr(sk, p, (size_t)s->sfx[i].len * 2) : NULL;
        if (m) s->sfx[i].mem = (sk_amem){ (const int8_t *)m, (const int8_t *)m + (size_t)s->sfx[i].len * 2 };
    }
    s->jingle_src = image_ptr(sk, JINGLE_ADDR, JINGLE_LEN);
}

static void free_level_sfx(sk_sound *s) {                           /* sub_1031D2 */
    for (int i = 0; i < SK_SFX_COUNT; i++)
        if (s->sfx_owned[i]) { free(s->sfx_owned[i]); s->sfx_owned[i] = NULL; s->sfx[i].mem = (sk_amem){ NULL, NULL }; }
}
static void free_slots(sk_sound *s) {
    for (int i = 0; i < 4; i++) { free(s->slot[i]); s->slot[i] = NULL; s->slot_n[i] = 0; }
}

void sound_free(sk_sound *s) {
    pt_stop(&s->pt);
    s->pt.mod = NULL; s->pt.n = 0;
    free_level_sfx(s);
    free_slots(s);
    sound_jingle_free(s);
}

void sound_set_module(sk_sound *s, int slot, uint8_t *mod, size_t n) {
    slot &= 3;
    if (s->slot[slot] && s->pt.mod == s->slot[slot]) { pt_stop(&s->pt); s->pt.mod = NULL; s->pt.n = 0; }
    free(s->slot[slot]);
    s->slot[slot] = mod; s->slot_n[slot] = mod ? n : 0;
}

static bool load_module(sk_disks *d, int disk, int file, uint8_t **out, size_t *n, sk_err *e) {
    if (!disks_read_file_no(d, disk, file, true, out, n, e)) return false;
    if (*n < 0x43C) { free(*out); *out = NULL; return sk_fail(e, "file%03d: not a module", file); }
    return true;
}

bool sound_load_level(sk_sound *s, sk_disks *d, const sk_tables *t, int data_index, sk_err *e) {   /* load_level_resources 0x102FB2.. */
    if (data_index < 0 || data_index >= SK_LEVEL_RECORDS) return sk_fail(e, "level %d: no record", data_index);
    const sk_level_record *r = &t->record[data_index];
    int disk = r->disk + 1;
    sound_stop(s);
    s->pt.mod = NULL; s->pt.n = 0;
    s->request = false;
    free_slots(s);
    free_level_sfx(s);
    for (int k = 0; k < r->n_sfx; k++) {                            /* sub_10307E */
        const sk_sfx_entry *x = tables_sfx(t, r->disk, r->sfx[k]);
        if (!x || x->effect >= SK_SFX_COUNT) continue;
        uint8_t *f = NULL; size_t n = 0;
        if (!disks_read_file_no(d, disk, x->file, false, &f, &n, e)) return false;
        sk_sections secs = {0};
        bool ok = sections_parse(f, n, &secs, e);
        free(f);
        if (!ok) return false;
        if (x->section < secs.count && secs.sec[x->section].len > 0x3E) {
            sk_section *sec = &secs.sec[x->section];
            uint8_t *copy = malloc(sec->len);
            if (!copy) { sections_free(&secs); return sk_fail(e, "out of memory"); }
            memcpy(copy, sec->data, sec->len);
            s->sfx_owned[x->effect] = copy;
            s->sfx[x->effect].mem = (sk_amem){ (const int8_t *)copy + 0x3E, (const int8_t *)copy + sec->len };   /* past the 0x3E-byte header */
        }
        sections_free(&secs);
    }
    if (!load_module(d, disk, r->music, &s->slot[0], &s->slot_n[0], e)) return false;
    for (int k = 0; k < r->n_extra && k < 3; k++) {
        sk_err ignored = {0};                                       /* COUNTRY2 lists file 1 (its level) here: the original keeps it packed and never plays it */
        if (!load_module(d, disk, r->extra[k], &s->slot[k + 1], &s->slot_n[k + 1], &ignored)) { s->slot[k + 1] = NULL; s->slot_n[k + 1] = 0; }
    }
    return true;
}

static void start_channel3(sk_sound *s, const sk_sfx *x) {          /* sfx_start_channel3 0x1021A6 */
    paula_dma(&s->paula, 0x0008);
    s->hs_260 |= 0x8008;
    paula_set_vol(&s->paula, 3, 64);
    paula_set_len(&s->paula, 3, x->len);
    paula_set_per(&s->paula, 3, x->per);
    paula_set_ptr(&s->paula, 3, x->mem);
}

void sound_play_sfx(sk_sound *s, int n) {                           /* play_sfx 0x102140 */
    if (s->sfx_muted || s->stop_request || n < 0 || n >= SK_SFX_COUNT) return;
    const sk_sfx *x = &s->sfx[n];
    if (x->dur < s->priority) return;
    s->priority = x->dur;
    s->frames_left = (int8_t)x->dur;
    start_channel3(s, x);
}

void sound_tick(sk_sound *s) {                                      /* sound_tick 0x102296 */
    if (--s->frames_left < 0) { s->priority = 0; s->frames_left = 0; }
    if (s->stop_request) {
        if (s->stop_request & 1) {
            s->stop_request &= (uint8_t)~1;
            for (int i = 0; i < 4; i++) paula_set_vol(&s->paula, i, 0);
            paula_dma(&s->paula, 0x000F);
        }
        return;
    }
    if (s->hs_258) paula_dma(&s->paula, s->hs_258);
    s->hs_258 = s->hs_25a;
    s->hs_25a = 0;
    if (!s->hs_25c) {
        if (s->hs_25e) { paula_dma(&s->paula, s->hs_25e); s->hs_25c = true; return; }
        if (s->hs_260) { s->hs_25e = s->hs_260; s->hs_260 = 0; }
        return;
    }
    for (int i = 0; i < 4; i++)
        if (s->hs_25e & (1u << i)) { paula_set_ptr(&s->paula, i, (sk_amem){ silent, silent + 2 }); paula_set_len(&s->paula, i, 1); }
    s->hs_25e = 0;
    s->hs_25c = false;
}

void sound_music(sk_sound *s, int module, int tune) {               /* music_request 0x100F80 */
    if (s->music_off) return;
    s->request = true;
    s->req_module = module;
    s->req_tune = tune;
}

void sound_music_slot(sk_sound *s, int slot) {                      /* sub_103770 */
    if (slot < 1 || slot > 3 || !s->slot[slot]) return;
    if (s->pt.mod == s->slot[0]) { pt_stop(&s->pt); s->pt.mod = NULL; s->pt.n = 0; }
    free(s->slot[0]);
    s->slot[0] = s->slot[slot]; s->slot_n[0] = s->slot_n[slot];
    s->slot[slot] = NULL; s->slot_n[slot] = 0;
    sound_music(s, SK_MUSIC_LEVEL, 0);
}

void sound_jingle_load(sk_sound *s) {                               /* sub_1037C4 */
    if (!s->jingle_src) return;
    sound_jingle_free(s);
    s->jingle = calloc(1, JINGLE_LEN + 4);
    if (!s->jingle) return;
    memcpy(s->jingle, s->jingle_src, JINGLE_LEN);
    s->jingle_n = JINGLE_LEN + 4;
}

void sound_jingle_free(sk_sound *s) {
    if (s->jingle && s->pt.mod == s->jingle) { pt_stop(&s->pt); s->pt.mod = NULL; s->pt.n = 0; }
    free(s->jingle);
    s->jingle = NULL; s->jingle_n = 0;
}

void sound_stop(sk_sound *s) { pt_stop(&s->pt); s->fading = 0; }

void sound_fade_out(sk_sound *s) { if (!s->fading) s->fading = 1; }

void sound_frame(sk_sound *s, int16_t *out, int frames) {
    sound_tick(s);                                                  /* frame loop step 6 */
    if (s->request) {                                               /* step 7 */
        s->request = false;
        uint8_t *m = s->req_module == SK_MUSIC_JINGLE ? s->jingle : s->slot[0];
        size_t n = s->req_module == SK_MUSIC_JINGLE ? s->jingle_n : s->slot_n[0];
        if (!s->music_off) { s->fading = 0; pt_start(&s->pt, &s->paula, m, n, s->req_tune, false); }
    }
    pt_phase1(&s->pt);                                              /* step 8: the CIA chain */
    pt_phase2(&s->pt);
    pt_phase3(&s->pt);
    if (s->fading) {                                                /* fade_music_out: the volumes less one more each frame, the music playing on */
        s->pt.fade = s->fading < 64 ? s->fading : 64;
        for (int i = 0; i < (s->pt.use4 ? 4 : 3); i++) {
            int v = s->pt.ch[i].volume - s->pt.fade;
            paula_set_vol(&s->paula, i, (uint16_t)(v < 0 ? 0 : v));
        }
        if (s->fading < 64) s->fading++;
    }
    if (out && frames > 0) paula_render(&s->paula, out, frames);
}
