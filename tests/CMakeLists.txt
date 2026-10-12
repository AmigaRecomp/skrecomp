add_executable(sk_tests
  test_main.c
  test_err.c)
target_include_directories(sk_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sk_tests PRIVATE skcore)
target_compile_definitions(sk_tests PRIVATE SK_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/")

# One ctest entry per test function; keep in step with the table in test_main.c.
set(SK_TEST_NAMES
  sk_fail_formats_and_returns_false)
foreach(t ${SK_TEST_NAMES})
  add_test(NAME ${t} COMMAND sk_tests ${t})
endforeach()
