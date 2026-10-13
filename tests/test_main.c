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
void test_tile_blit_writes_nibble_and_keeps_other_group(void);
void test_tile_blit_clips(void);
void test_bob_blit_respects_mask_and_clips(void);
void test_bob_blit_second_group(void);
void test_display_model_constants(void);
void test_rgb12_to_xrgb(void);
void test_palette_from_tilesets_follows_the_register_layout(void);
void test_present_rows_resolves_dual_playfield_and_sky(void);
void test_present_screen_places_the_play_window(void);
void test_png_writes_readable_file(void);
void test_png_unwritable_path(void);
void test_args_defaults_and_errors(void);
void test_scene_draw_level_on_fixture(void);
void test_scene_camera_clamps(void);
void test_scene_real_level0_screenshot(void);
void test_bobs_estimate_real_sheets(void);
void test_window_runs_headless_frames(void);
void test_window_camera_step_clamps(void);
void test_window_fixed_step_accumulator(void);
void test_adf_rejects_out_of_range_pointers(void);
void test_world_builds_cells_and_start(void);
void test_world_without_start_tile_defaults(void);
void test_mask_rebuild_copies_tile_bitmasks(void);
void test_mask_rebuild_clamps_to_the_map_edges(void);
void test_mask_rebuild_is_lazy(void);
void test_probes_on_fixture_geometry(void);

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
    { "tile_blit_writes_nibble_and_keeps_other_group", test_tile_blit_writes_nibble_and_keeps_other_group },
    { "tile_blit_clips", test_tile_blit_clips },
    { "bob_blit_respects_mask_and_clips", test_bob_blit_respects_mask_and_clips },
    { "bob_blit_second_group", test_bob_blit_second_group },
    { "display_model_constants", test_display_model_constants },
    { "rgb12_to_xrgb", test_rgb12_to_xrgb },
    { "palette_from_tilesets_follows_the_register_layout", test_palette_from_tilesets_follows_the_register_layout },
    { "present_rows_resolves_dual_playfield_and_sky", test_present_rows_resolves_dual_playfield_and_sky },
    { "present_screen_places_the_play_window", test_present_screen_places_the_play_window },
    { "png_writes_readable_file", test_png_writes_readable_file },
    { "png_unwritable_path", test_png_unwritable_path },
    { "args_defaults_and_errors", test_args_defaults_and_errors },
    { "scene_draw_level_on_fixture", test_scene_draw_level_on_fixture },
    { "scene_camera_clamps", test_scene_camera_clamps },
    { "scene_real_level0_screenshot", test_scene_real_level0_screenshot },
    { "bobs_estimate_real_sheets", test_bobs_estimate_real_sheets },
    { "window_runs_headless_frames", test_window_runs_headless_frames },
    { "window_camera_step_clamps", test_window_camera_step_clamps },
    { "window_fixed_step_accumulator", test_window_fixed_step_accumulator },
    { "adf_rejects_out_of_range_pointers", test_adf_rejects_out_of_range_pointers },
    { "world_builds_cells_and_start", test_world_builds_cells_and_start },
    { "world_without_start_tile_defaults", test_world_without_start_tile_defaults },
    { "mask_rebuild_copies_tile_bitmasks", test_mask_rebuild_copies_tile_bitmasks },
    { "mask_rebuild_clamps_to_the_map_edges", test_mask_rebuild_clamps_to_the_map_edges },
    { "mask_rebuild_is_lazy", test_mask_rebuild_is_lazy },
    { "probes_on_fixture_geometry", test_probes_on_fixture_geometry },
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
