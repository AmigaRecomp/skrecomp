/* Italy's object types (RUINS, VENICE, RIVIERA: 31 36 37 38 43 44 48 49 51 64 68 70 and their children). */
#ifndef SK_GAME_OBJ_ITALY_H
#define SK_GAME_OBJ_ITALY_H

void obj_italy_install(void);

typedef struct sk_game sk_game; typedef struct sk_slot sk_slot; typedef struct sk_platform sk_platform;
sk_platform *chain_platform_init(sk_game *g, sk_slot *s, int x, int y);   /* types 38 and 6: the platform on a chain of slot-46 links */
void chain_platform_place(sk_game *g, sk_slot *s, sk_platform *pf);        /* position, carry, the links, the free test */

#endif
