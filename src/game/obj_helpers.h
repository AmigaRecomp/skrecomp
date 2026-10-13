/* Helpers shared by the object handlers: the ball against objects, kills, revival, ground probes, frame sizes
   and child spawning. Transcribed from the listing (gameplay.md "Collision helpers", objects.md). */
#ifndef SK_GAME_OBJ_HELPERS_H
#define SK_GAME_OBJ_HELPERS_H

#include <stdbool.h>
#include <stdint.h>
#include "game/objects.h"

int  ball_overlap(sk_game *g, int x, int y, int w, int h);       /* 0x10E586: 0 none, 1 side, 2 top/bottom */
int  ball_overlap_box(const sk_game *g, int x, int y, int w, int h);   /* 0x10E65C: no bounce */
int  ball_hits_object(sk_game *g, sk_slot *s, int dx, int dy, int w, int h, int dead_frame, uint32_t score);   /* 0x10E3EA: -1 no hit, -2 hurt, 0 killed */
int  ball_hits_object_quiet(sk_game *g, sk_slot *s, int dx, int dy, int w, int h, int dead_frame, uint32_t score);  /* 0x10E4BE */
void ball_hits_object_simple(sk_game *g, sk_slot *s, int dx, int dy, int w, int h);                          /* 0x10E54A */
int  point_offscreen(const sk_game *g, int x, int y, int mx, int my);   /* 0x10D96E: -1 outside */
bool ball_kill_check(sk_game *g, sk_slot *s, int dead_frame, uint32_t score);   /* 0x10E108: true when killed */
int  revive_offscreen(sk_game *g, sk_slot *s, int frames, int frame);  /* 0x10E20C: -1 revived this frame */
int  ground_ahead(const sk_game *g, const sk_slot *s, int facing);     /* 0x10DCFC: -1 at an edge */
int  probe_rows_below(const sk_game *g, const sk_slot *s, int dx, int dy);   /* 0x10D9A2: 0, or -rows (up to 5) */
int  probe_rows_below2(const sk_game *g, const sk_slot *s, int dx, int dy);   /* 0x10DA3A: < 0 rows to climb out, else free rows below (up to 5) */
int  ledge_distance(const sk_game *g, const sk_slot *s, int dx, int dy, int *signed_out);   /* 0x10DD80 */
void frames_wh(sk_game *g, sk_slot *s, int w, int h, const uint8_t *frames, int prio);       /* set_frames_base 0x108124 */
#define SK_FRAMES(W, H) static inline void frames_##W##x##H(sk_game *g, sk_slot *s, const uint8_t *f, int prio) { frames_wh(g, s, W, H, f, prio); }
SK_FRAMES(16, 10) SK_FRAMES(16, 16) SK_FRAMES(16, 32) SK_FRAMES(32, 20) SK_FRAMES(32, 24) SK_FRAMES(32, 27) SK_FRAMES(32, 30)
SK_FRAMES(48, 28) SK_FRAMES(48, 32) SK_FRAMES(48, 49) SK_FRAMES(48, 64) SK_FRAMES(64, 32) SK_FRAMES(64, 56) SK_FRAMES(64, 64)
#undef SK_FRAMES
void step_animation_simple(sk_slot *s, const uint8_t *script, int facing);   /* 0x10DEA0: no hidden frames */
void apply_velocity_sym(sk_slot *s);                            /* 0x10E046: halves rounded toward zero */
void apply_velocity_nograv(sk_slot *s);                         /* 0x10E090 */
void free_if_far(sk_game *g, sk_slot *s);                       /* 0x10E290: the draw position 160 px out */
void set_frame2(sk_slot *s, int left, int right, int facing);   /* set_object_frame with d2 = facing */
void set_draw_size_at(sk_game *g, sk_slot *s, int w, int h, const uint8_t *frames);   /* 0x10816E: prio 1 */
size_t frames_room(const sk_game *g, const uint8_t *p);       /* bytes left in the resource section p points into, 0 unknown */
void size_frames(sk_game *g, sk_draw *d, int w, int h, const uint8_t *frames);   /* set_draw_size with the bound */
int  player_overlap(const sk_game *g, int x, int y, int w, int h);   /* 0x10E3A8: -1 overlapping, no hurt */
static inline int aux_c(const sk_slot *s) { return (uint16_t)s->aux >> 8; }        /* byte +0xC */
static inline int aux_d(const sk_slot *s) { return s->aux & 0xFF; }                /* byte +0xD */
static inline void set_aux_c(sk_slot *s, int v) { s->aux = (int16_t)((v & 0xFF) << 8 | (s->aux & 0xFF)); }
static inline void set_aux_d(sk_slot *s, int v) { s->aux = (int16_t)(((uint16_t)s->aux & 0xFF00) | (v & 0xFF)); }
/* sub_10CE92: x - off (x + off and vx bit 15 when the parent faces right), the child block, spawn_object */
sk_slot *spawn_child(sk_game *g, const sk_slot *parent, int x, int y, int type, int off, const uint8_t *frames,
                     const uint8_t *script, int w, int h, int vx, int vy);


/* ---- the script engine (sub_10E69C / sub_10E730 / sub_10E772): phases of (frames, motion offset, next offset),
   motion records of (vx, vy, animation script, code); t0 counts frames, t1 is the phase's byte offset ---- */
typedef struct { uint16_t frames; uint8_t motion, next; } sk_phase;
typedef struct { int16_t vx, vy; const uint8_t *script; int code; } sk_motion;

int  script_step(sk_game *g, sk_slot *s, const sk_phase *ph, const sk_motion *mo, int *changed);   /* sub_10E69C: the motion's code */
void script_start(sk_slot *s, int idx, const sk_phase *ph, const sk_motion *mo);                   /* sub_10E730 */
void script_end_next(sk_slot *s, const sk_phase *ph);                                              /* sub_10E772: end the phase next frame */

#endif
