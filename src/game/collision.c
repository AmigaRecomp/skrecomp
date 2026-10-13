#include <string.h>
#include "game/collision.h"

/* ceiling_masks (0x10B08A): a 12-bit window inside a 32-pixel longword, by x & 15 (VERIFIED). */
static const uint32_t ceiling_masks[16] = {
    0x1FFE0000, 0x0FFF0000, 0x07FF8000, 0x03FFC000, 0x01FFE000, 0x00FFF000, 0x007FF800, 0x003FFC00,
    0x001FFE00, 0x000FFF00, 0x0007FF80, 0x0003FFC0, 0x0001FFE0, 0x0000FFF0, 0x00007FF8, 0x00003FFC,
};

/* A bit in mask coordinates; outside the window reads as empty (the original reads neighbouring memory). */
static bool bit(const sk_mask *m, int mx, int my) {
    if (mx < 0 || mx >= SK_MASK_W || my < 0 || my >= SK_MASK_H) return false;
    return (m->bits[my * 8 + (mx >> 3)] >> (7 - (mx & 7))) & 1;
}

static uint32_t longword(const sk_mask *m, int mx, int my) {    /* the 32 pixels from mx & ~15 */
    int byte = (mx >> 4) * 2;
    if (my < 0 || my >= SK_MASK_H || byte < 0 || byte + 3 >= 8) {
        uint32_t v = 0;                                           /* partial: gather what exists */
        for (int i = 0; i < 32; i++) v = v << 1 | (bit(m, (mx & ~15) + i, my) ? 1u : 0u);
        return v;
    }
    const uint8_t *p = m->bits + my * 8 + byte;
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

void mask_rebuild(sk_mask *m, const sk_world *w, int x, int y) {
    if (m->built) return;
    m->built = true;
    int ox = (x & ~15) - 16, oy = (y & ~15) - 16;
    if (ox < 0) ox = 0;
    if (!m->dirty && ox == m->origin_x && oy == m->origin_y) return;
    m->dirty = false;
    m->origin_x = ox;
    m->origin_y = oy;
    memset(m->bits, 0, sizeof m->bits);
    for (int r = 0; r < 6; r++) {
        int cy = oy / 16 + r;
        for (int c = 0; c < 4; c++) {
            int cx = ox / 16 + c;
            uint8_t *dst = m->bits + r * 16 * 8 + c * 2;
            if (cy < 0) {                                           /* above the map: the all-solid constant */
                for (int k = 0; k < 16; k++) { dst[k * 8] = 0xFF; dst[k * 8 + 1] = 0xFF; }
                continue;
            }
            if (!world_inside(w, cx, cy)) continue;                 /* beyond the map: empty (ruling) */
            const uint8_t *tail = world_tail(w, cx, cy);
            if (!tail) continue;
            for (int k = 0; k < 16; k++) { dst[k * 8] = tail[k * 2]; dst[k * 8 + 1] = tail[k * 2 + 1]; }
        }
    }
    if (m->merge) m->merge(m, m->merge_ctx);
}

bool mask_solid(const sk_mask *m, int x, int y) { return bit(m, x - m->origin_x, y - m->origin_y); }

int probe_ground(const sk_mask *m, int x, int y) {
    int mx = x - m->origin_x, my = y - m->origin_y + 41, n = 0;
    for (int i = 0; i < 16 && bit(m, mx, my + i); i++) n++;
    return n;
}

void probe_floor(const sk_mask *m, int x, int y, int *gap_below, int *push_up) {
    int mx = x - m->origin_x, my = y - m->origin_y + 40;
    int gap = 0, push = 0, below = my + 1, above = my;
    for (int i = 0; i < 10; i++) {
        if (!bit(m, mx, below)) { gap++; below++; }
        if (bit(m, mx, above)) { push++; above--; }
    }
    *gap_below = gap;
    *push_up = push;
}

int probe_slope(const sk_mask *m, int x, int y, int facing) {
    int mx = x - m->origin_x - 4, my = y - m->origin_y + 39;
    int left = 0, right = 0;
    for (int i = 0; i < 8; i++) {
        if (!bit(m, mx, my + left)) left++;
        if (!bit(m, mx + 8, my + right)) right++;
    }
    int d = left - right;
    if (d >= -1 && d <= 1) return 16;
    if (d > 1) {                                                    /* the left foot hangs lower */
        if (facing >= 0) return 16;
        return d >= 3 ? 23 : 18;
    }
    if (facing < 0) return 16;                                      /* the right foot hangs lower */
    return -d >= 3 ? 23 : 18;
}

int probe_wall(const sk_mask *m, int probe_x, int y, int dir, int facing, bool sliding, int16_t *player_x, int16_t *speed) {
    int mx = probe_x - m->origin_x, my = y - m->origin_y - 5;       /* the routine's base is five rows above the mask */
    int budget = 0x2C;
    if (sliding) { budget -= 16; my += 16; }
    int row;                                                     /* the scan starts above the hit: at my, my + 6, my + 16, my + 26 */
    if (bit(m, mx, my + 6)) { row = my; }
    else if ((budget -= 6), bit(m, mx, my + 16)) { row = my + 6; }
    else if ((budget -= 10), bit(m, mx, my + 26)) { row = my + 16; }
    else if ((budget -= 10), bit(m, mx, my + 36)) { row = my + 26; }
    else return 0;
    int run = 0;
    for (;;) {
        run = bit(m, mx, row) ? run + 1 : 0;
        if (run >= 10) break;
        row++;
        if (--budget < 0) return 0;
    }
    if (dir >= 0) {
        if (facing >= 0) *speed = 0;
        for (int i = 0; i < 4; i++) { (*player_x)--; mx--; if (!bit(m, mx, row)) break; }
    } else {
        if (facing < 0) *speed = 0;
        for (int i = 0; i < 4; i++) { (*player_x)++; mx++; if (!bit(m, mx, row)) break; }
    }
    return -1;
}

int probe_ceiling(const sk_mask *m, int x, int y, int facing, bool sliding) {
    int mx = x - m->origin_x, my = y - m->origin_y - 5;             /* five rows above the mask base */
    if (facing >= 0) mx -= 16;
    uint32_t window = ceiling_masks[mx & 15];
    int result = 0;
    if (sliding) {
        if (longword(m, mx, my) & window) result |= SK_CEIL_SLIDE_BLOCKED;
        my += 16;
    }
    window &= longword(m, mx, my);                               /* narrowed to the first row's solid pixels */
    if (!window) return result;
    for (int i = 1; i <= 9; i++)
        if (!(longword(m, mx, my + i) & window)) return result;
    return result | SK_CEIL_BUMP;
}

bool probe_edge(const sk_mask *m, int x, int y, int facing) {
    int mx = x - m->origin_x - 4 + (facing >= 0 ? 8 : 0), my = y - m->origin_y + 41;
    for (int i = 0; i < 8; i++)
        if (bit(m, mx, my + i)) return true;
    return bit(m, mx, my + 7 + 7);
}

int probe_surroundings(const sk_mask *m, int x, int y, int facing, bool sliding, int16_t *player_x, int16_t *speed) {   /* 0x10ADBC */
    int flags = 0;
    if (probe_wall(m, x - 8, y, -1, facing, sliding, player_x, speed)) flags |= 0x40;   /* both from the entry x */
    if (probe_wall(m, x + 8, y, 0, facing, sliding, player_x, speed)) flags |= 0x40;
    if (probe_ceiling(m, x, y, facing, sliding) & SK_CEIL_BUMP) flags |= 0x80;
    return flags;
}
