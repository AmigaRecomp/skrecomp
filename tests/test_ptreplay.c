#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "audio/paula.h"
#include "audio/ptreplay.h"

/* A synthetic ProTracker module: 2 patterns, sample 1 = 64 bytes (no loop), sample 2 = 32 + loop 16..32. */
static uint8_t *g_mod; static size_t g_n;
static void cell(int pat, int row, int ch, int sample, int period, int cmd, int param) {
    uint8_t *c = g_mod + 0x43C + (size_t)pat * 1024 + (size_t)row * 16 + (size_t)ch * 4;
    c[0] = (uint8_t)((sample & 0xF0) | (period >> 8)); c[1] = (uint8_t)period;
    c[2] = (uint8_t)((sample & 0x0F) << 4 | cmd); c[3] = (uint8_t)param;
}
static void make_mod(int song_len) {
    g_n = 0x43C + 2 * 1024 + 64 + 32;
    free(g_mod); g_mod = calloc(1, g_n);
    uint8_t *s1 = g_mod + 20, *s2 = g_mod + 50;
    s1[22] = 0; s1[23] = 32; s1[24] = 0; s1[25] = 64; s1[26] = 0; s1[27] = 0; s1[28] = 0; s1[29] = 1;   /* 32 words, vol 64 */
    s2[22] = 0; s2[23] = 16; s2[24] = 0; s2[25] = 40; s2[26] = 0; s2[27] = 8; s2[28] = 0; s2[29] = 8;   /* 16 words, loop 8 + 8 */
    g_mod[0x3B6] = (uint8_t)song_len; g_mod[0x3B8] = 0; g_mod[0x3B9] = 1; g_mod[0x3BA] = 0;
    memcpy(g_mod + 0x438, "M.K.", 4);
    for (int i = 0; i < 96; i++) g_mod[0x43C + 2048 + i] = (uint8_t)(i * 3);
}
static sk_paula g_p; static sk_pt g_pt;
static void frame(void) { pt_phase1(&g_pt); pt_phase2(&g_pt); pt_phase3(&g_pt); }

void test_replay_starts_a_note(void) {
    make_mod(2);
    cell(0, 0, 0, 1, 428, 0, 0);
    paula_init(&g_p, 48000);
    CHECK(pt_start(&g_pt, &g_p, g_mod, g_n, 0, false));
    CHECK_EQ(g_pt.speed, 6);
    frame();
    CHECK(g_p.ch[0].on); CHECK_EQ(g_p.ch[0].per, 428); CHECK_EQ(g_p.ch[0].vol, 64);
    CHECK(g_p.ch[0].cur == (const int8_t *)g_mod + 0x43C + 2048);
    CHECK_EQ(g_pt.patternpos, 16);                                   /* the row advanced in phase 3 */
    CHECK_EQ(g_mod[0x43C + 2048], 0);                                /* music_start clears each sample's first long */
    CHECK_EQ(g_p.ch[0].reg_len, 1);                                  /* no loop: the one-word repeat */
}

void test_replay_speed_volume_and_jump(void) {
    make_mod(2);
    cell(0, 0, 1, 2, 214, 0xF, 3);                                   /* speed 3 */
    cell(0, 1, 1, 0, 0, 0xC, 20);                                    /* volume 20 */
    cell(0, 2, 1, 0, 0, 0xB, 1);                                     /* jump to position 1 */
    cell(1, 0, 1, 0, 0, 0xC, 0x7F);                                  /* volume capped at 64 */
    paula_init(&g_p, 48000);
    CHECK(pt_start(&g_pt, &g_p, g_mod, g_n, 0, false));
    frame();
    CHECK_EQ(g_pt.speed, 3); CHECK_EQ(g_p.ch[1].vol, 40);
    CHECK(g_p.ch[1].reg.p == (const int8_t *)g_mod + 0x43C + 2048 + 64 + 16);   /* the loop start latched next */
    CHECK_EQ(g_p.ch[1].reg_len, 8);
    for (int i = 0; i < 3; i++) frame();
    CHECK_EQ(g_p.ch[1].vol, 20);
    for (int i = 0; i < 3; i++) frame();                             /* row 2: the jump */
    CHECK_EQ(g_pt.songpos, 1); CHECK_EQ(g_pt.patternpos, 0);
    for (int i = 0; i < 3; i++) frame();
    CHECK_EQ(g_p.ch[1].vol, 64);
}

