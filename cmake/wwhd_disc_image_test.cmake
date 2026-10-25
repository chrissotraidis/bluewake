# Generated WUD/WUX reader fixtures; no game data or keys.
add_executable(bluewake_wwhd_disc_image_test
    ${BLUEWAKE_REPO_ROOT}/tests/wwhd_disc_image_test.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/disc_image.cpp)
target_compile_features(bluewake_wwhd_disc_image_test PRIVATE cxx_std_17)
target_include_directories(bluewake_wwhd_disc_image_test PRIVATE ${BLUEWAKE_HOST_SRC})
add_test(NAME bluewake_wwhd_disc_image_test COMMAND bluewake_wwhd_disc_image_test)
set_tests_properties(bluewake_wwhd_disc_image_test PROPERTIES TIMEOUT 15)

include("${CMAKE_CURRENT_LIST_DIR}/wwhd_disc_crypto.cmake")
add_executable(bluewake_wwhd_disc_crypto_test ${BLUEWAKE_REPO_ROOT}/tests/wwhd_disc_crypto_test.cpp)
target_compile_features(bluewake_wwhd_disc_crypto_test PRIVATE cxx_std_17)
target_include_directories(bluewake_wwhd_disc_crypto_test PRIVATE ${BLUEWAKE_HOST_SRC})
target_link_libraries(bluewake_wwhd_disc_crypto_test PRIVATE bluewake_wwhd_disc_crypto)
add_test(NAME bluewake_wwhd_disc_crypto_test COMMAND bluewake_wwhd_disc_crypto_test)
set_tests_properties(bluewake_wwhd_disc_crypto_test PROPERTIES TIMEOUT 15)

# Python is used only to create independent encrypted test data, never by the
# app. Developers can select their test venv without changing Dawn's Python.
set(BLUEWAKE_WWHD_TEST_PYTHON "" CACHE FILEPATH "Python with pycryptodome for HD disc test fixtures")
if(NOT BLUEWAKE_WWHD_TEST_PYTHON)
    find_program(BLUEWAKE_WWHD_TEST_PYTHON_DEFAULT NAMES python3 python REQUIRED)
    set(BLUEWAKE_WWHD_TEST_PYTHON "${BLUEWAKE_WWHD_TEST_PYTHON_DEFAULT}")
endif()
set(_wwhd_fixtures "${BLUEWAKE_REPO_ROOT}/build/wwhd-disc-import/fixtures")
add_executable(bluewake_wwhd_wiiu_disc_test
    ${BLUEWAKE_REPO_ROOT}/tests/wwhd_wiiu_disc_test.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/wiiu_disc.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/disc_image.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/formats.cpp)
target_compile_features(bluewake_wwhd_wiiu_disc_test PRIVATE cxx_std_17)
target_include_directories(bluewake_wwhd_wiiu_disc_test PRIVATE ${BLUEWAKE_HOST_SRC})
target_link_libraries(bluewake_wwhd_wiiu_disc_test PRIVATE bluewake_wwhd_disc_crypto xxhash)
add_test(NAME bluewake_wwhd_disc_fixtures COMMAND "${BLUEWAKE_WWHD_TEST_PYTHON}"
    -B "${BLUEWAKE_REPO_ROOT}/tests/wwhd_disc_fixture.py" "${_wwhd_fixtures}")
set_tests_properties(bluewake_wwhd_disc_fixtures PROPERTIES FIXTURES_SETUP wwhd_disc TIMEOUT 60)
add_test(NAME bluewake_wwhd_wiiu_disc_test COMMAND bluewake_wwhd_wiiu_disc_test "${_wwhd_fixtures}")
set_tests_properties(bluewake_wwhd_wiiu_disc_test PROPERTIES FIXTURES_REQUIRED wwhd_disc TIMEOUT 30)

# Exercise the complete image-to-PNG pipeline with synthetic BTI/BFRES data.
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
add_executable(bluewake_wwhd_cancellation_test
    ${BLUEWAKE_REPO_ROOT}/tests/wwhd_cancellation_test.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/sha256.cpp)
