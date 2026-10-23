/* A custom level made with skld (.skl): an original level's setting (its tiles, markers, attributes, backdrop, music)
   with a map of its own. "SKLEVEL1", BE16 setting (data index), width, height, marker count (0), the markers (3 bytes
   each), the cell words (BE16) and the cells' initial cell_state bytes. No game data is stored. */
#ifndef SK_FORMATS_CUSTOM_H
#define SK_FORMATS_CUSTOM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"
#include "formats/level.h"

#define SK_CUSTOM_MAX_CELLS 16384   /* cell_state lives in the map chunk's 0x4000-byte gap */
#define SK_CUSTOM_MIN_W 22          /* the 352-pixel scroll buffer: narrower, the camera cannot move and the exit walk (x - camera > 320) never ends */
#define SK_CUSTOM_MIN_H 13

typedef struct { int index; const char *name; const char *title; } sk_setting;
typedef struct sk_custom_level { int setting, w, h; uint16_t *cells; uint8_t *states; } sk_custom;

int custom_setting_count(void);
const sk_setting *custom_setting_at(int i);
const sk_setting *custom_setting_by_name(const char *name);     /* case-insensitive, NULL when unknown */
const sk_setting *custom_setting_by_index(int data_index);      /* NULL when the level cannot be a setting */

bool custom_alloc(sk_custom *c, int setting, int w, int h, sk_err *e);   /* zeroed cells and states */
void custom_free(sk_custom *c);
bool custom_copy(sk_custom *dst, const sk_custom *src);          /* false: out of memory */
bool custom_parse(const uint8_t *d, size_t n, sk_custom *c, sk_err *e);
bool custom_write(const sk_custom *c, uint8_t **out, size_t *n);
bool custom_save(const char *path, const sk_custom *c, sk_err *e);      /* a temporary file renamed over the target */
bool custom_load(const char *path, sk_custom *c, sk_err *e);
/* A cell flag the setting does not have becomes 0: markers 1..31 need a type in the PALS table or a use in the
   setting's map, attributes 33..127 a use in the setting's map, flag 32 is never valid. Returns the cells changed. */
int custom_clear_unknown_flags(sk_custom *c, const sk_level *setting);
bool custom_apply(const sk_custom *c, sk_level *lv);            /* replaces lv->map; false: out of memory */

#endif
