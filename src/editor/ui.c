#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor/editor.h"
#include "editor/names.h"
#include "editor/view.h"
#include "game/world.h"

void ed_message(skld *k, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(k->msg, sizeof k->msg, fmt, ap);
    va_end(ap);
    k->msg_until = SDL_GetTicks() + 6000;
}

void ed_view_size(const skld *k, int *w, int *h) {
    int ww, wh;
    SDL_GetCurrentRenderOutputSize(k->ren, &ww, &wh);
    *w = ww - ED_PANEL_W; *h = wh - ED_STATUS_H;
    if (*w < 16) *w = 16;
    if (*h < 16) *h = 16;
}

bool ed_build_tiles(skld *k) {
    const sk_tileset *ts = &k->scene.level.tiles_a;
    int rows = (ts->count + 15) / 16;
    if (rows <= 0) return true;
    uint32_t *px = calloc((size_t)256 * 16 * rows, sizeof *px);
    if (!px) return false;
    for (int t = 0; t < ts->count; t++)
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                int v = tile_pixel(ts->tiles + (size_t)t * SK_TILE_BYTES, x, y);
                px[((t / 16) * 16 + y) * 256 + (t % 16) * 16 + x] = v ? 0xFF000000u | k->scene.palette.xrgb[v] : 0;
            }
    k->tiles_tex = SDL_CreateTexture(k->ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 256, 16 * rows);
    if (k->tiles_tex) {
        SDL_SetTextureScaleMode(k->tiles_tex, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(k->tiles_tex, SDL_BLENDMODE_BLEND);
        SDL_UpdateTexture(k->tiles_tex, NULL, px, 256 * 4);
    }
    free(px);
    return k->tiles_tex != NULL;
}

static void colour(skld *k, uint32_t rgb, int a) {
    SDL_SetRenderDrawColor(k->ren, (Uint8)(rgb >> 16), (Uint8)(rgb >> 8), (Uint8)rgb, (Uint8)a);
}

static void text_small(skld *k, float x, float y, uint32_t rgb, const char *s) {   /* the 8 x 8 font: labels in map cells */
    colour(k, rgb, 255);
    SDL_RenderDebugText(k->ren, x, y, s);
}

static void text(skld *k, float x, float y, uint32_t rgb, const char *s) {         /* the panel, status and boxes: twice as big */
    colour(k, rgb, 255);
    SDL_SetRenderScale(k->ren, ED_CHAR / 8, ED_CHAR / 8);
    SDL_RenderDebugText(k->ren, x * 8 / ED_CHAR, y * 8 / ED_CHAR, s);
    SDL_SetRenderScale(k->ren, 1, 1);
}

static void rect(skld *k, float x, float y, float w, float h, uint32_t rgb, int a, bool fill) {
    SDL_FRect r = { x, y, w, h };
    colour(k, rgb, a);
    if (fill) SDL_RenderFillRect(k->ren, &r); else SDL_RenderRect(k->ren, &r);
}

static void tile_src(int t, SDL_FRect *r) { r->x = (float)(t % 16 * 16); r->y = (float)(t / 16 * 16); r->w = r->h = 16; }

/* A piece's tiles with their top left at (x, y), each tile size pixels. */
static void draw_piece(skld *k, const sk_piece *p, float x, float y, float size) {
    if (!k->tiles_tex) return;
    for (int cy = 0; cy < p->h; cy++)
        for (int cx = 0; cx < p->w; cx++) {
            int t = p->tiles[cy * p->w + cx];
            if (t < 0 || t >= k->scene.level.tiles_a.count) continue;
            SDL_FRect src, dst = { x + (float)cx * size, y + (float)cy * size, size, size };
            tile_src(t, &src);
            SDL_RenderTexture(k->ren, k->tiles_tex, &src, &dst);
        }
}

static float piece_size(const sk_piece *p) {                 /* the tile size that fits a piece in its box, at most 2x */
    float s = (float)(ED_PIECE_CELL - 8) / (float)(p->w > p->h ? p->w : p->h);
    return s > 32 ? 32 : s;
}

