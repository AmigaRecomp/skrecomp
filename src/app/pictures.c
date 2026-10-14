#include <stdlib.h>
#include <string.h>
#include "app/pictures.h"
#include "core/bytekiller.h"
#include "formats/screens.h"
#include "gfx/display_model.h"

#define LOADING_STRIP_ADDR 0x208CC2u    /* loading_screen: a ByteKiller container in sk */
#define LOADING_STRIP_PACKED (4u + 5444u)   /* the length word and its payload */
#define LOADING_TOP 58                  /* copper_loading DIW lines 102..186, the frame starts at line 44 */
#define LOADING_H 84
#define LOADING_FRAMES 25

/* map_locations (0x200B9A): the centre of each stop and its medallion in chunk 0 (VERIFIED, between-levels.md 1.5) */
static const struct { int16_t x, y; uint16_t off; } map_locations[6] = {
    { 149, 44, 0x0000 }, { 164, 70, 0x0630 }, { 179, 35, 0x0C60 }, { 274, 65, 0x1290 }, { 63, 61, 0x18C0 }, { 149, 44, 0x0000 } };
/* world_map 0x11E5DA..0x11E680: the cup pieces' overlays (64 x 40 from chunk 4) by cup_pieces bit */
static const struct { int16_t x, y; uint16_t off; } trophy_pieces[5] = {
    { 9, 159, 0x0000 }, { 8, 97, 0x0B40 }, { 9, 143, 0x1680 }, { 5, 116, 0x21C0 }, { 5, 91, 0x2D00 } };

static void clear(sk_pictures *pc, sk_pic_kind kind) {
    uint8_t *pack = pc->pack;
    free(pack);
    memset(pc, 0, sizeof *pc);
    pc->kind = kind;
}

bool pictures_loading_from(sk_pictures *pc, const uint8_t *strip, size_t n, sk_err *e) {
    sk_screen s;
    if (!screen_parse(strip, n, 320, LOADING_H, 8, true, 0, true, &s, e)) return false;
    clear(pc, SK_PIC_LOADING);
    for (int y = 0; y < LOADING_H; y++)
        for (int x = 0; x < 320; x++) pc->px[(LOADING_TOP + y) * 320 + x] = (uint8_t)screen_pixel(&s, x, y);
    memcpy(pc->pal, s.palette, sizeof pc->pal);
    pc->pal[0] = 0;                                              /* the frame outside the window: copper_loading's COLOR00 */
    return true;
}

static void draw_marker(sk_pictures *pc) {                       /* plane_erase + plane_draw (0x11E894): restore, then draw */
    memcpy(pc->px, pc->base, sizeof pc->base);
    int x = flight_draw_x(&pc->flight), y = (int16_t)(pc->flight.y >> 16);
    for (int wrap = -320; wrap <= 320; wrap += 320) {            /* ruling: clipped and wrapped (the original runs off the row) */
        masked8_draw(pc->spr.chunk[2], pc->spr.len[2], (size_t)pc->flight.big_off, 32, 19, pc->px, 320, 200, x + wrap - 16, y - 25);
        masked8_draw(pc->spr.chunk[3], pc->spr.len[3], (size_t)pc->flight.small_off, 16, 13, pc->px, 320, 200, x + wrap - 8, y - 6);
    }
}

bool pictures_map_from(sk_pictures *pc, const uint8_t *pic, size_t pic_n, const uint8_t *pack, size_t pack_n, int map_index,
                       uint8_t cup_pieces, sk_err *e) {
    if (map_index < 0 || map_index > 5) return sk_fail(e, "world map %d: there are six", map_index);
    sk_screen s;
    if (!screen_parse(pic, pic_n, 320, 200, 8, true, 0, true, &s, e)) return false;
    clear(pc, SK_PIC_MAP);
    pc->pack = malloc(pack_n ? pack_n : 1);
    if (!pc->pack) return sk_fail(e, "out of memory");
    memcpy(pc->pack, pack, pack_n);
    if (!mapsprites_parse(pc->pack, pack_n, &pc->spr, e)) return false;
    for (int y = 0; y < 200; y++)
        for (int x = 0; x < 320; x++) pc->base[y * 320 + x] = (uint8_t)screen_pixel(&s, x, y);
    memcpy(pc->pal, s.palette, sizeof pc->pal);
    for (int k = 0; k < 5; k++)                                  /* in bit order, before the medallion */
        if (cup_pieces & (1u << k))
            masked8_draw(pc->spr.chunk[4], pc->spr.len[4], trophy_pieces[k].off, 64, 40, pc->base, 320, 200, trophy_pieces[k].x, trophy_pieces[k].y);
    masked8_draw(pc->spr.chunk[0], pc->spr.len[0], map_locations[map_index].off, 32, 44, pc->base, 320, 200,
                 map_locations[map_index].x - 16, map_locations[map_index].y - 22);
    memcpy(pc->px, pc->base, sizeof pc->base);
    pc->map_index = map_index;
    if (map_index > 0) {                                         /* plane_flight from the previous stop */
        flight_init(&pc->flight, map_locations[map_index - 1].x, map_locations[map_index - 1].y,
                    map_locations[map_index].x, map_locations[map_index].y);
        draw_marker(pc);
    } else pc->phase = 2;
    return true;
}