target_compile_features(bluewake_wwhd_cancellation_test PRIVATE cxx_std_17)
target_include_directories(bluewake_wwhd_cancellation_test PRIVATE ${BLUEWAKE_HOST_SRC})
target_link_libraries(bluewake_wwhd_cancellation_test PRIVATE bluewake_wwhd_disc_crypto Threads::Threads)
add_test(NAME bluewake_wwhd_cancellation_test COMMAND bluewake_wwhd_cancellation_test)
set_tests_properties(bluewake_wwhd_cancellation_test PROPERTIES TIMEOUT 15)

add_executable(bluewake_wwhd_native_import
    ${BLUEWAKE_REPO_ROOT}/tests/wwhd_native_import.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/formats.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/addrlib.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/hd.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/imaging.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/sha256.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/ui.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/pack.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/output_staging.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/disc_image.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/wiiu_disc.cpp)
target_compile_features(bluewake_wwhd_native_import PRIVATE cxx_std_17)
target_compile_definitions(bluewake_wwhd_native_import PRIVATE WWHD_IMPORT_TESTING)
target_include_directories(bluewake_wwhd_native_import PRIVATE ${BLUEWAKE_HOST_SRC})
target_link_libraries(bluewake_wwhd_native_import PRIVATE
    bluewake_wwhd_disc_crypto xxhash ZLIB::ZLIB Threads::Threads)
add_test(NAME bluewake_wwhd_disc_import_test COMMAND "${BLUEWAKE_WWHD_TEST_PYTHON}"
    -B "${BLUEWAKE_REPO_ROOT}/tests/wwhd_disc_import_test.py" "$<TARGET_FILE:bluewake_wwhd_native_import>")
set_tests_properties(bluewake_wwhd_disc_import_test PROPERTIES TIMEOUT 60)

# Output ownership/recovery uses real process death, never private inputs.
add_executable(bluewake_wwhd_output_staging_test
    ${BLUEWAKE_REPO_ROOT}/tests/wwhd_output_staging_test.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/output_staging.cpp)
target_compile_features(bluewake_wwhd_output_staging_test PRIVATE cxx_std_17)
target_include_directories(bluewake_wwhd_output_staging_test PRIVATE ${BLUEWAKE_HOST_SRC})
target_link_libraries(bluewake_wwhd_output_staging_test PRIVATE Threads::Threads)
add_test(NAME bluewake_wwhd_output_staging_test COMMAND bluewake_wwhd_output_staging_test
    "${CMAKE_CURRENT_BINARY_DIR}/wwhd-output-staging-test")
set_tests_properties(bluewake_wwhd_output_staging_test PROPERTIES TIMEOUT 15)
add_test(NAME bluewake_wwhd_staging_recovery_test COMMAND "${BLUEWAKE_WWHD_TEST_PYTHON}"
    -B "${BLUEWAKE_REPO_ROOT}/tests/wwhd_staging_recovery_test.py" "$<TARGET_FILE:bluewake_wwhd_native_import>")
set_tests_properties(bluewake_wwhd_staging_recovery_test PROPERTIES TIMEOUT 90)

# Match the app's UTF-8 path/argv encoding on Windows. CMake embeds manifest
# sources with both clang-cl and the clang GNU driver used by the app builder.
if(WIN32)
    foreach(_wwhd_target IN ITEMS bluewake_wwhd_disc_image_test bluewake_wwhd_disc_crypto_test
            bluewake_wwhd_wiiu_disc_test bluewake_wwhd_native_import bluewake_wwhd_import_job_test
            bluewake_wwhd_cancellation_test bluewake_wwhd_output_staging_test)
        if(TARGET ${_wwhd_target})
            target_sources(${_wwhd_target} PRIVATE "${BLUEWAKE_REPO_ROOT}/windows/resources/BlueWake.manifest")
        endif()
    endforeach()
endif()
