#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/resources.h"

static const sk_sections *file_sections(sk_resources *r, sk_disks *d, int disk, int file, sk_err *e) {
    for (int i = 0; i < r->n_files; i++) if (r->files[i].file == file) return &r->files[i].secs;
    if (r->n_files >= SK_RES_FILES) { sk_fail(e, "too many graphics-set files"); return NULL; }
    char name[16];
    snprintf(name, sizeof name, "file%03d", file);
    uint8_t *data; size_t n;
    if (!disks_read(d, disk, name, &data, &n, e)) return NULL;
    sk_err inner = {0};
    bool ok = sections_parse(data, n, &r->files[r->n_files].secs, &inner);
    free(data);
    if (!ok) { sk_fail(e, "%s: %s", name, inner.msg); return NULL; }
    r->files[r->n_files].file = file;
    return &r->files[r->n_files++].secs;
}

bool resources_load(sk_resources *r, sk_disks *d, const sk_tables *t, int data_index, sk_err *e) {
    memset(r, 0, sizeof *r);
    if (data_index < 0 || data_index >= SK_LEVEL_RECORDS) return sk_fail(e, "level %d: no such record", data_index);
    const sk_level_record *rec = &t->record[data_index];
    for (int i = 0; i < rec->n_res; i++) {
        const sk_resource *res = tables_resource(t, rec->disk, rec->res[i]);
        if (!res || res->slot >= SK_SLOTS) continue;
        const sk_sections *secs = file_sections(r, d, rec->disk + 1, res->file, e);
        if (!secs) { resources_free(r); return false; }
        if (res->section < secs->count) r->slot[res->slot] = &secs->sec[res->section];
    }
    return true;
}

void resources_free(sk_resources *r) {
    for (int i = 0; i < r->n_files; i++) sections_free(&r->files[i].secs);
    memset(r, 0, sizeof *r);
}

const uint8_t *resource_slot(const sk_resources *r, int slot, size_t *len) {
    if (slot < 0 || slot >= SK_SLOTS || !r->slot[slot]) return NULL;
    if (len) *len = r->slot[slot]->len;
    return r->slot[slot]->data;
}
