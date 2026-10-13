#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "audio/sound.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "core/tables.h"

static int8_t g_smp[200];
static void fake_sfx(sk_sound *s, int n, uint16_t words, uint16_t per, uint16_t dur) {
    s->sfx[n].mem = (sk_amem){ g_smp, g_smp + sizeof g_smp };
    s->sfx[n].len = words; s->sfx[n].per = per; s->sfx[n].dur = dur;
}

void test_sfx_starts_after_the_handshake(void) {                   /* play_sfx 0x102140, sound_tick 0x102296 */
    static sk_sound s; sound_init(&s, NULL, 48000);
    fake_sfx(&s, 3, 50, 428, 5);
    for (int i = 0; i < 200; i++) g_smp[i] = 60;
    sound_play_sfx(&s, 3);
    sk_paula_ch *c = &s.paula.ch[3];
    CHECK(!c->on); CHECK_EQ(c->vol, 64); CHECK_EQ(c->per, 428); CHECK_EQ(c->reg_len, 50);
    sound_tick(&s);                                                  /* the next VBL: queued */
    CHECK(!c->on);
    sound_tick(&s);                                                  /* the one after: DMA on */
    CHECK(c->on); CHECK(c->cur == g_smp);
    sound_tick(&s);                                                  /* then the silent word as its loop */
    CHECK_EQ(c->reg_len, 1); CHECK(c->reg.p != g_smp);
}

void test_sfx_priority_holds(void) {                                /* Review Focus 3 */
    static sk_sound s; sound_init(&s, NULL, 48000);
    fake_sfx(&s, 1, 50, 300, 10); fake_sfx(&s, 2, 50, 500, 4);
    sound_play_sfx(&s, 1);
    CHECK_EQ(s.priority, 10);
    sound_play_sfx(&s, 2);                                           /* shorter: refused */
    CHECK_EQ(s.paula.ch[3].per, 300);
    for (int i = 0; i < 11; i++) { sound_tick(&s); sound_play_sfx(&s, 1); }   /* repeated: each restarts it (10 >= the frames left) */
    CHECK_EQ(s.priority, 10);
    for (int i = 0; i < 12; i++) sound_tick(&s);
    CHECK_EQ(s.priority, 0);                                         /* run out: anything plays */
    sound_play_sfx(&s, 2);
    CHECK_EQ(s.paula.ch[3].per, 500);
    s.sfx_muted = true;
    sound_play_sfx(&s, 1);
    CHECK_EQ(s.paula.ch[3].per, 500);
}

void test_music_requests_start_on_the_next_frame(void) {
    static sk_sound s; sound_init(&s, NULL, 48000);
    uint8_t *m = calloc(1, 0x43C + 1024 + 16);
    m[0x3B6] = 1; memcpy(m + 0x438, "M.K.", 4);
    sound_set_module(&s, 0, m, 0x43C + 1024 + 16);                    /* the level's module (slot 0, owned) */
    sound_music(&s, SK_MUSIC_LEVEL, 0);
    CHECK(!s.pt.playing);
    static int16_t out[2 * 960];
    sound_frame(&s, out, 960);
    CHECK(s.pt.playing);
    s.music_off = true;
    sound_music(&s, SK_MUSIC_LEVEL, 0);                              /* music switched off: ignored */
    CHECK(!s.request);
    s.music_off = false;
    sound_music(&s, SK_MUSIC_JINGLE, 1);                             /* no jingle module loaded: the music stops */
    sound_frame(&s, out, 960);
    CHECK(!s.pt.playing);
    sound_free(&s);
}

