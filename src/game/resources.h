/* slot_table (0x3009EA, 141 longs): the level's graphics-set sections by slot, as load_level_resources
   (0x102E88) fills it from the level record's resource list: entry = (slot, file, section) on the level's disk. */
#ifndef SK_GAME_RESOURCES_H
#define SK_GAME_RESOURCES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/disks.h"
#include "core/err.h"
#include "core/tables.h"
#include "formats/sections.h"

#define SK_SLOTS 141
#define SK_RES_FILES 32

typedef struct sk_resources {
    struct { int file; sk_sections secs; } files[SK_RES_FILES];   /* each graphics-set file loaded once */
    int n_files;
    const sk_section *slot[SK_SLOTS];
    struct { const sk_section *sec; uint8_t bytes[6]; } param[4];   /* param_resources (0x301AB8): load_param_resources 0x103114 */
} sk_resources;

bool resources_load(sk_resources *r, sk_disks *d, const sk_tables *t, int data_index, sk_err *e);
void resources_free(sk_resources *r);
/* The slot's decrunched section bytes, or NULL when the level does not load that slot. */
const uint8_t *resource_slot(const sk_resources *r, int slot, size_t *len);

#endif
