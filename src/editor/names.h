/* Names for skld's panel: object types and tile attributes (names.c is generated from re/docs/notes/objects.md). */
#ifndef SK_EDITOR_NAMES_H
#define SK_EDITOR_NAMES_H

#include <stddef.h>
#include "formats/level.h"

const char *edit_type_name(int type);      /* NULL when the type has no name */
const char *edit_attr_name(int attr);
/* What a cell flag means in the setting: "Rat (58), param 4", "type 126, param 0", "Exit", "attribute 94", "nothing". */
void edit_flag_label(const sk_level *setting, int flag, char *out, size_t n);

#endif
