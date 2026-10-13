#include "game/camera.h"
#include "game/game.h"
#include "game/objects.h"

void camera_target(sk_game *g, int *tx, int *ty) {               /* sub_106C32 */
    sk_player *p = &g->player;
    int x = p->x - 14, y = p->y;
    if (g->push_x) {
        x = g->camera_x + 144;
        int d = g->push_x - g->camera_x;
        if (d > 0) x += d < 4 ? d : 4;
    } else {
        int target;
        if (p->state == 44 || p->state == 48 || (p->state == 16 && p->trick == 4)) target = 0;
        else target = p->facing >= 0 ? 64 : -64;
        int d = g->look_x - target;
        if (d) {
            int step = (d < 0 ? -d : d) / 10;
            if (step == 0) step = 1;
            if (d > 0) { g->look_x -= step; if (g->look_x < target) g->look_x = target; }
            else { g->look_x += step; if (g->look_x > target) g->look_x = target; }
        }
        x += g->look_x;
    }
    if (p->dying) { *tx = x; *ty = g->death_target_y; return; }
    int limit = 10;
    if (g->scroll_mode == 1) { y += 44; limit += 44; }
    if (!g->view.full_redraw) {
        int ease = g->cam_ease;
        if (p->jumping && p->jump_y - y >= 0) {
            int d = y - g->camera_y_raw;
            if (d < limit) { p->jump_y = (int16_t)(g->camera_y_raw + 100); ease = limit - d - 1; }   /* near the top: follow */
            else y = p->jump_y;                                  /* otherwise the camera holds still during the jump */
        }
        if (p->crouching) y = p->jump_y;
        int centre = g->camera_y_raw + 100, d = centre - y;
        if (d > 0) y = centre - ((uint16_t)d < (uint16_t)ease ? d : ease);            /* bcc: an unsigned compare */
        else if (d < 0 && p->crouching) { if (p->falling) ease = 8; y = centre + ((uint16_t)-d < (uint16_t)ease ? -d : ease); }
    }
    *tx = x;
    *ty = y;
}

static void scroll_columns(sk_game *g) {                         /* sub_10641A */
    int old = g->view.cam_x & ~15, now = g->camera_x & ~15;
    g->view.cam_x = g->camera_x;
    int d = old - now;
    if (d == 0) return;
    if (d < 0) {                                                 /* scrolled right: the new buffer column at cam + 320 */
        g->scrolled_right = true;
        int n = -d >> 4;
        scan_column_strip(g, n, ((g->view.cam_x + 0x150) & ~15) - (-d & ~15));
    } else {
        g->scrolled_right = false;
        scan_column_strip(g, d >> 4, now);
    }
}

static void scroll_rows(sk_game *g) {                            /* sub_10646E */
    int old = g->view.cam_y & ~15, now = g->camera_y & ~15;
    g->view.cam_y = g->camera_y;
    int d = old - now;
    if (d == 0) return;
    if (d < 0) {
        g->scrolled_down = true;
        scan_row_strip(g, -d >> 4, ((g->view.cam_y + 0xE0) & ~15) - (-d & ~15));
    } else {
        g->scrolled_down = false;
        scan_row_strip(g, d >> 4, now);
    }
}

void update_camera(sk_game *g) {                                 /* 0x106190 */
    int tx, ty;
    camera_target(g, &tx, &ty);
    tx -= 144;
    if (tx < 0) tx = 0;
    if ((unsigned)g->push_x <= (unsigned)g->camera_x && (unsigned)tx < (unsigned)g->push_x) tx = g->push_x;
    if (tx >= g->world.fg_max_x) tx = g->world.fg_max_x;
    ty -= 100;
    if (ty < 0) ty = 0;
    if (ty >= g->world.fg_max_y) ty = g->world.fg_max_y;
    g->camera_y_raw = ty;
    ty += g->shake_amplitude;
    if (ty < 0) ty = 0;
    if (ty >= g->world.fg_max_y) ty = g->world.fg_max_y;
    g->camera_x = tx;
    g->camera_y = ty;
    if (g->view.full_redraw) {
        g->view.full_redraw = false;
        g->view.cam_x = g->camera_x;
        g->view.cam_y = g->camera_y;
        scan_column_strip(g, 21, g->camera_x & ~15);
    }
    scroll_columns(g);
    scroll_rows(g);
}

void scan_column_strip(sk_game *g, int n, int x) {               /* build_column_strip's marker scan */
    int cx0 = x >> 4, cy0 = (g->view.cam_y & ~15) >> 4;
    for (int c = 0; c < n; c++)
        for (int r = 0; r < 14; r++) scan_cell_for_marker(g, cx0 + c, cy0 + r);
    g->scan_col_left = 3;
    g->scan_col_cx = g->scrolled_right ? cx0 + n : cx0 - 3;
    g->scan_col_cy0 = cy0 - 3;
}

void scan_row_strip(sk_game *g, int n, int y) {                  /* build_row_strip's marker scan */
    int cy0 = y >> 4, cx0 = (g->view.cam_x & ~15) >> 4;
    for (int r = 0; r < n; r++)
        for (int c = 0; c < 21; c++) scan_cell_for_marker(g, cx0 + c, cy0 + r);
    g->scan_row_left = 3;
    g->scan_row_cy = g->scrolled_down ? cy0 + n : cy0 - 3;
    g->scan_row_cx0 = cx0 - 3;
}

void edge_scan_columns(sk_game *g) {                             /* 0x106DBE */
    if (g->scan_col_left <= 0) return;
    for (int r = 0; r < 20; r++) scan_cell_for_marker(g, g->scan_col_cx, g->scan_col_cy0 + r);
    g->scan_col_cx++;
    g->scan_col_left--;
}

void edge_scan_rows(sk_game *g) {                                /* 0x106E0C: only while no column scan is pending */
    if (g->scan_col_left > 0 || g->scan_row_left <= 0) return;
    for (int c = 0; c < 26; c++) scan_cell_for_marker(g, g->scan_row_cx0 + c, g->scan_row_cy);
    g->scan_row_cy++;
    g->scan_row_left--;
}