int ed_tile_at(const skld *k, int x, int y, bool clamp) {
    int vw, vh;
    ed_view_size(k, &vw, &vh);
    x -= vw + 8; y -= ED_TABS_H + 4;
    int col = x < 0 ? -1 : x / ED_TILE_CELL, row = y < 0 ? -1 : y / ED_TILE_CELL;
    if (clamp) { col = col < 0 ? 0 : col >= ED_TILE_COLS ? ED_TILE_COLS - 1 : col; row = row < 0 ? 0 : row; }
    if (col < 0 || row < 0 || col >= ED_TILE_COLS || y >= vh - ED_ISSUES_H - ED_TABS_H - 4) return -1;
    int t = (row + k->scroll[ED_TAB_TILES]) * ED_TILE_COLS + col, n = k->scene.level.tiles_a.count;
    return t < n ? t : clamp && n > 0 ? n - 1 : -1;
}

int ed_piece_at(const skld *k, int x, int y) {
    int vw, vh;
    ed_view_size(k, &vw, &vh);
    x -= vw + 8; y -= ED_TABS_H + 4;
    if (x < 0 || y < 0 || y >= vh - ED_ISSUES_H - ED_TABS_H - 4) return -1;
    int col = x / ED_PIECE_CELL, i = (y / ED_PIECE_CELL + k->scroll[ED_TAB_PIECES]) * ED_PIECE_COLS + col;
    return col < ED_PIECE_COLS && i < k->pieces.n ? i : -1;
}

void ed_piece_origin(const skld *k, int *x, int *y) {
    *x = k->cell_x; *y = k->cell_y;
    if (k->piece >= 0 && k->piece < k->pieces.n) *y -= k->pieces.v[k->piece].h - 1;
}

/* The map picture: the game's renderer, then the solidity tint and the area past the map's edges. */
static void draw_map(skld *k, int vw, int vh) {
    int gw = (vw + k->zoom - 1) / k->zoom, gh = (vh + k->zoom - 1) / k->zoom;
    if (gw != k->map_tw || gh != k->map_th) {
        if (k->map_tex) SDL_DestroyTexture(k->map_tex);
        free(k->map_px);
        k->map_px = malloc((size_t)gw * gh * sizeof *k->map_px);
        k->map_tex = SDL_CreateTexture(k->ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, gw, gh);
        if (!k->map_px || !k->map_tex) return;
        SDL_SetTextureScaleMode(k->map_tex, SDL_SCALEMODE_NEAREST);
        k->map_tw = gw; k->map_th = gh;
    }
    const sk_custom *c = &k->doc.lvl;
    if (!edit_render(&k->scene.level, &k->scene.palette, c, k->cam_x, k->cam_y, gw, gh, k->map_px)) return;
    const sk_tileset *ts = &k->scene.level.tiles_a;
    for (int y = 0; y < gh; y++)
        for (int x = 0; x < gw; x++) {
            int wx = k->cam_x + x, wy = k->cam_y + y;
            uint32_t *p = &k->map_px[y * gw + x];
            if (wx < 0 || wy < 0 || wx >= c->w * 16 || wy >= c->h * 16) { *p = 0x202024; continue; }
            if (!k->solid) continue;
            int t = c->cells[(wy >> 4) * c->w + (wx >> 4)] & SK_INDEX_MASK;
            if (!ts->tails || t >= ts->count) continue;
            const uint8_t *tail = ts->tails + (size_t)t * SK_TILE_TAIL + (wy & 15) * 2;
            if (tail[(wx & 15) >> 3] & (0x80 >> (wx & 7)))
                *p = (((*p >> 16 & 0xFF) + 0xFF) / 2) << 16 | ((*p >> 8 & 0xFF) / 2) << 8 | (*p & 0xFF) / 2;
        }
    SDL_UpdateTexture(k->map_tex, NULL, k->map_px, gw * 4);
    SDL_FRect dst = { 0, 0, (float)(gw * k->zoom), (float)(gh * k->zoom) };
    SDL_RenderTexture(k->ren, k->map_tex, NULL, &dst);
}

static void cell_rect(const skld *k, int cx, int cy, SDL_FRect *r) {
    r->x = (float)((cx * 16 - k->cam_x) * k->zoom); r->y = (float)((cy * 16 - k->cam_y) * k->zoom);
    r->w = r->h = (float)(16 * k->zoom);
}

