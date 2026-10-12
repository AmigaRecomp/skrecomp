add_executable(sk_tests
  test_main.c
  fixture.c
  test_err.c
  test_bytekiller.c
  test_zipfile.c
  test_adf.c
  test_disks.c
  test_hunk.c
  test_tables.c)
target_include_directories(sk_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sk_tests PRIVATE skcore)
target_compile_definitions(sk_tests PRIVATE SK_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/")

# One ctest entry per test function; keep in step with the table in test_main.c.
set(SK_TEST_NAMES
  sk_fail_formats_and_returns_false
  bk_unpacks_literal_fixture
  bk_rejects_bad_checksum
  bk_rejects_overrun
  zip_reads_deflated_adf_entry
  zip_reports_missing_entry_and_bad_file
  file_read_all_reads_and_reports
  adf_opens_volume_and_reads_file
  adf_rejects_damaged_images
  disks_opens_zip_or_adf_by_volume_name
  disks_reports_missing_volume
  disks_real_images
  hunk_loads_crunched_data_hunk_with_reloc
  hunk_rejects_truncated_file
  tables_real_sk
  tables_rejects_bad_pointers)
foreach(t ${SK_TEST_NAMES})
  add_test(NAME ${t} COMMAND sk_tests ${t})
endforeach()
