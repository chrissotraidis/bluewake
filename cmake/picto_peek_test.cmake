# Manual GPU oracle: run on real hardware with a fresh, isolated cache.
add_executable(bluewake_gxcore_picto_peek_pixels_test
    ${BLUEWAKE_REPO_ROOT}/tests/gxcore_picto_peek_pixels_test.cpp)
target_compile_features(bluewake_gxcore_picto_peek_pixels_test PRIVATE cxx_std_20)
target_include_directories(bluewake_gxcore_picto_peek_pixels_test PRIVATE
    ${GXRUNTIME_DIR}/graphics/aurora/lib)
target_link_libraries(bluewake_gxcore_picto_peek_pixels_test PRIVATE
    GXRuntime::aurora GXRuntime::gxcore)
if(WIN32)
    include(${GXRUNTIME_DIR}/graphics/aurora/cmake/AuroraCopyRuntimeDLLs.cmake)
    aurora_copy_runtime_dlls(bluewake_gxcore_picto_peek_pixels_test)
endif()
