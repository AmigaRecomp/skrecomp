/* The camera (update_camera 0x106190, camera_target 0x106C32) and the spawn scans its scrolling triggers
   (build_column_strip 0x1064C2, build_row_strip 0x106570, edge_scan_columns 0x106DBE, edge_scan_rows 0x106E0C). */
#ifndef SK_GAME_CAMERA_H
#define SK_GAME_CAMERA_H

typedef struct sk_game sk_game;

void camera_target(sk_game *g, int *tx, int *ty);   /* the look-ahead target before the (144, 100) offset */
void update_camera(sk_game *g);                     /* clamp, push_x, shake; strips for the camera's cell delta */
void scan_column_strip(sk_game *g, int n, int x);   /* n columns from x: 14 rows from the view's cam_y; arms 3 edge columns */
void scan_row_strip(sk_game *g, int n, int y);      /* n rows from y: 21 cells from the view's cam_x; arms 3 edge rows */
void edge_scan_columns(sk_game *g);                 /* one pending column, 20 cells */
void edge_scan_rows(sk_game *g);                    /* one pending row, 26 cells; waits for the columns */

#endif
