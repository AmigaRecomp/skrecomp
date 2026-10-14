/* The HUD panel (320 x 25, four planes, 160 bytes a line: four 40-byte plane rows) drawn by hud_init (0x102676) and
   hud_update (0x1027AA), and the centred message box (show_message_box 0x11C6BC, draw_box_frame 0x11C8DA). */
#ifndef SK_GAME_HUD_H
#define SK_GAME_HUD_H

#include <stdint.h>

typedef struct sk_game sk_game;

#define SK_HUD_BYTES (25 * 160)

typedef struct sk_hud {
    uint8_t bits[SK_HUD_BYTES];         /* hud_bitmap_ptr's panel */
    uint16_t palette12[16];             /* COLOR00 (0) and the 15 words of hud_palette_game (0x200E7C) */
    int shift;                          /* BPLCON1: the panel's pixels this far right (8 in the name entry) */
    const uint8_t *digits, *timer_font, *icons, *health;   /* digit_font_8, timer_font, hud_icons, health_icons */
} sk_hud;

void hud_init(sk_game *g);              /* clear, palette, every redraw flag, the card slots */
void hud_update(sk_game *g);            /* frame step 24 */
int  hud_pixel(const sk_hud *h, int x, int y);   /* colour 0..15 of the panel pixel */
void message_box_build(sk_game *g, int index);   /* the box bitmap for message index (show_message_box) */

#endif
