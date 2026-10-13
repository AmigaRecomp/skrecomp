/* Platform records (platform_records 0x3028D6, 16 x 32 bytes) and what they do to the kid and the ball:
   platform_carry (0x118014) and the masks merged into the collision masks (merge_platform_masks 0x1181AC,
   merge_platform_masks_ball 0x1181DA, merge_platform_mask 0x118208). gameplay.md "Platform records". */
#ifndef SK_GAME_PLATFORMS_H
#define SK_GAME_PLATFORMS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct sk_game sk_game;
typedef struct sk_slot sk_slot;

typedef struct sk_platform {
    bool used;                      /* the first long is not -1 */
    int32_t px, py;                 /* +0x0, +0x4: 16.16; px.hi is the left edge + half_w */
    int16_t half_w, half_h;         /* +0x8, +0xA */
    int32_t pvx, pvy;               /* +0xC, +0x10 */
    uint8_t pmode, pspeed, pangle;  /* +0x14, +0x16, +0x17 */
    const uint8_t *pframe;          /* +0x18: plane 0 of its first frame is the solid shape */
    uint8_t player_on, ball_on;     /* +0x1C, +0x1D */
    uint8_t stand_dy, frame_sel;    /* +0x1E, +0x1F */
} sk_platform;

#define SK_PLATFORMS 16

void platforms_reset(sk_game *g);                                   /* sub_118136: every record free */
sk_platform *alloc_platform(sk_game *g, int *index);                /* 0x118154: NULL (index -1) when all 16 are used */
sk_platform *slot_platform(sk_game *g, const sk_slot *s);           /* 0x11819E */
void free_platform(sk_game *g, sk_slot *s);                         /* 0x118184 */
int  platform_carry(sk_game *g, sk_platform *pf, int dx, int dy);   /* -1 when the kid rides it */
void merge_platform_masks(sk_game *g, uint8_t *mask, int origin_x, int origin_y, bool ball);   /* player_on / ball_on records */

#endif
