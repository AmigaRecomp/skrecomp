/* skld's window state, shared by the event loop (skld.c) and the drawing (ui.c). */
#ifndef SK_EDITOR_EDITOR_H
#define SK_EDITOR_EDITOR_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include "app/args.h"
#include "app/scene.h"
#include "editor/check.h"
#include "editor/doc.h"
#include "editor/pieces.h"

#define ED_CHAR 16                  /* the debug font drawn at twice its 8 x 8 */
#define ED_PANEL_W 480              /* the side panel */
#define ED_STATUS_H 34              /* the status line */
#define ED_TAB_W 228                /* the tabs: two rows of two */
#define ED_TAB_H 28
#define ED_TABS_H 76
#define ED_TILE_CELL 34             /* a 2x tile and its gap in the tile grid */
#define ED_TILE_COLS ((ED_PANEL_W - 16) / ED_TILE_CELL)
#define ED_PIECE_CELL 116           /* a piece's box in the pieces grid */
#define ED_PIECE_COLS ((ED_PANEL_W - 8) / ED_PIECE_CELL)
#define ED_ROW_H 24                 /* a row of the object and special lists */
#define ED_ISSUE_H 20               /* a line of the checks */
#define ED_ISSUES_H 136             /* the checks at the bottom of the panel */

enum { ED_TAB_TILES, ED_TAB_OBJECTS, ED_TAB_SPECIALS, ED_TAB_PIECES, ED_TABS };
enum { ED_DRAG_NONE, ED_DRAG_PAINT, ED_DRAG_ERASE, ED_DRAG_FILL, ED_DRAG_SELECT, ED_DRAG_SCROLL };

typedef struct {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *map_tex; int map_tw, map_th; uint32_t *map_px;   /* the map picture in game pixels */
    SDL_Texture *tiles_tex;                                         /* the setting's tiles, 16 to a row */
    sk_scene scene;                 /* disks, tables, the setting's own level (pristine) and its colours */
    sk_doc doc;
    char path[1024];
    sk_args base;                   /* the data and paths the play-test starts from */
    int markers[32], n_markers;     /* the setting's marker flags 1..31 */
    int attrs[96], n_attrs;         /* its attribute flags 33..127 */
    int cam_x, cam_y, zoom;         /* the world pixel at the view's top left */
    int tab, tile, flag, piece;     /* the selection: tile index (tiles), flag (objects, specials), piece */
    sk_pieces pieces;               /* the setting's pieces, from its original map */
    bool start;                     /* specials: the start position is selected */
    int scroll[ED_TABS];            /* each tab's list scroll, in rows */
    float wheel_list, wheel_zoom;   /* trackpad wheel steps not yet whole */
    bool grid, overlay, solid, frame, help, quit_prompt, running;
    int mouse_x, mouse_y, cell_x, cell_y;
    bool mouse_in;                  /* the mouse is in the window (it has moved there) */
    bool cell_ok;                   /* the mouse is over a cell of the map */
    int drag, ax, ay, last_x, last_y, drag_mx, drag_my, drag_cx, drag_cy;
    bool has_sel; int sx0, sy0, sx1, sy1;
    sk_clip clip;
    sk_issue issues[8]; int n_issues;
    char msg[200]; uint64_t msg_until;
    char title[256];
} skld;

void ed_view_size(const skld *k, int *w, int *h);    /* the map view in window pixels */
void ed_draw(skld *k);
void ed_message(skld *k, const char *fmt, ...);
bool ed_build_tiles(skld *k);                        /* the tile palette texture */
int ed_piece_at(const skld *k, int x, int y);         /* the piece under a window point on the panel, -1 none */
void ed_piece_origin(const skld *k, int *x, int *y);  /* the cell for the selected piece's top left: it stands on the mouse */

#endif
