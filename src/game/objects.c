#include <stdlib.h>
#include <string.h>
#include "gfx/sprites.h"
#include "game/game.h"
#include "game/objects.h"
#include "gfx/display_model.h"
#include "formats/bobs.h"
#include "game/obj_england.h"
#include "game/obj_country.h"
#include "game/obj_italy.h"
#include "game/obj_russia.h"
#include "game/obj_japan.h"
#include "game/obj_usa.h"
#include "game/obj_bonus.h"
#include "game/obj_london.h"

sk_obj_init_fn object_init_table[128];
sk_obj_update_fn object_update_table[128];

void obj_stub_init(sk_game *g, sk_slot *s, int x, int y) { init_object_slot(g, s, x, y, s->type4); }   /* marks its marker: spawns once */
void obj_stub_update(sk_game *g, sk_slot *s, int index) { (void)index; despawn_if_far(g, s); }

void objects_reset(sk_game *g) {                                /* clear_object_slots 0x10D594 + the lists */
    sk_objects *o = &g->objs;
    memset(o, 0, sizeof *o);
    o->spawn_cell = -1;
    for (int i = 0; i < 16; i++) { o->slot[i].x = -1; o->slot[i].y = -1; o->ptrs[i] = &o->dyn[i]; }
    for (int i = 0; i < 8; i++) { o->fixed[i].x = -1; o->fixed[i].y = -1; }
    g->object_count = 0;
    memset(g->occl, 0, sizeof g->occl); g->occl_count = 0;   /* sub_107536 at level start */
    obj_england_install();
    obj_london_install();
    obj_country_install();
    obj_italy_install();
    obj_russia_install();
    obj_japan_install();
    obj_usa_install();
    obj_bonus_install();
    for (int i = 0; i < 128; i++) {
        if (!object_init_table[i]) object_init_table[i] = obj_stub_init;
        if (!object_update_table[i]) object_update_table[i] = obj_stub_update;
    }
}

static void unmark(sk_game *g, int cell) {
    if (cell >= 0 && cell < g->world.w * g->world.h) g->world.tile[cell] &= (uint16_t)~SK_SPAWNED_BIT;
}

static void mark(sk_game *g, sk_slot *s) {                       /* move.l a4,$18(a6); bset.b #6,(a4) */
    s->marker_cell = g->objs.spawn_cell;
    if (s->marker_cell >= 0 && s->marker_cell < g->world.w * g->world.h) g->world.tile[s->marker_cell] |= SK_SPAWNED_BIT;
}

void objects_flush_deferred(sk_game *g) {
    for (int i = 0; i < g->objs.n_deferred; i++) unmark(g, g->objs.deferred[i]);
    g->objs.n_deferred = 0;
}

sk_slot *find_slot_by_type(sk_game *g, int type) {             /* sub_10DF8A: type4 without the skip bit */
    for (int i = 0; i < 16; i++) if (g->objs.slot[i].x >= 0 && (g->objs.slot[i].type4 & 0x7FFF) == type * 4) return &g->objs.slot[i];
    return NULL;
}

sk_slot *find_free_slot(sk_game *g) {                           /* 0x10D852 */
    if (g->object_count >= 16) return NULL;
    for (int i = 0; i < 16; i++) if (g->objs.slot[i].x < 0) return &g->objs.slot[i];
    return NULL;
}

sk_draw *alloc_draw_record(sk_game *g, sk_slot *s) {     /* 0x10D7F6 */
    sk_objects *o = &g->objs;
    if (o->draw_count >= 16) { s->draw_index = -1; return NULL; }   /* the port's bound: the original runs off the list */
    sk_draw *d = o->ptrs[o->draw_count];
    s->draw_index = (int16_t)(d - o->dyn);
    o->draw_count++;
    return d;
}

static sk_draw *alloc_fixed_record(sk_game *g, sk_slot *s) {    /* 0x10D8F4 */
    for (int i = 0; i < 8; i++)
        if (g->objs.fixed[i].x == -1 && g->objs.fixed[i].y == -1) { s->draw_index = (int16_t)(SK_FIXED_INDEX_BASE + i); return &g->objs.fixed[i]; }
    s->draw_index = -1;
    return NULL;
}