void test_replay_fade_and_channel_four(void) {
    make_mod(1);
    cell(0, 0, 3, 1, 428, 0, 0);
    paula_init(&g_p, 48000);
    CHECK(pt_start(&g_pt, &g_p, g_mod, g_n, 0, false));
    frame();
    CHECK(!g_p.ch[3].on);                                            /* channel 4 is the effects' */
    paula_init(&g_p, 48000);
    CHECK(pt_start(&g_pt, &g_p, g_mod, g_n, 0, true));
    g_pt.fade = 10;
    frame();
    CHECK(g_p.ch[3].on); CHECK_EQ(g_p.ch[3].vol, 54);
}

void test_replay_new_note_quirk_applies_the_offset(void) {          /* 0x1019E2: effects 0..A on a new note run the 9xx code */
    make_mod(1);
    cell(0, 0, 0, 1, 428, 0x0, 0x01);                                /* arpeggio 01 with a note: offset 256 bytes > 64: length 1 */
    paula_init(&g_p, 48000);
    CHECK(pt_start(&g_pt, &g_p, g_mod, g_n, 0, false));
    frame();
    CHECK_EQ(g_pt.ch[0].length, 1);
}

void test_replay_clamps_to_the_module(void) {                       /* Review Focus 1 */
    make_mod(1);
    g_mod[0x3B8] = 60;                                               /* a pattern far past the file */
    g_mod[20 + 23] = 0xFF; g_mod[20 + 22] = 0x7F;                    /* a sample longer than the file */
    cell(0, 0, 0, 1, 428, 0, 0);
    paula_init(&g_p, 48000);
    CHECK(pt_start(&g_pt, &g_p, g_mod, g_n, 0, false));
    for (int i = 0; i < 400; i++) frame();
    int16_t out[2 * 960];
    paula_render(&g_p, out, 960);
    CHECK(!pt_start(&g_pt, &g_p, g_mod, 100, 0, false));             /* too short for a module */
    free(g_mod); g_mod = NULL;
}

#include <stdio.h>
#include "core/disks.h"

void test_all_modules_play(void) {                                   /* ROM-backed: files 80..90 on disks 1..4, 30 s each */
    const char *roms = getenv("SOCCERKID_ROMS");
    if (!roms) { puts("skipped: SOCCERKID_ROMS not set"); return; }
    sk_disks d = {0}; sk_err e = {0};
    CHECK(disks_open(&d, roms, &e));
    int played = 0;
    for (int disk = 1; disk <= 4; disk++)
        for (int f = 80; f <= 90; f++) {
            uint8_t *m = NULL; size_t n = 0;
            if (!disks_read_file_no(&d, disk, f, true, &m, &n, &e)) continue;
            if (n < 0x43C || memcmp(m + 0x438, "M.K.", 4) != 0) { free(m); continue; }
            static sk_paula p; static sk_pt pt;
            paula_init(&p, 48000); pt_init(&pt, &p);
            CHECK(pt_start(&pt, &p, m, n, 0, false));
            static int16_t out[2 * 960];
            long energy = 0;
            for (int i = 0; i < 50 * 30; i++) {
                pt_phase1(&pt); pt_phase2(&pt); pt_phase3(&pt);
                paula_render(&p, out, 960);
                for (int k = 0; k < 2 * 960; k += 64) energy += out[k] < 0 ? -out[k] : out[k];
            }
            if (energy == 0) printf("disk %d file %03d: silent\n", disk, f);
            CHECK(energy > 0);
            played++;
            free(m);
        }
    printf("%d modules played\n", played);
    CHECK(played >= 30);
    disks_close(&d);
}
