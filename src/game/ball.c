/* The ball: update_ball (0x1098CA) and the routines it calls. Transcribed from the listing; the blitter tests
   (minterm 0xAA0 = A AND C, 0xA50 = A AND NOT C, BZERO) are reproduced as bit loops over the 64-pixel mask. */
#include <string.h>
#include "game/attrs.h"
#include "game/ball.h"
#include "game/game.h"

#define BALL_SHAPE 0x401F98
#define BALL_SHAPE2 0x401FB8
#define PROBE_TABLE 0x10A3DC            /* per direction / 8: two longs -> { i16 dx, i16 dy, u16 bltsize, u16 rows... } */
#define PROBE_TABLE2 0x10A3E0
#define ROLL_TABLE 0x2008CC
#define VANISH_TABLE 0x2008EC
#define SPIN_TABLE 0x200904

/* Exact copies of the image's tables (VERIFIED), so the maths runs without the sk image in unit tests. */
static const uint8_t sin_table[65] = {          /* 0x105E94 */
    0x00, 0x02, 0x03, 0x05, 0x06, 0x08, 0x09, 0x0B, 0x0C, 0x0E, 0x10, 0x11, 0x13, 0x14, 0x16, 0x17,
    0x18, 0x1A, 0x1B, 0x1D, 0x1E, 0x20, 0x21, 0x22, 0x24, 0x25, 0x26, 0x27, 0x29, 0x2A, 0x2B, 0x2C,
    0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x38, 0x39, 0x3A, 0x3B,
    0x3B, 0x3C, 0x3C, 0x3D, 0x3D, 0x3E, 0x3E, 0x3E, 0x3F, 0x3F, 0x3F, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
};
static const uint8_t atan_table[64] = {         /* 0x105F18 */
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x09,
    0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12,
    0x13, 0x13, 0x14, 0x14, 0x15, 0x15, 0x16, 0x16, 0x17, 0x17, 0x18, 0x18, 0x19, 0x19, 0x19, 0x1A,
    0x1A, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1D, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x20,
};
static const uint8_t contact_dir_left[16] = { 0x00, 0x80, 0x6A, 0x61, 0x59, 0x52, 0x4C, 0x40, 0x40, 0x34, 0x2E, 0x27, 0x1F, 0x16, 0x00, 0x00 };
static const uint8_t contact_dir_right[16] = { 0x00, 0x80, 0x96, 0x9F, 0xA7, 0xAE, 0xB4, 0xC0, 0xC0, 0xCC, 0xD2, 0xD9, 0xE1, 0xEA, 0x00, 0x00 };
static const uint16_t disc[16] = {              /* ball_shape, used when no image is loaded */
    0x0000, 0x03C0, 0x0FF0, 0x1FF8, 0x3FFC, 0x3FFC, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x3FFC, 0x3FFC, 0x1FF8, 0x0FF0, 0x03C0, 0x0000,
};
static const uint16_t ring[16] = {              /* ball_shape2 */
    0x0000, 0x03C0, 0x0E70, 0x1818, 0x300C, 0x2004, 0x6006, 0x4002, 0x4002, 0x6006, 0x2004, 0x300C, 0x1818, 0x0E70, 0x03C0, 0x0000,
};

void ball_world_init(sk_ball_world *bw) {
    memset(bw, 0, sizeof *bw);
    bw->gravity = 0x4000;
    bw->min_bounce = 0x0A00;
    bw->drag_div = 0x20;
}

int cos_256(int a) {                             /* 0x105E72 (named cos, but the table starts at 0: it is a sine) */
    int t = a & 0xFF;
    int neg = t & 0x80;
    if (neg) t = (-t) & 0xFF;
    if (t > 0x40) t = (-t) & 0x3F;
    int v = sin_table[t];
    return neg ? -v : v;
}

int sin_256(int a) { return cos_256(a + 0x40); } /* 0x105E6E */

static int atan_q(unsigned d0, unsigned d1) {    /* sub_105F08: d1 < d0 */
    unsigned q = ((uint32_t)d1 << 16) / d0;
    if (q > 0xFFFF) q = 0xFFFF;
    return atan_table[(q >> 10) & 63];
}

static int atan_pos(unsigned d0, unsigned d1) {  /* sub_105EF2: both >= 0 */
    if (d1 == d0) return 0x20;
    if (d1 < d0) return atan_q(d0, d1);
    return (0x40 - atan_q(d1, d0)) & 0xFF;        /* subi.b #$40; neg.b */
}

