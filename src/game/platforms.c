#include <string.h>
#include "game/game.h"
#include "game/platforms.h"

void platforms_reset(sk_game *g) {                               /* sub_118136 */
    memset(g->platforms, 0, sizeof g->platforms);
    g->platform_count = 0;
}

sk_platform *alloc_platform(sk_game *g, int *index) {            /* 0x118154 */
    for (int i = 0; i < SK_PLATFORMS; i++) {
        sk_platform *pf = &g->platforms[i];
        if (pf->used) continue;
        memset(pf, 0, sizeof *pf);                                /* move.l #0,(a4): the port clears the whole record */
        pf->used = true;
        g->platform_count++;
        *index = i;
        return pf;
    }
    *index = -1;
    return NULL;
}

sk_platform *slot_platform(sk_game *g, const sk_slot *s) {       /* 0x11819E: index * 32 in the owner's flags word */
    return s->platform >= 0 && s->platform < SK_PLATFORMS ? &g->platforms[s->platform] : NULL;
}

void free_platform(sk_game *g, sk_slot *s) {                     /* 0x118184 */
    sk_platform *pf = slot_platform(g, s);
    if (!pf || !pf->used) return;
    pf->used = false;
    g->platform_count--;
}

static int16_t hi(int32_t v) { return (int16_t)(v >> 16); }
static void add_hi(int32_t *v, int d) { *v = (int32_t)((uint32_t)(uint16_t)(hi(*v) + d) << 16 | ((uint32_t)*v & 0xFFFF)); }

int platform_carry(sk_game *g, sk_platform *pf, int dx, int dy) {   /* 0x118014 */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    int result = 0;
    g->bw.mask_dirty = true;
    pf->player_on = 0;
    int surface = hi(pf->py) + pf->stand_dy;
    int d6 = (int16_t)(p->x - hi(pf->px) + pf->half_w);
    if ((uint16_t)d6 < (uint16_t)(2 * pf->half_w)) {
        g->mask.dirty = true;
        int d7 = (int16_t)(p->y + 41 - surface);
        if (d7 <= 3) {
            pf->player_on = 1;
            if (!p->jumping && !p->falling && (uint16_t)d7 < 3) {
                result = -1;
                p->on_platform = 1;
                p->x = (int16_t)(p->x + dx);
                p->y = (int16_t)(p->y + dy);
                if (p->state == 16 || p->ball_hold < 0) {         /* the ball at his feet goes with him */
                    int d = (int16_t)(surface - (hi(b->y) + 16));
                    if (d < 0) dy += d;
                    b->prev_x = (int16_t)(b->prev_x + dx); b->prev_y = (int16_t)(b->prev_y + dy);
                    add_hi(&b->x, dx); add_hi(&b->y, dy);
                    b->touched = 0xFF;
                    pf->ball_on = 1;
                    return result;
                }
            }
        }
    }
    if (p->ball_hold < 0) return result;
    int bd6 = (int16_t)(hi(b->x) + 2 - hi(pf->px) + pf->half_w);
    if ((uint16_t)bd6 >= (uint16_t)(2 * pf->half_w - 12)) return result;
    pf->ball_on = 1;
    int d7 = (int16_t)(hi(b->y) + 16 - surface);
    if ((uint16_t)d7 >= 8) return result;
    dy -= d7;
    b->prev_y = (int16_t)(b->prev_y + dy);
    add_hi(&b->y, dy);
    b->touched = 0xFF;
    return result;
}

/* merge_platform_mask (0x118208): 10 rows of plane 0 of the record's frame OR-ed into a 64-pixel-wide mask
   (8 bytes a row) at (px - half_w, py + frame_sel) relative to the origin; skipped unless 0 <= row < 80. */
static void merge_one(const sk_platform *pf, uint8_t *mask, int origin_x, int origin_y) {
    if (!pf->pframe) return;
    int top = (int16_t)(hi(pf->py) + pf->frame_sel - origin_y);
    if (top < 0 || top >= 80) return;
    int left = (int16_t)(hi(pf->px) - pf->half_w - origin_x), w = 2 * pf->half_w, row_bytes = w >> 3;
    if (left + w < 0 || left >= 64) return;
    for (int r = 0; r < 10; r++)
        for (int x = 0; x < w; x++) {
            int mx = left + x;
            if (mx < 0 || mx >= 64) continue;
            if (!(pf->pframe[r * row_bytes + (x >> 3)] >> (7 - (x & 7)) & 1)) continue;
            mask[(top + r) * 8 + (mx >> 3)] |= (uint8_t)(0x80 >> (mx & 7));
        }
}

void merge_platform_masks(sk_game *g, uint8_t *mask, int origin_x, int origin_y, bool ball) {   /* 0x1181AC / 0x1181DA */
    for (int i = 0; i < SK_PLATFORMS; i++) {
        const sk_platform *pf = &g->platforms[i];
        if (pf->used && (ball ? pf->ball_on : pf->player_on)) merge_one(pf, mask, origin_x, origin_y);
    }
}
