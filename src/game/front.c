#include "game/front.h"
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
