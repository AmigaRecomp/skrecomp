#include "editor/view.h"
#include "app/scene.h"
#include "gfx/display_model.h"
#include "gfx/framebuffer.h"
#include "gfx/present.h"

bool edit_render(const sk_level *setting, const sk_palette *pal, const sk_custom *map, int x, int y, int w, int h, uint32_t *out) {
    sk_level lv = *setting;                                   /* the setting with the custom map in place */
    lv.map.width = (uint16_t)map->w; lv.map.height = (uint16_t)map->h;
    lv.map.cells = map->cells; lv.map.states = map->states;
    sk_fb fb;
    if (!fb_init(&fb, w + SK_VIEW_X, h)) return false;
    int cam_x = x - (SK_VIEW_X + SK_BUFFER_WORLD_X);           /* buffer column SK_VIEW_X shows world cam_x + 16 */
    int bg_x, bg_y;
    fb_clear(&fb, 0);
    scene_backdrop_pos(&lv, cam_x, y, &bg_x, &bg_y);
    scene_draw_backdrop(&lv, &fb, bg_x, bg_y);
    scene_draw_tiles(&lv, &fb, lv.map.cells, cam_x, y);
    sk_sky sky;
    scene_sky(&lv, bg_y, &sky);
    present_rows(&fb, pal, &sky, SK_VIEW_X, 0, w, h, out);
    fb_free(&fb);
    return true;
}
