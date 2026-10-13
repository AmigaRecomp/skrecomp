/* England 1 and 2's tile attributes (0 1 2 5 6 7 9 10 13 19 25 27 28 37 42 53 54 62 66 67), the bump block
   (type 122), the tile-restore lists and the message box's state. */
#ifndef SK_GAME_ATTR_ENGLAND_H
#define SK_GAME_ATTR_ENGLAND_H

#include <stdbool.h>
#include <stdint.h>

typedef struct sk_slot sk_slot;

typedef struct sk_game sk_game;

void attr_england_install(void);                /* fills attr_table and type 122's handlers */
void flush_tile_restores(sk_game *g);           /* 0x11B504 */
void replay_bump_restores(sk_game *g);          /* init_player's replay of the list at 0x300DAE */
void message_box_tick(sk_game *g);
int  message_sign_check(sk_game *g, int cx, int cy);   /* 0x11B5A2: -1 when the ball breaks the sign */
void swallow_ball(sk_game *g);
void flutter_spawn(sk_game *g, int x, int y, int type);   /* attributes 62 (type 103) and 65 (type 101) */
bool ball_hits_cell(const sk_game *g, int x, int y);     /* the ball in the cell and moving faster than 2 */                                 /* the drains' ball path: the ball vanishes */
void ball_target(sk_game *g, int cx, int cy, uint8_t *count, int param);   /* ball_target_tail 0x11C3A4 */              /* 0x11C66E, frame step 13 */
extern const uint8_t obj_target_hit_script[];   /* 0x11C4E0, type 115 */

#endif
