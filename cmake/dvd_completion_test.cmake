add_executable(bluewake_dvd_completion_test
    "${BLUEWAKE_REPO_ROOT}/tests/dvd_completion_test.c" "${BLUEWAKE_HOST_SRC}/save_state.c")
target_include_directories(bluewake_dvd_completion_test PRIVATE "${BLUEWAKE_HOST_SRC}")
add_test(NAME bluewake_dvd_completion_test COMMAND bluewake_dvd_completion_test)
