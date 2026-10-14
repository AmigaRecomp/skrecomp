/* The save prompt before maps 1..4 (save_game_screen 0x11DD80, save_slot_select 0x11E014) and RESUME GAME's slot list
   (resume_game_screen 0x11E1EA): a 320 x 90 four-plane picture (0x20A20A in sk) at frame lines 58..147, big-font titles, the
   tick and cross buttons, the five 8x8 slot rows (re/docs/notes/frontend.md 10.4). */
#ifndef SK_APP_SAVESCREEN_H
#define SK_APP_SAVESCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/err.h"
#include "core/hunk.h"

typedef enum { SK_SAVE_OFF, SK_SAVE_ASK, SK_SAVE_LIST, SK_SAVE_TYPE, SK_SAVE_SAVING, SK_SAVE_RESUME } sk_save_mode;
typedef enum { SK_SAVE_RESULT_STAY, SK_SAVE_RESULT_NONE, SK_SAVE_RESULT_SAVE, SK_SAVE_RESULT_LOAD } sk_save_result;

#define SK_SAVE_PICTURE_SIZE 16960

typedef struct {
    sk_save_mode mode;
    bool resume;                        /* the RESUME list (no buttons, no typing) */
    uint8_t pic[SK_SAVE_PICTURE_SIZE];
    uint32_t pal[16];
    uint8_t px[320 * 90];
    const sk_image *sk;
    int language;
    int tick;                           /* d3: 0 the cross highlighted, 1 the tick */
    int slot;                           /* 0x3002B4 */
    int typed;
    char rows[5][24];                   /* 0x2009D4: "N. " + 16 name bytes */
    char name[17];
    bool wait_release;
    int saving;                         /* frames left of SAVING GAME (ruling: shown 50 frames for the disk write) */
    uint8_t prev_input;
} sk_savescreen;

/* mode: SK_SAVE_ASK (the prompt) or SK_SAVE_RESUME (the list). picture: the unpacked 0x20A20A (NULL: from sk). */
bool savescreen_open(sk_savescreen *s, sk_save_mode mode, const sk_image *sk, int language, const uint8_t *picture, size_t n,
                     char rows[5][24], sk_err *e);
/* key: the last typed character (ASCII, 8 Backspace, 0x0D Return) or 0; esc: Escape pressed. */
sk_save_result savescreen_step(sk_savescreen *s, uint8_t input, int key, bool esc);
void savescreen_present(const sk_savescreen *s, uint32_t *out);

#endif
