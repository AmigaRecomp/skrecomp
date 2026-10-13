/* Object slots (0x3025A6), draw records (0x3023E6 / objects_a 0x302146), spawning, the type dispatch tables,
   animation helpers and drawing. Names follow re/symbols/sk.sym; records follow gameplay.md "Memory map". */
#ifndef SK_GAME_OBJECTS_H
#define SK_GAME_OBJECTS_H

#include <stdbool.h>
#include <stdint.h>
#include "gfx/framebuffer.h"

typedef struct sk_game sk_game;

typedef struct sk_draw {
    int16_t x, y, sx, sy, w, h, blit_size, plane_bytes, frame_bytes;
    const uint8_t *frames, *frame;
    uint8_t prio, dflags;          /* dflags: bit 0 clipped blitter, 1 flash, 2 hidden, 4 screen-relative, 6 playfield 2 */
    bool used;                     /* bit 6 of the record's first byte: set by the slot initialisers, cleared on free */
} sk_draw;

typedef struct sk_slot {
    int16_t x, y, x0, y0, type4, flags, aux, draw_index, vy, vx, t0, t1;
    sk_draw *draw;
    uint8_t anim, hits, frame, anim_tick, anim_index, timer;
} sk_slot;                         /* x < 0 = free; draw_index: 0..15 dynamic, 100 + n fixed, -1 none */

#define SK_FIXED_INDEX_BASE 100

typedef struct sk_objects {
    sk_slot slot[16];
    sk_draw dyn[16], fixed[8];     /* draw_records, objects_a */
    sk_draw *ptrs[16];             /* draw_record_ptrs: allocation order */
    int draw_count;                /* draw_record_count */
    sk_draw *deferred[17]; int n_deferred;
    int marker_param;              /* 0x300336 */
    sk_slot *last_slot; sk_draw *last_draw;   /* 0x300074 / 0x300078 */
    const uint8_t *child_frames, *child_script; int16_t child_vx, child_vy, child_w, child_h;   /* 0x300270.. */
} sk_objects;

typedef void (*sk_obj_init_fn)(sk_game *g, sk_slot *s, int x, int y);
typedef void (*sk_obj_update_fn)(sk_game *g, sk_slot *s, int index);
extern sk_obj_init_fn object_init_table[128];
extern sk_obj_update_fn object_update_table[128];

void objects_reset(sk_game *g);                   /* clear_object_slots + the record lists; installs stubs in NULL entries */
void objects_flush_deferred(sk_game *g);          /* releases the records freed by free_object_deferred */
sk_slot *find_free_slot(sk_game *g);
void spawn_marker(sk_game *g, int cx, int cy);    /* type/param from the PALS bytes; types >= 128 ignored */
sk_slot *spawn_object(sk_game *g, int x, int y, int type);
void scan_cell_for_marker(sk_game *g, int cx, int cy);
void init_object_slot(sk_game *g, sk_slot *s, int x, int y, int type4);
void init_object_slot_nodraw(sk_game *g, sk_slot *s, int x, int y, int type4);
void init_object_slot_fixed(sk_game *g, sk_slot *s, int x, int y, int type4);
void init_object_slot_fixed_nodraw(sk_game *g, sk_slot *s, int x, int y, int type4);
void init_object_slot_shared(sk_game *g, sk_slot *s, int x, int y, int type4, sk_draw *shared);
void init_object_slot_invisible(sk_game *g, sk_slot *s, int x, int y, int type4);
sk_draw *slot_draw_record(sk_game *g, const sk_slot *s);   /* dynamic or fixed record of the slot, or NULL */
void free_object(sk_game *g, sk_slot *s);
void free_object_deferred(sk_game *g, sk_slot *s);
void free_fixed_object(sk_game *g, sk_slot *s);
void update_objects(sk_game *g);
int  object_offscreen(const sk_game *g, int x, int y, int margin);   /* -1 when beyond the margin */
void despawn_if_far(sk_game *g, sk_slot *s);
void set_draw_size(sk_draw *d, int w, int h, const uint8_t *frames);
void set_object_frame(sk_slot *s, int frame);
int  step_animation(sk_slot *s, const uint8_t *script, int facing_bit, int *wrapped);   /* -1 when the frame changed */
void apply_velocity(sk_slot *s);
void draw_all_objects(sk_game *g, sk_fb *fb, int cam_x, int cam_y);
int  touch_player(sk_game *g, int x, int y, int half_w, int half_h);   /* hurt_player's result, 0 = no touch */
void add_score(sk_game *g, uint32_t bcd);
void obj_stub_init(sk_game *g, sk_slot *s, int x, int y);
void obj_stub_update(sk_game *g, sk_slot *s, int index);

#endif
