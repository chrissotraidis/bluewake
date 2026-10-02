add_executable(bluewake_haptics_test
    "${BLUEWAKE_REPO_ROOT}/tests/haptics_test.c" "${BLUEWAKE_HOST_SRC}/haptics.c")
target_include_directories(bluewake_haptics_test PRIVATE "${BLUEWAKE_HOST_SRC}")
target_link_libraries(bluewake_haptics_test PRIVATE gxruntime SDL3::SDL3)
add_test(NAME bluewake_haptics_classic_test COMMAND bluewake_haptics_test classic)
add_test(NAME bluewake_haptics_enhanced_test COMMAND bluewake_haptics_test enhanced)
add_test(NAME bluewake_haptics_ps5_test COMMAND bluewake_haptics_test ps5)
set_tests_properties(bluewake_haptics_classic_test bluewake_haptics_enhanced_test bluewake_haptics_ps5_test PROPERTIES TIMEOUT 15)