bool pictures_ending_from(sk_pictures *pc, const uint8_t *pic, size_t n, bool good, sk_err *e) {
    (void)good;
    sk_screen s;
    if (!screen_parse(pic, n, 320, 256, 8, true, 0, true, &s, e)) return false;
    clear(pc, SK_PIC_ENDING);
    for (int y = 0; y < 256; y++)
        for (int x = 0; x < 320; x++) pc->px[y * 320 + x] = (uint8_t)screen_pixel(&s, x, y);
    memcpy(pc->pal, s.palette, sizeof pc->pal);
    return true;
}

bool pictures_loading(sk_pictures *pc, const sk_image *sk, sk_err *e) {
    const uint8_t *c = image_ptr(sk, LOADING_STRIP_ADDR, LOADING_STRIP_PACKED);
    if (!c) return sk_fail(e, "sk: the loading strip is outside the image");
    uint8_t *strip = NULL; size_t n = 0;
    if (!bk_unpack(c, LOADING_STRIP_PACKED, &strip, &n, e)) return false;
    bool ok = pictures_loading_from(pc, strip, n, e);
    free(strip);
    return ok;
}

bool pictures_map(sk_pictures *pc, sk_disks *d, int map_index, uint8_t cup_pieces, sk_err *e) {
    uint8_t *pic = NULL, *pack = NULL; size_t pn = 0, kn = 0;   /* the same files on every disk: disk 1's */
    bool ok = disks_read_file_no(d, 1, 252, true, &pic, &pn, e) && disks_read_file_no(d, 1, 253, true, &pack, &kn, e) &&
              pictures_map_from(pc, pic, pn, pack, kn, map_index, cup_pieces, e);
    free(pic); free(pack);
    return ok;
}

bool pictures_ending(sk_pictures *pc, sk_disks *d, bool good, sk_err *e) {
    uint8_t *pic = NULL; size_t n = 0;
    bool ok = disks_read_file_no(d, 3, good ? 250 : 251, true, &pic, &n, e) && pictures_ending_from(pc, pic, n, good, e);
    free(pic);
    return ok;
}

bool pictures_step(sk_pictures *pc, uint8_t input) {
    int f = pc->frame++;
    switch (pc->kind) {
    case SK_PIC_LOADING: return f + 1 >= LOADING_FRAMES;         /* vbl_counter >= 25 */
    case SK_PIC_MAP:
        if (pc->phase == 0) {                                    /* standing: vbl_counter > 0x32 */
            if (f < 0x32) return false;
            pc->phase = 1; pc->frame = 0;
            return false;
        }
        if (pc->phase == 1) {                                    /* loc_11E842: move, draw, one VBL */
            bool there = flight_step(&pc->flight);
            draw_marker(pc);
            if (there) { pc->phase = 2; pc->frame = 0; }
            return false;
        }
        return f >= (pc->map_index ? 0x64 : 0xC8);              /* attract_counter: vbl_counter > it */
    case SK_PIC_ENDING:
        if (pc->phase == 0) {
            if (!(input & SK_IN_FIRE)) return false;             /* input_state bit 4, no timeout */
            pc->phase = 1; pc->frame = 0; pc->fading = true;
            return false;
        }
        return f + 1 >= 64;                                      /* fade_music_out */
    default: return true;
    }
}

void pictures_present(const sk_pictures *pc, uint32_t *out) {
    for (int i = 0; i < SK_SCREEN_W * SK_SCREEN_H; i++) out[i] = pc->pal[pc->px[i]];
}

void pictures_free(sk_pictures *pc) { clear(pc, SK_PIC_OFF); }
