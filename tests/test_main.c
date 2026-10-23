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
void test_country_london_run_without_a_crash(void);
void test_ball_on_clears_when_the_ball_leaves(void);
void test_a_rising_ball_passes_through_a_platform(void);
void test_country_london_slots_cover_the_frames(void);
void test_london1_kid_rides_a_platform(void);
void test_dark_kit_keeps_colour_49(void);
void test_tile_animation_groups_step(void);
void test_animated_cells_draw_the_next_tile(void);
void test_home1_cards_and_signs_animate(void);
void test_pendulum_swings_and_reverses(void);
void test_jumper_lands_waits_and_jumps_again(void);
void test_swing_platform_hangs_on_its_chain(void);
void test_strider_walks_turns_and_leaps(void);
void test_thrown_rider_falls_after_50(void);
void test_bouncer_hops_then_leaves(void);
void test_rock_trembles_then_drops_on_the_kid(void);
void test_hopper_49_bounces_and_leaves(void);
void test_thrower_64_patrols_and_throws_at_the_kid(void);
void test_thrower_68_throws_and_waits_25(void);
void test_flyer_70_bobs_and_chases(void);
void test_wings_88_follow_then_fly_off(void);
void test_frog_31_hops_its_pattern_and_spits(void);
void test_hazard_63_takes_a_rising_ball_and_hurts_from_below(void);
void test_hazard_64_takes_a_falling_ball_and_hurts_from_above(void);
void test_ball_targets_69_71_74_count_apart(void);
void test_pendulum_knocks_the_kid_along_its_swing(void);
void test_backdrop_relative_records_draw_at_the_backdrop_scroll(void);
void test_backdrop_cells_paint_save_and_restore(void);
void test_fill_cells_writes_a_column(void);
void test_halfstep_velocity_adds_the_whole_step_on_odd_frames(void);
void test_the_backdrop_draws_from_the_writable_cells(void);
void test_riviera_boss_waits_for_the_arena_wipe(void);
void test_riviera_boss_rises_on_its_lift_and_shoots(void);
void test_riviera_boss_suction_and_the_end(void);
void test_find_slot_by_type_sees_a_slot_spawned_this_frame(void);
void test_italy_markers_spawn_real_types(void);
void test_italy_slots_cover_the_frames(void);
void test_italy_run_without_a_crash(void);
void test_strider_kill_popup_shows_its_500(void);
void test_set_tile_at_ignores_cells_off_the_map(void);
void test_args_start_near_and_markers(void);
void test_play_starts_near_a_type(void);
void test_a_test_start_takes_the_layer_switches_on_its_left(void);
void test_paula_plays_at_the_period_rate(void);
void test_paula_latches_the_loop_at_the_block_end(void);
void test_paula_short_and_bad_loops(void);
void test_replay_starts_a_note(void);
void test_replay_speed_volume_and_jump(void);
void test_replay_fade_and_channel_four(void);
void test_replay_new_note_quirk_applies_the_offset(void);
void test_replay_clamps_to_the_module(void);
void test_all_modules_play(void);
void test_sfx_starts_after_the_handshake(void);
void test_sfx_priority_holds(void);
void test_music_requests_start_on_the_next_frame(void);
void test_music_slot_swaps_once(void);
void test_fade_keeps_the_music_playing(void);
void test_level_music_and_effects_load(void);
void test_kick_off_plays_the_jingle_then_the_level_tune(void);
void test_play_runs_without_audio(void);
void test_music_survives_a_level_change(void);
void test_paula_without_a_rate_is_silent(void);
void test_music_start_leaves_the_effect_channel(void);
void test_freeing_level_effects_stops_channel_four(void);
void test_audio_queue_keeps_its_latency(void);
void test_game_over_fades_the_music(void);
void test_mine_35_explodes_once_set_off(void);
void test_sled_65_speeds_downhill_and_is_kicked_off(void);
void test_icicle_66_drops_waits_and_resets(void);
void test_burrower_72_rises_hops_out_and_walks(void);
void test_walkers_74_78_patrol(void);
void test_sprout_11_grows_and_rests(void);
void test_shell_20_fires_every_100_frames(void);
void test_conveyor_42_pushes_and_feeds_its_belt(void);
void test_gunner_46_fires_both_ways(void);
void test_bullet_97_bursts_on_the_kid(void);
void test_wall_cannon_52_fires_every_100(void);
void test_leaper_71_leaps_from_the_sea(void);
void test_occluders_40_41_hide_the_sprites(void);
void test_tilt_plate_24_tilts_under_the_kid(void);
void test_spring_40_launches_a_falling_kid(void);
void test_side_drain_41_needs_a_side_entry(void);
void test_object_marks_52_77_and_trap_60_61(void);
void test_red_square_boss_76_dies_on_the_21st_hit(void);
void test_rope_1_hangs_and_swings(void);
void test_kid_catches_swings_and_lets_go(void);
void test_edge_row_scan_covers_27_cells(void);
void test_russia_markers_spawn_real_types(void);
void test_russia_slots_cover_the_frames(void);
void test_russia_run_without_a_crash(void);
void test_kid_occluder_reference_is_sx_minus_16(void);
void test_cannon_without_a_fixed_record(void);
void test_insect_25_homes_on_the_kid(void);
void test_thrower_67_wakes_and_throws(void);
void test_wheel_platform_6_turns_full_circles(void);
void test_crab_18_wakes_attacks_and_explodes(void);
void test_random_matches_the_shift_register(void);
void test_popper_33_appears_throws_and_sinks(void);
void test_walker_80_walks_and_turns(void);
void test_press_19_rises_slams_and_fills_its_column(void);
void test_bubble_21_blinks_then_rises(void);
void test_tube_22_carries_the_kid_up(void);
void test_kicker_32_shoots_a_fire_trail(void);
void test_factory_boss_34_stomps_and_drops_rocks(void);
void test_train_23_warns_then_passes(void);
void test_kit_shade_11_12_by_height(void);
void test_freeze_18_holds_the_kick_animation(void);
void test_gap_wind_45_carries_and_belts_46_47_push(void);
void test_rail_arrows_48_to_51_steer_an_object(void);
void test_rail_turn_21_turns_a_platform_a_quarter(void);
void test_hide_56_to_59_by_half_cell(void);
void test_train_trigger_78_starts_one_train(void);
void test_train_setup_starts_the_ramp(void);
void test_train_speeds_up_to_ten_and_rattles(void);
void test_train_tunnel_fades_down_and_back(void);
void test_backdrop_streams_new_columns_from_the_window(void);
void test_battle_ship_rocks_and_drifts(void);
void test_train_exit_walks_left_once_stopped(void);
void test_japan_markers_spawn_real_types(void);
void test_japan_slots_cover_the_frames(void);
void test_japan_run_without_a_crash(void);
void test_tube_ride_is_not_a_game_over(void);
void test_press_despawn_keeps_its_marker_and_attribute(void);
void test_crab_flash_in_the_tunnel_ends_dark(void);
void test_train_warning_blinks_on_ten_off_five(void);
void test_queue_tile_change_takes_its_first_step_at_once(void);
void test_hedgehog_3_walks_and_curls_at_an_edge(void);
void test_roller_47_bounces_and_is_knocked_by_the_ball(void);
void test_gunman_69_shoots_a_kid_in_front(void);
void test_rock_73_shakes_drops_and_breaks(void);
void test_skater_7_pushes_on_the_flat(void);
void test_bouncer_29_bounces_and_is_turned_by_the_ball(void);
void test_egg_30_hatches_a_walker(void);
void test_flyer_14_patrols_drops_and_falls_apart(void);
void test_pot_13_drops_on_a_kid_below(void);
void test_rat_15_rests_then_runs(void);
void test_jumper_17_jumps_onto_a_wall(void);
void test_final_boss_9_brings_its_parts(void);
void test_final_boss_9_last_phase_ends_the_game(void);
void test_ladder_3_catches_a_standing_kid(void);
void test_spikes_4_hurt_the_kid(void);
void test_hoop_36_scores_baskets_and_an_extra_life(void);
void test_usa_markers_spawn_real_types(void);
void test_usa_slots_cover_the_frames(void);
void test_usa_run_without_a_crash(void);
void test_jumper_17_ignores_a_low_step(void);
void test_play_level_entry_runs_maps_and_endings_on_their_own_slot(void);
void test_play_level_entry_walks_the_games_tail(void);
void test_flight_heads_down_to_italy_with_the_empty_shadow(void);
void test_flight_heads_up_to_russia_and_level_elsewhere(void);
void test_flight_wraps_east_from_japan_to_the_usa(void);
void test_masked8_draws_the_index_where_the_mask_is_set(void);
void test_mapsprites_parse_rejects_a_short_pack(void);
void test_mapsprites_real_pack_and_pictures(void);
void test_world_map_zero_holds_201_frames(void);
void test_world_map_draws_the_collected_pieces_only(void);
void test_world_map_one_waits_flies_and_holds(void);
void test_loading_strip_shows_25_frames_in_lines_58_to_141(void);
void test_ending_waits_for_fire_then_fades_64_frames(void);
void test_play_whole_game_runs_map_levels_and_the_scoreboard(void);
void test_play_game_end_starts_over(void);
void test_scoreboard_setup_turns_the_counters_to_bcd(void);
void test_scoreboard_setup_needs_all_cards_and_no_piece(void);
void test_scoreboard_kid_walks_in_then_fetches_the_ball(void);
void test_scoreboard_counts_the_cards_into_the_bonus(void);
void test_scoreboard_pays_the_bonus_into_the_score(void);
void test_scoreboard_ends_with_the_exit_walk(void);
void test_level_end_stats_add_time_and_balls(void);
void test_scoreboard_free_clears_the_cards(void);
void test_play_draws_the_board_text_into_playfield_one(void);
void test_scoreboard_level_counts_and_moves_on(void);
void test_card_resets_clear_all_cards_too(void);
void test_new_game_clears_the_statistics(void);
void test_ending_goes_black_while_the_music_fades(void);
void test_bonus_check_sends_the_world_to_its_bonus_level(void);
void test_bonus_level_setup_keeps_its_clock_and_time_out(void);
void test_cup_piece_81_waits_for_fifty_tokens_then_ends_the_bonus_level(void);
void test_bonus_token_80_counts_as_a_coin(void);
void test_alien_82_without_the_cup_sends_the_kid_home(void);
void test_alien_82_with_the_cup_fights_and_dies(void);
void test_alien_82_starts_over_after_the_kid_dies(void);
void test_alien_shot_83_flies_along_the_arm_and_clamps_its_frame(void);
void test_bonus_levels_and_alien_load_and_run(void);
void test_args_cup_sets_the_pieces(void);
void test_tokens_left_85_shows_in_the_bonus_levels(void);
void test_alien_children_come_from_the_spawns_not_a_stale_slot(void);
void test_alien_lets_go_of_a_head_freed_under_it(void);
void test_bonus_level_game_over_and_direct_start(void);
void test_image_write_stays_inside_a_segment(void);
void test_kit_apply_writes_the_rows_into_the_kid_palette(void);
void test_language_texts_load_into_the_message_tables(void);
void test_language_screen_moves_without_wrapping_and_fire_chooses(void);
void test_title_menu_wraps_and_starts_on_fire(void);
void test_title_menu_times_out_after_750_frames(void);
void test_title_menu_shows_the_kit_on_the_kid(void);
void test_title_sparkle_plays_eleven_steps_after_fifty_frames(void);
void test_play_starts_at_the_language_screen_and_the_title(void);
void test_title_menu_needs_fire_released_once(void);
void test_savegame_slot_round_trips_through_the_xor_and_checksum(void);
void test_savegame_names_show_empty_slots(void);
void test_hiscore_insert_shifts_the_table_down(void);
void test_window_frontend_keys_map_tab_and_f_keys(void);
void test_kit_design_grows_its_panel_and_changes_the_kit(void);
void test_options_toggle_music_and_ask_for_the_tutorial(void);
void test_panel_text_draws_body_and_outline_colours(void);
void test_tab_shows_the_version_box_and_holds_the_menu(void);
void test_hiscore_rows_are_written_as_tiles(void);
void test_name_entry_scrolls_four_pixels_a_frame_and_types(void);
void test_name_entry_scrolls_left_and_wraps(void);
void test_letter_strip_lands_in_the_hud(void);
void test_demo_banner_102_sits_at_the_bottom_of_the_view(void);
void test_tutorial_86_shows_the_joystick_moves_then_closes(void);
void test_fire_ends_a_demo_at_once(void);
void test_title_idle_runs_the_hiscore_pan_then_the_demo(void);
void test_options_tutorial_runs_and_returns(void);
void test_a_qualifying_score_asks_for_a_name(void);
void test_save_prompt_cross_leaves_without_saving(void);
void test_save_prompt_saves_a_typed_name_in_a_slot(void);
void test_resume_list_escape_cancels_and_fire_picks(void);
void test_save_before_a_map_and_resume_it(void);
void test_window_typed_keys_for_the_save_name(void);
void test_replay_master_volume_ramps_two_a_tick(void);
void test_anim_byterun1_body_fills_five_planes_row_by_row(void);
void test_anim_op5_delta_writes_down_columns(void);
void test_anim_real_files_hold_eight_animations(void);
void test_replay_cue_restarts_at_a_position_keeping_the_samples(void);
void test_intro_fade_steps_floor_toward_the_target(void);
void test_intro_types_text_one_and_ends_on_a_key(void);
void test_intro_plays_through_in_about_111_seconds(void);
void test_play_starts_with_the_intro_and_fire_skips_it(void);
void test_resume_waits_for_fire_to_be_let_go(void);
void test_tutorial_survives_the_fire_that_chose_it(void);
void test_savegame_rejects_a_level_past_the_sequence(void);
void test_intro_keeps_a_tap_until_its_next_poll_and_fades_text_one(void);
void test_anim_open_bounds_the_offset_table(void);
void test_escape_closes_the_curtain_and_leaves_the_level(void);
void test_escape_is_ignored_in_a_demo_or_while_the_curtain_moves(void);
void test_f10_pauses_with_the_help_box(void);
void test_escape_leaves_a_game_for_the_title_and_quits_from_the_title(void);
void test_typed_keys_queue_between_frames(void);
void test_anim_open_survives_huge_chunk_lengths(void);
void test_a_pause_does_not_survive_a_level_setup(void);
void test_the_pause_waits_for_a_message_box_and_lifts_its_own_box(void);
void test_pause_only_in_a_running_level(void);
void test_controls_defaults_map_keys_and_pad_to_the_five_inputs(void);
void test_controls_file_round_trips_and_keeps_defaults_for_unknown_lines(void);
void test_controls_set_replaces_one_device_and_keeps_the_other(void);
void test_remap_walks_the_actions_and_sets_each(void);
void test_remap_escape_cancels_and_f5_restores_defaults(void);
void test_controls_file_is_created_with_the_defaults_and_read_back(void);
void test_controls_set_takes_the_input_from_other_actions(void);
void test_controls_file_keeps_comma_keys_bad_lines_and_a_bom(void);
void test_controls_stick_press_needs_a_dominant_axis_and_a_return(void);
void test_remap_ignores_the_fixed_keys(void);
void test_remap_text_fits_the_screen(void);
void test_remap_help_stays_above_the_hud(void);
void test_disks_read_from_an_archive(void);
void test_disks_extract_matches_the_disks(void);
void test_scale_up_repeats_pixels_and_down_averages_them(void);
void test_viewer_shows_frames_only_when_asked(void);

