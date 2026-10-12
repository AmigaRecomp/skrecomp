/* The 256 AGA colour registers as the game fills them in play (display.md, Colours). */
#ifndef SK_GFX_PALETTE_H
#define SK_GFX_PALETTE_H

#include <stdint.h>
#include "formats/level.h"

typedef struct { uint32_t xrgb[256]; } sk_palette;

/* 12-bit Amiga colour word to XRGB, each nibble scaled by 17 (as skre.formats.copp.rgb12). */
uint32_t rgb12_to_xrgb(uint16_t w);

/* install_tile_palettes (0x10382E): set A colours 1..15 into registers 1..15 (playfield 1),
   set B colours 1..15 into registers 17..31 (playfield 2, PF2OF = 16); 0 and 16 stay black. */
void palette_from_tilesets(sk_palette *p, const sk_tileset *a, const sk_tileset *b);

#endif