static void draw_overlays(skld *k, int vw, int vh) {
    const sk_custom *c = &k->doc.lvl;
    const sk_level *lv = &k->scene.level;
    int x0 = k->cam_x >> 4, y0 = k->cam_y >> 4, x1 = (k->cam_x + vw / k->zoom) >> 4, y1 = (k->cam_y + vh / k->zoom) >> 4;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= c->w) x1 = c->w - 1;
    if (y1 >= c->h) y1 = c->h - 1;
    SDL_SetRenderDrawBlendMode(k->ren, SDL_BLENDMODE_BLEND);
    if (k->grid) {
        colour(k, 0xFFFFFF, 40);
        for (int x = x0; x <= x1 + 1; x++) { float sx = (float)((x * 16 - k->cam_x) * k->zoom); SDL_RenderLine(k->ren, sx, 0, sx, (float)vh); }
        for (int y = y0; y <= y1 + 1; y++) { float sy = (float)((y * 16 - k->cam_y) * k->zoom); SDL_RenderLine(k->ren, 0, sy, (float)vw, sy); }
    }
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            uint16_t w = c->cells[y * c->w + x];
            int f = w >> SK_FLAG_SHIFT;
            SDL_FRect r; cell_rect(k, x, y, &r);
            char s[8];
            if ((w & SK_INDEX_MASK) == SK_START_TILE) {
                rect(k, r.x, r.y, r.w, r.h, 0x30E040, 140, true);
                text_small(k, r.x + 2, r.y + 2, 0xFFFFFF, "S");
                continue;
            }
            if (!k->overlay || !f) continue;
            if (f <= 31) {
                rect(k, r.x, r.y, r.w, r.h, 0xFFD020, 230, false);
                snprintf(s, sizeof s, "%d", f < (int)lv->pals_len - SK_PALS_TYPE ? lv->pals[SK_PALS_TYPE + f] : 0);
                text_small(k, r.x + 1, r.y + 1, 0xFFD020, s);
            } else if (f >= 33) {
                rect(k, r.x, r.y, r.w, r.h, 0x40D0F0, 120, false);
                int at = SK_PALS_ATTR + (size_t)(f - 33) < lv->pals_len ? lv->pals[SK_PALS_ATTR + f - 33] : -1;
                if (k->zoom > 1 || at == 13) { snprintf(s, sizeof s, at == 13 ? "EX" : "%d", at); text_small(k, r.x + 1, r.y + r.h - 9, 0x40D0F0, s); }
            }
        }
    if (k->has_sel || k->drag == ED_DRAG_SELECT || k->drag == ED_DRAG_FILL || k->drag == ED_DRAG_PICK) {      /* the selection or the fill being dragged */
        int a = k->drag ? k->ax : k->sx0, b = k->drag ? k->ay : k->sy0, cx = k->drag ? k->cell_x : k->sx1, cy = k->drag ? k->cell_y : k->sy1;
        if (a > cx) { int t = a; a = cx; cx = t; }
        if (b > cy) { int t = b; b = cy; cy = t; }
        SDL_FRect r0, r1; cell_rect(k, a, b, &r0); cell_rect(k, cx, cy, &r1);
        uint32_t col = k->drag == ED_DRAG_FILL ? 0xFF8040 : k->drag == ED_DRAG_PICK ? 0x60FF80 : 0x80C0FF;
        rect(k, r0.x, r0.y, r1.x + r1.w - r0.x, r1.y + r1.h - r0.y, col, 50, true);
        rect(k, r0.x, r0.y, r1.x + r1.w - r0.x, r1.y + r1.h - r0.y, col, 255, false);
    }
    if (k->cell_ok) {
        SDL_FRect r; cell_rect(k, k->cell_x, k->cell_y, &r);
        if (k->tab == ED_TAB_TILES && k->tiles_tex && !k->drag) {        /* the selected tile or brush under the mouse */
            SDL_SetTextureAlphaMod(k->tiles_tex, 160);
            if (k->brush.tiles) {
                draw_piece(k, &k->brush, r.x, r.y, r.w);
                rect(k, r.x, r.y, r.w * (float)k->brush.w, r.h * (float)k->brush.h, 0xFFFFFF, 120, false);
            } else { SDL_FRect src; tile_src(k->tile, &src); SDL_RenderTexture(k->ren, k->tiles_tex, &src, &r); }
            SDL_SetTextureAlphaMod(k->tiles_tex, 255);
        }
        if (k->tab == ED_TAB_PIECES && k->piece < k->pieces.n && !k->drag) {   /* the piece, standing on the mouse */
            const sk_piece *p = &k->pieces.v[k->piece];
            int ox, oy; ed_piece_origin(k, &ox, &oy);
            SDL_FRect o; cell_rect(k, ox, oy, &o);
            if (k->tiles_tex) SDL_SetTextureAlphaMod(k->tiles_tex, 170);
            draw_piece(k, p, o.x, o.y, o.w);
            if (k->tiles_tex) SDL_SetTextureAlphaMod(k->tiles_tex, 255);
            rect(k, o.x, o.y, o.w * (float)p->w, o.h * (float)p->h, 0xFFFFFF, 120, false);
        }
        rect(k, r.x, r.y, r.w, r.h, 0xFFFFFF, 255, false);
        if (k->frame) {                                                   /* the game's 288 x 200 view around the cell */
            int fx = k->cell_x * 16 - 144, fy = k->cell_y * 16 - 100;
            rect(k, (float)((fx - k->cam_x) * k->zoom), (float)((fy - k->cam_y) * k->zoom), (float)(288 * k->zoom), (float)(200 * k->zoom), 0xFF60FF, 255, false);
        }
    }
}