void free_draw_record(sk_game *g, sk_draw *d) {          /* 0x10D820: remove from the list, append at the end */
    sk_objects *o = &g->objs;
    int i = 0;
    while (i < o->draw_count && o->ptrs[i] != d) i++;
    if (i >= o->draw_count) return;
    for (int k = i; k + 1 < o->draw_count; k++) o->ptrs[k] = o->ptrs[k + 1];
    o->ptrs[o->draw_count - 1] = d;
    o->draw_count--;
}

sk_draw *slot_draw_record(sk_game *g, const sk_slot *s) {
    if (s->draw_index < 0) return NULL;
    if (s->draw_index >= SK_FIXED_INDEX_BASE) return &g->objs.fixed[s->draw_index - SK_FIXED_INDEX_BASE];
    return s->draw_index < 16 ? &g->objs.dyn[s->draw_index] : NULL;
}

static void init_common(sk_game *g, sk_slot *s, int x, int y, int type4) {
    s->type4 = (int16_t)type4;
    s->flags = 0; s->flags_b = 0; s->aux = 0; s->t0 = 0; s->t1 = 0;
    s->anim = 0; s->frame = 0; s->anim_tick = 0; s->anim_index = 0; s->timer = 0; s->script = NULL; s->marker_cell = -1; s->platform = -1;
    s->x = s->x0 = (int16_t)x;
    s->y = s->y0 = (int16_t)y;
    g->object_count++;
}

void init_object_slot(sk_game *g, sk_slot *s, int x, int y, int type4) {            /* 0x10D5D6 */
    s->draw = alloc_draw_record(g, s); if (s->draw) s->draw->used = true; init_common(g, s, x, y, type4); mark(g, s);
}
void init_object_slot_nodraw(sk_game *g, sk_slot *s, int x, int y, int type4) {     /* 0x10D620 */
    s->draw = alloc_draw_record(g, s); if (s->draw) s->draw->used = true; init_common(g, s, x, y, type4);   /* the original draws the whole dynamic list */
}
void init_object_slot_fixed(sk_game *g, sk_slot *s, int x, int y, int type4) {      /* 0x10D666 */
    s->draw = alloc_fixed_record(g, s); if (s->draw) s->draw->used = true; init_common(g, s, x, y, type4); mark(g, s);
}
void init_object_slot_fixed_nodraw(sk_game *g, sk_slot *s, int x, int y, int type4) {   /* 0x10D6B0 */
    alloc_fixed_record(g, s); s->draw = NULL; init_common(g, s, x, y, type4);
}
void init_object_slot_shared(sk_game *g, sk_slot *s, int x, int y, int type4, sk_draw *shared) {   /* 0x10D6F6 */
    s->draw = shared; shared->used = true; s->draw_index = -1; init_common(g, s, x, y, type4); mark(g, s);
}
void init_object_slot_invisible(sk_game *g, sk_slot *s, int x, int y, int type4) {  /* 0x10D73A */
    s->draw = NULL; s->draw_index = -1; init_common(g, s, x, y, type4);
}

void spawn_marker(sk_game *g, int cx, int cy) {                 /* 0x10CECE */
    sk_slot *s = find_free_slot(g);
    if (!s) return;
    int param = 0;
    int type = world_marker_type(&g->world, cx, cy, &param);
    if (type < 0 || type >= 128) return;                        /* beyond the 128-entry table: ignored (ruling) */
    g->objs.marker_param = param;
    s->type4 = (int16_t)(type * 4);
    g->objs.spawn_cell = cy * g->world.w + cx;
    object_init_table[type](g, s, cx * 16, cy * 16);
    g->objs.spawn_cell = -1;
}

sk_slot *spawn_object(sk_game *g, int x, int y, int type) {     /* 0x10CEFC */
    sk_slot *s = find_free_slot(g);
    if (!s || type < 0 || type >= 128) return NULL;
    s->type4 = (int16_t)(type * 4);
    object_init_table[type](g, s, x, y);
    g->objs.last_slot = s;
    g->objs.last_draw = slot_draw_record(g, s);
    return s;
}

