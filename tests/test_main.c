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
void test_init_player_values(void);
void test_player_lands_on_the_floor_from_the_start(void);
void test_player_runs_and_skids(void);
void test_player_jump_integrator(void);
void test_player_falls_off_a_ledge_and_may_late_jump(void);
void test_player_is_stopped_by_a_wall(void);
void test_hurt_player_knockback_and_death(void);
void test_kid_frame_decodes_from_the_chip_hunk(void);
void test_kit_palette_scales_the_base_colours(void);
void test_present_sprite_priority(void);
void test_player_logic_picks_walk_and_jump_frames(void);
void test_knockback_lands_and_grants_invulnerability(void);
void test_death_counts_a_life_and_respawns(void);
void test_death_with_no_lives_left_is_game_over(void);
void test_player_falls_out_of_an_empty_level_and_respawns(void);
void test_exit_walk_sets_the_outcome(void);
void test_ball_states_are_stubs_that_return_to_normal(void);
void test_spawn_marker_allocates_a_slot_with_type_and_param(void);
void test_spawn_marker_ignores_types_beyond_the_table(void);
void test_slots_fill_up_at_sixteen(void);
void test_despawn_if_far_uses_the_160_margin(void);
void test_step_animation_follows_the_script(void);
void test_touch_player_box_and_add_score(void);
void test_draw_all_objects_blits_fixture_frames(void);
void test_camera_clamps_and_follows(void);
void test_push_x_forces_the_camera(void);
void test_crossing_a_column_scans_the_new_column_and_three_beyond(void);
void test_camera_scans_stay_inside_a_tiny_level(void);
void test_exit_attribute_starts_the_exit_walk(void);
void test_checkpoint_moves_the_respawn_and_fires_once(void);
void test_layer_switch_changes_the_priority_word(void);
void test_tile_queue_kinds_step_as_the_table_says(void);
void test_frame_step_runs_the_steps_in_the_notes_order(void);
void test_timer_counts_down_in_bcd_and_times_out(void);
void test_curtain_opens_and_closes(void);
void test_play_level_next_skips_non_tile_levels(void);
void test_level_setup_and_outcome(void);
void test_present_curtain_blacks_rows(void);
void test_scene_backdrop_pos_and_sky(void);
void test_window_input_bits_map_keys(void);
void test_args_script_and_frames(void);
void test_play_script_runs_headless_on_the_fixture(void);
void test_play_script_survives_game_over(void);
void test_play_real_level0_screenshot(void);
void test_camera_holds_its_height_during_a_jump(void);
void test_wall_probe_scans_from_above_the_hit_to_row_39(void);
void test_ceiling_probe_narrows_the_window_to_the_first_row(void);
void test_resources_real_home1(void);
void test_ball_frame_decodes(void);
void test_sin_and_atan_tables(void);
void test_ball_falls_and_comes_to_rest(void);
void test_ball_bounces_off_the_wall_with_reversed_vx(void);
void test_ball_falls_out_and_goes_inactive(void);
void test_ball_sprite_frame_follows_the_roll_phase(void);
void test_ground_kick_sends_the_ball(void);
void test_kick_ignores_an_inactive_ball(void);
void test_tricks_set_the_table_velocities(void);
void test_volley_right_and_left(void);
void test_spring_off_the_ball(void);
void test_kick_charge_table(void);
void test_ball_hits_object_counts_hits_and_kills(void);
void test_ball_overlap_ignores_a_resting_ball(void);
void test_child_spawn_with_full_slots_is_skipped(void);
void test_spawn_child_mirrors_for_a_right_facing_parent(void);
void test_ground_ahead_at_the_pit_edge(void);
void test_frames_32x30_sizes(void);
void test_type_without_its_slot_spawns_nothing(void);
void test_bird_init_and_flight(void);
void test_dropping_falls_and_splashes(void);
void test_bulldog_turns_at_the_pit(void);
void test_crate_breaks_into_debris_and_a_bonus(void);
void test_bonus_gives_its_kind(void);
void test_workman_winds_up_and_punches(void);
void test_rat_waits_then_runs(void);
void test_walker_pauses_away_from_home(void);
void test_cyclist_crashes_when_hit_twice(void);
void test_score_popup_picks_its_number(void);
void test_kick_off_banner_holds_the_kid(void);
void test_every_home_marker_spawns_a_real_type(void);
void test_coin_scores_and_sparkles(void);
void test_full_queue_writes_the_fallback_tile(void);
void test_card_is_collected(void);
void test_kit_shades_follow_the_position(void);
void test_drain_hurts_the_kid_and_swallows_the_ball(void);
void test_bump_block_falls_and_is_restored(void);
void test_message_sign_raises_the_box_once(void);
void test_butterflies_rise_from_a_hit_bush(void);
void test_ball_target_counts_hits(void);
void test_hud_lives_draw_without_a_leading_zero(void);
void test_hud_score_draws_seven_digits(void);
void test_hud_digits_wrap(void);
void test_hud_timer_draws_in_three_planes(void);
void test_hud_health_icons(void);
void test_hud_card_slots_and_collect(void);
void test_message_box_draws_the_text(void);
void test_hud_palette_is_the_game_palette(void);
void test_timer_restarts_after_a_respawn(void);
void test_deferred_free_does_not_blank_a_reused_record(void);
void test_alloc_draw_record_refuses_past_16(void);
void test_probe_surroundings_pushes_the_kid(void);
void test_play_step_reports_a_failed_load(void);
void test_respawn_reapplies_the_kit_palette(void);
void test_play_home1_kick_scores(void);
void test_a_reused_record_gets_fresh_draw_flags(void);
void test_score_popup_blinks_out(void);
void test_missing_or_short_frames_draw_nothing(void);
void test_a_marker_spawns_once_while_its_object_lives(void);
void test_flash_draws_the_flash_colours(void);
void test_flash_does_not_leak(void);
void test_ball_vanish_ends_on_the_roll_table(void);
void test_message_box_rejects_a_tiny_box(void);
void test_kid_stands_on_a_platform(void);
void test_platform_carries_the_kid(void);
void test_ball_rides_a_platform(void);
void test_platform_carry_against_a_ceiling(void);
void test_platform_records_full(void);
void test_bumped_block_carries_the_kid(void);
void test_platform_type_5_moves_and_reverses(void);
void test_triggered_platform_type_10_starts_when_stood_on(void);
void test_sin_1024_quadrants(void);
void test_sinking_platform_type_26(void);
void test_stomper_type_61_bounces_the_kid(void);
void test_fade_colour_steps_toward_white(void);
void test_goose_turns_at_its_range(void);
void test_hopper_hops_and_turns(void);
void test_farmer_walks_and_takes_two_hits(void);
void test_flytrap_swallows_the_ball(void);
void test_mole_throws_a_clod(void);
void test_leaper_jumps_from_below(void);
void test_flies_use_a_fixed_record(void);
void test_country_london_markers_spawn_real_types(void);
void test_deadly_water_43_kills(void);
void test_splash_55_once_until_a_respawn(void);
void test_oneway_38_pushes_back(void);
void test_bare_sign_26_breaks_without_a_message(void);
void test_ball_targets_70_72_73(void);
void test_boss_type_75_runs_its_script_and_ends_the_level(void);
void test_boss_disc_is_thrown_and_bounces(void);

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
    { "init_player_values", test_init_player_values },
    { "player_lands_on_the_floor_from_the_start", test_player_lands_on_the_floor_from_the_start },
    { "player_runs_and_skids", test_player_runs_and_skids },
    { "player_jump_integrator", test_player_jump_integrator },
    { "player_falls_off_a_ledge_and_may_late_jump", test_player_falls_off_a_ledge_and_may_late_jump },
    { "player_is_stopped_by_a_wall", test_player_is_stopped_by_a_wall },
    { "hurt_player_knockback_and_death", test_hurt_player_knockback_and_death },
    { "kid_frame_decodes_from_the_chip_hunk", test_kid_frame_decodes_from_the_chip_hunk },
    { "kit_palette_scales_the_base_colours", test_kit_palette_scales_the_base_colours },
    { "present_sprite_priority", test_present_sprite_priority },
    { "player_logic_picks_walk_and_jump_frames", test_player_logic_picks_walk_and_jump_frames },
    { "knockback_lands_and_grants_invulnerability", test_knockback_lands_and_grants_invulnerability },
    { "death_counts_a_life_and_respawns", test_death_counts_a_life_and_respawns },
    { "death_with_no_lives_left_is_game_over", test_death_with_no_lives_left_is_game_over },
    { "player_falls_out_of_an_empty_level_and_respawns", test_player_falls_out_of_an_empty_level_and_respawns },
    { "exit_walk_sets_the_outcome", test_exit_walk_sets_the_outcome },
    { "ball_states_are_stubs_that_return_to_normal", test_ball_states_are_stubs_that_return_to_normal },
    { "spawn_marker_allocates_a_slot_with_type_and_param", test_spawn_marker_allocates_a_slot_with_type_and_param },
    { "spawn_marker_ignores_types_beyond_the_table", test_spawn_marker_ignores_types_beyond_the_table },
    { "slots_fill_up_at_sixteen", test_slots_fill_up_at_sixteen },
    { "despawn_if_far_uses_the_160_margin", test_despawn_if_far_uses_the_160_margin },
    { "step_animation_follows_the_script", test_step_animation_follows_the_script },
    { "touch_player_box_and_add_score", test_touch_player_box_and_add_score },
    { "draw_all_objects_blits_fixture_frames", test_draw_all_objects_blits_fixture_frames },
    { "camera_clamps_and_follows", test_camera_clamps_and_follows },
    { "push_x_forces_the_camera", test_push_x_forces_the_camera },
    { "crossing_a_column_scans_the_new_column_and_three_beyond", test_crossing_a_column_scans_the_new_column_and_three_beyond },
    { "camera_scans_stay_inside_a_tiny_level", test_camera_scans_stay_inside_a_tiny_level },
    { "exit_attribute_starts_the_exit_walk", test_exit_attribute_starts_the_exit_walk },
    { "checkpoint_moves_the_respawn_and_fires_once", test_checkpoint_moves_the_respawn_and_fires_once },
    { "layer_switch_changes_the_priority_word", test_layer_switch_changes_the_priority_word },
    { "tile_queue_kinds_step_as_the_table_says", test_tile_queue_kinds_step_as_the_table_says },
    { "frame_step_runs_the_steps_in_the_notes_order", test_frame_step_runs_the_steps_in_the_notes_order },
    { "timer_counts_down_in_bcd_and_times_out", test_timer_counts_down_in_bcd_and_times_out },
    { "curtain_opens_and_closes", test_curtain_opens_and_closes },
    { "play_level_next_skips_non_tile_levels", test_play_level_next_skips_non_tile_levels },
    { "level_setup_and_outcome", test_level_setup_and_outcome },
    { "present_curtain_blacks_rows", test_present_curtain_blacks_rows },
    { "scene_backdrop_pos_and_sky", test_scene_backdrop_pos_and_sky },
    { "window_input_bits_map_keys", test_window_input_bits_map_keys },
    { "args_script_and_frames", test_args_script_and_frames },
    { "play_script_runs_headless_on_the_fixture", test_play_script_runs_headless_on_the_fixture },
    { "play_script_survives_game_over", test_play_script_survives_game_over },
    { "play_real_level0_screenshot", test_play_real_level0_screenshot },
    { "camera_holds_its_height_during_a_jump", test_camera_holds_its_height_during_a_jump },
    { "wall_probe_scans_from_above_the_hit_to_row_39", test_wall_probe_scans_from_above_the_hit_to_row_39 },
    { "ceiling_probe_narrows_the_window_to_the_first_row", test_ceiling_probe_narrows_the_window_to_the_first_row },
    { "resources_real_home1", test_resources_real_home1 },
    { "ball_frame_decodes", test_ball_frame_decodes },
    { "sin_and_atan_tables", test_sin_and_atan_tables },
    { "ball_falls_and_comes_to_rest", test_ball_falls_and_comes_to_rest },
    { "ball_bounces_off_the_wall_with_reversed_vx", test_ball_bounces_off_the_wall_with_reversed_vx },
    { "ball_falls_out_and_goes_inactive", test_ball_falls_out_and_goes_inactive },
    { "ball_sprite_frame_follows_the_roll_phase", test_ball_sprite_frame_follows_the_roll_phase },
    { "ground_kick_sends_the_ball", test_ground_kick_sends_the_ball },
    { "kick_ignores_an_inactive_ball", test_kick_ignores_an_inactive_ball },
    { "tricks_set_the_table_velocities", test_tricks_set_the_table_velocities },
    { "volley_right_and_left", test_volley_right_and_left },
    { "spring_off_the_ball", test_spring_off_the_ball },
    { "kick_charge_table", test_kick_charge_table },
    { "ball_hits_object_counts_hits_and_kills", test_ball_hits_object_counts_hits_and_kills },
    { "ball_overlap_ignores_a_resting_ball", test_ball_overlap_ignores_a_resting_ball },
    { "child_spawn_with_full_slots_is_skipped", test_child_spawn_with_full_slots_is_skipped },
    { "spawn_child_mirrors_for_a_right_facing_parent", test_spawn_child_mirrors_for_a_right_facing_parent },
    { "ground_ahead_at_the_pit_edge", test_ground_ahead_at_the_pit_edge },
    { "frames_32x30_sizes", test_frames_32x30_sizes },
    { "type_without_its_slot_spawns_nothing", test_type_without_its_slot_spawns_nothing },
    { "bird_init_and_flight", test_bird_init_and_flight },
    { "dropping_falls_and_splashes", test_dropping_falls_and_splashes },
    { "bulldog_turns_at_the_pit", test_bulldog_turns_at_the_pit },
    { "crate_breaks_into_debris_and_a_bonus", test_crate_breaks_into_debris_and_a_bonus },
    { "bonus_gives_its_kind", test_bonus_gives_its_kind },
    { "workman_winds_up_and_punches", test_workman_winds_up_and_punches },
    { "rat_waits_then_runs", test_rat_waits_then_runs },
    { "walker_pauses_away_from_home", test_walker_pauses_away_from_home },
    { "cyclist_crashes_when_hit_twice", test_cyclist_crashes_when_hit_twice },
    { "score_popup_picks_its_number", test_score_popup_picks_its_number },
    { "kick_off_banner_holds_the_kid", test_kick_off_banner_holds_the_kid },
    { "every_home_marker_spawns_a_real_type", test_every_home_marker_spawns_a_real_type },
    { "coin_scores_and_sparkles", test_coin_scores_and_sparkles },
    { "full_queue_writes_the_fallback_tile", test_full_queue_writes_the_fallback_tile },
    { "card_is_collected", test_card_is_collected },
    { "kit_shades_follow_the_position", test_kit_shades_follow_the_position },
    { "drain_hurts_the_kid_and_swallows_the_ball", test_drain_hurts_the_kid_and_swallows_the_ball },
    { "bump_block_falls_and_is_restored", test_bump_block_falls_and_is_restored },
    { "message_sign_raises_the_box_once", test_message_sign_raises_the_box_once },
    { "butterflies_rise_from_a_hit_bush", test_butterflies_rise_from_a_hit_bush },
    { "ball_target_counts_hits", test_ball_target_counts_hits },
    { "hud_lives_draw_without_a_leading_zero", test_hud_lives_draw_without_a_leading_zero },
    { "hud_score_draws_seven_digits", test_hud_score_draws_seven_digits },
    { "hud_digits_wrap", test_hud_digits_wrap },
    { "hud_timer_draws_in_three_planes", test_hud_timer_draws_in_three_planes },
    { "hud_health_icons", test_hud_health_icons },
    { "hud_card_slots_and_collect", test_hud_card_slots_and_collect },
    { "message_box_draws_the_text", test_message_box_draws_the_text },
    { "hud_palette_is_the_game_palette", test_hud_palette_is_the_game_palette },
    { "timer_restarts_after_a_respawn", test_timer_restarts_after_a_respawn },
    { "deferred_free_does_not_blank_a_reused_record", test_deferred_free_does_not_blank_a_reused_record },
    { "alloc_draw_record_refuses_past_16", test_alloc_draw_record_refuses_past_16 },
    { "probe_surroundings_pushes_the_kid", test_probe_surroundings_pushes_the_kid },
    { "play_step_reports_a_failed_load", test_play_step_reports_a_failed_load },
    { "respawn_reapplies_the_kit_palette", test_respawn_reapplies_the_kit_palette },
    { "play_home1_kick_scores", test_play_home1_kick_scores },
    { "a_reused_record_gets_fresh_draw_flags", test_a_reused_record_gets_fresh_draw_flags },
    { "score_popup_blinks_out", test_score_popup_blinks_out },
    { "missing_or_short_frames_draw_nothing", test_missing_or_short_frames_draw_nothing },
    { "a_marker_spawns_once_while_its_object_lives", test_a_marker_spawns_once_while_its_object_lives },
    { "flash_draws_the_flash_colours", test_flash_draws_the_flash_colours },
    { "flash_does_not_leak", test_flash_does_not_leak },
    { "ball_vanish_ends_on_the_roll_table", test_ball_vanish_ends_on_the_roll_table },
    { "message_box_rejects_a_tiny_box", test_message_box_rejects_a_tiny_box },
    { "kid_stands_on_a_platform", test_kid_stands_on_a_platform },
    { "platform_carries_the_kid", test_platform_carries_the_kid },
    { "ball_rides_a_platform", test_ball_rides_a_platform },
    { "platform_carry_against_a_ceiling", test_platform_carry_against_a_ceiling },
    { "platform_records_full", test_platform_records_full },
    { "bumped_block_carries_the_kid", test_bumped_block_carries_the_kid },
    { "platform_type_5_moves_and_reverses", test_platform_type_5_moves_and_reverses },
    { "triggered_platform_type_10_starts_when_stood_on", test_triggered_platform_type_10_starts_when_stood_on },
    { "sin_1024_quadrants", test_sin_1024_quadrants },
    { "sinking_platform_type_26", test_sinking_platform_type_26 },
    { "stomper_type_61_bounces_the_kid", test_stomper_type_61_bounces_the_kid },
    { "fade_colour_steps_toward_white", test_fade_colour_steps_toward_white },
    { "goose_turns_at_its_range", test_goose_turns_at_its_range },
    { "hopper_hops_and_turns", test_hopper_hops_and_turns },
    { "farmer_walks_and_takes_two_hits", test_farmer_walks_and_takes_two_hits },
    { "flytrap_swallows_the_ball", test_flytrap_swallows_the_ball },
    { "mole_throws_a_clod", test_mole_throws_a_clod },
    { "leaper_jumps_from_below", test_leaper_jumps_from_below },
    { "flies_use_a_fixed_record", test_flies_use_a_fixed_record },
    { "country_london_markers_spawn_real_types", test_country_london_markers_spawn_real_types },
    { "deadly_water_43_kills", test_deadly_water_43_kills },
    { "splash_55_once_until_a_respawn", test_splash_55_once_until_a_respawn },
    { "oneway_38_pushes_back", test_oneway_38_pushes_back },
    { "bare_sign_26_breaks_without_a_message", test_bare_sign_26_breaks_without_a_message },
    { "ball_targets_70_72_73", test_ball_targets_70_72_73 },
    { "boss_type_75_runs_its_script_and_ends_the_level", test_boss_type_75_runs_its_script_and_ends_the_level },
    { "boss_disc_is_thrown_and_bounces", test_boss_disc_is_thrown_and_bounces },
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
