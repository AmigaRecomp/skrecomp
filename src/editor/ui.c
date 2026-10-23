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

static void text(skld *k, float x, float y, uint32_t rgb, const char *s) {
    colour(k, rgb, 255);
    SDL_RenderDebugText(k->ren, x, y, s);
}

static void rect(skld *k, float x, float y, float w, float h, uint32_t rgb, int a, bool fill) {
    SDL_FRect r = { x, y, w, h };
    colour(k, rgb, a);
    if (fill) SDL_RenderFillRect(k->ren, &r); else SDL_RenderRect(k->ren, &r);
}

static void tile_src(int t, SDL_FRect *r) { r->x = (float)(t % 16 * 16); r->y = (float)(t / 16 * 16); r->w = r->h = 16; }

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
                text(k, r.x + 2, r.y + 2, 0xFFFFFF, "S");
                continue;
            }
            if (!k->overlay || !f) continue;
            if (f <= 31) {
                rect(k, r.x, r.y, r.w, r.h, 0xFFD020, 230, false);
                snprintf(s, sizeof s, "%d", f < (int)lv->pals_len - SK_PALS_TYPE ? lv->pals[SK_PALS_TYPE + f] : 0);
                text(k, r.x + 1, r.y + 1, 0xFFD020, s);
            } else if (f >= 33) {
                rect(k, r.x, r.y, r.w, r.h, 0x40D0F0, 120, false);
                int at = SK_PALS_ATTR + (size_t)(f - 33) < lv->pals_len ? lv->pals[SK_PALS_ATTR + f - 33] : -1;
                if (k->zoom > 1 || at == 13) { snprintf(s, sizeof s, at == 13 ? "EX" : "%d", at); text(k, r.x + 1, r.y + r.h - 9, 0x40D0F0, s); }
            }
        }
    if (k->has_sel || k->drag == ED_DRAG_SELECT || k->drag == ED_DRAG_FILL) {      /* the selection or the fill being dragged */
        int a = k->drag ? k->ax : k->sx0, b = k->drag ? k->ay : k->sy0, cx = k->drag ? k->cell_x : k->sx1, cy = k->drag ? k->cell_y : k->sy1;
        if (a > cx) { int t = a; a = cx; cx = t; }
        if (b > cy) { int t = b; b = cy; cy = t; }
        SDL_FRect r0, r1; cell_rect(k, a, b, &r0); cell_rect(k, cx, cy, &r1);
        uint32_t col = k->drag == ED_DRAG_FILL ? 0xFF8040 : 0x80C0FF;
        rect(k, r0.x, r0.y, r1.x + r1.w - r0.x, r1.y + r1.h - r0.y, col, 50, true);
        rect(k, r0.x, r0.y, r1.x + r1.w - r0.x, r1.y + r1.h - r0.y, col, 255, false);
    }
    if (k->cell_ok) {
        SDL_FRect r; cell_rect(k, k->cell_x, k->cell_y, &r);
        if (k->tab == ED_TAB_TILES && k->tiles_tex && !k->drag) {        /* the selected tile under the mouse */
            SDL_FRect src; tile_src(k->tile, &src);
            SDL_SetTextureAlphaMod(k->tiles_tex, 160);
            SDL_RenderTexture(k->ren, k->tiles_tex, &src, &r);
            SDL_SetTextureAlphaMod(k->tiles_tex, 255);
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
    static const char *const names[ED_TABS] = { "Tiles", "Objects", "Specials" };
    for (int t = 0; t < ED_TABS; t++) {
        float x = (float)(px + 8 + t * 102);
        rect(k, x, 6, 98, 20, t == k->tab ? 0x4A5A8A : 0x34343C, 255, true);
        text(k, x + 49 - (float)strlen(names[t]) * 4, 12, 0xFFFFFF, names[t]);
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
            if (i == k->tile) { rect(k, x - 1, y - 1, 34, 34, 0xFFFFFF, 255, false); rect(k, x - 2, y - 2, 36, 36, 0xFFFFFF, 255, false); }
        }
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
            if (sel) rect(k, (float)px + 4, y - 3, ED_PANEL_W - 8, ED_ROW_H, 0x4A5A8A, 255, true);
            s[38] = 0;
            text(k, (float)px + 8, y, k->tab == ED_TAB_OBJECTS ? 0xFFE070 : 0x90E0FF, s);
        }
        if (n == (k->tab == ED_TAB_OBJECTS ? 0 : 1)) text(k, (float)px + 8, (float)top, 0xA0A0A0, "(none in this setting)");
    }
    rect(k, (float)px, (float)bottom, ED_PANEL_W, 1, 0x50505A, 255, true);
    if (!k->n_issues) text(k, (float)px + 8, (float)bottom + 8, 0x80E080, "No problems found");
    for (int i = 0; i < k->n_issues && i < 6; i++) {
        snprintf(s, sizeof s, "%s", k->issues[i].text);
        s[38] = 0;
        text(k, (float)px + 8, (float)(bottom + 8 + i * 12), k->issues[i].error ? 0xFF6060 : 0xFFB040, s);
    }
}

