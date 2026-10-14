/* The SaveGame file (re/docs/notes/frontend.md section 7): 162 bytes, a mask of used slots and five 32-byte slots whose
   first 14 words are XORed with 0x6482 and summed into a checksum. */
#ifndef SK_GAME_SAVEGAME_H
#define SK_GAME_SAVEGAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SK_SAVE_SIZE 0xA2
#define SK_SAVE_SLOTS 5

typedef struct {
    char name[17];                      /* 16 characters, space padded on disk */
    uint32_t score;                     /* BCD */
    int level_number;
    int lives;
    uint8_t cup_pieces, shirt, shorts, extra_heart;
} sk_save_slot;

void savegame_put(uint8_t file[SK_SAVE_SIZE], int slot, const sk_save_slot *s);   /* sub_1034B0's record */
bool savegame_get(const uint8_t *file, size_t n, int slot, sk_save_slot *s);       /* sub_103566: false when unused or damaged */
void savegame_names(const uint8_t *file, size_t n, char rows[SK_SAVE_SLOTS][24]);  /* the list at 0x2009D4 */

#endif
