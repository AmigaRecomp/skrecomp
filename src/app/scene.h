/* The level viewer's state: disks, tables, one level, one graphics-set section, the framebuffer. */
#ifndef SK_APP_SCENE_H
#define SK_APP_SCENE_H

#include <stdbool.h>
#include <stdint.h>
#include "app/args.h"
#include "core/disks.h"
#include "core/hunk.h"
#include "core/tables.h"
#include "formats/bobs.h"
#include "formats/level.h"
#include "formats/sections.h"
#include "gfx/framebuffer.h"
#include "gfx/palette.h"
#include "gfx/present.h"

typedef struct {
    sk_disks disks;
    sk_image sk;
    sk_tables tables;
    sk_level level;
    bool has_level;
    int level_index;            /* data index */
    int level_disk;             /* 0..4 */
    sk_sections sections;
    bool has_sections;
    sk_frames frames;
    bool has_frames;
    char section_name[32];
    int section;
    sk_fb fb;
    sk_palette palette;
    int cam_x, cam_y, max_cam_x, max_cam_y;
    bool backdrop;
    bool show_frames;
} sk_scene;

bool scene_open(sk_scene *s, const sk_args *a, sk_err *e);
bool scene_load_level(sk_scene *s, int data_index, sk_err *e);
/* frame_w == 0: estimate the geometry from the data. */
bool scene_load_section(sk_scene *s, const char *file_name, int section, int frame_w, int frame_h, sk_err *e);
void scene_set_camera(sk_scene *s, int x, int y);
void scene_draw(sk_scene *s);
void scene_present(const sk_scene *s, uint32_t *out);          /* SK_SCREEN_W x SK_SCREEN_H */
void scene_close(sk_scene *s);

/* Pure parts, also used by the tests and by the play mode. */
void scene_clamp_camera(const sk_level *lv, int *x, int *y);
/* backdrop_scroll (0x10661A): bg_x = ((cam_x * 4) / parallax + 3) >> 2, bg_y = cam_y / parallax, both clamped. */
void scene_backdrop_pos(const sk_level *lv, int cam_x, int cam_y, int *bg_x, int *bg_y);
void scene_sky(const sk_level *lv, int bg_y, sk_sky *sky);
void scene_draw_backdrop(const sk_level *lv, sk_fb *fb, int bg_x, int bg_y);
/* Foreground tiles from an index grid (the map's words or the live cell table), low 9 bits used. */
void scene_draw_backdrop_stream(const sk_level *lv, const uint8_t *cells, const uint16_t ring[32], sk_fb *fb, int bg_x, int bg_y);   /* scroll modes */
void scene_draw_backdrop_cells(const sk_level *lv, const uint8_t *cells, sk_fb *fb, int bg_x, int bg_y);   /* the world's writable copy */
void scene_draw_tiles(const sk_level *lv, sk_fb *fb, const uint16_t *indices, int cam_x, int cam_y);
/* With the animated cells: a cell draws tile index + anim[state] / 0x80 (blit_fg_strips 0x109092). */
void scene_draw_tiles_anim(const sk_level *lv, sk_fb *fb, const uint16_t *indices, const uint8_t *states, const uint16_t *anim, int cam_x, int cam_y);
void scene_draw_level(const sk_level *lv, sk_fb *fb, int cam_x, int cam_y, bool backdrop, const sk_frames *frames);

#endif