void scan_cell_for_marker(sk_game *g, int cx, int cy) {         /* 0x10CE40 */
    if (g->game_flags) return;
    if (!world_inside(&g->world, cx, cy)) return;
    uint16_t t = g->world.tile[cy * g->world.w + cx];
    if (!(t & SK_MARKER_BIT) || (t & SK_SPAWNED_BIT)) return;   /* andi.b #$7f on the top byte: still alive */
    spawn_marker(g, cx, cy);
}

void free_object_keep(sk_game *g, sk_slot *s) {                 /* 0x10D7C6 */
    s->x = -1;
    g->object_count--;
    sk_draw *d = slot_draw_record(g, s);
    if (d && s->draw_index < SK_FIXED_INDEX_BASE) free_draw_record(g, d);
}

void free_object(sk_game *g, sk_slot *s) {                      /* 0x10D77A */
    unmark(g, s->marker_cell);
    if (s->draw) s->draw->used = false;
    s->x = -1;
    g->object_count--;
    sk_draw *d = slot_draw_record(g, s);
    if (d && s->draw_index < SK_FIXED_INDEX_BASE) free_draw_record(g, d);
}

void free_object_deferred(sk_game *g, sk_slot *s) {             /* 0x10D79C */
    s->x = -1;
    if (s->marker_cell >= 0 && g->objs.n_deferred < 64) g->objs.deferred[g->objs.n_deferred++] = s->marker_cell;
    g->object_count--;
    sk_draw *d = slot_draw_record(g, s);
    if (d && s->draw_index < SK_FIXED_INDEX_BASE) free_draw_record(g, d);
}

void free_fixed_object(sk_game *g, sk_slot *s) {                /* 0x10D878 */
    unmark(g, s->marker_cell);
    if (s->draw) s->draw->used = false;
    s->x = -1;
    g->object_count--;
    sk_draw *d = slot_draw_record(g, s);
    if (d && s->draw_index >= SK_FIXED_INDEX_BASE) { d->x = -1; d->y = -1; }
}

void update_objects(sk_game *g) {                               /* 0x10D214 */
    for (int i = 0; i < 16; i++) {
        sk_slot *s = &g->objs.slot[i];
        if (s->x < 0 || s->type4 < 0) continue;                 /* dispatch_object_update skips the skip bit */
        object_update_table[(s->type4 & 0x1FF) / 4](g, s, i);
    }
    for (int i = 0; i < 16; i++) g->objs.slot[i].type4 &= 0x7FFF;
}

int object_offscreen(const sk_game *g, int x, int y, int margin) {   /* 0x10D93A */
    int dx = x - g->camera_x;
    if (dx < 0) dx = -dx + 320;
    if (dx - 320 > margin) return -1;
    int dy = y - g->camera_y;
    if (dy < 0) dy = -dy + 200;
    if (dy - 200 > margin) return -1;
    return 0;
}

void despawn_if_far(sk_game *g, sk_slot *s) {                   /* 0x10E184 */
    if (!(s->flags & 2) && object_offscreen(g, s->x, s->y, 160) == 0) return;
    const sk_draw *d = s->draw;
    int dx = d ? d->x : s->x, dy = d ? d->y : s->y;
    if (object_offscreen(g, dx, dy, 160) == 0) return;
    if ((s->flags & 2) && (s->flags & 8)) free_object_deferred(g, s);
    else free_object(g, s);
}

void set_draw_size(sk_draw *d, int w, int h, const uint8_t *frames) {   /* 0x10813A */
    d->w = (int16_t)w;
    d->h = (int16_t)h;
    int row_bytes = w >> 3;
    d->plane_bytes = (int16_t)(row_bytes * h);
    d->blit_size = (int16_t)((h << 6) + (row_bytes >> 1) + 1);
    d->frame_bytes = (int16_t)(5 * d->plane_bytes);
    d->frames = d->frame = frames;
    d->frames_len = 0;
}

