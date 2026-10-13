/* Hardware sprites of the gameplay: the kid (and later the ball) are 64 x 48 attached pairs stored in the sk
   chip data hunk (display_model.h SK_KID_FRAMES_ADDR); their colours are a kit palette in bank 1 (49..63). */
#ifndef SK_GFX_SPRITES_H
#define SK_GFX_SPRITES_H

#include <stdbool.h>
#include <stdint.h>
#include "core/hunk.h"
#include "gfx/display_model.h"
#include "gfx/palette.h"

typedef struct sk_sprite {
    int x, y;                       /* buffer pixels of the top-left corner */
    bool visible, in_front;         /* in_front: over playfield 1 (BPLCON2 priority) */
    uint8_t px[SK_SPR_H * SK_SPR_W];    /* 0 transparent, 1..15 -> colour SK_SPR_COLOUR_BASE + value */
} sk_sprite;

typedef struct sk_sprite_layer { sk_sprite spr[2]; } sk_sprite_layer;   /* 0 = kid, 1 = ball (5b) */

/* Decodes the attached pair whose first sprite starts at frame_addr (16 bytes of control words, then 48 rows
   of 8 bytes plane A + 8 bytes plane B; the partner at + SK_SPR_FRAME holds planes C and D). */
bool sprite_decode_pair(const sk_image *sk, uint32_t frame_addr, int height, sk_sprite *out);   /* height 48 (kid) or 16 (ball) */

/* Colours 49..63 for kit 0..7 (build_kit_palettes 0x11B0A4 + set_kit_colours 0x11B010 / 0x11B05A):
   each nibble of the 13 base words and the 2 extra words is scaled by (16 - kit) / 16; kits 4..7 skip 49. */
void kit_palette(sk_palette *p, const sk_image *sk, int kit);

#endif
