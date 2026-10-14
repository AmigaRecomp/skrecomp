/* The frontend's screens (re/docs/notes/frontend.md): the language screen (language_select 0x1006D8), the title menu
   (title_menu 0x11CB56) with its sparkle and arrows, KIT DESIGN (0x11CF92) and OPTIONS (0x11D504). Each frame is a 320 x 256
   index image through a 256-colour palette, stepped at 50 Hz with the joystick bits. */
#ifndef SK_APP_FRONTEND_H
#define SK_APP_FRONTEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/disks.h"
#include "core/err.h"
#include "core/hunk.h"

typedef enum { SK_FE_OFF, SK_FE_LANGUAGE, SK_FE_MENU, SK_FE_KIT, SK_FE_OPTIONS } sk_fe_mode;
typedef enum { SK_FE_STAY, SK_FE_LANGUAGE_CHOSEN, SK_FE_START, SK_FE_IDLE, SK_FE_TUTORIAL, SK_FE_RESUME, SK_FE_QUIT } sk_fe_result;

#define SK_TITLE_SIZE 91224             /* file078 unpacked: the picture, its palette, 14 kit palettes, 7 sparkle frames, the arrow */
#define SK_FE_KEY_TAB 1                 /* keys besides the joystick bits */
#define SK_FE_KEY_QUIT 2                /* CTRL held + ESC */
#define SK_FE_KEY_F1 0x100              /* F1..F10: 0x100 << n */

typedef struct {
    sk_fe_mode mode;
    uint8_t px[320 * 256];
    uint32_t pal[256];
    uint8_t base[320 * 256];            /* the picture as decrunched (what the save/restore blits put back) */
    uint8_t *title;                     /* file078 unpacked (owned) */
    uint8_t lang_pic[40768];            /* language.pak unpacked */
    const sk_image *sk;
    uint8_t prev_input;                 /* 0x300386 */
    int language;                       /* 0x30028E */
    int item;                           /* 0x300388: START GAME, KIT DESIGN, OPTIONS */
    int attract_counter;                /* 0x3002A8 */
    int shirt, shorts;                  /* 0x3002EA / 0x3002EC */
    int sparkle_step, sparkle_wait, sparkle_phase; bool sparkle_on;   /* 0x3002EE, 0x3002F0, 0x30037C, 0x30037B */
    int panel_step;                     /* the panel's grow animation, 0..16 */
    int option;                         /* the OPTIONS cursor */
    bool *music_off, *sfx_muted;        /* the options' toggles (0x300367 / 0x300368), NULL in tests */
    bool tutorial, resume;              /* 0x300393 / 0x300390 */
    bool fire_released;                 /* ruling: START GAME needs fire let go once since the menu opened */
    const uint16_t *hiscore_kits;       /* F1..F10: 10 x (shirt, shorts), NULL none */
} sk_frontend;

bool frontend_language_from(sk_frontend *f, const sk_image *sk, const uint8_t *pak, size_t n, sk_err *e);
bool frontend_menu_from(sk_frontend *f, const sk_image *sk, const uint8_t *title, size_t n, sk_err *e);   /* keeps item, kit */
bool frontend_language(sk_frontend *f, const sk_image *sk, sk_disks *d, sk_err *e);
bool frontend_menu(sk_frontend *f, const sk_image *sk, sk_disks *d, sk_err *e);
/* One frame: input = the joystick bits (game/input.h), keys = SK_FE_KEY_*. */
sk_fe_result frontend_step(sk_frontend *f, uint8_t input, unsigned keys);
void frontend_present(const sk_frontend *f, uint32_t *out);
void frontend_free(sk_frontend *f);

#endif
