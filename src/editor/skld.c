/* skld: the Soccer Kid level designer. A new level is made on an original level's setting (its tiles, objects, cell
   behaviours, backdrop and music) and saved as a .skl file, which holds no game data; soccerkid --custom plays it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "app/embedded.h"
#include "app/window.h"
#include "core/zipfile.h"
#include "editor/editor.h"
#include "editor/playtest.h"
#include "editor/skld_args.h"
#include "game/world.h"
#include "gfx/png.h"

static skld g_k;

static void mouse_cell(skld *k);

static void refresh(skld *k) {                               /* after any change: the checks, the title, the cell under the mouse */
    mouse_cell(k);                                          /* an undo or a resize may have taken its cell away */
    k->n_issues = edit_check(&k->doc, &k->scene.level, k->issues, 8);
    const char *name = strrchr(k->path, '/'), *back = strrchr(k->path, '\\');
    if (back && (!name || back > name)) name = back;
    char t[256];
    const sk_setting *st = custom_setting_by_index(k->doc.lvl.setting);
    snprintf(t, sizeof t, "skld - %s - %s%s", name ? name + 1 : k->path, st ? st->title : "?", k->doc.dirty ? " *" : "");
    if (strcmp(t, k->title) != 0) { snprintf(k->title, sizeof k->title, "%s", t); SDL_SetWindowTitle(k->win, t); }
}

static void clamp_camera(skld *k) {
    int vw, vh;
    ed_view_size(k, &vw, &vh);
    int maxx = k->doc.lvl.w * 16 - vw / k->zoom + 64, maxy = k->doc.lvl.h * 16 - vh / k->zoom + 64;
    if (k->cam_x > maxx) k->cam_x = maxx;
    if (k->cam_y > maxy) k->cam_y = maxy;
    if (k->cam_x < -64) k->cam_x = -64;
    if (k->cam_y < -64) k->cam_y = -64;
}

static void mouse_cell(skld *k) {
    int vw, vh;
    ed_view_size(k, &vw, &vh);
    int wx = k->cam_x + k->mouse_x / k->zoom, wy = k->cam_y + k->mouse_y / k->zoom;
    k->cell_x = wx >= 0 ? wx >> 4 : -1; k->cell_y = wy >= 0 ? wy >> 4 : -1;
    k->cell_ok = k->mouse_in && k->mouse_x < vw && k->mouse_y < vh && k->cell_x >= 0 && k->cell_y >= 0 && k->cell_x < k->doc.lvl.w && k->cell_y < k->doc.lvl.h;
}

static void set_zoom(skld *k, int z) {                       /* keeps the cell under the mouse in place */
    if (z < 1) z = 1;
    if (z > 3) z = 3;
    int wx = k->cam_x + k->mouse_x / k->zoom, wy = k->cam_y + k->mouse_y / k->zoom;
    k->zoom = z;
    k->cam_x = wx - k->mouse_x / z; k->cam_y = wy - k->mouse_y / z;
    clamp_camera(k);
}

static int layer_of_tab(const skld *k) { return k->tab == ED_TAB_TILES || k->tab == ED_TAB_PIECES ? SK_LAYER_TILE : SK_LAYER_FLAG; }

static bool start_selected(const skld *k) { return k->tab == ED_TAB_SPECIALS && k->start; }

static bool has_tool(const skld *k) {                        /* something to paint on this tab */
    if (k->tab == ED_TAB_PIECES) return k->piece < k->pieces.n;
    return k->tab == ED_TAB_TILES || start_selected(k) || (k->tab == ED_TAB_OBJECTS ? k->n_markers > 0 : k->n_attrs > 0 && k->flag >= 33);
}

