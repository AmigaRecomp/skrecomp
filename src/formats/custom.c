#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "formats/custom.h"
#include "core/zipfile.h"
#include "game/world.h"
#ifdef _WIN32
#include <windows.h>
#endif

#define HEADER 16

static const sk_setting settings[] = {
    { 0, "home1", "ENGLISH HOME1" },      { 1, "home2", "ENGLISH HOME2" },
    { 2, "country1", "ENG COUNTRY1" },    { 3, "country2", "ENG COUNTRY2" },
    { 4, "london1", "ENGLAND LONDON1" },
    { 6, "ruins1", "ITALY RUINS 1" },     { 7, "ruins2", "ITALY RUINS 2" },
    { 8, "venice1", "ITALY VENICE 1" },   { 9, "venice2", "ITALY VENICE 2" },
    { 10, "riviera1", "ITALY RIVIERA 1" },
    { 13, "forest1", "RUSSIAN FOREST1" }, { 14, "forest2", "RUSSIAN FOREST2" },
    { 16, "redsquare1", "RED SQUARE 1" },
    { 18, "japan1", "JAPAN COUNTRY1" },   { 19, "japan2", "JAPAN COUNTRY2" },
    { 21, "factory1", "JAPAN FACTORY1" },
    { 23, "wildwest1", "WILDWEST 1" },    { 24, "wildwest2", "WILDWEST 2" },
    { 25, "beach1", "USA BEACH1" },       { 26, "beach2", "USA BEACH2" },
    { 27, "city1", "USA CITY1" },
    { 36, "bonus1", "ENGLAND BONUS" }, { 37, "bonus2", "ITALY BONUS" }, { 38, "bonus3", "RUSSIA BONUS" },   /* each world's bonus level */
    { 39, "bonus4", "JAPAN BONUS" },   { 40, "bonus5", "USA BONUS" },
};
#define N_SETTINGS (int)(sizeof settings / sizeof *settings)

static const char *const excluded[] = {                  /* the levels that cannot be settings, by data index */
    [5] = "ENGLAND LONDON2", [11] = "ITALY RIVIERA 2", [12] = "SCOREBOARD", [15] = "BATTLE SHIP", [17] = "RED SQUARE 2",
    [20] = "JAPAN TRAIN", [22] = "JAPAN FACTORY2", [28] = "USA CITY2", [41] = "ALIEN" };

int custom_setting_count(void) { return N_SETTINGS; }
const sk_setting *custom_setting_at(int i) { return i >= 0 && i < N_SETTINGS ? &settings[i] : NULL; }

const sk_setting *custom_setting_by_name(const char *name) {
    for (int i = 0; i < N_SETTINGS; i++) if (strcasecmp(settings[i].name, name) == 0) return &settings[i];
    return NULL;
}

const sk_setting *custom_setting_by_index(int data_index) {
    for (int i = 0; i < N_SETTINGS; i++) if (settings[i].index == data_index) return &settings[i];
    return NULL;
}

bool custom_alloc(sk_custom *c, int setting, int w, int h, sk_err *e) {
    memset(c, 0, sizeof *c);
    c->cells = calloc((size_t)w * h, sizeof *c->cells);
    c->states = calloc((size_t)w * h, 1);
    if (!c->cells || !c->states) { custom_free(c); return sk_fail(e, "out of memory"); }
    c->setting = setting; c->w = w; c->h = h;
    return true;
}

void custom_free(sk_custom *c) {
    free(c->cells); free(c->states);
    memset(c, 0, sizeof *c);
}

bool custom_copy(sk_custom *dst, const sk_custom *src) {
    sk_err e = {0};
    if (!custom_alloc(dst, src->setting, src->w, src->h, &e)) return false;
    memcpy(dst->cells, src->cells, (size_t)src->w * src->h * sizeof *src->cells);
    memcpy(dst->states, src->states, (size_t)src->w * src->h);
    return true;
}

static unsigned be16(const uint8_t *p) { return (unsigned)p[0] << 8 | p[1]; }

bool custom_check_size(int w, int h, sk_err *e) {
    if ((long)w * h > SK_CUSTOM_MAX_CELLS) return sk_fail(e, "map %dx%d exceeds %d cells", w, h, SK_CUSTOM_MAX_CELLS);
    if (w < SK_CUSTOM_MIN_W || h < SK_CUSTOM_MIN_H) return sk_fail(e, "map %dx%d: a level is at least %dx%d", w, h, SK_CUSTOM_MIN_W, SK_CUSTOM_MIN_H);
    return true;
}

