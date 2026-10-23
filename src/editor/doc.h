/* skld's document: a custom level on its setting, the editing operations and their undo. No SDL. */
#ifndef SK_EDITOR_DOC_H
#define SK_EDITOR_DOC_H

#include <stdbool.h>
#include <stdint.h>
#include "core/err.h"
#include "formats/custom.h"
#include "formats/level.h"

#define SK_UNDO_MAX 256
#define SK_START_TILE 0x1FF        /* find_start_position */
enum { SK_LAYER_TILE = 1, SK_LAYER_FLAG = 2, SK_LAYER_ALL = 3 };

typedef struct { int w, h; uint16_t *cells; uint8_t *states; } sk_clip;
typedef struct {
    sk_custom lvl;
    uint8_t tile_state[512];        /* the state the setting's map most often gives each tile */
    sk_custom past[SK_UNDO_MAX]; int past_n;    /* snapshots before each operation, oldest first */
    sk_custom future[SK_UNDO_MAX]; int future_n;
    bool in_stroke, dirty;
} sk_doc;

bool doc_new_copy(sk_doc *d, const sk_level *setting, int setting_index, sk_err *e);
/* An empty level: the setting's floor tile on the two bottom rows, the start at (1, h - 3), the exit at column w - 2. */
bool doc_new_empty(sk_doc *d, const sk_level *setting, int setting_index, int w, int h, sk_err *e);
void doc_open(sk_doc *d, const sk_level *setting, sk_custom *loaded);    /* takes the level over */
void doc_free(sk_doc *d);

int doc_flag_for_attr(const sk_level *setting, int attr);   /* the first flag 33..127 the setting's map uses for attr, -1 none */
int doc_floor_tile(const sk_level *setting);                /* the first solid tile below the setting's start, -1 none */

/* Operations: one undo step each (a stroke makes many one step); cells outside the map are ignored. */
void doc_paint(sk_doc *d, int x, int y, int tile);          /* keeps the flag; the state from tile_state */
void doc_place_flag(sk_doc *d, int x, int y, int flag);     /* a marker or attribute replaces the other */
void doc_place_start(sk_doc *d, int x, int y);
void doc_move_start(sk_doc *d, int x, int y);              /* every start cell emptied, then one here: one step */
void doc_erase(sk_doc *d, int x, int y, int layer);
void doc_fill(sk_doc *d, int x0, int y0, int x1, int y1, int layer, int tile, int flag);
bool doc_copy(const sk_doc *d, int x0, int y0, int x1, int y1, sk_clip *out);
void doc_paste(sk_doc *d, const sk_clip *c, int x, int y);
void clip_free(sk_clip *c);
bool doc_resize(sk_doc *d, int left, int top, int right, int bottom, sk_err *e);   /* > 0 adds, < 0 removes on that side */
bool doc_undo(sk_doc *d);
bool doc_redo(sk_doc *d);
void doc_begin_stroke(sk_doc *d);
void doc_end_stroke(sk_doc *d);

#endif
