/* The in-game display as read from sk: see re/docs/notes/display.md (copper_game at 0x400548).
   Every value here cites that note; a correction later is one edit in this file. */
#ifndef SK_GFX_DISPLAY_MODEL_H
#define SK_GFX_DISPLAY_MODEL_H

#define SK_FB_W 352             /* playfield bitmaps: 44 bytes per plane row (VERIFIED: modulo 0x84 = 176 - 44) */
#define SK_FB_H 200             /* lines 44..243 carry the per-line colour writes (VERIFIED) */
#define SK_VIEW_X 32            /* visible buffer columns 32..319 (READING: DDFSTRT 0x30, DIWSTRT 0x91) */
#define SK_VIEW_W 288           /* DIW 145..432 (VERIFIED) */
#define SK_VIEW_H 200
#define SK_SCREEN_W 320         /* the standard PAL frame the window is placed in */
#define SK_SCREEN_H 256
#define SK_PLAY_X 16            /* play area at (16, 0) of that frame (VERIFIED) */
#define SK_PLAY_Y 0
#define SK_HUD_Y 201            /* lines 245..269: a 320x25 four-plane panel (VERIFIED) */
#define SK_HUD_H 25
#define SK_GROUP_TILES_A 0      /* foreground = playfield 1 = low nibble (VERIFIED) */
#define SK_GROUP_TILES_B 1      /* backdrop = playfield 2 = high nibble (VERIFIED) */
#define SK_GROUP_BOB 0          /* object flag bit 6 clear; set -> group 1 (VERIFIED, sprites.md) */
#define SK_DUAL_PLAYFIELD 1     /* BPLCON0 0x0610 (VERIFIED) */
#define SK_PF2_COLOUR_BASE 16   /* BPLCON3 PF2OF = 16 (VERIFIED) */
#define SK_CAM_DX 144           /* camera = clamp(player - (144, 100)) (VERIFIED, update_camera) */
#define SK_CAM_DY 100
#define SK_CAM_MARGIN_W 352     /* fg_scroll_max = (w*16 - 352, h*16 - 200) (VERIFIED) */
#define SK_CAM_MARGIN_H 200
#define SK_BG_MARGIN_W 320      /* bg_scroll_max = (w*16 - 320, h*16 - 200) (VERIFIED) */
#define SK_SPR_W 64             /* FMODE 64-pixel sprites: a kid frame is 0x320 bytes = 16 + 48*16 + 16 (VERIFIED) */
#define SK_SPR_H 48
#define SK_SPR_FRAME 0x320      /* one sprite of the attached pair; the pair = 0x640 */
#define SK_SPR_COLOUR_BASE 48   /* attached pair in bank 1: value v shows colour 48 + v (VERIFIED display.md) */
#define SK_KID_FRAMES_ADDR 0x401FE8   /* chip hunk: sprite_data_ptr (0x401FD8, 8-aligned) + 16 */
#define SK_PLAYER_ANIM_TABLE 0x2005EC /* 74 x (u32 frame offset, i16 dx, i16 dy) */
#define SK_KIT_BASE_ADDR 0x200EB8     /* 13 colour words of the kit palette */
#define SK_KIT_EXTRA_ADDR 0x200ED2    /* 2 more (colours 62, 63) */

#endif
