#include "editor/playtest.h"

bool playtest_open(sk_play *p, const sk_args *base, const sk_doc *d, bool from_cursor, int cx, int cy, sk_err *e) {
    sk_args a = *base;
    a.level = d->lvl.setting;
    a.custom = &d->lvl;                                      /* play_open copies it */
    a.script = NULL; a.frames = 0; a.screenshot = NULL; a.viewer = false; a.new_game = false;
    a.has_start = from_cursor;
    a.start_x = cx * 16; a.start_y = cy * 16 - 24;           /* as find_start_position places the kid on a start tile */
    return play_open(p, &a, e);
}
