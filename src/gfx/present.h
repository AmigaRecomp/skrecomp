/* Framebuffer + palette + per-line colours -> XRGB pixels, the way the display would show them. */
#ifndef SK_GFX_PRESENT_H
#define SK_GFX_PRESENT_H

#include <stdint.h>
#include "gfx/framebuffer.h"
#include "gfx/palette.h"
#include "gfx/sprites.h"

/* The sky lines (sky_lines 0x106E74, display.md corrections): COLOR17 for window line k = c17[c17_off + k] (COPP chunk 4
   at min(bg_y / 2, 312)), COLOR18 = c18[c18_off + k] (COPP chunk 5 at bg_y); each clamped to its table's last word. */
typedef struct { const uint16_t *c17; int c17_len, c17_off; const uint16_t *c18; int c18_len, c18_off; } sk_sky;

/* Resolves the w x h window at (src_x, src_y) of the framebuffer into out (w * h XRGB values).
   With SK_DUAL_PLAYFIELD: playfield 1 (low nibble) wins when non-zero, else playfield 2 (high nibble)
   shows colour 16 + value; colours 17 and 18 come from the sky table for window line y. */
void present_rows(const sk_fb *fb, const sk_palette *pal, const sk_sky *sky,
                  int src_x, int src_y, int w, int h, uint32_t *out);

/* The whole 320x256 frame: colour 0 everywhere, the play window from buffer columns SK_VIEW_X.. at
   (SK_PLAY_X, SK_PLAY_Y); the HUD area is left colour 0 by the engine core. spr (may be NULL): the hardware
   sprites, each over playfield 2 and in front of playfield 1 only when its in_front flag is set; curtain_rows play rows
   at the top and the bottom are colour 0 (run_curtain). */
void present_screen(const sk_fb *fb, const sk_palette *pal, const sk_sky *sky, const sk_sprite_layer *spr, int curtain_rows, uint32_t *out);

#endif
