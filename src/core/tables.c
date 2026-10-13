#include <stdlib.h>
#include <string.h>
#include "core/tables.h"
#include "core/bytes.h"

static void parse_record(const uint8_t *raw, sk_level_record *r) {
    memset(r, 0, sizeof *r);
    r->disk = raw[2];
    for (int i = 7; i < 11; i++) r->param_raw[i - 7] = raw[i];
    for (int i = 7; i < 11; i++)
        if (raw[i] != 0xFF) r->params[r->n_params++] = raw[i];
    for (int i = 11; i < 43 && raw[i] != 0xFF; i++)
        r->res[r->n_res++] = raw[i];
    r->music = raw[43];
    for (int i = 44; i < 47 && raw[i] != 0xFF; i++)
        r->extra[r->n_extra++] = raw[i];
}

bool tables_load(sk_tables *t, const sk_image *sk, sk_err *e) {
    memset(t, 0, sizeof *t);
    const uint8_t *seq = image_ptr(sk, SK_LEVEL_SEQUENCE_ADDR, SK_LEVEL_SEQUENCE_LEN);
    if (!seq) return sk_fail(e, "sk: level sequence outside the image");
    memcpy(t->sequence, seq, SK_LEVEL_SEQUENCE_LEN);
    const uint8_t *recs = image_ptr(sk, SK_LEVEL_RECORDS_ADDR, SK_LEVEL_RECORD * SK_LEVEL_RECORDS);
    if (!recs) return sk_fail(e, "sk: level records outside the image");
    for (int i = 0; i < SK_LEVEL_RECORDS; i++) parse_record(recs + i * SK_LEVEL_RECORD, &t->record[i]);
    for (int d = 0; d < SK_TABLE_DISKS; d++) {
        const uint8_t *p = image_ptr(sk, SK_DISK_TABLES_ADDR + (uint32_t)d * 16, 12);
        if (!p) { tables_free(t); return sk_fail(e, "sk: disk table %d outside the image", d); }
        uint32_t p_res = be32(p), p_tab2 = be32(p + 4), p_par = be32(p + 8);
        uint32_t next_start = SK_TABLES_END;
        if (d + 1 < SK_TABLE_DISKS) {
            const uint8_t *q = image_ptr(sk, SK_DISK_TABLES_ADDR + (uint32_t)(d + 1) * 16, 4);
            if (q) next_start = be32(q);
        }
        int64_t n_res = ((int64_t)p_tab2 - (int64_t)p_res) / SK_RESOURCE_ENTRY;
        int64_t n_par = ((int64_t)next_start - (int64_t)p_par) / SK_PARAM_ENTRY;
        const uint8_t *entries = n_res >= 0 ? image_ptr(sk, p_res, (size_t)n_res * SK_RESOURCE_ENTRY) : NULL;
        if (n_res < 0 || n_par < 0 || n_res > 4096 || !entries) {
            tables_free(t);
            return sk_fail(e, "sk: disk %d: inconsistent table pointers %06X %06X %06X", d, p_res, p_tab2, p_par);
        }
        t->res[d] = calloc(n_res ? (size_t)n_res : 1, sizeof(sk_resource));
        if (!t->res[d]) { tables_free(t); return sk_fail(e, "out of memory"); }
        for (int64_t i = 0; i < n_res; i++) {
            const uint8_t *q = entries + i * SK_RESOURCE_ENTRY;
            t->res[d][i] = (sk_resource){ q[0], q[1], q[2] };
        }
        t->n_res[d] = (int)n_res;
        const uint8_t *pe = n_par > 0 && n_par <= 256 ? image_ptr(sk, p_par, (size_t)n_par * SK_PARAM_ENTRY) : NULL;
        t->par[d] = calloc(pe ? (size_t)n_par : 1, sizeof(sk_param_entry));
        if (!t->par[d]) { tables_free(t); return sk_fail(e, "out of memory"); }
        for (int64_t i = 0; pe && i < n_par; i++) {
            const uint8_t *q = pe + i * SK_PARAM_ENTRY;
            t->par[d][i].file = q[0]; t->par[d][i].section = q[1];
            memcpy(t->par[d][i].bytes, q + 2, 6);
        }
        t->n_par[d] = pe ? (int)n_par : 0;
    }
    return true;
}

void tables_free(sk_tables *t) {
    for (int d = 0; d < SK_TABLE_DISKS; d++) {
        free(t->res[d]);
        t->res[d] = NULL;
        t->n_res[d] = 0;
        free(t->par[d]);
        t->par[d] = NULL;
        t->n_par[d] = 0;
    }
}

const sk_resource *tables_resource(const sk_tables *t, int disk, int idx) {
    if (disk < 0 || disk >= SK_TABLE_DISKS || idx < 0 || idx >= t->n_res[disk]) return NULL;
    return &t->res[disk][idx];
}

const sk_param_entry *tables_param(const sk_tables *t, int disk, int idx) {
    if (disk < 0 || disk >= SK_TABLE_DISKS || idx < 0 || idx >= t->n_par[disk]) return NULL;
    return &t->par[disk][idx];
}
