#include "game/front.h"
#include <string.h>
#include "core/bytes.h"

#define KIT_BASE 0x200EB8u                  /* kit_palette_base: 13 words, colours 49..61 */

void kit_apply(sk_image *img, int shirt, int shorts) {          /* 0x11CCFE */
    if (shirt < 0 || shirt >= SK_KITS || shorts < 0 || shorts >= SK_KITS) return;
    static const uint8_t shirt_at[3] = { 0x10, 0x12, 0x02 }, shorts_at[3] = { 0x16, 0x18, 0x14 };
    for (int k = 0; k < 3; k++) {
        uint8_t w[2];
        put_be16(w, image_u16(img, SK_KIT_TABLE_ADDR + (uint32_t)shirt * 8 + 2u * (uint32_t)k));
        image_write(img, KIT_BASE + shirt_at[k], w, 2);
        put_be16(w, image_u16(img, SK_KIT_TABLE_ADDR + (uint32_t)shorts * 8 + 2u * (uint32_t)k));
        image_write(img, KIT_BASE + shorts_at[k], w, 2);
    }
}

bool language_apply(sk_image *img, const uint8_t *bin, size_t n) {   /* language_select: load_file into 0x202484 */
    return image_write(img, SK_LANG_TEXTS_ADDR, bin, n);
}

const char *language_file(int language) {                         /* the name table at 0x10085A */
    static const char *names[5] = { "english.bin", "french.bin", "german.bin", "italian.bin", "spanish.bin" };
    return language >= 0 && language < 5 ? names[language] : NULL;
}
int hiscore_insert(sk_image *img, uint32_t score, int shirt, int shorts) {   /* attract_hiscores 0x1053B6 */
    int rank = 0;
    while (rank < 10 && score < image_u32(img, SK_HISCORES_ADDR + 16u * (uint32_t)rank + 8)) rank++;   /* cmp.l; bcc */
    if (rank == 10) return -1;
    for (int k = 8; k >= rank; k--) {                            /* the entries below move down, the last is lost */
        const uint8_t *src = image_ptr(img, SK_HISCORES_ADDR + 16u * (uint32_t)k, 16);
        uint8_t tmp[16];
        if (!src) return -1;
        memcpy(tmp, src, 16);
        image_write(img, SK_HISCORES_ADDR + 16u * (uint32_t)(k + 1), tmp, 16);
    }
    uint8_t e[16];
    memset(e, ' ', 8);
    put_be32(e + 8, score); put_be16(e + 12, (uint16_t)shirt); put_be16(e + 14, (uint16_t)shorts);
    image_write(img, SK_HISCORES_ADDR + 16u * (uint32_t)rank, e, 16);
    return rank;
}
