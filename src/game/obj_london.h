/* England London's object types (5 10 26 61 75) and their children. */
#ifndef SK_GAME_OBJ_LONDON_H
#define SK_GAME_OBJ_LONDON_H

typedef struct sk_game sk_game;
typedef struct sk_slot sk_slot;

void obj_london_install(void);
void probe_tile_attr_dir(sk_game *g, sk_slot *s);   /* 0x11A4FE: the tile ahead of a moving platform */

#endif