void test_custom_round_trip(void);
void test_custom_rejects_bad_files(void);
void test_custom_settings_table(void);
void test_custom_save_is_atomic(void);
void test_custom_clears_unknown_flags(void);
void test_args_custom(void);
void test_custom_copy_plays_like_the_original(void);
void test_custom_exit_ends_the_run(void);
void test_doc_tile_states_and_flags(void);
void test_doc_paint_flag_erase(void);
void test_doc_fill_copy_paste(void);
void test_doc_resize(void);
void test_doc_undo_redo(void);
void test_doc_new_empty(void);
void test_check_clean_level(void);
void test_check_start_exit_cards_crowding(void);
void test_check_skips_the_exit_without_one_in_the_setting(void);
void test_edit_names(void);
void test_edit_render_matches_the_viewer(void);
void test_skld_args(void);
void test_playtest_leaves_the_document_alone(void);
void test_skld_usage_lists_the_settings(void);
void test_doc_undo_and_resize_end_an_open_stroke(void);
void test_custom_bonus_keeps_its_timer_after_game_over(void);
void test_custom_round_trip_through_the_file_plays_the_same(void);
void test_playtest_game_over_returns(void);
void test_doc_move_start(void);
void test_present_rows_clamps_a_sky_offset_above_the_table(void);
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
    { "country_london_run_without_a_crash", test_country_london_run_without_a_crash },
    { "ball_on_clears_when_the_ball_leaves", test_ball_on_clears_when_the_ball_leaves },
    { "a_rising_ball_passes_through_a_platform", test_a_rising_ball_passes_through_a_platform },
    { "country_london_slots_cover_the_frames", test_country_london_slots_cover_the_frames },
    { "london1_kid_rides_a_platform", test_london1_kid_rides_a_platform },
    { "dark_kit_keeps_colour_49", test_dark_kit_keeps_colour_49 },
    { "tile_animation_groups_step", test_tile_animation_groups_step },
    { "animated_cells_draw_the_next_tile", test_animated_cells_draw_the_next_tile },
    { "home1_cards_and_signs_animate", test_home1_cards_and_signs_animate },
    { "pendulum_swings_and_reverses", test_pendulum_swings_and_reverses },
    { "jumper_lands_waits_and_jumps_again", test_jumper_lands_waits_and_jumps_again },
    { "swing_platform_hangs_on_its_chain", test_swing_platform_hangs_on_its_chain },
    { "strider_walks_turns_and_leaps", test_strider_walks_turns_and_leaps },
    { "thrown_rider_falls_after_50", test_thrown_rider_falls_after_50 },
    { "bouncer_hops_then_leaves", test_bouncer_hops_then_leaves },
    { "rock_trembles_then_drops_on_the_kid", test_rock_trembles_then_drops_on_the_kid },
    { "hopper_49_bounces_and_leaves", test_hopper_49_bounces_and_leaves },
    { "thrower_64_patrols_and_throws_at_the_kid", test_thrower_64_patrols_and_throws_at_the_kid },
    { "thrower_68_throws_and_waits_25", test_thrower_68_throws_and_waits_25 },
    { "flyer_70_bobs_and_chases", test_flyer_70_bobs_and_chases },
    { "wings_88_follow_then_fly_off", test_wings_88_follow_then_fly_off },
    { "frog_31_hops_its_pattern_and_spits", test_frog_31_hops_its_pattern_and_spits },
    { "hazard_63_takes_a_rising_ball_and_hurts_from_below", test_hazard_63_takes_a_rising_ball_and_hurts_from_below },
    { "hazard_64_takes_a_falling_ball_and_hurts_from_above", test_hazard_64_takes_a_falling_ball_and_hurts_from_above },
    { "ball_targets_69_71_74_count_apart", test_ball_targets_69_71_74_count_apart },
    { "pendulum_knocks_the_kid_along_its_swing", test_pendulum_knocks_the_kid_along_its_swing },
    { "backdrop_relative_records_draw_at_the_backdrop_scroll", test_backdrop_relative_records_draw_at_the_backdrop_scroll },
    { "backdrop_cells_paint_save_and_restore", test_backdrop_cells_paint_save_and_restore },
    { "fill_cells_writes_a_column", test_fill_cells_writes_a_column },
    { "halfstep_velocity_adds_the_whole_step_on_odd_frames", test_halfstep_velocity_adds_the_whole_step_on_odd_frames },
    { "the_backdrop_draws_from_the_writable_cells", test_the_backdrop_draws_from_the_writable_cells },
    { "riviera_boss_waits_for_the_arena_wipe", test_riviera_boss_waits_for_the_arena_wipe },
    { "riviera_boss_rises_on_its_lift_and_shoots", test_riviera_boss_rises_on_its_lift_and_shoots },
    { "riviera_boss_suction_and_the_end", test_riviera_boss_suction_and_the_end },
    { "find_slot_by_type_sees_a_slot_spawned_this_frame", test_find_slot_by_type_sees_a_slot_spawned_this_frame },
    { "italy_markers_spawn_real_types", test_italy_markers_spawn_real_types },
    { "italy_slots_cover_the_frames", test_italy_slots_cover_the_frames },
    { "italy_run_without_a_crash", test_italy_run_without_a_crash },
    { "strider_kill_popup_shows_its_500", test_strider_kill_popup_shows_its_500 },
    { "set_tile_at_ignores_cells_off_the_map", test_set_tile_at_ignores_cells_off_the_map },
    { "args_start_near_and_markers", test_args_start_near_and_markers },
    { "play_starts_near_a_type", test_play_starts_near_a_type },
    { "a_test_start_takes_the_layer_switches_on_its_left", test_a_test_start_takes_the_layer_switches_on_its_left },
    { "paula_plays_at_the_period_rate", test_paula_plays_at_the_period_rate },
    { "paula_latches_the_loop_at_the_block_end", test_paula_latches_the_loop_at_the_block_end },
    { "paula_short_and_bad_loops", test_paula_short_and_bad_loops },
    { "replay_starts_a_note", test_replay_starts_a_note },
    { "replay_speed_volume_and_jump", test_replay_speed_volume_and_jump },
    { "replay_fade_and_channel_four", test_replay_fade_and_channel_four },
    { "replay_new_note_quirk_applies_the_offset", test_replay_new_note_quirk_applies_the_offset },
    { "replay_clamps_to_the_module", test_replay_clamps_to_the_module },
    { "all_modules_play", test_all_modules_play },
    { "sfx_starts_after_the_handshake", test_sfx_starts_after_the_handshake },
    { "sfx_priority_holds", test_sfx_priority_holds },
    { "music_requests_start_on_the_next_frame", test_music_requests_start_on_the_next_frame },
    { "music_slot_swaps_once", test_music_slot_swaps_once },
    { "fade_keeps_the_music_playing", test_fade_keeps_the_music_playing },
    { "level_music_and_effects_load", test_level_music_and_effects_load },
    { "kick_off_plays_the_jingle_then_the_level_tune", test_kick_off_plays_the_jingle_then_the_level_tune },
    { "play_runs_without_audio", test_play_runs_without_audio },
    { "music_survives_a_level_change", test_music_survives_a_level_change },
    { "paula_without_a_rate_is_silent", test_paula_without_a_rate_is_silent },
    { "music_start_leaves_the_effect_channel", test_music_start_leaves_the_effect_channel },
    { "freeing_level_effects_stops_channel_four", test_freeing_level_effects_stops_channel_four },
    { "audio_queue_keeps_its_latency", test_audio_queue_keeps_its_latency },
    { "game_over_fades_the_music", test_game_over_fades_the_music },
    { "mine_35_explodes_once_set_off", test_mine_35_explodes_once_set_off },
    { "sled_65_speeds_downhill_and_is_kicked_off", test_sled_65_speeds_downhill_and_is_kicked_off },
    { "icicle_66_drops_waits_and_resets", test_icicle_66_drops_waits_and_resets },
    { "burrower_72_rises_hops_out_and_walks", test_burrower_72_rises_hops_out_and_walks },
    { "walkers_74_78_patrol", test_walkers_74_78_patrol },
    { "sprout_11_grows_and_rests", test_sprout_11_grows_and_rests },
    { "shell_20_fires_every_100_frames", test_shell_20_fires_every_100_frames },
    { "conveyor_42_pushes_and_feeds_its_belt", test_conveyor_42_pushes_and_feeds_its_belt },
    { "gunner_46_fires_both_ways", test_gunner_46_fires_both_ways },
    { "bullet_97_bursts_on_the_kid", test_bullet_97_bursts_on_the_kid },
    { "wall_cannon_52_fires_every_100", test_wall_cannon_52_fires_every_100 },
    { "leaper_71_leaps_from_the_sea", test_leaper_71_leaps_from_the_sea },
    { "occluders_40_41_hide_the_sprites", test_occluders_40_41_hide_the_sprites },
    { "tilt_plate_24_tilts_under_the_kid", test_tilt_plate_24_tilts_under_the_kid },
    { "spring_40_launches_a_falling_kid", test_spring_40_launches_a_falling_kid },
    { "side_drain_41_needs_a_side_entry", test_side_drain_41_needs_a_side_entry },
    { "object_marks_52_77_and_trap_60_61", test_object_marks_52_77_and_trap_60_61 },
    { "red_square_boss_76_dies_on_the_21st_hit", test_red_square_boss_76_dies_on_the_21st_hit },
    { "rope_1_hangs_and_swings", test_rope_1_hangs_and_swings },
    { "kid_catches_swings_and_lets_go", test_kid_catches_swings_and_lets_go },
    { "edge_row_scan_covers_27_cells", test_edge_row_scan_covers_27_cells },
    { "russia_markers_spawn_real_types", test_russia_markers_spawn_real_types },
    { "russia_slots_cover_the_frames", test_russia_slots_cover_the_frames },
    { "russia_run_without_a_crash", test_russia_run_without_a_crash },
    { "kid_occluder_reference_is_sx_minus_16", test_kid_occluder_reference_is_sx_minus_16 },
    { "cannon_without_a_fixed_record", test_cannon_without_a_fixed_record },
    { "insect_25_homes_on_the_kid", test_insect_25_homes_on_the_kid },
    { "thrower_67_wakes_and_throws", test_thrower_67_wakes_and_throws },
    { "wheel_platform_6_turns_full_circles", test_wheel_platform_6_turns_full_circles },
    { "crab_18_wakes_attacks_and_explodes", test_crab_18_wakes_attacks_and_explodes },
    { "random_matches_the_shift_register", test_random_matches_the_shift_register },
    { "popper_33_appears_throws_and_sinks", test_popper_33_appears_throws_and_sinks },
    { "walker_80_walks_and_turns", test_walker_80_walks_and_turns },
    { "press_19_rises_slams_and_fills_its_column", test_press_19_rises_slams_and_fills_its_column },
    { "bubble_21_blinks_then_rises", test_bubble_21_blinks_then_rises },
    { "tube_22_carries_the_kid_up", test_tube_22_carries_the_kid_up },
    { "kicker_32_shoots_a_fire_trail", test_kicker_32_shoots_a_fire_trail },
    { "factory_boss_34_stomps_and_drops_rocks", test_factory_boss_34_stomps_and_drops_rocks },
    { "train_23_warns_then_passes", test_train_23_warns_then_passes },
    { "kit_shade_11_12_by_height", test_kit_shade_11_12_by_height },
    { "freeze_18_holds_the_kick_animation", test_freeze_18_holds_the_kick_animation },
    { "gap_wind_45_carries_and_belts_46_47_push", test_gap_wind_45_carries_and_belts_46_47_push },
    { "rail_arrows_48_to_51_steer_an_object", test_rail_arrows_48_to_51_steer_an_object },
    { "rail_turn_21_turns_a_platform_a_quarter", test_rail_turn_21_turns_a_platform_a_quarter },
    { "hide_56_to_59_by_half_cell", test_hide_56_to_59_by_half_cell },
    { "train_trigger_78_starts_one_train", test_train_trigger_78_starts_one_train },
    { "train_setup_starts_the_ramp", test_train_setup_starts_the_ramp },
    { "train_speeds_up_to_ten_and_rattles", test_train_speeds_up_to_ten_and_rattles },
    { "train_tunnel_fades_down_and_back", test_train_tunnel_fades_down_and_back },
    { "backdrop_streams_new_columns_from_the_window", test_backdrop_streams_new_columns_from_the_window },
    { "battle_ship_rocks_and_drifts", test_battle_ship_rocks_and_drifts },
    { "train_exit_walks_left_once_stopped", test_train_exit_walks_left_once_stopped },
    { "japan_markers_spawn_real_types", test_japan_markers_spawn_real_types },
    { "japan_slots_cover_the_frames", test_japan_slots_cover_the_frames },
    { "japan_run_without_a_crash", test_japan_run_without_a_crash },
    { "tube_ride_is_not_a_game_over", test_tube_ride_is_not_a_game_over },
    { "press_despawn_keeps_its_marker_and_attribute", test_press_despawn_keeps_its_marker_and_attribute },
    { "crab_flash_in_the_tunnel_ends_dark", test_crab_flash_in_the_tunnel_ends_dark },
    { "train_warning_blinks_on_ten_off_five", test_train_warning_blinks_on_ten_off_five },
    { "queue_tile_change_takes_its_first_step_at_once", test_queue_tile_change_takes_its_first_step_at_once },
    { "hedgehog_3_walks_and_curls_at_an_edge", test_hedgehog_3_walks_and_curls_at_an_edge },
    { "roller_47_bounces_and_is_knocked_by_the_ball", test_roller_47_bounces_and_is_knocked_by_the_ball },
    { "gunman_69_shoots_a_kid_in_front", test_gunman_69_shoots_a_kid_in_front },
    { "rock_73_shakes_drops_and_breaks", test_rock_73_shakes_drops_and_breaks },
    { "skater_7_pushes_on_the_flat", test_skater_7_pushes_on_the_flat },
    { "bouncer_29_bounces_and_is_turned_by_the_ball", test_bouncer_29_bounces_and_is_turned_by_the_ball },
    { "egg_30_hatches_a_walker", test_egg_30_hatches_a_walker },
    { "flyer_14_patrols_drops_and_falls_apart", test_flyer_14_patrols_drops_and_falls_apart },
    { "pot_13_drops_on_a_kid_below", test_pot_13_drops_on_a_kid_below },
    { "rat_15_rests_then_runs", test_rat_15_rests_then_runs },
    { "jumper_17_jumps_onto_a_wall", test_jumper_17_jumps_onto_a_wall },
    { "final_boss_9_brings_its_parts", test_final_boss_9_brings_its_parts },
    { "final_boss_9_last_phase_ends_the_game", test_final_boss_9_last_phase_ends_the_game },
    { "ladder_3_catches_a_standing_kid", test_ladder_3_catches_a_standing_kid },
    { "spikes_4_hurt_the_kid", test_spikes_4_hurt_the_kid },
    { "hoop_36_scores_baskets_and_an_extra_life", test_hoop_36_scores_baskets_and_an_extra_life },
    { "usa_markers_spawn_real_types", test_usa_markers_spawn_real_types },
    { "usa_slots_cover_the_frames", test_usa_slots_cover_the_frames },
    { "usa_run_without_a_crash", test_usa_run_without_a_crash },
    { "jumper_17_ignores_a_low_step", test_jumper_17_ignores_a_low_step },
    { "play_level_entry_runs_maps_and_endings_on_their_own_slot", test_play_level_entry_runs_maps_and_endings_on_their_own_slot },
    { "play_level_entry_walks_the_games_tail", test_play_level_entry_walks_the_games_tail },
    { "flight_heads_down_to_italy_with_the_empty_shadow", test_flight_heads_down_to_italy_with_the_empty_shadow },
    { "flight_heads_up_to_russia_and_level_elsewhere", test_flight_heads_up_to_russia_and_level_elsewhere },
    { "flight_wraps_east_from_japan_to_the_usa", test_flight_wraps_east_from_japan_to_the_usa },
    { "masked8_draws_the_index_where_the_mask_is_set", test_masked8_draws_the_index_where_the_mask_is_set },
    { "mapsprites_parse_rejects_a_short_pack", test_mapsprites_parse_rejects_a_short_pack },
    { "mapsprites_real_pack_and_pictures", test_mapsprites_real_pack_and_pictures },
    { "world_map_zero_holds_201_frames", test_world_map_zero_holds_201_frames },
    { "world_map_draws_the_collected_pieces_only", test_world_map_draws_the_collected_pieces_only },
    { "world_map_one_waits_flies_and_holds", test_world_map_one_waits_flies_and_holds },
    { "loading_strip_shows_25_frames_in_lines_58_to_141", test_loading_strip_shows_25_frames_in_lines_58_to_141 },
    { "ending_waits_for_fire_then_fades_64_frames", test_ending_waits_for_fire_then_fades_64_frames },
    { "play_whole_game_runs_map_levels_and_the_scoreboard", test_play_whole_game_runs_map_levels_and_the_scoreboard },
    { "play_game_end_starts_over", test_play_game_end_starts_over },
    { "scoreboard_setup_turns_the_counters_to_bcd", test_scoreboard_setup_turns_the_counters_to_bcd },
    { "scoreboard_setup_needs_all_cards_and_no_piece", test_scoreboard_setup_needs_all_cards_and_no_piece },
    { "scoreboard_kid_walks_in_then_fetches_the_ball", test_scoreboard_kid_walks_in_then_fetches_the_ball },
    { "scoreboard_counts_the_cards_into_the_bonus", test_scoreboard_counts_the_cards_into_the_bonus },
    { "scoreboard_pays_the_bonus_into_the_score", test_scoreboard_pays_the_bonus_into_the_score },
    { "scoreboard_ends_with_the_exit_walk", test_scoreboard_ends_with_the_exit_walk },
    { "level_end_stats_add_time_and_balls", test_level_end_stats_add_time_and_balls },
    { "scoreboard_free_clears_the_cards", test_scoreboard_free_clears_the_cards },
    { "play_draws_the_board_text_into_playfield_one", test_play_draws_the_board_text_into_playfield_one },
    { "scoreboard_level_counts_and_moves_on", test_scoreboard_level_counts_and_moves_on },
    { "card_resets_clear_all_cards_too", test_card_resets_clear_all_cards_too },
    { "new_game_clears_the_statistics", test_new_game_clears_the_statistics },
    { "ending_goes_black_while_the_music_fades", test_ending_goes_black_while_the_music_fades },
    { "bonus_check_sends_the_world_to_its_bonus_level", test_bonus_check_sends_the_world_to_its_bonus_level },
    { "bonus_level_setup_keeps_its_clock_and_time_out", test_bonus_level_setup_keeps_its_clock_and_time_out },
    { "cup_piece_81_waits_for_fifty_tokens_then_ends_the_bonus_level", test_cup_piece_81_waits_for_fifty_tokens_then_ends_the_bonus_level },
    { "bonus_token_80_counts_as_a_coin", test_bonus_token_80_counts_as_a_coin },
    { "alien_82_without_the_cup_sends_the_kid_home", test_alien_82_without_the_cup_sends_the_kid_home },
    { "alien_82_with_the_cup_fights_and_dies", test_alien_82_with_the_cup_fights_and_dies },
    { "alien_82_starts_over_after_the_kid_dies", test_alien_82_starts_over_after_the_kid_dies },
    { "alien_shot_83_flies_along_the_arm_and_clamps_its_frame", test_alien_shot_83_flies_along_the_arm_and_clamps_its_frame },
    { "bonus_levels_and_alien_load_and_run", test_bonus_levels_and_alien_load_and_run },
    { "args_cup_sets_the_pieces", test_args_cup_sets_the_pieces },
    { "tokens_left_85_shows_in_the_bonus_levels", test_tokens_left_85_shows_in_the_bonus_levels },
    { "alien_children_come_from_the_spawns_not_a_stale_slot", test_alien_children_come_from_the_spawns_not_a_stale_slot },
    { "alien_lets_go_of_a_head_freed_under_it", test_alien_lets_go_of_a_head_freed_under_it },
    { "bonus_level_game_over_and_direct_start", test_bonus_level_game_over_and_direct_start },
    { "image_write_stays_inside_a_segment", test_image_write_stays_inside_a_segment },
    { "kit_apply_writes_the_rows_into_the_kid_palette", test_kit_apply_writes_the_rows_into_the_kid_palette },
    { "language_texts_load_into_the_message_tables", test_language_texts_load_into_the_message_tables },
    { "language_screen_moves_without_wrapping_and_fire_chooses", test_language_screen_moves_without_wrapping_and_fire_chooses },
    { "title_menu_wraps_and_starts_on_fire", test_title_menu_wraps_and_starts_on_fire },
    { "title_menu_times_out_after_750_frames", test_title_menu_times_out_after_750_frames },
    { "title_menu_shows_the_kit_on_the_kid", test_title_menu_shows_the_kit_on_the_kid },
    { "title_sparkle_plays_eleven_steps_after_fifty_frames", test_title_sparkle_plays_eleven_steps_after_fifty_frames },
    { "play_starts_at_the_language_screen_and_the_title", test_play_starts_at_the_language_screen_and_the_title },
    { "title_menu_needs_fire_released_once", test_title_menu_needs_fire_released_once },
    { "savegame_slot_round_trips_through_the_xor_and_checksum", test_savegame_slot_round_trips_through_the_xor_and_checksum },
    { "savegame_names_show_empty_slots", test_savegame_names_show_empty_slots },
    { "hiscore_insert_shifts_the_table_down", test_hiscore_insert_shifts_the_table_down },
    { "window_frontend_keys_map_tab_and_f_keys", test_window_frontend_keys_map_tab_and_f_keys },
    { "kit_design_grows_its_panel_and_changes_the_kit", test_kit_design_grows_its_panel_and_changes_the_kit },
    { "options_toggle_music_and_ask_for_the_tutorial", test_options_toggle_music_and_ask_for_the_tutorial },
    { "panel_text_draws_body_and_outline_colours", test_panel_text_draws_body_and_outline_colours },
    { "tab_shows_the_version_box_and_holds_the_menu", test_tab_shows_the_version_box_and_holds_the_menu },
    { "hiscore_rows_are_written_as_tiles", test_hiscore_rows_are_written_as_tiles },
    { "name_entry_scrolls_four_pixels_a_frame_and_types", test_name_entry_scrolls_four_pixels_a_frame_and_types },
    { "name_entry_scrolls_left_and_wraps", test_name_entry_scrolls_left_and_wraps },
    { "letter_strip_lands_in_the_hud", test_letter_strip_lands_in_the_hud },
    { "demo_banner_102_sits_at_the_bottom_of_the_view", test_demo_banner_102_sits_at_the_bottom_of_the_view },
    { "tutorial_86_shows_the_joystick_moves_then_closes", test_tutorial_86_shows_the_joystick_moves_then_closes },
    { "fire_ends_a_demo_at_once", test_fire_ends_a_demo_at_once },
    { "title_idle_runs_the_hiscore_pan_then_the_demo", test_title_idle_runs_the_hiscore_pan_then_the_demo },
    { "options_tutorial_runs_and_returns", test_options_tutorial_runs_and_returns },
    { "a_qualifying_score_asks_for_a_name", test_a_qualifying_score_asks_for_a_name },
    { "save_prompt_cross_leaves_without_saving", test_save_prompt_cross_leaves_without_saving },
    { "save_prompt_saves_a_typed_name_in_a_slot", test_save_prompt_saves_a_typed_name_in_a_slot },
    { "resume_list_escape_cancels_and_fire_picks", test_resume_list_escape_cancels_and_fire_picks },
    { "save_before_a_map_and_resume_it", test_save_before_a_map_and_resume_it },
    { "window_typed_keys_for_the_save_name", test_window_typed_keys_for_the_save_name },
    { "replay_master_volume_ramps_two_a_tick", test_replay_master_volume_ramps_two_a_tick },
    { "anim_byterun1_body_fills_five_planes_row_by_row", test_anim_byterun1_body_fills_five_planes_row_by_row },
    { "anim_op5_delta_writes_down_columns", test_anim_op5_delta_writes_down_columns },
    { "anim_real_files_hold_eight_animations", test_anim_real_files_hold_eight_animations },
    { "replay_cue_restarts_at_a_position_keeping_the_samples", test_replay_cue_restarts_at_a_position_keeping_the_samples },
    { "intro_fade_steps_floor_toward_the_target", test_intro_fade_steps_floor_toward_the_target },
    { "intro_types_text_one_and_ends_on_a_key", test_intro_types_text_one_and_ends_on_a_key },
    { "intro_plays_through_in_about_111_seconds", test_intro_plays_through_in_about_111_seconds },
    { "play_starts_with_the_intro_and_fire_skips_it", test_play_starts_with_the_intro_and_fire_skips_it },
    { "resume_waits_for_fire_to_be_let_go", test_resume_waits_for_fire_to_be_let_go },
    { "tutorial_survives_the_fire_that_chose_it", test_tutorial_survives_the_fire_that_chose_it },
    { "savegame_rejects_a_level_past_the_sequence", test_savegame_rejects_a_level_past_the_sequence },
    { "intro_keeps_a_tap_until_its_next_poll_and_fades_text_one", test_intro_keeps_a_tap_until_its_next_poll_and_fades_text_one },
    { "anim_open_bounds_the_offset_table", test_anim_open_bounds_the_offset_table },
    { "escape_closes_the_curtain_and_leaves_the_level", test_escape_closes_the_curtain_and_leaves_the_level },
    { "escape_is_ignored_in_a_demo_or_while_the_curtain_moves", test_escape_is_ignored_in_a_demo_or_while_the_curtain_moves },
    { "f10_pauses_with_the_help_box", test_f10_pauses_with_the_help_box },
    { "escape_leaves_a_game_for_the_title_and_quits_from_the_title", test_escape_leaves_a_game_for_the_title_and_quits_from_the_title },
    { "typed_keys_queue_between_frames", test_typed_keys_queue_between_frames },
    { "anim_open_survives_huge_chunk_lengths", test_anim_open_survives_huge_chunk_lengths },
    { "a_pause_does_not_survive_a_level_setup", test_a_pause_does_not_survive_a_level_setup },
    { "the_pause_waits_for_a_message_box_and_lifts_its_own_box", test_the_pause_waits_for_a_message_box_and_lifts_its_own_box },
    { "pause_only_in_a_running_level", test_pause_only_in_a_running_level },
    { "controls_defaults_map_keys_and_pad_to_the_five_inputs", test_controls_defaults_map_keys_and_pad_to_the_five_inputs },
    { "controls_file_round_trips_and_keeps_defaults_for_unknown_lines", test_controls_file_round_trips_and_keeps_defaults_for_unknown_lines },
    { "controls_set_replaces_one_device_and_keeps_the_other", test_controls_set_replaces_one_device_and_keeps_the_other },
    { "remap_walks_the_actions_and_sets_each", test_remap_walks_the_actions_and_sets_each },
    { "remap_escape_cancels_and_f5_restores_defaults", test_remap_escape_cancels_and_f5_restores_defaults },
    { "controls_file_is_created_with_the_defaults_and_read_back", test_controls_file_is_created_with_the_defaults_and_read_back },
    { "controls_set_takes_the_input_from_other_actions", test_controls_set_takes_the_input_from_other_actions },
    { "controls_file_keeps_comma_keys_bad_lines_and_a_bom", test_controls_file_keeps_comma_keys_bad_lines_and_a_bom },
    { "controls_stick_press_needs_a_dominant_axis_and_a_return", test_controls_stick_press_needs_a_dominant_axis_and_a_return },
    { "remap_ignores_the_fixed_keys", test_remap_ignores_the_fixed_keys },
    { "remap_text_fits_the_screen", test_remap_text_fits_the_screen },
    { "remap_help_stays_above_the_hud", test_remap_help_stays_above_the_hud },
    { "disks_read_from_an_archive", test_disks_read_from_an_archive },
    { "disks_extract_matches_the_disks", test_disks_extract_matches_the_disks },
    { "scale_up_repeats_pixels_and_down_averages_them", test_scale_up_repeats_pixels_and_down_averages_them },
    { "viewer_shows_frames_only_when_asked", test_viewer_shows_frames_only_when_asked },
    { "custom_round_trip", test_custom_round_trip },
    { "custom_rejects_bad_files", test_custom_rejects_bad_files },
    { "custom_settings_table", test_custom_settings_table },
    { "custom_save_is_atomic", test_custom_save_is_atomic },
    { "custom_clears_unknown_flags", test_custom_clears_unknown_flags },
    { "args_custom", test_args_custom },
    { "custom_copy_plays_like_the_original", test_custom_copy_plays_like_the_original },
    { "custom_exit_ends_the_run", test_custom_exit_ends_the_run },
    { "doc_tile_states_and_flags", test_doc_tile_states_and_flags },
    { "doc_paint_flag_erase", test_doc_paint_flag_erase },
    { "doc_fill_copy_paste", test_doc_fill_copy_paste },
    { "doc_resize", test_doc_resize },
    { "doc_undo_redo", test_doc_undo_redo },
    { "doc_new_empty", test_doc_new_empty },
    { "check_clean_level", test_check_clean_level },
    { "check_start_exit_cards_crowding", test_check_start_exit_cards_crowding },
    { "check_skips_the_exit_without_one_in_the_setting", test_check_skips_the_exit_without_one_in_the_setting },
    { "edit_names", test_edit_names },
    { "edit_render_matches_the_viewer", test_edit_render_matches_the_viewer },
    { "skld_args", test_skld_args },
    { "playtest_leaves_the_document_alone", test_playtest_leaves_the_document_alone },
    { "skld_usage_lists_the_settings", test_skld_usage_lists_the_settings },
    { "doc_undo_and_resize_end_an_open_stroke", test_doc_undo_and_resize_end_an_open_stroke },
    { "custom_bonus_keeps_its_timer_after_game_over", test_custom_bonus_keeps_its_timer_after_game_over },
    { "custom_round_trip_through_the_file_plays_the_same", test_custom_round_trip_through_the_file_plays_the_same },
    { "playtest_game_over_returns", test_playtest_game_over_returns },
    { "doc_move_start", test_doc_move_start },
    { "present_rows_clamps_a_sky_offset_above_the_table", test_present_rows_clamps_a_sky_offset_above_the_table },
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
