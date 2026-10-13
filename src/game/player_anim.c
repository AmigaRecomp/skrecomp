/* player_logic (0x10B586): the animation frame from the state flags; update_player (0x10A4DC): the kid's sprite. */
#include <string.h>
#include "game/game.h"

#define IDLE_TABLE_RIGHT 0x10B7CE       /* 414 bytes each; a negative entry restarts at 150 idle frames */
#define IDLE_TABLE_LEFT 0x10B96C
#define IDLE_TABLE_LEN 0x19E

static void advance_kick_phase(sk_player *p) {                   /* sub_10B794 / sub_10B75A: only this side effect survives */
    if (p->anim_hold) return;
    if (++p->kick_phase == 12) p->kick_phase--;
}

void player_logic(sk_game *g) {
    sk_player *p = &g->player;
    if (!p->controllable) return;
    int left = p->facing < 0;
    if (p->crushed) {
        p->death_phase = (uint8_t)((p->death_phase + 1) & 7);
        p->anim = (int16_t)(0x30 + ((p->death_phase >> 1) & 3));
        return;
    }
    if (p->sliding) { advance_kick_phase(p); p->anim = (int16_t)(left ? 0x59 : 0x58); return; }
    if (p->landing) {
        if (p->vy <= 0) p->landing = 0;
        p->anim = (int16_t)(left ? 0x45 : 0x44);
        return;
    }
    if (p->jumping) {
        p->walk_acc = 0;
        p->walk_step = 0;
        int a = left ? 0x1F : 0x1C;
        if (p->vy < 0) a += 2;
        else if (p->vy < 7) a += 1;
        p->anim = (int16_t)a;
        return;
    }
    if (p->falling && p->vy <= -14) { p->anim = (int16_t)(left ? 0x21 : 0x1E); return; }
    if (p->crouching) { p->anim = (int16_t)(left ? 0x10 : 0x0F); return; }
    if (p->skidding) { advance_kick_phase(p); p->anim = (int16_t)(left ? 0x0E : 0x0D); return; }
    if (p->pushing) {                                            /* teetering at an edge */
        if (p->idle_frames == 57) p->idle_frames = 25;
        int a = left ? 0x24 : 0x22;
        if ((p->idle_frames - 25) & 0x10) a++;
        p->anim = (int16_t)a;
        return;
    }
    if (p->idle_frames >= 150) {
        int i = (p->idle_frames - 150) >> 2;
        if (i >= IDLE_TABLE_LEN) { p->idle_frames = 150; i = 0; }
        uint32_t table = left ? IDLE_TABLE_LEFT : IDLE_TABLE_RIGHT;
        int a = g->sk ? (int8_t)image_u8(g->sk, table + (uint32_t)i) : (left ? 0x12 : 0x11);
        if (a < 0) { p->idle_frames = 150; a = g->sk ? (int8_t)image_u8(g->sk, table) : 0x11; }
        p->anim = (int16_t)a;
        return;
    }
    if (p->speed <= 3) {
        p->walk_acc = 0;
        p->walk_step = 2;
        p->anim = (int16_t)(left ? 0x12 : 0x11);
        return;
    }
    int acc = p->walk_acc + p->speed;
    if (acc >= 64) { acc = 0; if (++p->walk_step >= 6) p->walk_step = 0; }
    p->walk_acc = (int16_t)acc;
    p->anim = (int16_t)(p->walk_step + (left ? 6 : 0));
}

void update_player(sk_game *g, sk_sprite *kid) {
    sk_player *p = &g->player;
    player_logic(g);
    uint32_t entry = SK_PLAYER_ANIM_TABLE + 8u * (uint32_t)(uint16_t)p->anim;
    uint32_t offset = g->sk ? image_u32(g->sk, entry) : 0;
    int dx = g->sk ? (int16_t)image_u16(g->sk, entry + 4) : 0;
    int dy = g->sk ? (int16_t)image_u16(g->sk, entry + 6) : 0;
    int sx = p->x + dx - g->view.cam_x;                          /* buffer x (the window starts at column 32) */
    int sy = p->y + dy - 7 - g->view.cam_y;
    bool off = sx < 0 || sy < -40 || sx >= 0x1E0;
    kid->visible = false;
    if (p->exit_walk || off) return;
    if (p->invulnerable && p->invulnerable < 1000 && (p->invulnerable & 1)) return;   /* the flicker */
    kid->x = sx - 1;                                             /* the sprite pipeline shows one pixel left (READING) */
    kid->y = sy + 1;
    kid->in_front = false;
    if (g->sk) {
        if (!sprite_decode_pair(g->sk, SK_KID_FRAMES_ADDR + offset, kid)) return;
    }
    kid->visible = true;
}