static int atan_x(int dx, unsigned dy) {         /* atan_256 0x105EE2 */
    if (dx >= 0) return atan_pos((unsigned)dx, dy);
    int r = atan_pos((unsigned)-dx, dy) ^ 0x80;
    return (-r) & 0xFF;
}

int atan2_256(int dx, int dy) {                  /* 0x105ED6 */
    dx = (int16_t)dx; dy = (int16_t)dy;
    if (dy >= 0) return atan_x(dx, (unsigned)dy);
    return (-atan_x(dx, (unsigned)-dy)) & 0xFF;
}

static uint16_t shape_row(const sk_game *g, uint32_t addr, const uint16_t *fallback, int r) {
    return g->sk ? image_u16(g->sk, addr + 2u * (uint32_t)r) : fallback[r];
}

/* A blit test of a 16-wide shape (rows at addr, or the fallback) at mask position (mx, my): with `and_not`,
   minterm A AND NOT C. Returns true when the result is non-zero (BZERO clear). */
static bool blit_hits(const sk_game *g, int mx, int my, int rows, uint32_t addr, const uint16_t *fallback, bool and_not) {
    const sk_ball_world *bw = &g->bw;
    int col = mx >> 4, sh = mx & 15;
    for (int r = 0; r < rows; r++) {
        int row = my + r;
        uint32_t a = (uint32_t)shape_row(g, addr, fallback, r) << 16 >> sh;
        uint32_t c = 0;
        if (row >= 0 && row < SK_BALL_MASK_ROWS && col >= 0 && col < 4) {
            const uint8_t *p = bw->mask + row * 8 + col * 2;
            c = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16;
            if (col < 3) c |= (uint32_t)p[2] << 8 | p[3];
        }
        if (and_not ? (a & ~c) : (a & c)) return true;
    }
    return false;
}

static bool ball_overlaps(const sk_game *g, int x, int y) {   /* ball_shape at world (x, y) */
    return blit_hits(g, x - g->bw.mask_x, y - g->bw.mask_y, 16, BALL_SHAPE, disc, false);
}

/* The four 16-row columns of tile masks around the ball (0x109ACE). */
void rebuild_ball_mask(sk_game *g) {
    sk_ball_world *bw = &g->bw;
    int ox = ((g->ball.x >> 16) & ~15) - 16, oy = ((g->ball.y >> 16) & ~15) - 16;
    ox = (int16_t)ox; oy = (int16_t)oy;
    if (ox < 0) ox = 0;
    if (bw->mask_valid && !bw->mask_dirty && ox == bw->mask_x && oy == bw->mask_y) return;
    bw->mask_dirty = false;
    bw->mask_valid = true;
    bw->mask_x = ox;
    bw->mask_y = oy;
    memset(bw->mask, 0, sizeof bw->mask);
    int tiles = 4, above = 0;
    if (oy < 0) {                                /* above the map: solid tiles, then six in all */
        above = (-oy) >> 4;
        if (above > 6) above = 6;
        tiles = 6;
    }
    for (int c = 0; c < 4; c++) {
        int cx = ox / 16 + c;
        for (int t = 0; t < tiles; t++) {
            uint8_t *dst = bw->mask + t * 16 * 8 + c * 2;
            if (t < above) { for (int k = 0; k < 16; k++) { dst[k * 8] = 0xFF; dst[k * 8 + 1] = 0xFF; } continue; }
            int cy = (oy < 0 ? 0 : oy / 16) + (t - above);
            if (!world_inside(&g->world, cx, cy)) continue;
            const uint8_t *tail = world_tail(&g->world, cx, cy);
            if (!tail) continue;
            for (int k = 0; k < 16; k++) { dst[k * 8] = tail[k * 2]; dst[k * 8 + 1] = tail[k * 2 + 1]; }
        }
    }
    /* merge_platform_masks_ball (0x1181DA): moving platforms are sub-project 5c */
}

static bool ball_near_feet(const sk_game *g) {    /* sub_1099A6 */
    int dx = (int16_t)(g->ball.x >> 16) - g->player.x, dy = (int16_t)(g->ball.y >> 16) - g->player.y;
    return dx > -32 && dx < 18 && dy > -16 && dy <= 40;
}

bool ball_near_player(const sk_game *g) {         /* 0x109978 */
    int dx = (int16_t)(g->ball.x >> 16) - g->player.x, dy = (int16_t)(g->ball.y >> 16) - g->player.y;
    return dx > -32 && dx < 18 && dy > -8 && dy <= 32;
}

