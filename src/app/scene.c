#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app/scene.h"
#include "gfx/display_model.h"
#include "gfx/present.h"

static int clamp_max(int size_px, int margin) {
    int m = size_px - margin;
    return m > 0 ? m : 0;
}

void scene_clamp_camera(const sk_level *lv, int *x, int *y) {
    int mx = clamp_max(lv->map.width * 16, SK_CAM_MARGIN_W), my = clamp_max(lv->map.height * 16, SK_CAM_MARGIN_H);
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
    if (*x > mx) *x = mx;
    if (*y > my) *y = my;
}

static void draw_grid(sk_fb *fb, const sk_tileset *ts, int cols, int rows, int scroll_x, int scroll_y, int group,
                      int (*index_at)(const void *, int, int), const void *ctx) {
    int first_col = scroll_x >> 4, off_x = scroll_x & 15;          /* floor division: scroll_x can be negative */
    int first_row = scroll_y >> 4, off_y = scroll_y & 15;
    for (int ty = 0; ty <= fb->h / 16 + 1; ty++) {
        int cy = first_row + ty;
        if (cy < 0 || cy >= rows) continue;
        for (int tx = 0; tx <= fb->w / 16 + 1; tx++) {
            int cx = first_col + tx;
            if (cx < 0 || cx >= cols) continue;
            int index = index_at(ctx, cx, cy);
            if (index < 0) continue;
            fb_blit_tile(fb, ts, index, tx * 16 - off_x, ty * 16 - off_y, group);
        }
    }
}

typedef struct { const uint16_t *indices; int w; } index_grid;

static int fg_index(const void *ctx, int x, int y) {
    const index_grid *gr = ctx;
    return gr->indices[y * gr->w + x] & SK_INDEX_MASK;
}

static int bg_index(const void *ctx, int x, int y) {
    const sk_level *lv = ctx;
    return lv->backdrop.grid[y * lv->backdrop.width + x];
}

void scene_backdrop_pos(const sk_level *lv, int cam_x, int cam_y, int *bg_x, int *bg_y) {
    int divisor = lv->backdrop.values[0] ? lv->backdrop.values[0] : 1;            /* the parallax divisor (VERIFIED) */
    int bx = ((cam_x * 4) / divisor + 3) >> 2, by = cam_y / divisor;
    int mx = clamp_max(lv->backdrop.width * 16, SK_BG_MARGIN_W), my = clamp_max(lv->backdrop.height * 16, SK_CAM_MARGIN_H);
    *bg_x = bx > mx ? mx : bx;
    *bg_y = by > my ? my : by;
}

void scene_sky(const sk_level *lv, int bg_y, sk_sky *sky) {
    sky->c17 = lv->sky; sky->c17_len = lv->sky_len; sky->c17_off = bg_y / 2 < 312 ? bg_y / 2 : 312;
    sky->c18 = lv->copp2; sky->c18_len = lv->copp2_len; sky->c18_off = bg_y;
}

void scene_draw_backdrop(const sk_level *lv, sk_fb *fb, int bg_x, int bg_y) {
    if (lv->backdrop.grid) draw_grid(fb, &lv->tiles_b, lv->backdrop.width, lv->backdrop.height, bg_x + SK_BUFFER_WORLD_X, bg_y, SK_GROUP_TILES_B, bg_index, lv);
}

void scene_draw_tiles(const sk_level *lv, sk_fb *fb, const uint16_t *indices, int cam_x, int cam_y) {
    index_grid gr = { indices, lv->map.width };
    draw_grid(fb, &lv->tiles_a, lv->map.width, lv->map.height, cam_x + SK_BUFFER_WORLD_X, cam_y, SK_GROUP_TILES_A, fg_index, &gr);
}

void scene_draw_level(const sk_level *lv, sk_fb *fb, int cam_x, int cam_y, bool backdrop, const sk_frames *frames) {
    fb_clear(fb, 0);
    if (backdrop) {
        int bg_x, bg_y;
        scene_backdrop_pos(lv, cam_x, cam_y, &bg_x, &bg_y);
        scene_draw_backdrop(lv, fb, bg_x, bg_y);
    }
    scene_draw_tiles(lv, fb, lv->map.cells, cam_x, cam_y);
    if (frames && frames->count) {
        int x = 8, y = 32;
        for (int i = 0; i < frames->count; i++) {
            if (x + frames->width > fb->w) { x = 8; y += frames->height + 4; }
            if (y + frames->height > fb->h) break;
            fb_blit_bob(fb, &frames->frame[i], x, y, SK_GROUP_BOB);
            x += frames->width + 4;
        }
    }
}

static void drop_section(sk_scene *s) {
    if (s->has_frames) bobs_free(&s->frames);
    if (s->has_sections) sections_free(&s->sections);
    s->has_frames = s->has_sections = false;
}

