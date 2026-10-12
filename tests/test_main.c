/* sk_tests [name]: runs one named test or all of them; exit code = number of failed checks. */
#include <stdio.h>
#include <string.h>
#include "check.h"

int sk_failures = 0;

void test_sk_fail_formats_and_returns_false(void);
void test_bk_unpacks_literal_fixture(void);
void test_bk_rejects_bad_checksum(void);
void test_bk_rejects_overrun(void);
void test_zip_reads_deflated_adf_entry(void);
void test_zip_reports_missing_entry_and_bad_file(void);
void test_file_read_all_reads_and_reports(void);
void test_adf_opens_volume_and_reads_file(void);
void test_adf_rejects_damaged_images(void);
void test_disks_opens_zip_or_adf_by_volume_name(void);
void test_disks_reports_missing_volume(void);
void test_disks_real_images(void);
void test_hunk_loads_crunched_data_hunk_with_reloc(void);
void test_hunk_rejects_truncated_file(void);
void test_tables_real_sk(void);
void test_tables_rejects_bad_pointers(void);
void test_bk_test_pack_round_trips(void);
void test_level_parses_tiny_fixture(void);
void test_level_rejects_short_map(void);
void test_level_rejects_wrong_tags_and_truncation(void);
void test_level_real_level0(void);
void test_sections_parses_packed_and_raw(void);
void test_sections_rejects_bad_layouts(void);
void test_bobs_split_and_pixels(void);
void test_bobs_rejects_partial_frame(void);
void test_bobs_real_kid(void);
void test_screen_parse_synthetic_with_palette_tail(void);
void test_screen_real_title_palette(void);

static const struct { const char *name; void (*fn)(void); } tests[] = {
    { "sk_fail_formats_and_returns_false", test_sk_fail_formats_and_returns_false },
    { "bk_unpacks_literal_fixture", test_bk_unpacks_literal_fixture },
    { "bk_rejects_bad_checksum", test_bk_rejects_bad_checksum },
    { "bk_rejects_overrun", test_bk_rejects_overrun },
    { "zip_reads_deflated_adf_entry", test_zip_reads_deflated_adf_entry },
    { "zip_reports_missing_entry_and_bad_file", test_zip_reports_missing_entry_and_bad_file },
    { "file_read_all_reads_and_reports", test_file_read_all_reads_and_reports },
    { "adf_opens_volume_and_reads_file", test_adf_opens_volume_and_reads_file },
    { "adf_rejects_damaged_images", test_adf_rejects_damaged_images },
    { "disks_opens_zip_or_adf_by_volume_name", test_disks_opens_zip_or_adf_by_volume_name },
    { "disks_reports_missing_volume", test_disks_reports_missing_volume },
    { "disks_real_images", test_disks_real_images },
    { "hunk_loads_crunched_data_hunk_with_reloc", test_hunk_loads_crunched_data_hunk_with_reloc },
    { "hunk_rejects_truncated_file", test_hunk_rejects_truncated_file },
    { "tables_real_sk", test_tables_real_sk },
    { "tables_rejects_bad_pointers", test_tables_rejects_bad_pointers },
    { "bk_test_pack_round_trips", test_bk_test_pack_round_trips },
    { "level_parses_tiny_fixture", test_level_parses_tiny_fixture },
    { "level_rejects_short_map", test_level_rejects_short_map },
    { "level_rejects_wrong_tags_and_truncation", test_level_rejects_wrong_tags_and_truncation },
    { "level_real_level0", test_level_real_level0 },
    { "sections_parses_packed_and_raw", test_sections_parses_packed_and_raw },
    { "sections_rejects_bad_layouts", test_sections_rejects_bad_layouts },
    { "bobs_split_and_pixels", test_bobs_split_and_pixels },
    { "bobs_rejects_partial_frame", test_bobs_rejects_partial_frame },
    { "bobs_real_kid", test_bobs_real_kid },
    { "screen_parse_synthetic_with_palette_tail", test_screen_parse_synthetic_with_palette_tail },
    { "screen_real_title_palette", test_screen_real_title_palette },
};

int main(int argc, char **argv) {
    size_t n = sizeof tests / sizeof tests[0];
    int ran = 0;
    for (size_t i = 0; i < n; i++) {
        if (argc < 2 || strcmp(argv[1], tests[i].name) == 0) {
            tests[i].fn();
            ran++;
        }
    }
    if (ran == 0) {
        fprintf(stderr, "no test named %s\n", argv[1]);
        return 2;
    }
    if (sk_failures) fprintf(stderr, "%d check(s) failed\n", sk_failures);
    return sk_failures ? 1 : 0;
}