static void ball_follow_player(sk_game *g) {      /* 0x109940 */
    sk_player *p = &g->player;
    if (p->ball_hold) { if (p->ball_hold > 0) p->ball_hold--; return; }
    if (!ball_near_player(g) || g->ball.inactive || p->state == 16 || p->tricking) return;
    p->ball_hold = (int8_t)0x80;
    g->ball.vx = g->ball.vy = 0;
}

static bool cell_pixel_solid(const sk_game *g, int x, int y) {   /* sub_109A7E */
    int cx = (x & ~15) >> 4, cy = (y & ~15) >> 4;
    if (!world_inside(&g->world, cx, cy)) return false;
    const uint8_t *tail = world_tail(&g->world, cx, cy);
    if (!tail) return false;
    uint16_t row = (uint16_t)(tail[(y & 15) * 2] << 8 | tail[(y & 15) * 2 + 1]);
    return row >> (15 - (x & 15)) & 1;
}

static void ball_at_feet(sk_game *g) {            /* 0x1099D4 */
    sk_player *p = &g->player;
    sk_ball *b = &g->ball;
    if (p->state == 16 || p->ball_hold >= 0) return;
    if (!ball_near_feet(g)) { p->ball_hold = 8; return; }
    int16_t bx = (int16_t)(b->x >> 16);
    if (p->on_platform) {
        int target = p->x + (p->facing >= 0 ? -6 : -10);
        b->x = (int32_t)((uint32_t)(uint16_t)target << 16 | ((uint32_t)b->x & 0xFFFF));
        b->vx = 0;
        return;
    }
    int d0 = p->facing >= 0 ? 7 : -23, d3 = p->facing >= 0 ? 16 : -1;
    int dist = p->x + d0 - bx;                     /* sub_109A60: step at most 7 px toward the foot */
    if (dist >= 0) bx = (int16_t)(bx + (dist < 7 ? dist : 7));
    else bx = (int16_t)(bx - (-dist < 7 ? -dist : 7));
    b->x = (int32_t)((uint32_t)(uint16_t)bx << 16 | ((uint32_t)b->x & 0xFFFF));
    b->vx = 0;
    if (b->prev_y - p->y <= 0) return;
    if (cell_pixel_solid(g, d3 + b->prev_x, b->prev_y - 8)) return;
    if (cell_pixel_solid(g, d3 + bx, b->prev_y - 8)) return;
    b->lifted = 1;
}

void ball_move(sk_game *g) {                      /* 0x10A28E */
    sk_ball *b = &g->ball;
    b->y += b->vy;
    b->x += b->vx;
    int16_t hi = (int16_t)((b->x >> 16) + g->bw.carry_dx);
    b->x = (int32_t)((uint32_t)(uint16_t)hi << 16 | ((uint32_t)b->x & 0xFFFF));
    if (hi < 0) { b->x = 0; b->vx = -b->vx; }
    int bottom = g->world.fg_max_y + 0xB8;
    if ((int16_t)(b->y >> 16) > bottom) {
        b->y = (int32_t)((uint32_t)(uint16_t)bottom << 16 | ((uint32_t)b->y & 0xFFFF));
        b->inactive = 1;
    }
}

static int32_t ball_drag(const sk_game *g) {       /* 0x10A2C6: the part of vy taken off each free frame */
    int32_t vy = g->ball.vy;
    uint32_t m = (uint32_t)(vy < 0 ? -vy : vy);
    uint32_t d;
    if (!g->bw.drag_div) d = 0;
    else {
        uint32_t q = m / (uint32_t)g->bw.drag_div;
        d = q > 0xFFFF ? (m & 0xFFFF) : q;          /* divu.w overflow leaves the dividend; andi.l #$ffff */
    }
    return vy < 0 ? -(int32_t)d : (int32_t)d;
}

/* ball_probe_dir / ball_probe_dir2 (0x10A18A / 0x10A20C): the record at the long *a2 (then a2 += 4).
   Returns true when the blit result is zero ("bne" in the listing). */
static bool probe(sk_game *g, uint32_t *a2, int px, int py, bool and_not) {
    uint32_t rec = image_u32(g->sk, *a2);
    *a2 += 4;
    int dx = (int16_t)image_u16(g->sk, rec), dy = (int16_t)image_u16(g->sk, rec + 2);
    int rows = image_u16(g->sk, rec + 4) >> 6;
    int mx = px + dx + g->bw.contact_dx - g->bw.mask_x, my = py + dy + g->bw.contact_dy - g->bw.mask_y;
    return !blit_hits(g, mx, my, rows, rec + 6, NULL, and_not);
}

