cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(bluewake_native_work_pool_tests LANGUAGES C)
    include(CTest)
endif()

get_filename_component(BLUEWAKE_POOL_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# Compile the production pool directly: this target deliberately has no
# GXRuntime, renderer, translated-module, or player-owned-data dependency.
add_executable(bluewake_native_work_pool_test
    ${BLUEWAKE_POOL_REPO_ROOT}/tests/native_work_pool_test.c
    ${BLUEWAKE_POOL_REPO_ROOT}/cmake/composite/native_work_pool.c)
target_include_directories(bluewake_native_work_pool_test PRIVATE
    ${BLUEWAKE_POOL_REPO_ROOT}/cmake/composite)
target_compile_definitions(bluewake_native_work_pool_test PRIVATE
    BLUEWAKE_POOL_TESTING=1)

if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(bluewake_native_work_pool_test PRIVATE Threads::Threads m)
endif()

# Worker selection is initialized once per process, so every environment
# configuration must be a separate CTest invocation of the same binary.
add_test(NAME bluewake_native_work_pool_serial_test
    COMMAND bluewake_native_work_pool_test 0)
set_tests_properties(bluewake_native_work_pool_serial_test PROPERTIES
    ENVIRONMENT "BLUEWAKE_NATIVE_WORKERS=invalid"
    LABELS "native-work-pool;fast"
    TIMEOUT 15)

if(NOT WIN32)
    # Cover both serial parser outcomes, representative worker counts, and the
    # production maximum-worker clamp. Windows intentionally remains serial.
    add_test(NAME bluewake_native_work_pool_zero_test
        COMMAND bluewake_native_work_pool_test 0)
    set_tests_properties(bluewake_native_work_pool_zero_test PROPERTIES
        ENVIRONMENT "BLUEWAKE_NATIVE_WORKERS=0"
        LABELS "native-work-pool;fast"
        TIMEOUT 15)

    foreach(BLUEWAKE_POOL_WORKERS IN ITEMS 1 2 3)
        add_test(NAME bluewake_native_work_pool_workers_${BLUEWAKE_POOL_WORKERS}_test
            COMMAND bluewake_native_work_pool_test ${BLUEWAKE_POOL_WORKERS})
        set_tests_properties(bluewake_native_work_pool_workers_${BLUEWAKE_POOL_WORKERS}_test PROPERTIES
            ENVIRONMENT "BLUEWAKE_NATIVE_WORKERS=${BLUEWAKE_POOL_WORKERS}"
            LABELS "native-work-pool;fast"
            TIMEOUT 15)
    endforeach()

    add_test(NAME bluewake_native_work_pool_clamped_test
        COMMAND bluewake_native_work_pool_test 8)
    set_tests_properties(bluewake_native_work_pool_clamped_test PROPERTIES
        ENVIRONMENT "BLUEWAKE_NATIVE_WORKERS=99"
        LABELS "native-work-pool;fast"
        TIMEOUT 15)
    add_test(NAME bluewake_native_work_pool_creation_failure_test
        COMMAND bluewake_native_work_pool_test 0)
    set_tests_properties(bluewake_native_work_pool_creation_failure_test PROPERTIES
        ENVIRONMENT "BLUEWAKE_NATIVE_WORKERS=4;BLUEWAKE_NATIVE_WORKER_FAIL_AFTER=0"
        LABELS "native-work-pool;fast"
        TIMEOUT 15)

    add_test(NAME bluewake_native_work_pool_partial_creation_test
        COMMAND bluewake_native_work_pool_test 2)
    set_tests_properties(bluewake_native_work_pool_partial_creation_test PROPERTIES
        ENVIRONMENT "BLUEWAKE_NATIVE_WORKERS=4;BLUEWAKE_NATIVE_WORKER_FAIL_AFTER=2"
        LABELS "native-work-pool;fast"
        TIMEOUT 15)

    # Load and unload the production pool as a DSO repeatedly. Its destructor
    # must join every worker before dlclose unmaps their code.
    add_library(bluewake_native_work_pool_fixture SHARED
        ${BLUEWAKE_POOL_REPO_ROOT}/tests/native_work_pool_fixture.c
        ${BLUEWAKE_POOL_REPO_ROOT}/cmake/composite/native_work_pool.c)
    target_include_directories(bluewake_native_work_pool_fixture PRIVATE
        ${BLUEWAKE_POOL_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_native_work_pool_fixture PRIVATE Threads::Threads m)

    add_executable(bluewake_native_work_pool_lifecycle_test
        ${BLUEWAKE_POOL_REPO_ROOT}/tests/native_work_pool_lifecycle_test.c)
    target_link_libraries(bluewake_native_work_pool_lifecycle_test PRIVATE ${CMAKE_DL_LIBS})
    # A targeted CI build must also produce the DSO loaded by this executable;
    # mentioning TARGET_FILE in add_test does not create a build dependency.
    add_dependencies(bluewake_native_work_pool_lifecycle_test bluewake_native_work_pool_fixture)
    add_test(NAME bluewake_native_work_pool_lifecycle_test
        COMMAND bluewake_native_work_pool_lifecycle_test
            $<TARGET_FILE:bluewake_native_work_pool_fixture>)
    set_tests_properties(bluewake_native_work_pool_lifecycle_test PROPERTIES
        ENVIRONMENT "BLUEWAKE_NATIVE_WORKERS=2"
        LABELS "native-work-pool;lifecycle"
        TIMEOUT 30)
endif()