bool scene_load_section(sk_scene *s, const char *file_name, int section, int frame_w, int frame_h, sk_err *e) {
    if (!s->has_level) return sk_fail(e, "no level loaded");
    uint8_t *data;
    size_t n;
    if (!disks_read(&s->disks, s->level_disk, file_name, &data, &n, e)) return false;
    sk_sections secs = {0};
    sk_err inner = {0};
    bool ok = sections_parse(data, n, &secs, &inner);
    free(data);
    if (!ok) return sk_fail(e, "%s: %s", file_name, inner.msg);
    if (section < 0 || section >= secs.count) {
        sections_free(&secs);
        return sk_fail(e, "%s: section %d does not exist (%d sections)", file_name, section, secs.count);
    }
    if (!secs.sec[section].packed) {
        sections_free(&secs);
        return sk_fail(e, "%s: section %d is a raw table, not frames", file_name, section);
    }
    int w = frame_w, h = frame_h, count;
    if (w == 0 && !bobs_estimate(&secs.sec[section], &w, &h, &count)) {
        sections_free(&secs);
        return sk_fail(e, "%s: section %d: no frame geometry fits %zu bytes", file_name, section, secs.sec[section].len);
    }
    sk_frames frames = {0};
    if (!bobs_split(&secs.sec[section], w, h, &frames, e)) {
        sections_free(&secs);
        return false;
    }
    drop_section(s);
    s->sections = secs;
    s->frames = frames;
    s->has_sections = s->has_frames = true;
    snprintf(s->section_name, sizeof s->section_name, "%s", file_name);
    s->section = section;
    return true;
}

void scene_set_camera(sk_scene *s, int x, int y) {
    if (s->has_level) scene_clamp_camera(&s->level, &x, &y);
    s->cam_x = x;
    s->cam_y = y;
}

bool scene_load_level(sk_scene *s, int data_index, sk_err *e) {
    if (data_index < 0 || data_index >= SK_LEVEL_RECORDS)
        return sk_fail(e, "level %d: no such level (0..%d)", data_index, SK_LEVEL_RECORDS - 1);
    const sk_level_record *rec = &s->tables.record[data_index];
    int disk = rec->disk + 1;
    uint8_t *data;
    size_t n;
    sk_err inner = {0};
    if (!disks_read_file_no(&s->disks, disk, data_index, false, &data, &n, &inner))
        return sk_fail(e, "level %d: %s", data_index, inner.msg);
    sk_level lv = {0};
    bool ok = level_parse(data, n, &lv, &inner);
    free(data);
    if (!ok) return sk_fail(e, "level %d (disk %d file%03d): %s", data_index, disk, data_index, inner.msg);
    drop_section(s);
    if (s->has_level) level_free(&s->level);
    s->level = lv;
    s->has_level = true;
    s->level_index = data_index;
    s->level_disk = disk;
    palette_from_tilesets(&s->palette, &lv.tiles_a, &lv.tiles_b);
    s->max_cam_x = clamp_max(lv.map.width * 16, SK_CAM_MARGIN_W);
    s->max_cam_y = clamp_max(lv.map.height * 16, SK_CAM_MARGIN_H);
    scene_set_camera(s, s->cam_x, s->cam_y);
    /* the record's first graphics set, when it holds frames */
    if (rec->n_res > 0) {
        const sk_resource *r = tables_resource(&s->tables, rec->disk, rec->res[0]);
        if (r) {
            char name[16];
            snprintf(name, sizeof name, "file%03d", r->file);
            sk_err ignored = {0};
            scene_load_section(s, name, r->section, 0, 0, &ignored);
        }
    }
    return true;
}

bool scene_open(sk_scene *s, const sk_args *a, sk_err *e) {
    memset(s, 0, sizeof *s);
    s->backdrop = !a->no_backdrop;
    s->show_frames = !a->no_frames;
    s->cam_x = a->cam_x;
    s->cam_y = a->cam_y;
    if (!disks_open(&s->disks, a->roms, e)) { scene_close(s); return false; }
    uint8_t *sk;
    size_t n;
    if (!disks_read(&s->disks, 0, "sk.pak", &sk, &n, e)) { scene_close(s); return false; }
    bool ok = hunk_load(sk, n, &s->sk, e);
    free(sk);
    if (!ok || !tables_load(&s->tables, &s->sk, e)) { scene_close(s); return false; }
    if (!fb_init(&s->fb, SK_FB_W, SK_FB_H)) { scene_close(s); return sk_fail(e, "out of memory"); }
    if (!scene_load_level(s, a->level, e)) { scene_close(s); return false; }
    if (a->section_file && !scene_load_section(s, a->section_file, a->section, a->frame_w, a->frame_h, e)) {
        scene_close(s);
        return false;
    }
    return true;
}

void scene_draw(sk_scene *s) {
    scene_draw_level(&s->level, &s->fb, s->cam_x, s->cam_y, s->backdrop,
                     s->has_frames && s->show_frames ? &s->frames : NULL);
}

void scene_present(const sk_scene *s, uint32_t *out) {
    int bg_x, bg_y;
    scene_backdrop_pos(&s->level, s->cam_x, s->cam_y, &bg_x, &bg_y);
    sk_sky sky;
    scene_sky(&s->level, bg_y, &sky);
    present_screen(&s->fb, &s->palette, &sky, NULL, 0, out);
}

void scene_close(sk_scene *s) {
    drop_section(s);
    if (s->has_level) level_free(&s->level);
    fb_free(&s->fb);
    tables_free(&s->tables);
    image_free(&s->sk);
    disks_close(&s->disks);
    memset(s, 0, sizeof *s);
}