static int u8(int v) { return v & 0xFF; }

void ball_bounce(sk_game *g) {                     /* 0x109C90 */
    sk_ball *b = &g->ball;
    sk_ball_world *bw = &g->bw;
    rebuild_ball_mask(g);
    int x = (int16_t)(b->x >> 16), y = (int16_t)(b->y >> 16);
    if (y < 0 || !ball_overlaps(g, x, y)) {        /* free flight */
        if ((b->vx >> 16) == 0 && (b->vy >> 16) == 0) {
            if (--b->rest_timer >= 0) goto gravity;
            b->at_rest = 1;
        }
        b->rest_timer = 50;
    gravity:
        b->vy += bw->gravity;
        b->vy -= ball_drag(g);
        b->contact_frames = 0;
        return;
    }
    b->contact_frames++;
    int d2 = b->prev_x, d3 = b->prev_y, d4 = x, d5 = y;    /* free point, colliding point */
    if (b->lifted) d3 -= 12;
    for (;;) {                                     /* bisect until the two are within one pixel */
        int mx = (int16_t)(d4 + d2) >> 1, my = (int16_t)(d5 + d3) >> 1;
        if (ball_overlaps(g, mx, my)) { d4 = mx; d5 = my; } else { d2 = mx; d3 = my; }
        if ((unsigned)(uint16_t)(d2 - d4 + 1) >= 3) continue;
        if ((unsigned)(uint16_t)(d3 - d5 + 1) >= 3) continue;
        break;
    }
    if (ball_overlaps(g, d2, d3)) { d2 = b->prev_x; d3 = b->prev_y; }
    /* the ring at the colliding point AND the mask, written over the mask's first 64 bytes (as the original) */
    {
        int mx = d4 - bw->mask_x, my = d5 - bw->mask_y, col = mx >> 4, sh = mx & 15;
        uint8_t out[64];
        for (int r = 0; r < 16; r++) {
            int row = my + r;
            uint32_t a = (uint32_t)shape_row(g, BALL_SHAPE2, ring, r) << 16 >> sh, c = 0;
            if (row >= 0 && row < SK_BALL_MASK_ROWS && col >= 0 && col < 4) {
                const uint8_t *p = bw->mask + row * 8 + col * 2;
                c = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16;
                if (col < 3) c |= (uint32_t)p[2] << 8 | p[3];
            }
            uint32_t d = a & c;
            out[r * 4] = (uint8_t)(d >> 24); out[r * 4 + 1] = (uint8_t)(d >> 16); out[r * 4 + 2] = (uint8_t)(d >> 8); out[r * 4 + 3] = (uint8_t)d;
        }
        memcpy(bw->mask, out, 64);
    }
    int d7, d0;
    {
        int row = -1;
        for (int r = 0; r < 16; r++) if (bw->mask[r * 4] | bw->mask[r * 4 + 1] | bw->mask[r * 4 + 2] | bw->mask[r * 4 + 3]) { row = r; break; }
        if (row < 0) {                             /* no contact found: the red debug flash, straight down */
            d0 = 8; d7 = 15; b->vy = 0;
        } else {
            d0 = 31;
            int j = 3;
            while (bw->mask[row * 4 + j] == 0) { d0 -= 8; j--; }
            uint8_t byte = bw->mask[row * 4 + j];
            while (!(byte & 1)) { byte >>= 1; d0--; }
            d0 -= d4 & 15;
            bw->contact_dx = d0;
            bw->contact_dy = row;
            d7 = row;
        }
    }
    d7 = (d0 & 8) ? contact_dir_left[d7 & 15] : contact_dir_right[d7 & 15];
    bw->contact_dir = d7;
    d7 = (d7 + 4) & 0xF8;
    if (g->sk) {                                   /* the refinement by 5-pixel probe segments */
        uint32_t a2 = PROBE_TABLE + (uint32_t)d7;
        if (probe(g, &a2, d2, d3, false)) goto l6e;
        d7 = u8(d7 + 8); a2 = PROBE_TABLE + (uint32_t)d7;
        if (probe(g, &a2, d2, d3, false)) goto l64;
        d7 = u8(d7 + 8); a2 = PROBE_TABLE + (uint32_t)d7;
        if (probe(g, &a2, d2, d3, false)) goto l64;
        d7 = u8(d7 - 0x10);
        goto l10a;
    l64:
        if (probe(g, &a2, d2, d3, false)) goto la6;
        d7 = u8(d7 - 8);
        goto la6;
    l6e:
        if (probe(g, &a2, d2, d3, false)) goto la6;
        d7 = u8(d7 - 8); a2 = PROBE_TABLE2 + (uint32_t)d7;
        if (probe(g, &a2, d2, d3, false)) goto l9c;
        d7 = u8(d7 - 8); a2 = PROBE_TABLE2 + (uint32_t)d7;
        if (probe(g, &a2, d2, d3, false)) goto l9c;
        d7 = u8(d7 + 0x10);
        goto l10a;
    l9c:
        a2 -= 8;
        if (probe(g, &a2, d2, d3, false)) goto la6;
        d7 = u8(d7 + 8);
        goto la6;
    l10a:                                          /* the same against free space around the colliding point */
        d7 = u8(d7 + 0x80); a2 = PROBE_TABLE + (uint32_t)d7;
        if (probe(g, &a2, d4, d5, true)) goto m14c;
        d7 = u8(d7 + 8); a2 = PROBE_TABLE + (uint32_t)d7;
        if (probe(g, &a2, d4, d5, true)) goto m142;
        d7 = u8(d7 + 8); a2 = PROBE_TABLE + (uint32_t)d7;
        if (probe(g, &a2, d4, d5, true)) goto m142;
        d7 = u8(d7 - 0x10);
        goto m182;
    m142:
        if (probe(g, &a2, d4, d5, true)) goto m182;
        d7 = u8(d7 - 8);
        goto m182;
    m14c:
        if (probe(g, &a2, d4, d5, true)) goto m182;
        d7 = u8(d7 - 8); a2 = PROBE_TABLE2 + (uint32_t)d7;
        if (probe(g, &a2, d4, d5, true)) goto m178;
        d7 = u8(d7 - 8); a2 = PROBE_TABLE2 + (uint32_t)d7;
        if (probe(g, &a2, d4, d5, true)) goto m178;
        d7 = u8(d7 + 0x10);
        goto m182;
    m178:
        a2 -= 8;
        if (probe(g, &a2, d4, d5, true)) goto m182;
        d7 = u8(d7 + 8);
    m182:
        d7 = u8(d7 + 0x80);
    }
la6:
    d7 = u8(d7 + 0x40);
    {
        int d6 = d7;                               /* the surface normal */
        int in = atan2_256(x - b->prev_x, b->prev_y - y);
        bw->in_dir = in;
        int d1 = in;
        d7 = u8(u8(d7 + d7) - in + 0x80);          /* the reflection */
        int t = u8(u8(in - d7) + 0x10);
        if (t < 0x10) {                            /* close to the normal: snap */
            d7 = u8(d7 + 8);
            if ((int8_t)u8(d1 - d6) >= 0) d7 = u8(d7 - 0x10);
        }
        /* speed = sqrt(vx^2 + vy^2) in 1/16 px (16.16 >> 10 per axis), then friction */
        uint32_t ax = (uint32_t)(b->vx < 0 ? -b->vx : b->vx), ay = (uint32_t)(b->vy < 0 ? -b->vy : b->vy);
        uint32_t sx = (uint16_t)((ax << 6) >> 16), sy = (uint16_t)((ay << 6) >> 16);
        uint32_t sum = sx * sx + sy * sy;
        int s = 0, step = 0x1000;
        while (step) {
            uint32_t sq = (uint32_t)(uint16_t)s * (uint16_t)s;
            if (sum < sq) s -= step; else s += step;
            step >>= 1;
        }
        if ((uint32_t)(uint16_t)s * (uint16_t)s > sum) s--;
        int16_t speed = (int16_t)(s << 4);
        int fr = (uint16_t)speed >> 8;             /* move.b $14(a6): the speed word's high byte */
        if (!fr) fr = 1;
        speed = (int16_t)(speed - fr);
        if ((unsigned)bw->contact_dy < 8 || bw->min_bounce < speed) {
            b->vx = sin_256(d7) * speed;
            b->vy = -cos_256(d7) * speed;
        } else {                                   /* rolling along the surface */
            if ((uint16_t)speed < 0x200) speed = 0;
            d7 = u8(d6 + 0x40) | 0x80;
            int ang = (int16_t)(d7 - in);
            int32_t v = (int32_t)sin_256(ang) * speed;
            speed = (int16_t)(v < 0 ? -(int32_t)((uint32_t)-v >> 6) : (int32_t)((uint32_t)v >> 6));
            d7 = u8(d6 + 0x40) | 0x80;
            b->vx = sin_256(d7) * speed;
            b->vy = -cos_256(d7) * speed;
            int32_t slope = (int32_t)cos_256(u8(d6 + 0x40)) * 0x200 + 0x1000;
            if ((uint32_t)slope >= 0x2000) b->vx += slope - 0x1000;
        }
        b->speed = speed;
    }
    b->x = (int32_t)((uint32_t)(uint16_t)d2 << 16 | ((uint32_t)b->x & 0xFFFF));
    b->y = (int32_t)((uint32_t)(uint16_t)d3 << 16 | ((uint32_t)b->y & 0xFFFF));
}