static void draw_status(skld *k, int ww, int wh) {
    rect(k, 0, (float)(wh - ED_STATUS_H), (float)ww, ED_STATUS_H, 0x18181C, 255, true);
    char s[300], l[64] = "";
    const sk_custom *c = &k->doc.lvl;
    if (k->msg[0] && SDL_GetTicks() < k->msg_until) snprintf(s, sizeof s, "%s", k->msg);
    else if (k->cell_ok) {
        uint16_t w = c->cells[k->cell_y * c->w + k->cell_x];
        edit_flag_label(&k->scene.level, w >> SK_FLAG_SHIFT, l, sizeof l);
        snprintf(s, sizeof s, "%d,%d  tile %03X  %s", k->cell_x, k->cell_y, w & SK_INDEX_MASK, (w & SK_INDEX_MASK) == SK_START_TILE ? "start" : l);
    } else s[0] = 0;
    text(k, 8, (float)(wh - ED_STATUS_H + 7), 0xE0E0E0, s);
    int objects = 0;
    for (int i = 0; i < c->w * c->h; i++) { int f = c->cells[i] >> SK_FLAG_SHIFT; objects += f >= 1 && f <= 31; }
    snprintf(s, sizeof s, "%dx%d  %d objects  zoom %d  F1 help", c->w, c->h, objects, k->zoom);
    text(k, (float)(ww - 8 - (int)strlen(s) * 8), (float)(wh - ED_STATUS_H + 7), 0x9090A0, s);
}

static void draw_box(skld *k, int ww, int wh, const char *const *lines, int n) {
    int w = 0;
    for (int i = 0; i < n; i++) if ((int)strlen(lines[i]) > w) w = (int)strlen(lines[i]);
    float bw = (float)(w * 8 + 32), bh = (float)(n * 14 + 24), x = ((float)ww - bw) / 2, y = ((float)wh - bh) / 2;
    rect(k, x, y, bw, bh, 0x1C1C24, 240, true);
    rect(k, x, y, bw, bh, 0x8080A0, 255, false);
    for (int i = 0; i < n; i++) text(k, x + 16, y + 12 + (float)(i * 14), i == 0 ? 0xFFFFFF : 0xD0D0D8, lines[i]);
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
            "Left button        paint the selection (a tile, an object, a special)",
            "Right button       erase (tiles on the Tiles tab, else objects and specials)",
            "Shift + drag       fill a rectangle      Alt + click   pick from the map",
            "Ctrl + drag        select    Ctrl+C copy    Ctrl+V paste at the mouse    Del clear",
            "Middle drag, arrows, wheel   scroll       Ctrl + wheel, 1 2 3   zoom",
            "Ctrl+Z / Ctrl+Y    undo / redo           Ctrl+S   save",
            "Ctrl+arrow         add a column or row on that side (with Shift: remove one)",
            "Tab                next panel tab        G grid   O objects and specials   B solid parts",
            "V                  the game's view around the mouse",
            "F5                 play from the start   Shift+F5   play from the mouse (Esc returns)",
            "Ctrl+Q             quit",
        };
        draw_box(k, ww, wh, help, (int)(sizeof help / sizeof *help));
    }
    if (k->quit_prompt) {
        static const char *const q[] = { "Unsaved changes", "S  save and quit", "Q  quit without saving", "Esc  keep editing" };
        draw_box(k, ww, wh, q, 4);
    }
    SDL_RenderPresent(k->ren);
}