static void draw_panel(skld *k, int px, int ph) {
    rect(k, (float)px, 0, ED_PANEL_W, (float)ph, 0x26262C, 255, true);
    static const char *const names[ED_TABS] = { "Tiles", "Objects", "Specials", "Pieces" };
    for (int t = 0; t < ED_TABS; t++) {
        float x = (float)(px + 8 + t % 2 * (ED_TAB_W + 8)), y = (float)(6 + t / 2 * (ED_TAB_H + 6));
        rect(k, x, y, ED_TAB_W, ED_TAB_H, t == k->tab ? 0x4A5A8A : 0x34343C, 255, true);
        text(k, x + (float)(ED_TAB_W - (int)strlen(names[t]) * ED_CHAR) / 2, y + (ED_TAB_H - ED_CHAR) / 2, 0xFFFFFF, names[t]);
    }
    int top = ED_TABS_H + 4, bottom = ph - ED_ISSUES_H;
    const sk_level *lv = &k->scene.level;
    char s[96];
    if (k->tab == ED_TAB_TILES) {
        int count = lv->tiles_a.count;
        for (int i = k->scroll[0] * ED_TILE_COLS; i < count; i++) {
            int row = i / ED_TILE_COLS - k->scroll[0];
            float x = (float)(px + 8 + i % ED_TILE_COLS * ED_TILE_CELL), y = (float)(top + row * ED_TILE_CELL);
            if (y + ED_TILE_CELL > (float)bottom) break;
            rect(k, x, y, 32, 32, 0x101014, 255, true);
            SDL_FRect src, dst = { x, y, 32, 32 }; tile_src(i, &src);
            if (k->tiles_tex) SDL_RenderTexture(k->ren, k->tiles_tex, &src, &dst);
            if (i == k->tile && !k->brush.tiles && !k->pal_drag) { rect(k, x - 1, y - 1, 34, 34, 0xFFFFFF, 255, false); rect(k, x - 2, y - 2, 36, 36, 0xFFFFFF, 255, false); }
        }
        if (k->pal_drag || k->pal_brush) {                              /* the palette rectangle of the brush */
            int c0 = k->pal_a % ED_TILE_COLS, r0 = k->pal_a / ED_TILE_COLS - k->scroll[0], c1 = k->pal_b % ED_TILE_COLS, r1 = k->pal_b / ED_TILE_COLS - k->scroll[0];
            if (c0 > c1) { int t = c0; c0 = c1; c1 = t; }
            if (r0 > r1) { int t = r0; r0 = r1; r1 = t; }
            float x = (float)(px + 8 + c0 * ED_TILE_CELL), y = (float)(top + r0 * ED_TILE_CELL);
            float w = (float)((c1 - c0 + 1) * ED_TILE_CELL - 2), h = (float)((r1 - r0 + 1) * ED_TILE_CELL - 2);
            SDL_Rect clip = { px, top, ED_PANEL_W, bottom - top };
            SDL_SetRenderClipRect(k->ren, &clip);
            rect(k, x - 1, y - 1, w + 2, h + 2, 0xFFFFFF, 255, false); rect(k, x - 2, y - 2, w + 4, h + 4, 0xFFFFFF, 255, false);
            SDL_SetRenderClipRect(k->ren, NULL);
        }
    } else if (k->tab == ED_TAB_PIECES) {
        for (int i = k->scroll[ED_TAB_PIECES] * ED_PIECE_COLS; i < k->pieces.n; i++) {
            int row = i / ED_PIECE_COLS - k->scroll[ED_TAB_PIECES];
            float x = (float)(px + 8 + i % ED_PIECE_COLS * ED_PIECE_CELL), y = (float)(top + row * ED_PIECE_CELL);
            if (y + ED_PIECE_CELL > (float)bottom) break;
            const sk_piece *p = &k->pieces.v[i];
            float size = piece_size(p), bw = ED_PIECE_CELL - 4;
            rect(k, x, y, bw, bw, 0x101014, 255, true);
            draw_piece(k, p, x + (bw - size * (float)p->w) / 2, y + (bw - size * (float)p->h) / 2, size);
            if (i == k->piece) { rect(k, x - 1, y - 1, bw + 2, bw + 2, 0xFFFFFF, 255, false); rect(k, x - 2, y - 2, bw + 4, bw + 4, 0xFFFFFF, 255, false); }
        }
        if (!k->pieces.n) text(k, (float)px + 8, (float)top, 0xA0A0A0, "(none found)");
    } else {
        int n = k->tab == ED_TAB_OBJECTS ? k->n_markers : k->n_attrs + 1;
        for (int i = k->scroll[k->tab]; i < n; i++) {
            float y = (float)(top + (i - k->scroll[k->tab]) * ED_ROW_H);
            if (y + ED_ROW_H > (float)bottom) break;
            bool sel;
            if (k->tab == ED_TAB_OBJECTS) {
                int f = k->markers[i];
                char l[64]; edit_flag_label(lv, f, l, sizeof l);
                snprintf(s, sizeof s, "%2d %s", f, l);
                sel = k->flag == f;
            } else if (i == 0) {
                snprintf(s, sizeof s, "   Start position");
                sel = k->start;
            } else {
                int f = k->attrs[i - 1];
                char l[64]; edit_flag_label(lv, f, l, sizeof l);
                snprintf(s, sizeof s, "%2d %s", f, l);
                sel = !k->start && k->flag == f;
            }
            if (sel) rect(k, (float)px + 4, y - 4, ED_PANEL_W - 8, ED_ROW_H, 0x4A5A8A, 255, true);
            s[(ED_PANEL_W - 16) / ED_CHAR] = 0;
            text(k, (float)px + 8, y, k->tab == ED_TAB_OBJECTS ? 0xFFE070 : 0x90E0FF, s);
        }
        if (n == (k->tab == ED_TAB_OBJECTS ? 0 : 1)) text(k, (float)px + 8, (float)top, 0xA0A0A0, "(none in this setting)");
    }
    rect(k, (float)px, (float)bottom, ED_PANEL_W, 1, 0x50505A, 255, true);
    if (!k->n_issues) text(k, (float)px + 8, (float)bottom + 8, 0x80E080, "No problems found");
    for (int i = 0; i < k->n_issues && i < 6; i++) {
        snprintf(s, sizeof s, "%s", k->issues[i].text);
        s[(ED_PANEL_W - 16) / ED_CHAR] = 0;
        text(k, (float)px + 8, (float)(bottom + 8 + i * ED_ISSUE_H), k->issues[i].error ? 0xFF6060 : 0xFFB040, s);
    }
}

