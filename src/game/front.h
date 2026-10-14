/* The frontend's game-side state and rules (re/docs/notes/frontend.md): the language and kit choices written into the image,
   the menus' logic, the hi-score table. */
#ifndef SK_GAME_FRONT_H
#define SK_GAME_FRONT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/hunk.h"

#define SK_KIT_TABLE_ADDR 0x11D2B2u     /* 14 rows of 3 colour words and a zero word */
#define SK_KITS 14
#define SK_LANG_TEXTS_ADDR 0x202484u    /* the in-level message tables, loaded from <language>.bin */

void kit_apply(sk_image *img, int shirt, int shorts);          /* 0x11CCFE: the rows into kit_palette_base */
bool language_apply(sk_image *img, const uint8_t *bin, size_t n);
const char *language_file(int language);                         /* 0x10085A: english french german italian spanish */

#endif