void ball_tile_attrs(sk_game *g) {                 /* 0x11A620: the 3 x 3 cells around the ball, probe_kind 1 */
    g->bw.carry_dx = 0;
    int x0 = ((g->ball.x >> 16) & 0xFFF0) - 16, y0 = ((g->ball.y >> 16) & 0xFFF0) - 16;
    x0 = (int16_t)x0; y0 = (int16_t)y0;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) return;
    attrs_run_cells(g, 1, x0, y0, 3, 3);
}

void update_ball(sk_game *g) {                     /* 0x1098CA */
    sk_ball *b = &g->ball;
    sk_player *p = &g->player;
    b->lifted = 0;
    ball_follow_player(g);
    if (b->inactive) return;
    if (p->ball_hold >= 0 && g->bw.carry_dx == 0 && b->at_rest && (b->vx >> 16) == 0 && (b->vy >> 16) == 0) {
        b->prev_x = (int16_t)(b->x >> 16);
        b->prev_y = (int16_t)(b->y >> 16);
        b->vx = b->vy = 0;
        ball_tile_attrs(g);
        return;
    }
    b->at_rest = 0;
    b->prev_x = (int16_t)(b->x >> 16);
    b->prev_y = (int16_t)(b->y >> 16);
    ball_at_feet(g);
    ball_move(g);
    ball_bounce(g);
    ball_tile_attrs(g);
}