void set_draw_frame_ptr(sk_draw *d, const uint8_t *frame) {
    size_t off = (size_t)(frame - d->frames);
    if (!d->frames || frame < d->frames || (d->frames_len && off + (size_t)(uint16_t)d->frame_bytes > d->frames_len)) { d->frame = NULL; return; }   /* port guard */
    d->frame = frame;
}

void set_object_frame(sk_slot *s, int frame) {                  /* 0x10DF6C */
    s->frame = (uint8_t)frame;
    sk_draw *d = s->draw;
    if (!d) return;
    size_t off = (size_t)frame * (size_t)(uint16_t)d->frame_bytes;
    if (!d->frames || (d->frames_len && off + (size_t)(uint16_t)d->frame_bytes > d->frames_len)) { d->frame = NULL; return; }   /* port guard */
    d->frame = d->frames + off;
}

int step_animation(sk_slot *s, const uint8_t *script, int facing_bit, int *wrapped) {   /* 0x10DE3E */
    int count = script[0] << 8 | script[1], ticks = script[2] << 8 | script[3];
    const uint8_t *frames = script + 4;
    int changed = 0;
    s->anim_tick++;
    if (s->anim_tick == ticks) {
        s->anim_tick = 0;
        s->anim_index++;
        changed = -1;
        if (s->anim_index == count) { s->anim_index = 0; *wrapped = -1; }
    }
    if (facing_bit & 1) frames += count;
    int f = frames[s->anim_index];
    if (f & 0x80) { if (s->draw) s->draw->dflags |= 4; return changed; }
    if (s->draw) s->draw->dflags &= (uint8_t)~4;
    set_object_frame(s, f);
    return changed;
}

void apply_velocity(sk_slot *s) {                               /* 0x10E01C */
    sk_draw *d = s->draw;
    if (s->flags & 4) { s->vy++; if (d) d->y = (int16_t)(d->y + (s->vy >> 1)); }
    int dx = (uint16_t)s->vx >> 1;
    if (!(s->flags & 1)) dx = -dx;
    if (d) d->x = (int16_t)(d->x + dx);
}

static void draw_record(sk_fb *fb, const sk_draw *d, int cam_x, int cam_y, int bg_x, int bg_y) {   /* draw_object 0x1083FA */
    if (!d->used || (d->dflags & 4) || d->y == (int16_t)0x8000 || !d->frame || d->w <= 0 || d->h <= 0) return;
    int sx = d->x, sy = d->y;
    if ((d->dflags & 9) == 9) { sx += 32; }                        /* clipped path, bit 3: x + cam_x + 16 on the window (0x108946) */
    else if (!(d->dflags & 0x10)) { sx -= cam_x; sy -= cam_y; }
    else { sx -= bg_x + SK_BUFFER_WORLD_X; sy -= bg_y; }           /* the backdrop's own buffer offset */                               /* bit 4: buffer coordinates, whose origin is the backdrop scroll (view +0x18 = 0x30019E) */
    sk_frame fr = { d->w, d->h, d->w / 8, { d->frame, d->frame + d->plane_bytes, d->frame + 2 * d->plane_bytes, d->frame + 3 * d->plane_bytes }, d->mask ? d->mask : d->frame + 4 * d->plane_bytes };
    int group = (d->dflags & 0x40) ? SK_GROUP_TILES_B : SK_GROUP_TILES_A;
    if (d->dflags & 2) fb_blit_bob_flash(fb, &fr, sx, sy, group);   /* 0x108D96 */
    else fb_blit_bob(fb, &fr, sx, sy, group);
}