void test_music_slot_swaps_once(void) {                             /* sub_103770 */
    static sk_sound s; sound_init(&s, NULL, 48000);
    size_t n = 0x43C + 1024;
    uint8_t *a = calloc(1, n), *b = calloc(1, n);
    a[0x3B6] = 1; b[0x3B6] = 1; memcpy(a + 0x438, "M.K.", 4); memcpy(b + 0x438, "M.K.", 4); b[0] = 'B';
    sound_set_module(&s, 0, a, n); sound_set_module(&s, 1, b, n);
    sound_music_slot(&s, 1);
    CHECK(s.slot[1] == NULL); CHECK(s.slot[0] != NULL); CHECK_EQ(s.slot[0][0], 'B');
    CHECK(s.request);
    sound_music_slot(&s, 1);                                         /* used up: nothing */
    CHECK_EQ(s.slot[0][0], 'B');
    sound_free(&s);
}

#include <stdio.h>

void test_fade_keeps_the_music_playing(void) {
    static sk_sound s; sound_init(&s, NULL, 48000);
    size_t n = 0x43C + 1024 + 64;
    uint8_t *m = calloc(1, n);
    m[0x3B6] = 1; memcpy(m + 0x438, "M.K.", 4);
    m[20 + 23] = 32; m[20 + 25] = 64; m[20 + 29] = 1;               /* sample 1: 32 words, volume 64 */
    m[0x43C] = 0x01; m[0x43D] = 0xAC; m[0x43E] = 0x10;              /* row 0: sample 1, period 428 */
    sound_set_module(&s, 0, m, n);
    sound_music(&s, SK_MUSIC_LEVEL, 0);
    sound_frame(&s, NULL, 0);
    CHECK_EQ(s.paula.ch[0].vol, 64);
    sound_fade_out(&s);
    for (int i = 0; i < 10; i++) sound_frame(&s, NULL, 0);
    CHECK_EQ(s.paula.ch[0].vol, 64 - 10); CHECK(s.pt.playing);
    for (int i = 0; i < 70; i++) sound_frame(&s, NULL, 0);
    CHECK_EQ(s.paula.ch[0].vol, 0); CHECK(s.pt.playing);
    sound_free(&s);
}

void test_level_music_and_effects_load(void) {                       /* ROM-backed: indices 0..11 */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_image img = {0}; sk_tables t = {0}; sk_err e = {0};
    uint8_t *f = NULL; size_t n = 0;
    CHECK(disks_open(&d, roms, &e) && disks_read(&d, 0, "sk.pak", &f, &n, &e));
    CHECK(hunk_load(f, n, &img, &e)); free(f);
    CHECK(tables_load(&t, &img, &e));
    static sk_sound s; sound_init(&s, &img, 48000);
    for (int i = 0; i < 15; i++) CHECK(s.sfx[i].mem.p != NULL);     /* the built-in effects */
    CHECK(s.jingle_src != NULL);
    for (int lv = 0; lv <= 11; lv++) {
        if (lv == 12) continue;
        CHECK(sound_load_level(&s, &d, &t, lv, &e));
        CHECK(s.slot[0] != NULL);
        const sk_level_record *r = &t.record[lv];
        for (int k = 0; k < r->n_sfx; k++) {
            const sk_sfx_entry *x = tables_sfx(&t, r->disk, r->sfx[k]);
            CHECK(x && s.sfx[x->effect].mem.p != NULL);
            CHECK(s.sfx[x->effect].mem.p + (size_t)s.sfx[x->effect].len * 2 <= s.sfx[x->effect].mem.end);   /* the sample fits its section */
        }
        for (int k = 0; k < r->n_extra; k++) CHECK(s.slot[k + 1] != NULL || r->extra[k] < 80);   /* COUNTRY2's stray file 1 */
    }
    sound_music(&s, SK_MUSIC_LEVEL, 0);
    static int16_t out[2 * 960];
    long energy = 0;
    for (int i = 0; i < 150; i++) { sound_frame(&s, out, 960); for (int k = 0; k < 2 * 960; k += 16) energy += out[k] < 0 ? -out[k] : out[k]; }
    CHECK(energy > 0);
    sound_free(&s);
    tables_free(&t); image_free(&img); disks_close(&d);
}