void ball_sprites(sk_game *g, sk_sprite *spr) {     /* 0x10A2EA */
    sk_ball *b = &g->ball;
    uint32_t entry;
    if (b->kick_anim) {
        b->kick_anim--;
        b->inactive = 0;
        entry = SPIN_TABLE + (uint32_t)((5 - (b->kick_anim >> 1)) * 8);
    } else if (b->vanishing) {
        if (++g->bw.roll_phase < 12) entry = VANISH_TABLE + (uint32_t)((g->bw.roll_phase >> 2) * 8);
        else {                                     /* 0x10A340: vanished, then the roll path below */
            b->vanishing = 0; b->inactive = 1; g->player.ball_hold = 0;
            g->bw.roll_phase = (g->bw.roll_phase + ((int16_t)(b->x >> 16) - b->prev_x)) & 15;
            entry = ROLL_TABLE + (uint32_t)((g->bw.roll_phase >> 2) * 8);
        }
    } else {
        g->bw.roll_phase = (g->bw.roll_phase + ((int16_t)(b->x >> 16) - b->prev_x)) & 15;
        entry = ROLL_TABLE + (uint32_t)((g->bw.roll_phase >> 2) * 8);
    }
    int sx = (int16_t)(b->x >> 16) - g->view.cam_x;
    if (sx > 0x190 || sx <= -0x40) sx = 0x190;
    int sy = (int16_t)(b->y >> 16) - g->view.cam_y;
    spr->visible = false;
    if (b->inactive || sx == 0x190) return;
    spr->x = sx + 16;                              /* HSTART = x + 0x80; buffer column 0 = world cam_x - 16 */
    spr->y = sy;
    if (g->sk) {
        uint32_t off = image_u32(g->sk, entry);
        if (!sprite_decode_pair(g->sk, SK_KID_FRAMES_ADDR + off, SK_BALL_SPR_H, spr)) return;
    } else {
        memset(spr->px, 0, sizeof spr->px);
        for (int r = 0; r < 16; r++) for (int c = 0; c < 16; c++) if (disc[r] >> (15 - c) & 1) spr->px[r * SK_SPR_W + c] = 1;
    }
    spr->visible = true;
}
