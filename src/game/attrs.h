/* Tile attributes (flags >= 33 of a map word -> level_pals[5 + flag - 33] -> tile_attr_table 0x11A72C), the
   player's probe (0x11A5E2), the tile change queue (0x301CFE) and the attribute handlers 5a needs. */
#ifndef SK_GAME_ATTRS_H
#define SK_GAME_ATTRS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct sk_game sk_game;

typedef struct sk_tile_change { int cell; int kind; int ticks; uint16_t start; bool used; } sk_tile_change;

typedef struct sk_attrs {
    sk_tile_change queue[32];      /* tile_queue */
    int probe_kind;                /* 0x30024E: 0 player, 1 ball, 2 object, 0xFF consumed */
} sk_attrs;

typedef void (*sk_attr_fn)(sk_game *g, int cx, int cy);   /* the handler sees the cell's position; probe_kind says who probes */
extern sk_attr_fn attr_table[95];                          /* NULL = no-op */

void attrs_reset(sk_game *g);                     /* empties the queue; installs attributes 2, 13, 53, 54 where NULL */
int  probe_tile_attr(sk_game *g, int x, int y);   /* the object probe (probe_kind 2): -1 no attribute, 0 ran */
void player_tile_attrs(sk_game *g);               /* 3 x 4 cells around the kid, probe_kind 0 */
void queue_tile_change(sk_game *g, int cx, int cy, int kind);
void process_tile_queue(sk_game *g);
void clear_cell(sk_game *g, int cx, int cy);
void set_cell_tile(sk_game *g, int cx, int cy, int index, int state);
void select_layers(sk_game *g);                   /* 0x10BDAA: bplcon2_value from layer_flags */
void attr_checkpoint(sk_game *g, int cx, int cy);
void attr_exit(sk_game *g, int cx, int cy);
void attr_layer_switch_a(sk_game *g, int cx, int cy);
void attr_layer_switch_b(sk_game *g, int cx, int cy);

#endif
