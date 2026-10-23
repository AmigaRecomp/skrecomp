/* skld's checks: what would stop a custom level from working, and what may surprise its player. */
#ifndef SK_EDITOR_CHECK_H
#define SK_EDITOR_CHECK_H

#include <stdbool.h>
#include "editor/doc.h"

#define SK_CHECK_SLOTS 16           /* the game's object slots */
#define SK_CHECK_SCREEN_COLS 18     /* one 288-pixel screen */

typedef enum { SK_ISSUE_NO_START, SK_ISSUE_MANY_STARTS, SK_ISSUE_NO_EXIT, SK_ISSUE_CARDS, SK_ISSUE_CROWDED } sk_issue_kind;
typedef struct { sk_issue_kind kind; bool error; int x, y, count; char text[96]; } sk_issue;

/* Fills at most max issues, errors first; returns how many. */
int edit_check(const sk_doc *d, const sk_level *setting, sk_issue *out, int max);

#endif