bool custom_parse(const uint8_t *d, size_t n, sk_custom *c, sk_err *e) {
    if (n < HEADER) return sk_fail(e, "custom level: file too short (%zu bytes)", n);
    if (memcmp(d, "SKLEVEL1", 8) != 0) return sk_fail(e, "custom level: not a Soccer Kid level (no SKLEVEL1 header)");
    int setting = (int)be16(d + 8), w = (int)be16(d + 10), h = (int)be16(d + 12), markers = (int)be16(d + 14);
    if (!custom_setting_by_index(setting)) {
        if (setting < (int)(sizeof excluded / sizeof *excluded) && excluded[setting])
            return sk_fail(e, "custom level: setting %d (%s) cannot be used for custom levels", setting, excluded[setting]);
        return sk_fail(e, "custom level: unknown setting %d", setting);
    }
    if (!custom_check_size(w, h, e)) return false;
    if (markers != 0) return sk_fail(e, "custom level: a marker table (%d entries) is not supported by this version", markers);
    size_t want = HEADER + (size_t)w * h * 3;
    if (n < want) return sk_fail(e, "custom level: file is %zu bytes short", want - n);
    if (n > want) return sk_fail(e, "custom level: file is %zu byte%s too long", n - want, n - want == 1 ? "" : "s");
    if (!custom_alloc(c, setting, w, h, e)) return false;
    const uint8_t *p = d + HEADER;
    for (int i = 0; i < w * h; i++) c->cells[i] = (uint16_t)be16(p + i * 2);
    memcpy(c->states, p + (size_t)w * h * 2, (size_t)w * h);
    return true;
}

bool custom_write(const sk_custom *c, uint8_t **out, size_t *n) {
    size_t cells = (size_t)c->w * c->h, len = HEADER + cells * 3;
    uint8_t *d = malloc(len);
    if (!d) return false;
    memcpy(d, "SKLEVEL1", 8);
    int head[4] = { c->setting, c->w, c->h, 0 };
    for (int i = 0; i < 4; i++) { d[8 + i * 2] = (uint8_t)(head[i] >> 8); d[9 + i * 2] = (uint8_t)head[i]; }
    for (size_t i = 0; i < cells; i++) { d[HEADER + i * 2] = (uint8_t)(c->cells[i] >> 8); d[HEADER + i * 2 + 1] = (uint8_t)c->cells[i]; }
    memcpy(d + HEADER + cells * 2, c->states, cells);
    *out = d; *n = len;
    return true;
}

bool custom_save(const char *path, const sk_custom *c, sk_err *e) {
    uint8_t *d; size_t n;
    if (!custom_write(c, &d, &n)) return sk_fail(e, "out of memory");
    char tmp[1100];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    bool ok = f && fwrite(d, 1, n, f) == n;
    if (f && fclose(f) != 0) ok = false;
    free(d);
    if (!ok) { remove(tmp); return sk_fail(e, "%s: cannot write the level", path); }
#ifdef _WIN32
    bool moved = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING) != 0;   /* rename does not replace on Windows */
#else
    bool moved = rename(tmp, path) == 0;
#endif
    if (!moved) return sk_fail(e, "%s: cannot replace the level (the new version is in %s)", path, tmp);
    return true;
}

bool custom_load(const char *path, sk_custom *c, sk_err *e) {
    uint8_t *d = NULL; size_t n = 0;
    sk_err inner = {0};
    if (!file_read_all(path, &d, &n, &inner)) return sk_fail(e, "%s", inner.msg);
    bool ok = custom_parse(d, n, c, &inner);
    free(d);
    return ok ? true : sk_fail(e, "%s: %s", path, inner.msg);
}

void custom_defined_flags(const sk_level *setting, bool used[128]) {
    memset(used, 0, 128 * sizeof *used);
    for (int i = 0; i < setting->map.width * setting->map.height; i++) used[setting->map.cells[i] >> SK_FLAG_SHIFT] = true;
    for (int f = 1; f <= 31; f++)
        if (SK_PALS_TYPE + (size_t)f < setting->pals_len && setting->pals[SK_PALS_TYPE + f]) used[f] = true;
    used[0] = true; used[32] = false;
}

int custom_clear_unknown_flags(sk_custom *c, const sk_level *setting) {
    bool used[128];
    custom_defined_flags(setting, used);
    int changed = 0;
    for (int i = 0; i < c->w * c->h; i++)
        if (!used[c->cells[i] >> SK_FLAG_SHIFT]) { c->cells[i] &= (1 << SK_FLAG_SHIFT) - 1; changed++; }
    return changed;
}

bool custom_apply(const sk_custom *c, sk_level *lv) {
    size_t cells = (size_t)c->w * c->h;
    uint16_t *m = malloc(cells * sizeof *m);
    uint8_t *s = malloc(cells);
    if (!m || !s) { free(m); free(s); return false; }
    memcpy(m, c->cells, cells * sizeof *m);
    memcpy(s, c->states, cells);
    free(lv->map.cells); free(lv->map.states);
    lv->map.cells = m; lv->map.states = s;
    lv->map.width = (uint16_t)c->w; lv->map.height = (uint16_t)c->h;
    return true;
}