static void apply(skld *k, int x, int y, bool erase) {       /* one cell of a paint or erase stroke */
    if (erase) {
        uint16_t w = k->doc.lvl.cells[y * k->doc.lvl.w + x];
        int layer = layer_of_tab(k);
        if (k->tab == ED_TAB_SPECIALS && (w & SK_INDEX_MASK) == SK_START_TILE) layer = SK_LAYER_TILE;   /* the start is a tile */
        doc_erase(&k->doc, x, y, layer);
        return;
    }
    if (k->tab == ED_TAB_TILES) doc_paint(&k->doc, x, y, k->tile);
    else if (k->tab == ED_TAB_OBJECTS ? k->flag >= 1 && k->flag <= 31 : k->flag >= 33) doc_place_flag(&k->doc, x, y, k->flag);
}

static void end_drag(skld *k) {                              /* whatever ends a drag ends its stroke */
    if (k->drag == ED_DRAG_PAINT || k->drag == ED_DRAG_ERASE) doc_end_stroke(&k->doc);
    k->drag = ED_DRAG_NONE;
}

static void stroke_to(skld *k, int x, int y, bool erase) {   /* every cell on the line from the last one, so fast moves leave no gaps */
    int dx = abs(x - k->last_x), dy = -abs(y - k->last_y), sx = k->last_x < x ? 1 : -1, sy = k->last_y < y ? 1 : -1, err = dx + dy;
    int cx = k->last_x, cy = k->last_y;
    for (;;) {
        apply(k, cx, cy, erase);
        if (cx == x && cy == y) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; cx += sx; }
        if (e2 <= dx) { err += dx; cy += sy; }
    }
    k->last_x = x; k->last_y = y;
}

static void pick(skld *k) {                                  /* Alt + click: the tile, object or special under the mouse */
    uint16_t w = k->doc.lvl.cells[k->cell_y * k->doc.lvl.w + k->cell_x];
    int f = w >> SK_FLAG_SHIFT;
    if (k->tab == ED_TAB_PIECES) k->tab = ED_TAB_TILES;          /* a piece cannot be picked: its tile can */
    if (k->tab == ED_TAB_TILES) { k->tile = w & SK_INDEX_MASK; if (k->tile == SK_START_TILE) { k->tab = ED_TAB_SPECIALS; k->start = true; } return; }
    if ((w & SK_INDEX_MASK) == SK_START_TILE) { k->tab = ED_TAB_SPECIALS; k->start = true; return; }
    if (f >= 1 && f <= 31) { k->tab = ED_TAB_OBJECTS; k->flag = f; k->start = false; }
    else if (f >= 33) { k->tab = ED_TAB_SPECIALS; k->flag = f; k->start = false; }
}

static bool save(skld *k) {
    sk_err e = {0};
    refresh(k);
    if (!custom_save(k->path, &k->doc.lvl, &e)) { ed_message(k, "Not saved: %s", e.msg); return false; }
    k->doc.dirty = false;
    int errors = 0;
    for (int i = 0; i < k->n_issues; i++) errors += k->issues[i].error;
    if (errors) ed_message(k, "Saved %s, with problems: %s", k->path, k->issues[0].text);
    else ed_message(k, "Saved %s", k->path);
    return true;
}

static void playtest(skld *k, bool from_cursor) {
    static sk_play p;
    sk_err e = {0};
    end_drag(k);
    if (from_cursor && !k->cell_ok) { ed_message(k, "Shift+F5: put the mouse on the map where the kid should start"); return; }
    if (!playtest_open(&p, &k->base, &k->doc, from_cursor, k->cell_x, k->cell_y, &e)) { ed_message(k, "Cannot play: %s", e.msg); return; }
    sk_window w;
    if (!window_attach(&w, k->win, k->ren, &e)) { play_close(&p); ed_message(k, "Cannot play: %s", e.msg); return; }
    window_run(&w, &p, 0);
    window_detach(&w);
    if (p.load_failed) ed_message(k, "The play-test stopped: %s", p.load_err.msg);
    else if (p.custom_done) ed_message(k, "Play-test: the exit was reached");
    else if (p.custom_game_over) ed_message(k, "Play-test: game over");
    else ed_message(k, "Play-test ended");
    play_close(&p);
}

