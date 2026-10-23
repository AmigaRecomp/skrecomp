/* skld's map picture: a custom map drawn by the game's own tile renderer, backdrop and sky lines included. */
#ifndef SK_EDITOR_VIEW_H
#define SK_EDITOR_VIEW_H

#include <stdbool.h>
#include <stdint.h>
#include "formats/custom.h"
#include "formats/level.h"
#include "gfx/palette.h"

/* w x h XRGB pixels of the map from world (x, y); the backdrop scrolls as the game's would with the camera there
   (the game's camera is 16 px left of its first visible pixel). False: out of memory. */
bool edit_render(const sk_level *setting, const sk_palette *pal, const sk_custom *map, int x, int y, int w, int h, uint32_t *out);

#endif
