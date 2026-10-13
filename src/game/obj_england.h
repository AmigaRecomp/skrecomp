/* England 1 and 2's object types (8 12 16 24 28 45 58 60 63) and the child types they reach (99 103 105 112 113
   116 123 124 125 126). Types 89 117 118 are spawned only by other worlds' code (5c); type 122 rides a
   platform record and comes with attribute 25. */
#ifndef SK_GAME_OBJ_ENGLAND_H
#define SK_GAME_OBJ_ENGLAND_H

typedef struct sk_game sk_game;

void obj_england_install(void);                 /* fills object_init_table / object_update_table */
void spawn_jingle(sk_game *g, int tune);        /* 0x1045A6: the banner object (type 105) with the tune as its parameter */

#endif
