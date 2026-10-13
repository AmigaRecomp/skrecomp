/* Object slots (0x3025A6), draw records (0x3023E6 / objects_a 0x302146), spawning, the type dispatch tables,
   animation helpers and drawing. Names follow re/symbols/sk.sym; records follow gameplay.md "Memory map". */
#ifndef SK_GAME_OBJECTS_H
#define SK_GAME_OBJECTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gfx/framebuffer.h"

typedef struct sk_game sk_game;

typedef struct sk_draw {
    int16_t x, y, sx, sy, w, h, blit_size, plane_bytes, frame_bytes;
    const uint8_t *frames, *frame;
    size_t frames_len;             /* bytes from frames to the end of its section; 0 = unknown (not checked) */
    uint8_t prio, dflags;          /* dflags: bit 0 clipped blitter, 1 flash, 2 hidden, 4 screen-relative, 6 playfield 2 */
    bool used;                     /* bit 6 of the record's first byte: set by the slot initialisers, cleared on free */
    const uint8_t *mask;           /* port: the mask plane when it is not frame + 4 planes (the rope's tiles keep it in their table) */
} sk_draw;

typedef struct sk_slot {
    int16_t x, y, x0, y0, type4, flags, aux, draw_index, vy, vx, t0, t1;
    sk_draw *draw;
    uint8_t flags_b;               /* byte +0xB: the flags word's low byte (types that count in the whole word) */
    uint8_t anim, hits, frame, anim_tick, anim_index, timer;
    const uint8_t *script;         /* the long at +0x14 for the child types that keep a script there */
    int marker_cell;               /* +0x18: the marker cell that spawned it (cell index), -1 none */
    int platform;                  /* the platform record index the flags word holds (index * 32), -1 none */
} sk_slot;                         /* x < 0 = free; draw_index: 0..15 dynamic, 100 + n fixed, -1 none */

#define SK_FIXED_INDEX_BASE 100

typedef struct sk_objects {
    sk_slot slot[16];
    sk_draw dyn[16], fixed[8];     /* draw_records, objects_a */
    sk_draw *ptrs[16];             /* draw_record_ptrs: allocation order */
    sk_draw chain[8];              /* objects_c 0x302306: the records of types 36 and 38 */
    bool chain_a, chain_b, chain_c;   /* 0x300345 (all 8, before the list), 0x300346 (before), 0x300347 (after) */
    int chain_count;               /* 0x3002FC: records - 1 drawn for chain_b / chain_c */
    sk_draw rope[8];               /* objects_b 0x302226: the rope's links (type 1) */
    int8_t rope_active;            /* 0x300344: 1 after init, -1 (drawn) after the first update */
    int16_t rope_link_step, rope_links;   /* 0x3001C8, 0x3001CA */
    uint16_t rope_angle;           /* 0x3001CC: 0x4000 straight down */
    int16_t rope_swing;            /* 0x3001CE: the angular speed */
    int16_t rope_base_x, rope_base_y, rope_end_x, rope_end_y, rope_end_x0, rope_end_y0;   /* 0x3001BC.. */
    bool strip_row;                /* 0x300348: the marker was found by vertical scrolling */
    int draw_count;                /* draw_record_count */
    int deferred[64]; int n_deferred;   /* 0x300C1E: marker cells of objects freed by free_object_deferred */
    int spawn_cell;                /* a4 during spawn_marker: the cell index, -1 otherwise */
    int marker_param;              /* 0x300336 */
    sk_slot *last_slot; sk_draw *last_draw;   /* 0x300074 / 0x300078 */
    const uint8_t *child_frames, *child_script; int16_t child_vx, child_vy, child_w, child_h;   /* 0x300270.. */
} sk_objects;

typedef void (*sk_obj_init_fn)(sk_game *g, sk_slot *s, int x, int y);
typedef void (*sk_obj_update_fn)(sk_game *g, sk_slot *s, int index);
extern sk_obj_init_fn object_init_table[128];
extern sk_obj_update_fn object_update_table[128];

void objects_reset(sk_game *g);                   /* clear_object_slots + the record lists; installs stubs in NULL entries */
void objects_flush_deferred(sk_game *g);          /* init_player (0x10BC68): those markers may spawn again */
void free_object_keep(sk_game *g, sk_slot *s);    /* 0x10D7C6: its marker stays used */
sk_slot *find_free_slot(sk_game *g);
sk_slot *find_slot_by_type(sk_game *g, int type);   /* sub_10DF8A: the first live slot of that type, NULL none */
void spawn_marker(sk_game *g, int cx, int cy);    /* type/param from the PALS bytes; types >= 128 ignored */
sk_slot *spawn_object(sk_game *g, int x, int y, int type);
void scan_cell_for_marker(sk_game *g, int cx, int cy);
void init_object_slot(sk_game *g, sk_slot *s, int x, int y, int type4);
void init_object_slot_nodraw(sk_game *g, sk_slot *s, int x, int y, int type4);
void init_object_slot_fixed(sk_game *g, sk_slot *s, int x, int y, int type4);
void init_object_slot_fixed_nodraw(sk_game *g, sk_slot *s, int x, int y, int type4);
void init_object_slot_shared(sk_game *g, sk_slot *s, int x, int y, int type4, sk_draw *shared);
void init_object_slot_invisible(sk_game *g, sk_slot *s, int x, int y, int type4);
sk_draw *slot_draw_record(sk_game *g, const sk_slot *s);
sk_draw *alloc_draw_record(sk_game *g, sk_slot *s);   /* 0x10D7F6: the next record of the dynamic list */
void free_draw_record(sk_game *g, sk_draw *d);         /* 0x10D820 */   /* dynamic or fixed record of the slot, or NULL */
void free_object(sk_game *g, sk_slot *s);
void free_object_deferred(sk_game *g, sk_slot *s);
void free_fixed_object(sk_game *g, sk_slot *s);
void update_objects(sk_game *g);
int  object_offscreen(const sk_game *g, int x, int y, int margin);   /* -1 when beyond the margin */
void despawn_if_far(sk_game *g, sk_slot *s);
void set_draw_size(sk_draw *d, int w, int h, const uint8_t *frames);
void set_object_frame(sk_slot *s, int frame);
struct sk_sprite;
/* sub_107554 / sub_10756C: the occluders blank the pair's pixels; ref = the pair's column 0 and row 0 in world pixels, as the original
   measures them (its d0/d1 plus the camera), height 48 (kid) or 16 (ball). */
void occlude_sprite(const sk_game *g, struct sk_sprite *spr, int ref_x, int ref_y, int height);
void set_draw_frame_ptr(sk_draw *d, const uint8_t *frame);   /* move.l a1,$16(a0); NULL outside the record's section */
int  step_animation(sk_slot *s, const uint8_t *script, int facing_bit, int *wrapped);   /* -1 when the frame changed */
void apply_velocity(sk_slot *s);
void draw_all_objects(sk_game *g, sk_fb *fb, int cam_x, int cam_y);
int  touch_player(sk_game *g, int x, int y, int half_w, int half_h);   /* hurt_player's result, 0 = no touch */
void add_score(sk_game *g, uint32_t bcd);
uint32_t bcd_add32(uint32_t a, uint32_t b);
void obj_stub_init(sk_game *g, sk_slot *s, int x, int y);
void obj_stub_update(sk_game *g, sk_slot *s, int index);

#endif
