#include <string.h>
#include "game/savegame.h"
#include "core/bytes.h"

#define KEY 0x6482

void savegame_put(uint8_t file[SK_SAVE_SIZE], int slot, const sk_save_slot *s) {   /* save_game_slot 0x1034B0 */
    if (slot < 0 || slot >= SK_SAVE_SLOTS) return;
    file[0] |= (uint8_t)(1u << slot);
    uint8_t *r = file + 2 + 32 * slot;
    memset(r, ' ', 16);
    size_t l = strlen(s->name);
    memcpy(r, s->name, l > 16 ? 16 : l);
    put_be32(r + 16, s->score);
    put_be16(r + 20, (uint16_t)s->level_number);
    r[22] = (uint8_t)s->lives; r[23] = s->cup_pieces;
    r[24] = s->shorts; r[25] = s->shirt;                        /* 0x3002ED then 0x3002EB */
    r[26] = s->extra_heart; r[27] = 0xFF;
    uint32_t sum = 0;
    for (int k = 0; k < 14; k++) {                              /* the stored words are summed */
        uint16_t w = (uint16_t)(be16(r + 2 * k) ^ KEY);
        put_be16(r + 2 * k, w);
        sum += w;
    }
    put_be32(r + 28, sum);
}

bool savegame_get(const uint8_t *file, size_t n, int slot, sk_save_slot *s) {   /* load_game_slot 0x103566 */
    if (!file || n < SK_SAVE_SIZE || slot < 0 || slot >= SK_SAVE_SLOTS || !(file[0] & (1u << slot))) return false;
    const uint8_t *r = file + 2 + 32 * slot;
    uint8_t d[28];
    uint32_t sum = 0;
    for (int k = 0; k < 14; k++) { uint16_t w = be16(r + 2 * k); sum += w; put_be16(d + 2 * k, (uint16_t)(w ^ KEY)); }
    if (sum != be32(r + 28)) return false;
    memcpy(s->name, d, 16); s->name[16] = 0;
    for (int i = 15; i >= 0 && s->name[i] == ' '; i--) s->name[i] = 0;
    s->score = be32(d + 16);
    s->level_number = be16(d + 20);
    if (s->level_number >= 58) return false;                     /* port guard: past level_sequence */
    s->lives = (int8_t)d[22]; s->cup_pieces = d[23];
    s->shorts = d[24]; s->shirt = d[25]; s->extra_heart = d[26];
    return true;
}

void savegame_names(const uint8_t *file, size_t n, char rows[SK_SAVE_SLOTS][24]) {   /* load_save_names 0x10340A */
    for (int k = 0; k < SK_SAVE_SLOTS; k++) {
        char name[17] = "EMPTY           ";                          /* dat_10349E */
        if (file && n >= SK_SAVE_SIZE && (file[0] & (1u << k)))
            for (int i = 0; i < 8; i++) {                            /* the names' eight words un-XORed */
                uint16_t w = (uint16_t)(be16(file + 2 + 32 * k + 2 * i) ^ KEY);
                char a = (char)(w >> 8), b = (char)w;
                name[2 * i] = a >= 0x20 && a < 0x7F ? a : ' ';      /* port guard: printable */
                name[2 * i + 1] = b >= 0x20 && b < 0x7F ? b : ' ';
            }
        rows[k][0] = (char)('1' + k); rows[k][1] = '.'; rows[k][2] = ' ';
        memcpy(rows[k] + 3, name, 16); rows[k][19] = 0;
    }
}
