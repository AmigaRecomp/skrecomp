/* The game's own tables inside sk (hunk 1 data): the level sequence, the 64-byte level records and
   the per-disk resource tables (slot, file, section). Addresses from re/docs/notes/sprites.md. */
#ifndef SK_CORE_TABLES_H
#define SK_CORE_TABLES_H

#include <stdbool.h>
#include <stdint.h>
#include "core/err.h"
#include "core/hunk.h"

#define SK_LEVEL_SEQUENCE_ADDR 0x2011EA
#define SK_LEVEL_SEQUENCE_LEN 58
#define SK_LEVEL_RECORDS_ADDR 0x201224
#define SK_LEVEL_RECORD 64
#define SK_LEVEL_RECORDS 42
#define SK_DISK_TABLES_ADDR 0x201CA4
#define SK_TABLE_DISKS 4
#define SK_TABLES_END 0x2021E4
#define SK_RESOURCE_ENTRY 4
#define SK_PARAM_ENTRY 8

typedef struct { uint8_t slot, file, section; } sk_resource;
typedef struct { uint8_t file, section, bytes[6]; } sk_param_entry;   /* the per-disk param table (8 bytes an entry) */

typedef struct {
    uint8_t disk;               /* 0..3 = SKDisk1..SKDisk4 */
    uint8_t n_res, res[32];     /* indices into that disk's resource table, in load order */
    uint8_t music;              /* music file number */
    uint8_t n_params, params[4];
    uint8_t param_raw[4];       /* the four positions as stored (0xFF = none): load_param_resources keeps positions */
    uint8_t n_extra, extra[3];
} sk_level_record;

typedef struct {
    uint8_t sequence[SK_LEVEL_SEQUENCE_LEN];     /* data indices in play order */
    sk_level_record record[SK_LEVEL_RECORDS];
    sk_resource *res[SK_TABLE_DISKS];           /* owned */
    int n_res[SK_TABLE_DISKS];
    sk_param_entry *par[SK_TABLE_DISKS];        /* owned */
    int n_par[SK_TABLE_DISKS];
} sk_tables;

bool tables_load(sk_tables *t, const sk_image *sk, sk_err *e);
void tables_free(sk_tables *t);

/* Entry idx of disk's resource table, or NULL when out of range. */
const sk_resource *tables_resource(const sk_tables *t, int disk, int idx);
const sk_param_entry *tables_param(const sk_tables *t, int disk, int idx);

#endif
