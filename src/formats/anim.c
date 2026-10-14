#include <string.h>
#include "formats/anim.h"
#include "core/bytes.h"

static bool tag(const uint8_t *p, const char *t) { return memcmp(p, t, 4) == 0; }

bool anim_open(const uint8_t *file, size_t n, int index, sk_anim *a, sk_err *e) {
    memset(a, 0, sizeof *a);
    if (n < 4 || index < 0 || (size_t)(index + 1) * 4 > be32(file) || (size_t)(index + 1) * 4 > n) return sk_fail(e, "anim: no animation %d", index);
    uint32_t at = be32(file + 4 * (size_t)index);
    if ((size_t)at + 12 > n || !tag(file + at, "FORM") || !tag(file + at + 8, "ANIM")) return sk_fail(e, "anim %d: not a FORM ANIM", index);
    size_t end = be32(file + at + 4) > n - at - 8 ? n : at + 8 + be32(file + at + 4);   /* compared as what is left: no wrap */
    for (size_t p = at + 12; p + 12 <= end; ) {                  /* the FORM ILBMs */
        size_t flen = be32(file + p + 4);
        if (!tag(file + p, "FORM") || flen > end - p - 8) return sk_fail(e, "anim %d: a bad FORM at %zu", index, p);
        size_t form_end = p + 8 + flen;
        sk_anim_frame f = { false, NULL, 0, NULL };
        for (size_t c = p + 12; c + 8 <= form_end; ) {           /* its chunks */
            size_t len = be32(file + c + 4);
            if (len > form_end - c - 8) break;
            if (tag(file + c, "CMAP") && len >= 96) f.cmap = file + c + 8;
            if (tag(file + c, "BODY")) { f.key = true; f.data = file + c + 8; f.n = len; }
            if (tag(file + c, "DLTA")) { f.data = file + c + 8; f.n = len; }
            c += 8 + len + (len & 1);
        }
        if (f.data) {
            if (a->count >= SK_ANIM_MAX) return sk_fail(e, "anim %d: more than %d frames", index, SK_ANIM_MAX);
            a->frame[a->count++] = f;
        }
        p = form_end + ((form_end - p) & 1);
    }
    if (a->count < 1 || !a->frame[0].key) return sk_fail(e, "anim %d: no key frame", index);
    return true;
}

void anim_decode_body(const uint8_t *body, size_t n, uint8_t planes[6][8000]) {   /* decode_ilbm_body 0x100D52 */
    size_t i = 0;
    for (int row = 0; row < 200; row++)
        for (int p = 0; p < 5; p++) {
            uint8_t *dst = planes[p] + row * 40;
            for (int x = 0; x < 40 && i < n; ) {
                int c = (int8_t)body[i++];
                if (c >= 0) { for (int k = 0; k <= c && x < 40 && i < n; k++) dst[x++] = body[i++]; }
                else {                                           /* -128 repeats 129 times, as neg.b leaves it */
                    int count = c == -128 ? 129 : -c + 1;
                    if (i >= n) return;
                    uint8_t v = body[i++];
                    for (int k = 0; k < count && x < 40; k++) dst[x++] = v;
                }
            }
        }
}

void anim_apply_delta(const uint8_t *d, size_t n, uint8_t planes[6][8000]) {   /* apply_delta_frame 0x100E4E */
    if (n < 24) return;
    for (int p = 0; p < 6; p++) {
        uint32_t off = be32(d + 4 * p);
        if (!off || off >= n) continue;
        size_t i = off;
        for (int col = 0; col < 40; col++) {                     /* decode_op5_plane 0x100E7C */
            if (i >= n) return;
            int ops = d[i++], row = 0;
            for (int k = 0; k < ops; k++) {
                if (i >= n) return;
                uint8_t op = d[i++];
                if (op == 0) {                                   /* a run: count, value */
                    if (i + 2 > n) return;
                    int cnt = d[i++]; uint8_t v = d[i++];
                    for (int r = 0; r < cnt; r++, row++) if (row < 200) planes[p][row * 40 + col] = v;
                } else if (op < 0x80) row += op;                 /* skip */
                else for (int r = 0; r < (op & 0x7F); r++, row++) {   /* literals */
                    if (i >= n) return;
                    uint8_t v = d[i++];
                    if (row < 200) planes[p][row * 40 + col] = v;
                }
            }
        }
    }
}