static void request_quit(skld *k) { if (k->doc.dirty) k->quit_prompt = true; else k->running = false; }

static void resize(skld *k, SDL_Keycode key, bool remove) {
    sk_err e = {0};
    int n = remove ? -1 : 1;
    bool ok = key == SDLK_LEFT ? doc_resize(&k->doc, n, 0, 0, 0, &e) : key == SDLK_RIGHT ? doc_resize(&k->doc, 0, 0, n, 0, &e)
            : key == SDLK_UP ? doc_resize(&k->doc, 0, n, 0, 0, &e) : doc_resize(&k->doc, 0, 0, 0, n, &e);
    if (!ok) { ed_message(k, "%s", e.msg); return; }
    if (key == SDLK_LEFT) k->cam_x += 16 * n;                   /* the view stays on the same cells */
    if (key == SDLK_UP) k->cam_y += 16 * n;
    clamp_camera(k);
    ed_message(k, "The level is now %dx%d", k->doc.lvl.w, k->doc.lvl.h);
}

static void key_down(skld *k, const SDL_KeyboardEvent *ev) {
    bool ctrl = (ev->mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0, shift = (ev->mod & SDL_KMOD_SHIFT) != 0;
    SDL_Keycode key = ev->key;
    if (k->quit_prompt) {
        if (key == SDLK_S) { if (save(k)) k->running = false; else k->quit_prompt = false; }
        else if (key == SDLK_Q) k->running = false;
        else if (key == SDLK_ESCAPE) k->quit_prompt = false;
        return;
    }
    if (ctrl) {
        switch (key) {
        case SDLK_Z: end_drag(k); if (shift ? doc_redo(&k->doc) : doc_undo(&k->doc)) clamp_camera(k); break;
        case SDLK_Y: end_drag(k); if (doc_redo(&k->doc)) clamp_camera(k); break;
        case SDLK_S: save(k); break;
        case SDLK_Q: request_quit(k); break;
        case SDLK_C:
            if (!k->has_sel) { ed_message(k, "Ctrl + drag on the map to select first"); break; }
            clip_free(&k->clip);
            if (doc_copy(&k->doc, k->sx0, k->sy0, k->sx1, k->sy1, &k->clip)) ed_message(k, "Copied %dx%d cells", k->clip.w, k->clip.h);
            break;
        case SDLK_V:
            if (!k->clip.cells) ed_message(k, "Nothing copied yet");
            else if (k->cell_ok) doc_paste(&k->doc, &k->clip, k->cell_x, k->cell_y);
            break;
        case SDLK_LEFT: case SDLK_RIGHT: case SDLK_UP: case SDLK_DOWN: end_drag(k); resize(k, key, shift); break;
        default: break;
        }
        return;
    }
    int step = shift ? 128 : 32;
    switch (key) {
    case SDLK_ESCAPE: if (k->help) k->help = false; else k->has_sel = false; break;
    case SDLK_F1: k->help = !k->help; break;
    case SDLK_F5: playtest(k, shift); break;
    case SDLK_TAB: k->tab = (k->tab + (shift ? ED_TABS - 1 : 1)) % ED_TABS; break;
    case SDLK_1: case SDLK_2: case SDLK_3: set_zoom(k, (int)(key - SDLK_0)); break;
    case SDLK_G: k->grid = !k->grid; break;
    case SDLK_O: k->overlay = !k->overlay; break;
    case SDLK_B: k->solid = !k->solid; break;
    case SDLK_V: k->frame = !k->frame; break;
    case SDLK_LEFT: k->cam_x -= step; clamp_camera(k); break;
    case SDLK_RIGHT: k->cam_x += step; clamp_camera(k); break;
    case SDLK_UP: k->cam_y -= step; clamp_camera(k); break;
    case SDLK_DOWN: k->cam_y += step; clamp_camera(k); break;
    case SDLK_DELETE: case SDLK_BACKSPACE:
        if (k->has_sel) doc_fill(&k->doc, k->sx0, k->sy0, k->sx1, k->sy1, SK_LAYER_ALL, 0, 0);
        break;
    default: break;
    }
}

static void panel_click(skld *k, int x, int y) {
    int vw, vh;
    ed_view_size(k, &vw, &vh);
    x -= vw;
    if (y < ED_TABS_H) {
        int col = (x - 8) / (ED_TAB_W + 8), row = (y - 6) / (ED_TAB_H + 6), t = row * 2 + col;
        if (x >= 8 && y >= 6 && col < 2 && t < ED_TABS) k->tab = t;
        return;
    }
    if (y >= vh - ED_ISSUES_H) {                              /* a check: show where it is */
        int i = (y - (vh - ED_ISSUES_H) - 8) / ED_ISSUE_H;
        if (i >= 0 && i < k->n_issues && k->issues[i].x > 0) { k->cam_x = k->issues[i].x * 16 - 64; clamp_camera(k); }
        return;
    }
    int top = ED_TABS_H + 4;
    if (k->tab == ED_TAB_PIECES) { int i = ed_piece_at(k, x + vw, y); if (i >= 0) k->piece = i; return; }
    if (k->tab == ED_TAB_TILES) {
        int col = (x - 8) / ED_TILE_CELL, row = (y - top) / ED_TILE_CELL + k->scroll[0];
        int t = row * ED_TILE_COLS + col;
        if (col >= 0 && col < ED_TILE_COLS && y >= top && t < k->scene.level.tiles_a.count) k->tile = t;
        return;
    }
    int row = (y - top + 3) / ED_ROW_H + k->scroll[k->tab];
    if (y < top - 3 || row < 0) return;
    if (k->tab == ED_TAB_OBJECTS) { if (row < k->n_markers) { k->flag = k->markers[row]; k->start = false; } }
    else if (row == 0) k->start = true;
    else if (row - 1 < k->n_attrs) { k->flag = k->attrs[row - 1]; k->start = false; }
}

static void event(skld *k, const SDL_Event *ev) {
    int vw, vh;
    ed_view_size(k, &vw, &vh);
    switch (ev->type) {
    case SDL_EVENT_QUIT: case SDL_EVENT_WINDOW_CLOSE_REQUESTED: request_quit(k); break;
    case SDL_EVENT_KEY_DOWN: key_down(k, &ev->key); break;
    case SDL_EVENT_MOUSE_MOTION:
        k->mouse_x = (int)ev->motion.x; k->mouse_y = (int)ev->motion.y; k->mouse_in = true;
        mouse_cell(k);
        if (k->drag == ED_DRAG_SCROLL) {
            k->cam_x = k->drag_cx - (k->mouse_x - k->drag_mx) / k->zoom; k->cam_y = k->drag_cy - (k->mouse_y - k->drag_my) / k->zoom;
            clamp_camera(k);
        } else if ((k->drag == ED_DRAG_PAINT || k->drag == ED_DRAG_ERASE) && k->cell_ok && (k->cell_x != k->last_x || k->cell_y != k->last_y))
            stroke_to(k, k->cell_x, k->cell_y, k->drag == ED_DRAG_ERASE);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (k->help || k->quit_prompt) { k->help = false; break; }
        int mx = (int)ev->button.x, my = (int)ev->button.y;
        k->mouse_x = mx; k->mouse_y = my; k->mouse_in = true;
        mouse_cell(k);
        if (mx >= vw) { if (ev->button.button == SDL_BUTTON_LEFT && my < vh) panel_click(k, mx, my); break; }
        if (k->drag) break;                                   /* one drag at a time */
        if (ev->button.button == SDL_BUTTON_MIDDLE) {
            k->drag = ED_DRAG_SCROLL; k->drag_mx = mx; k->drag_my = my; k->drag_cx = k->cam_x; k->drag_cy = k->cam_y;
            break;
        }
        if (!k->cell_ok) break;
        SDL_Keymod mod = SDL_GetModState();
        k->ax = k->cell_x; k->ay = k->cell_y;
        if (ev->button.button == SDL_BUTTON_LEFT && (mod & SDL_KMOD_ALT)) { pick(k); break; }
        if (ev->button.button == SDL_BUTTON_LEFT && (mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI))) { k->drag = ED_DRAG_SELECT; k->has_sel = false; break; }
        if (ev->button.button != SDL_BUTTON_LEFT && ev->button.button != SDL_BUTTON_RIGHT) break;
        bool erase = ev->button.button == SDL_BUTTON_RIGHT;
        if (!erase && !has_tool(k)) { ed_message(k, "Choose something in the panel first"); break; }
        if (!erase && k->tab == ED_TAB_PIECES) {                /* the whole piece, standing on the mouse: one step */
            const sk_piece *p = &k->pieces.v[k->piece];
            int ox, oy; ed_piece_origin(k, &ox, &oy);
            doc_stamp(&k->doc, ox, oy, p->w, p->h, p->tiles);
            break;
        }
        if (!erase && start_selected(k)) { doc_move_start(&k->doc, k->cell_x, k->cell_y); break; }   /* one start: a click moves it */
        if (mod & SDL_KMOD_SHIFT) { k->drag = ED_DRAG_FILL; k->last_x = ev->button.button; break; }   /* the button decides paint or erase */
        k->drag = erase ? ED_DRAG_ERASE : ED_DRAG_PAINT;
        doc_begin_stroke(&k->doc);
        k->last_x = k->cell_x; k->last_y = k->cell_y;
        stroke_to(k, k->cell_x, k->cell_y, k->drag == ED_DRAG_ERASE);
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (k->drag == ED_DRAG_FILL && k->cell_ok) {
            bool erase = k->last_x == SDL_BUTTON_RIGHT;
            int layer = layer_of_tab(k);
            if (erase) doc_fill(&k->doc, k->ax, k->ay, k->cell_x, k->cell_y, layer, 0, 0);
            else if (k->tab == ED_TAB_TILES) doc_fill(&k->doc, k->ax, k->ay, k->cell_x, k->cell_y, SK_LAYER_TILE, k->tile, 0);
            else if (!start_selected(k)) doc_fill(&k->doc, k->ax, k->ay, k->cell_x, k->cell_y, SK_LAYER_FLAG, 0, k->flag);
        } else if (k->drag == ED_DRAG_SELECT && k->cell_ok) {
            k->has_sel = true; k->sx0 = k->ax; k->sy0 = k->ay; k->sx1 = k->cell_x; k->sy1 = k->cell_y;
            if (k->sx0 > k->sx1) { int t = k->sx0; k->sx0 = k->sx1; k->sx1 = t; }
            if (k->sy0 > k->sy1) { int t = k->sy0; k->sy0 = k->sy1; k->sy1 = t; }
        }
        end_drag(k);
        break;
    case SDL_EVENT_MOUSE_WHEEL: {
        float dy = ev->wheel.y, dx = ev->wheel.x;
        SDL_Keymod mod = SDL_GetModState();
        if (k->mouse_x >= vw) {                               /* the panel's list */
            bool grid = k->tab == ED_TAB_TILES || k->tab == ED_TAB_PIECES;
            int n = k->tab == ED_TAB_TILES ? (k->scene.level.tiles_a.count + ED_TILE_COLS - 1) / ED_TILE_COLS
                  : k->tab == ED_TAB_PIECES ? (k->pieces.n + ED_PIECE_COLS - 1) / ED_PIECE_COLS : k->tab == ED_TAB_OBJECTS ? k->n_markers : k->n_attrs + 1;
            k->wheel_list += dy * (grid ? 1.0f : 3.0f);      /* a trackpad sends fractions of a step */
            int rows = (int)k->wheel_list;
            k->wheel_list -= (float)rows;
            int visible = (vh - ED_ISSUES_H - ED_TABS_H - 4) / (k->tab == ED_TAB_TILES ? ED_TILE_CELL : k->tab == ED_TAB_PIECES ? ED_PIECE_CELL : ED_ROW_H);
            k->scroll[k->tab] -= rows;
            if (k->scroll[k->tab] > n - visible) k->scroll[k->tab] = n - visible;
            if (k->scroll[k->tab] < 0) k->scroll[k->tab] = 0;
        } else if (mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) {
            k->wheel_zoom += dy;
            if (k->wheel_zoom >= 1 || k->wheel_zoom <= -1) { set_zoom(k, k->zoom + (k->wheel_zoom > 0 ? 1 : -1)); k->wheel_zoom = 0; }
        } else {
            if (mod & SDL_KMOD_SHIFT) { dx = dy; dy = 0; }
            k->cam_x -= (int)(dx * 32); k->cam_y -= (int)(dy * 32);
            clamp_camera(k);
        }
        break;
    }
    case SDL_EVENT_WINDOW_RESIZED: clamp_camera(k); break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE: k->mouse_in = false; mouse_cell(k); break;
    case SDL_EVENT_WINDOW_FOCUS_LOST: end_drag(k); break;
    default: break;
    }
}

