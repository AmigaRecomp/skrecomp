/* The intro (the `anim` program, re/docs/notes/intro.md): eight IFF ANIM animations, five typed texts, palette fades, the
   half-brightness dimming and the module of `anim`'s own DATA hunk, run as main_sequence's fixed script on a 50 Hz tick.
   Any key or fire ends it at the original's polling points (a key frame, a text). */
#ifndef SK_APP_INTRO_H
#define SK_APP_INTRO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "audio/sound.h"
#include "core/disks.h"
#include "core/err.h"
#include "core/hunk.h"
#include "formats/anim.h"

typedef struct {
    sk_image anim;                      /* the `anim` executable: font 0x20012C, texts, char_to_glyph 0x100C8E, module 0x200ECC */
    uint8_t *file[2]; size_t file_n[2]; /* animfil1 / animfil2 unpacked */
    uint8_t screen[2][6][8000];         /* screen records A and B: 5 colour planes and the EHB plane */
    int work, shown, displayed;         /* work_screen, shown_screen, the one the copper points at */
    uint16_t pal_full[32], pal_half[32], pal_current[32], copper[32];
    uint16_t bplcon0;                   /* 0x5200, or 0x6200 (Extra Half-Brite) */
    int tick_counter;                   /* 0x300244 */
    bool quit;                          /* the script reached a polling point with the request set: exit_intro */
    bool quit_req;                      /* quit_flag: a key, the mouse or fire since the start */
    bool fade_in_flag; int cue_pending, hold_ticks; const char *pending_text;
    /* typewriter (0x100B62 / 0x100BB2) */
    bool tw_active; int tw_countdown, tw_reload; const uint8_t *tw_text; int tw_cell, tw_line;
    /* the script */
    int op, sub, wait, node, fade_step;
    uint16_t fade_from[32]; const uint16_t *fade_to;
    sk_anim cur;                        /* the animation playing */
    int delay;
    bool done;
    sk_sound *snd;
} sk_intro;

bool intro_open(sk_intro *it, sk_disks *d, sk_sound *snd, sk_err *e);   /* reads anim and animfil1/2; the first tick starts the music */
void intro_step(sk_intro *it, bool skip);    /* one tick; skip: a key, the mouse button or fire was pressed */
void intro_present(const sk_intro *it, uint32_t *out);   /* the 320 x 200 window at frame lines 0..199 */
void intro_free(sk_intro *it);
uint16_t intro_lerp(uint16_t from, uint16_t to, int step);   /* lerp_colour 0x10122E: step 0..16 */

#endif