static void draw_status(skld *k, int ww, int wh) {
    rect(k, 0, (float)(wh - ED_STATUS_H), (float)ww, ED_STATUS_H, 0x18181C, 255, true);
    char s[300], l[64] = "";
    const sk_custom *c = &k->doc.lvl;
    int hover = k->tab == ED_TAB_PIECES ? ed_piece_at(k, k->mouse_x, k->mouse_y) : -1;
    if (k->msg[0] && SDL_GetTicks() < k->msg_until) snprintf(s, sizeof s, "%s", k->msg);
    else if (hover >= 0) {
        const sk_piece *p = &k->pieces.v[hover];
        snprintf(s, sizeof s, "Piece %dx%d, %d tiles, %d in the original", p->w, p->h, p->cells, p->uses);
    } else if (k->cell_ok && k->cell_x < c->w && k->cell_y < c->h) {
        uint16_t w = c->cells[k->cell_y * c->w + k->cell_x];
        edit_flag_label(&k->scene.level, w >> SK_FLAG_SHIFT, l, sizeof l);
        snprintf(s, sizeof s, "%d,%d  tile %03X  %s", k->cell_x, k->cell_y, w & SK_INDEX_MASK, (w & SK_INDEX_MASK) == SK_START_TILE ? "start" : l);
    } else s[0] = 0;
    text(k, 8, (float)(wh - ED_STATUS_H + 9), 0xE0E0E0, s);
    int objects = 0;
    for (int i = 0; i < c->w * c->h; i++) { int f = c->cells[i] >> SK_FLAG_SHIFT; objects += f >= 1 && f <= 31; }
    snprintf(s, sizeof s, "%dx%d  %d objects  zoom %d  F1 help", c->w, c->h, objects, k->zoom);
    text(k, (float)(ww - 8 - (int)strlen(s) * ED_CHAR), (float)(wh - ED_STATUS_H + 9), 0x9090A0, s);
}

