/* No-op bodies for hooks that later modules provide (Task 7 and Task 8 remove them). */
#include "game/game.h"

void player_tile_attrs(sk_game *g) { (void)g; }
void select_layers(sk_game *g) { g->bplcon2_value = 0x20; }
