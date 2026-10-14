/* The full-screen modes between the levels (re/docs/notes/between-levels.md): the loading strip (loading_screen 0x102B5A),
   the world map (world_map 0x11E45C, plane_flight 0x11E778) and the end screens (end_screens 0x11EFAC). Each is a 320 x 256
   index image through a 256-colour palette, stepped once a frame. */
#ifndef SK_APP_PICTURES_H
#define SK_APP_PICTURES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/disks.h"
#include "core/err.h"
#include "core/hunk.h"
#include "formats/mapsprites.h"
#include "game/between.h"
#include "game/input.h"

typedef enum { SK_PIC_OFF, SK_PIC_LOADING, SK_PIC_MAP, SK_PIC_ENDING } sk_pic_kind;

typedef struct {
    sk_pic_kind kind;
    uint8_t px[320 * 256];          /* frame lines 0..255 (DIWSTRT line 44) */
    uint32_t pal[256];
    uint8_t base[320 * 200];        /* the map with its pieces and medallion (what plane_erase restores) */
    int frame;                      /* frames into the current phase */
    int phase;                      /* map: 0 standing, 1 flying, 2 holding; ending: 0 waiting, 1 fading */
    int map_index;
    uint8_t *pack; sk_mapsprites spr;
    sk_flight flight;
    bool fading;                    /* the ending's fade_music_out has begun */
} sk_pictures;

/* From the files' bytes (tests); the strip is 320 x 84 + palette, the map 320 x 200 + palette with file253, the ending
   320 x 256 + palette. */
bool pictures_loading_from(sk_pictures *pc, const uint8_t *strip, size_t n, sk_err *e);
bool pictures_map_from(sk_pictures *pc, const uint8_t *pic, size_t pic_n, const uint8_t *pack, size_t pack_n, int map_index,
                       uint8_t cup_pieces, sk_err *e);
bool pictures_ending_from(sk_pictures *pc, const uint8_t *pic, size_t n, bool good, sk_err *e);
/* From the game's data: the strip in sk (0x208CC2), file252 / file253 (disk 1), file250 / file251 (disk 3). */
bool pictures_loading(sk_pictures *pc, const sk_image *sk, sk_err *e);
bool pictures_map(sk_pictures *pc, sk_disks *d, int map_index, uint8_t cup_pieces, sk_err *e);
bool pictures_ending(sk_pictures *pc, sk_disks *d, bool good, sk_err *e);
bool pictures_step(sk_pictures *pc, uint8_t input);     /* one frame; true when the mode is over */
void pictures_present(const sk_pictures *pc, uint32_t *out);   /* SK_SCREEN_W x SK_SCREEN_H */
void pictures_free(sk_pictures *pc);

#endif