static int load_data(const skld_args *a, sk_args *base, uint8_t **file) {
    memset(base, 0, sizeof *base);
    base->section = -1; base->scale = 3; base->near_type = -1; base->cup = -1; base->level = -1;
    base->roms = a->roms;
    sk_err e = {0}; size_t n = 0;
    if (!a->roms && a->data_path) {
        if (!file_read_all(a->data_path, file, &n, &e)) { fprintf(stderr, "skld: %s\n", e.msg); return 2; }
        base->data = *file; base->data_len = n;
    } else if (!a->roms && sk_embedded_size) { base->data = sk_embedded_data; base->data_len = sk_embedded_size; }
    else if (!a->roms) { fprintf(stderr, "skld: no game data: run skextract <roms dir> then rebuild, or pass --roms or --data\n"); return 2; }
    return 0;
}

static void find_flags(skld *k) {                            /* the setting's marker and attribute flags for the panel */
    bool def[128];
    custom_defined_flags(&k->scene.level, def);
    for (int f = 1; f <= 31; f++) if (def[f]) k->markers[k->n_markers++] = f;
    for (int f = 33; f < 128; f++) if (def[f]) k->attrs[k->n_attrs++] = f;
    if (k->n_markers) k->flag = k->markers[0];
}

int main(int argc, char **argv) {
    skld_args a; sk_err e = {0};
    if (!skld_args_parse(argc, argv, &a, &e)) { fprintf(stderr, "skld: %s\n%s", e.msg, skld_usage()); return 2; }
    if (a.mode == SKLD_HELP) { fputs(skld_usage(), stdout); return 0; }
    if (a.mode == SKLD_SETTINGS) {
        const char *u = skld_usage(), *from = strstr(u, "SETTING is one of"), *to = strstr(u, "\nexamples");
        printf("%.*s", (int)(to - from), from);
        return 0;
    }
    skld *k = &g_k;
    uint8_t *file = NULL;
    int rc = load_data(&a, &k->base, &file);
    if (rc) return rc;
    snprintf(k->path, sizeof k->path, "%s", a.file);
    sk_custom loaded = {0};
    if (a.mode == SKLD_NEW) {
        FILE *f = fopen(a.file, "rb");
        if (f) { fclose(f); fprintf(stderr, "skld: %s exists: open it with skld %s\n", a.file, a.file); free(file); return 2; }
        k->base.level = a.setting->index;
    } else {
        if (!custom_load(a.file, &loaded, &e)) { fprintf(stderr, "skld: %s\n", e.msg); free(file); return 2; }
        k->base.level = loaded.setting;
    }
    if (!scene_open(&k->scene, &k->base, &e)) { fprintf(stderr, "skld: %s\n", e.msg); custom_free(&loaded); free(file); return 1; }
    char *pref = SDL_GetPrefPath("soccerkid", "soccerkid");           /* the game's own key bindings for the play-test */
    static char controls[512];
    if (pref) { snprintf(controls, sizeof controls, "%scontrols.cfg", pref); k->base.controls_path = controls; SDL_free(pref); }
    if (a.mode == SKLD_NEW) {
        bool ok = a.copy ? doc_new_copy(&k->doc, &k->scene.level, a.setting->index, &e)
                         : doc_new_empty(&k->doc, &k->scene.level, a.setting->index, a.w, a.h, &e);
        if (!ok) { fprintf(stderr, "skld: %s\n", e.msg); scene_close(&k->scene); free(file); return 1; }
        k->doc.dirty = true;
    } else {
        int cleared = custom_clear_unknown_flags(&loaded, &k->scene.level);
        doc_open(&k->doc, &k->scene.level, &loaded);
        if (cleared) { ed_message(k, "%d cells used flags this setting does not have; they were cleared", cleared); k->doc.dirty = true; }
    }
    find_flags(k);
    if (!edit_find_pieces(&k->scene.level.map, 3, &k->pieces)) ed_message(k, "Out of memory finding the pieces");
    k->zoom = 2; k->overlay = true; k->running = true; k->tile = 1;
    for (int i = 0; i < k->doc.lvl.w * k->doc.lvl.h; i++)       /* the view starts at the start position */
        if ((k->doc.lvl.cells[i] & SK_INDEX_MASK) == SK_START_TILE) { k->cam_x = (i % k->doc.lvl.w) * 16 - 64; k->cam_y = (i / k->doc.lvl.w) * 16 - 160; break; }
    SDL_SetHint(SDL_HINT_FORCE_RAISEWINDOW, "1");                    /* started from a terminal: take the focus */
    SDL_SetHint(SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED, "1");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) { fprintf(stderr, "skld: SDL: %s\n", SDL_GetError()); return 1; }
    k->win = SDL_CreateWindow("skld", 1280, 800, SDL_WINDOW_RESIZABLE);
    if (k->win) SDL_RaiseWindow(k->win);
    k->ren = k->win ? SDL_CreateRenderer(k->win, NULL) : NULL;
    if (!k->ren) { fprintf(stderr, "skld: SDL: %s\n", SDL_GetError()); return 1; }
    SDL_SetWindowMinimumSize(k->win, 1000, 640);
    SDL_SetRenderVSync(k->ren, 1);
    SDL_SetRenderDrawBlendMode(k->ren, SDL_BLENDMODE_BLEND);
    ed_build_tiles(k);
    clamp_camera(k);
    if (!k->msg[0]) ed_message(k, "F1: the keys.  F5: play the level.");
    int redraws = 0;
    while (k->running && !(a.exit_after && redraws++ >= a.exit_after)) {
        SDL_Event ev;
        if (SDL_WaitEventTimeout(&ev, 100)) {
            do event(k, &ev); while (k->running && SDL_PollEvent(&ev));
        }
        refresh(k);
        ed_draw(k);
        if (a.screenshot) {                                     /* tests: the frame just drawn */
            SDL_Surface *s = SDL_RenderReadPixels(k->ren, NULL), *c = s ? SDL_ConvertSurface(s, SDL_PIXELFORMAT_XRGB8888) : NULL;
            if (c && c->pitch == c->w * 4 && !png_write(a.screenshot, c->w, c->h, c->pixels, &e)) fprintf(stderr, "skld: %s\n", e.msg);
            SDL_DestroySurface(s); SDL_DestroySurface(c);
            break;
        }
    }
    clip_free(&k->clip);
    edit_pieces_free(&k->pieces);
    doc_free(&k->doc);
    scene_close(&k->scene);
    if (k->map_tex) SDL_DestroyTexture(k->map_tex);
    if (k->tiles_tex) SDL_DestroyTexture(k->tiles_tex);
    free(k->map_px);
    SDL_DestroyRenderer(k->ren);
    SDL_DestroyWindow(k->win);
    SDL_Quit();
    free(file);
    return 0;
}
