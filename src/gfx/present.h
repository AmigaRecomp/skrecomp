/* Framebuffer + palette + per-line colours -> XRGB pixels, the way the display would show them. */
#ifndef SK_GFX_PRESENT_H
#define SK_GFX_PRESENT_H

#include <stdint.h>
#include "gfx/framebuffer.h"
#include "gfx/palette.h"

/* The level's sky table: (COLOR17, COLOR18) word pairs, one pair per display line; the last pair holds. */
typedef struct { const uint16_t *words; int len; } sk_line_colours;

/* Resolves the w x h window at (src_x, src_y) of the framebuffer into out (w * h XRGB values).
   With SK_DUAL_PLAYFIELD: playfield 1 (low nibble) wins when non-zero, else playfield 2 (high nibble)
   shows colour 16 + value; colours 17 and 18 come from the sky table for window line y. */
void present_rows(const sk_fb *fb, const sk_palette *pal, const sk_line_colours *sky,
                  int src_x, int src_y, int w, int h, uint32_t *out);

/* The whole 320x256 frame: colour 0 everywhere, the play window from buffer columns SK_VIEW_X.. at
   (SK_PLAY_X, SK_PLAY_Y); the HUD area is left colour 0 by the engine core. */
void present_screen(const sk_fb *fb, const sk_palette *pal, const sk_line_colours *sky, uint32_t *out);

#endif