static void draw_box(skld *k, int ww, int wh, const char *const *lines, int n) {
    int w = 0;
    for (int i = 0; i < n; i++) if ((int)strlen(lines[i]) > w) w = (int)strlen(lines[i]);
    float bw = (float)(w * ED_CHAR + 32), bh = (float)(n * (ED_CHAR + 6) + 24), x = ((float)ww - bw) / 2, y = ((float)wh - bh) / 2;
    rect(k, x, y, bw, bh, 0x1C1C24, 240, true);
    rect(k, x, y, bw, bh, 0x8080A0, 255, false);
    for (int i = 0; i < n; i++) text(k, x + 16, y + 12 + (float)(i * (ED_CHAR + 6)), i == 0 ? 0xFFFFFF : 0xD0D0D8, lines[i]);
}

void ed_draw(skld *k) {
    int ww, wh, vw, vh;
    SDL_GetCurrentRenderOutputSize(k->ren, &ww, &wh);
    ed_view_size(k, &vw, &vh);
    SDL_SetRenderDrawColor(k->ren, 0, 0, 0, 255);
    SDL_RenderClear(k->ren);
    SDL_Rect clip = { 0, 0, vw, vh };
    SDL_SetRenderClipRect(k->ren, &clip);
    draw_map(k, vw, vh);
    draw_overlays(k, vw, vh);
    SDL_SetRenderClipRect(k->ren, NULL);
    draw_panel(k, vw, vh);
    draw_status(k, ww, wh);
    if (k->help) {
        static const char *const help[] = {
            "skld keys",
            "Left button    paint the selection",
            "Right button   erase: tiles on Tiles and Pieces,",
            "               else objects and specials",
            "Shift + drag   fill a rectangle",
            "Alt + click    pick from the map",
            "Drag on the tiles, or Alt + drag on the map:",
            "               a brush of several tiles",
            "Ctrl + drag    select: Ctrl+C copy, Ctrl+V paste",
            "               at the mouse, Del clear",
            "Pieces tab     a click places the whole piece,",
            "               standing on the mouse",
            "Middle drag, arrows, wheel   scroll",
            "Ctrl + wheel, 1 2 3          zoom",
            "Ctrl+Z, Ctrl+Y undo, redo     Ctrl+S save",
            "Ctrl+arrow     add a column or row on that side",
            "               (with Shift: remove one)",
            "Tab            next panel tab",
            "G grid   O objects   B solid parts",
            "V              the game's view around the mouse",
            "F5             play from the start",
            "Shift+F5       play from the mouse (Esc returns)",
            "Ctrl+Q         quit",
        };
        draw_box(k, ww, wh, help, (int)(sizeof help / sizeof *help));
    }
    if (k->quit_prompt) {
        static const char *const q[] = { "Unsaved changes", "S  save and quit", "Q  quit without saving", "Esc  keep editing" };
        draw_box(k, ww, wh, q, 4);
    }
    SDL_RenderPresent(k->ren);
}
