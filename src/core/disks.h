/* The five Soccer Kid disks: found in a directory as .adf or .zip files, identified by volume name. */
#ifndef SK_CORE_DISKS_H
#define SK_CORE_DISKS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/adf.h"
#include "core/err.h"

#define SK_DISKS 5

typedef struct {
    uint8_t *image[SK_DISKS];   /* owned; NULL when that disk was not found */
    size_t len[SK_DISKS];
    sk_adf vol[SK_DISKS];
    char path[SK_DISKS][512];
    const uint8_t *archive;     /* soccerkid.dat instead of the images (not owned), NULL none */
    size_t archive_len;
    uint32_t archive_count;
} sk_disks;

/* Scans dir; returns false naming the first missing SKDisk<n>, but keeps whatever was found. */
bool disks_open(sk_disks *d, const char *dir, sk_err *e);

/* Reads a file by name from disk n (0..4). */
bool disks_read(sk_disks *d, int disk, const char *name, uint8_t **out, size_t *len, sk_err *e);

/* Reads "file%03d" from disk n; with unpack, the ByteKiller container is decompressed. */
bool disks_read_file_no(sk_disks *d, int disk, int file_no, bool unpack, uint8_t **out, size_t *len, sk_err *e);

/* soccerkid.dat: "SKDATA01", a big-endian count, entries of 32 bytes (disk byte, name[23], offset, size), then the files.
   The data is not copied: it must outlive d (the embedded copy does). */
bool disks_open_archive(sk_disks *d, const uint8_t *data, size_t len, sk_err *e);
/* Every file the port reads (sk.pak, anim, animfil1/2.pak, language.pak, the five .bin files, file000..file255 on each disk)
   into one archive (malloc'ed). */
bool disks_extract(sk_disks *d, uint8_t **out, size_t *len, sk_err *e);

void disks_close(sk_disks *d);

#endif
