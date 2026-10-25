include_guard(GLOBAL)

function(bluewake_add_native_math_tests)
    # Keep the private differential oracle identical across host entry points.
    # It is available to developers with an extracted module, but excluded from
    # ordinary builds and never registered with CTest.
    add_executable(bluewake_native_math_test EXCLUDE_FROM_ALL
        ${BLUEWAKE_REPO_ROOT}/tests/native_math_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_math.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_work_pool.c)
    target_include_directories(bluewake_native_math_test PRIVATE
        ${BLUEWAKE_REPO_ROOT}/cmake/composite
        ${BLUEWAKE_REPO_ROOT}/ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp)
    target_link_libraries(bluewake_native_math_test PRIVATE gxruntime ${CMAKE_DL_LIBS})
    target_compile_options(bluewake_native_math_test PRIVATE -ffp-contract=off)

    # The synthetic guard test has no player-owned inputs. Register it normally
    # so public builds exercise native routing and the serial implementation.
    add_executable(bluewake_native_math_guard_test
        ${BLUEWAKE_REPO_ROOT}/tests/native_math_guard_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_math.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_work_pool.c)
    target_include_directories(bluewake_native_math_guard_test PRIVATE
        ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_native_math_guard_test PRIVATE gxruntime)
    target_compile_options(bluewake_native_math_guard_test PRIVATE -ffp-contract=off)
    add_test(NAME bluewake_native_math_guard_test COMMAND bluewake_native_math_guard_test)

    if(NOT WIN32)
        # POSIX builds compile the real worker backend. Reuse the guard binary
        # in a fresh process so its one-time worker configuration sees W=2.
        find_package(Threads REQUIRED)
        target_link_libraries(bluewake_native_math_test PRIVATE Threads::Threads)
        target_link_libraries(bluewake_native_math_guard_test PRIVATE Threads::Threads)
        add_test(NAME bluewake_native_math_workers_test COMMAND bluewake_native_math_guard_test)
        set_tests_properties(bluewake_native_math_workers_test PROPERTIES
            ENVIRONMENT "BLUEWAKE_NATIVE_WORKERS=2")
    endif()
endfunction()
