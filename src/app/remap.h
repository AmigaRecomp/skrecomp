/* The remap screen (a port addition, F12): one action at a time, the next key or pad input becomes its binding. Backspace keeps
   the action's binding, F5 restores every default, Escape cancels; the caller saves the file when it is done. */
#ifndef SK_APP_REMAP_H
#define SK_APP_REMAP_H

#include <stdbool.h>
#include <stdint.h>
#include "app/controls.h"
#include "core/hunk.h"

typedef enum { SK_REMAP_STAY, SK_REMAP_DONE, SK_REMAP_CANCEL } sk_remap_result;

typedef struct {
    bool open;
    int step;                           /* the action being asked for */
    sk_controls *live, before;
} sk_remap;

void remap_open(sk_remap *r, sk_controls *live);
sk_remap_result remap_input(sk_remap *r, sk_bind b);
/* The screen over the game's frame: dimmed, the actions and their bindings in the game's 8x8 font (sk may be NULL). */
void remap_draw(const sk_remap *r, const sk_image *sk, uint32_t *out);

#endif
