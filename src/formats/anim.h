/* The intro's animations (re/docs/notes/intro.md): animfil1/2 are a table of offsets to IFF FORM ANIM blocks, each a
   ByteRun1 ILBM key frame (320 x 200, 5 planes, a 32-colour CMAP) then FORM ILBM { ANHD, DLTA } op-5 deltas, interleave 2. */
#ifndef SK_FORMATS_ANIM_H
#define SK_FORMATS_ANIM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"

#define SK_ANIM_MAX 80

typedef struct { bool key; const uint8_t *data; size_t n; const uint8_t *cmap; } sk_anim_frame;   /* BODY or DLTA */
typedef struct { sk_anim_frame frame[SK_ANIM_MAX]; int count; } sk_anim;

/* Animation index of an unpacked file (build_frame_list 0x100F34 without the in-place rewrite). */
bool anim_open(const uint8_t *file, size_t n, int index, sk_anim *a, sk_err *e);
void anim_decode_body(const uint8_t *body, size_t n, uint8_t planes[6][8000]);   /* ByteRun1, 5 planes a row */
void anim_apply_delta(const uint8_t *dlta, size_t n, uint8_t planes[6][8000]);   /* op 5, the first six plane offsets */

#endif
