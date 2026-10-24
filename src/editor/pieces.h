/* skld's pieces: groups of tiles the setting's original map always places together (a car, a post box), found
   from the map itself. Two tiles join when one's neighbour on a side is always the other and the other's on the
   opposite side is always the one, at least twice; the map's three commonest tiles and the start never join. */
#ifndef SK_EDITOR_PIECES_H
#define SK_EDITOR_PIECES_H

#include <stdbool.h>
#include <stdint.h>
#include "formats/level.h"

typedef struct { int w, h, cells, uses; int16_t *tiles; } sk_piece;   /* tiles: w * h, -1 outside the piece */
typedef struct { sk_piece *v; int n; } sk_pieces;

/* The distinct pieces of at least min_cells cells, largest first, then the most used. */
bool edit_find_pieces(const sk_tilemap *map, int min_cells, sk_pieces *out);
void edit_pieces_free(sk_pieces *p);

#endif