void draw_all_objects(sk_game *g, sk_fb *fb, int cam_x, int cam_y) {   /* 0x1082A8: objects_a first, then the dynamic list */
    int bx = g->view.bg_x, by = g->view.bg_y;
    for (int i = 0; i < 8; i++)
        if (!(g->objs.fixed[i].x == -1 && g->objs.fixed[i].y == -1)) draw_record(fb, &g->objs.fixed[i], cam_x, cam_y, bx, by);
    const sk_objects *o = &g->objs;
    if (o->rope_active < 0) for (int i = 0; i < o->rope_links && i < 8; i++) draw_record(fb, &o->rope[i], cam_x, cam_y, bx, by);   /* 0x108308 */
    if (o->chain_a) for (int i = 0; i < 8; i++) draw_record(fb, &o->chain[i], cam_x, cam_y, bx, by);   /* 0x10832E */
    if (o->chain_b) for (int i = 0; i <= o->chain_count && i < 8; i++) draw_record(fb, &o->chain[i], cam_x, cam_y, bx, by);
    for (int i = 0; i < g->objs.draw_count; i++) draw_record(fb, g->objs.ptrs[i], cam_x, cam_y, bx, by);
    if (o->chain_c) for (int i = 0; i <= o->chain_count && i < 8; i++) draw_record(fb, &o->chain[i], cam_x, cam_y, bx, by);   /* 0x1083D4 */
}

int touch_player(sk_game *g, int x, int y, int half_w, int half_h) {   /* 0x10E354 */
    sk_player *p = &g->player;
    if (g->untouchable || g->player_hidden) return 0;
    int dx = p->x - 8 - x;
    if (dx < 0) { dx = -dx; half_w = 16; }
    int py = p->y, reach = 0x26;
    if (p->crouching || p->sliding) { py += 14; reach -= 14; }
    int dy = py - y;
    if (dy < 0) { dy = -dy; half_h = reach; }
    if (half_w < dx || half_h < dy) return 0;
    return hurt_player(g);
}

uint32_t bcd_add32(uint32_t a, uint32_t b) {
    uint32_t r = 0, carry = 0;
    for (int i = 0; i < 8; i++) {
        uint32_t d = (a >> (4 * i) & 15) + (b >> (4 * i) & 15) + carry;
        carry = d >= 10;
        if (carry) d -= 10;
        r |= d << (4 * i);
    }
    return r;
}

void add_score(sk_game *g, uint32_t bcd) {                      /* 0x102732 */
    uint32_t before = g->player.score & 0xF00000;
    g->player.score = bcd_add32(g->player.score, bcd);
    uint32_t after = g->player.score & 0xF00000;
    if (after != before && (after == 0 || after == 0x200000 || after == 0x400000 || after == 0x600000 || after == 0x800000)) {
        g->player.lives++;
        g->hud_redraw |= 2;
    }
    g->hud_redraw |= 4;
}

void occlude_sprite(const sk_game *g, struct sk_sprite *spr, int ref_x, int ref_y, int height) {   /* sub_107584 */
    if (height > SK_SPR_H) height = SK_SPR_H;
    for (int i = 0; i < 4; i++) {
        if (!g->occl[i].used) continue;
        int ox = g->occl[i].x, oy = g->occl[i].y, h = g->occl[i].h;
        int d3 = (int16_t)(ox - ref_x), d4 = (int16_t)(oy - ref_y);
        int srow = d4 >= 0 ? d4 : 0, mrow = d4 >= 0 ? 0 : -d4;
        if (d4 >= 0 ? d4 >= height : -d4 >= h) continue;
        int rows = h - mrow;
        if (srow + rows > height) rows = height - srow;                 /* port guard: the original runs past the pair */
        if (!g->occl[i].mask) {                                         /* sub_107798: whole rows */
            if ((uint16_t)(d3 + 32) >= 0x60) continue;
            for (int r = 0; r < rows; r++) memset(&spr->px[(srow + r) * SK_SPR_W], 0, SK_SPR_W);
            continue;
        }
        if (d3 <= -32 || d3 >= 64) continue;                            /* sub_1075C8: a 32-px mask at column d3 */
        for (int r = 0; r < rows; r++) {
            size_t m = (size_t)(mrow + r) * 4;
            if (m + 4 > g->occl[i].mask_len) break;
            const uint8_t *q = g->occl[i].mask + m;
            uint32_t bits = (uint32_t)q[0] << 24 | (uint32_t)q[1] << 16 | (uint32_t)q[2] << 8 | q[3];
            for (int k = 0; k < 32; k++) {
                int c = d3 + k;
                if (c < 0 || c >= 64 || !(bits >> (31 - k) & 1)) continue;
                spr->px[(srow + r) * SK_SPR_W + c] = 0;
            }
        }
    }
}
