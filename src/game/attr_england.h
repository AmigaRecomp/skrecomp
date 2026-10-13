/* England 1 and 2's tile attributes (0 1 2 5 6 7 9 10 13 19 25 27 28 37 42 53 54 62 66 67), the bump block
   (type 122), the tile-restore lists and the message box's state. */
#ifndef SK_GAME_ATTR_ENGLAND_H
#define SK_GAME_ATTR_ENGLAND_H

#include <stdint.h>

typedef struct sk_game sk_game;

void attr_england_install(void);                /* fills attr_table and type 122's handlers */
void flush_tile_restores(sk_game *g);           /* 0x11B504 */
void replay_bump_restores(sk_game *g);          /* init_player's replay of the list at 0x300DAE */
void message_box_tick(sk_game *g);              /* 0x11C66E, frame step 13 */
extern const uint8_t obj_target_hit_script[];   /* 0x11C4E0, type 115 */

#endif
