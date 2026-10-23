/* Play-testing skld's document: the same path as soccerkid --custom, on a private copy of the level. */
#ifndef SK_EDITOR_PLAYTEST_H
#define SK_EDITOR_PLAYTEST_H

#include <stdbool.h>
#include "app/play.h"
#include "editor/doc.h"

/* Opens the game on the document's level; from_cursor starts the kid at cell (cx, cy). The caller runs and closes it. */
bool playtest_open(sk_play *p, const sk_args *base, const sk_doc *d, bool from_cursor, int cx, int cy, sk_err *e);

#endif
