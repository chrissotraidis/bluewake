# Shared, game-data-free worker lifecycle check on every desktop builder.
find_package(Threads REQUIRED)
add_executable(bluewake_wwhd_import_job_test
    ${BLUEWAKE_REPO_ROOT}/tests/wwhd_import_job_test.cpp
    ${BLUEWAKE_HOST_SRC}/wwhd/import_job.cpp)
target_compile_features(bluewake_wwhd_import_job_test PRIVATE cxx_std_17)
target_include_directories(bluewake_wwhd_import_job_test PRIVATE ${BLUEWAKE_HOST_SRC})
target_link_libraries(bluewake_wwhd_import_job_test PRIVATE Threads::Threads)
add_test(NAME bluewake_wwhd_import_job_test COMMAND bluewake_wwhd_import_job_test)
set_tests_properties(bluewake_wwhd_import_job_test PROPERTIES TIMEOUT 15)
