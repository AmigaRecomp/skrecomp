#include <stdio.h>
#include "editor/names.h"
#include "game/world.h"

void edit_flag_label(const sk_level *lv, int flag, char *out, size_t n) {
    if (flag <= 0) { snprintf(out, n, "nothing"); return; }
    if (flag <= 31 && SK_PALS_PARAM + (size_t)flag < lv->pals_len) {
        int type = lv->pals[SK_PALS_TYPE + flag], param = lv->pals[SK_PALS_PARAM + flag];
        const char *name = edit_type_name(type);
        if (name) snprintf(out, n, "%s (%d), param %d", name, type, param);
        else snprintf(out, n, "type %d, param %d", type, param);
        return;
    }
    if (flag >= 33 && SK_PALS_ATTR + (size_t)(flag - 33) < lv->pals_len) {
        int attr = lv->pals[SK_PALS_ATTR + flag - 33];
        const char *name = edit_attr_name(attr);
        if (name) snprintf(out, n, "%s", name);
        else snprintf(out, n, "attribute %d", attr);
        return;
    }
    snprintf(out, n, "flag %d", flag);
}
