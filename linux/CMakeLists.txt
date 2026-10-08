# BlueWake for Linux (x86-64): the unchanged host, GXRuntime/Aurora (Dawn on
# Vulkan or OpenGL, SDL3) and the donor DSP, as windows/CMakeLists.txt builds
# them for Windows. The only Linux-specific sources are the entry shim, which
# fills in default paths beside the executable, the disc chooser (SDL3's
# native open-file dialog) and the crash reporter.
#
# Built by scripts/linux/build.py with gcc (the generated game-module chunks
# compile far faster under gcc than clang); see docs/LINUX.md.
cmake_minimum_required(VERSION 3.25)
project(bluewake_linux LANGUAGES C CXX)

if(NOT UNIX OR APPLE OR WIN32)
    message(FATAL_ERROR "linux/ builds BlueWake for Linux only")
endif()

get_filename_component(BLUEWAKE_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(BLUEWAKE_RECOMPCORE ${BLUEWAKE_REPO_ROOT}/ref/recompcore CACHE PATH "The RecompCore checkout to build with")
set(GXRUNTIME_DIR ${BLUEWAKE_RECOMPCORE}/GXRuntime)
set(BLUEWAKE_HOST_SRC ${BLUEWAKE_REPO_ROOT}/runtime/host/src)

option(BLUEWAKE_LINUX_DEVELOPER_TRACING "Compile BLUEWAKE_TRACE_* probes into the host" OFF)
option(BLUEWAKE_LINUX_REGRESSION_TESTS "Build source-only Linux host regressions" OFF)

add_compile_options("-ffile-prefix-map=${BLUEWAKE_REPO_ROOT}/=")

add_subdirectory(${GXRUNTIME_DIR} gxruntime_build EXCLUDE_FROM_ALL)

set(BLUEWAKE_HOST_SOURCES
    ${BLUEWAKE_HOST_SRC}/main.c
    ${BLUEWAKE_HOST_SRC}/aram_dma.c
    ${BLUEWAKE_HOST_SRC}/audio_capture.c
    ${BLUEWAKE_HOST_SRC}/audio_watch.c
    ${BLUEWAKE_HOST_SRC}/ipl_sram.c
    ${BLUEWAKE_HOST_SRC}/card_runtime.c
    ${BLUEWAKE_HOST_SRC}/callback_delivery.c
    ${BLUEWAKE_HOST_SRC}/cycle_domain.c
    ${BLUEWAKE_HOST_SRC}/delivery_digest.c
    ${BLUEWAKE_HOST_SRC}/edge_intercepts.c
    ${BLUEWAKE_HOST_SRC}/return_census.c
    ${BLUEWAKE_HOST_SRC}/interrupt_sources.c
    ${BLUEWAKE_HOST_SRC}/cold_fallback.c
    ${BLUEWAKE_HOST_SRC}/external_memory.c
    ${BLUEWAKE_HOST_SRC}/fpu_context.c
    ${BLUEWAKE_HOST_SRC}/pad_event_schedule.c
    ${BLUEWAKE_HOST_SRC}/pad_waypoint.c
    ${BLUEWAKE_HOST_SRC}/pad_wire.c
    ${BLUEWAKE_HOST_SRC}/rel_scratch_allocator.c
    ${BLUEWAKE_HOST_SRC}/scheduler_contract.c
    ${BLUEWAKE_HOST_SRC}/fast_load.c
    ${BLUEWAKE_HOST_SRC}/jump_button.c
    ${BLUEWAKE_HOST_SRC}/climb.c
    ${BLUEWAKE_HOST_SRC}/save_state.c
    ${BLUEWAKE_HOST_SRC}/sprint.c
    ${BLUEWAKE_HOST_SRC}/quick_doors.c
    ${BLUEWAKE_HOST_SRC}/haptics.c
    ${BLUEWAKE_HOST_SRC}/draw_tags.c
    ${BLUEWAKE_HOST_SRC}/forest_water.c
    ${BLUEWAKE_HOST_SRC}/fps_watch.c
    ${BLUEWAKE_HOST_SRC}/game_options.c
    ${BLUEWAKE_HOST_SRC}/settings_menu.cpp
    ${BLUEWAKE_HOST_SRC}/mouse_camera.c)

add_executable(bluewake
    src/linux_entry.c
    src/linux_disc.c
    ${BLUEWAKE_REPO_ROOT}/apple/ios/src/disc_import.c
    src/linux_crash.cpp
    ${BLUEWAKE_HOST_SOURCES})
# The unchanged host keeps its own entry point under another name; the shim
# owns main() and hands over after setting the defaults.
set_source_files_properties(${BLUEWAKE_HOST_SRC}/main.c PROPERTIES
    COMPILE_DEFINITIONS "main=bluewake_host_main")
target_link_libraries(bluewake PRIVATE gxruntime GXRuntime::aurora xxhash ${CMAKE_DL_LIBS} pthread)
if(TARGET imgui)
    target_link_libraries(bluewake PRIVATE imgui)
endif()
set_target_properties(bluewake PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME bluewake)
target_include_directories(bluewake PRIVATE
    ${BLUEWAKE_HOST_SRC}
    ${BLUEWAKE_REPO_ROOT}/windows/src
    ${BLUEWAKE_REPO_ROOT}/apple/ios/src
    ${GXRUNTIME_DIR}/include
    ${BLUEWAKE_RECOMPCORE}/Source/Core/Core/PowerPC/StaticRecomp)
target_compile_definitions(bluewake PRIVATE
    DOLRECOMP_CPU_HEADER="core/cpu.h"
    BLUEWAKE_ENABLE_DEVELOPER_TRACING=$<BOOL:${BLUEWAKE_LINUX_DEVELOPER_TRACING}>
    BLUEWAKE_EDGE_CENSUS=0
    BLUEWAKE_LINUX=1)

# The donor DSP (Dolphin's DSP interpreter and its high-level Zelda ucode, from
# RecompCore) compiled from source, with only the Common services it needs
# (apple/ios/src/dsp_common_shim.cpp), as on iOS and Windows.
set(_donor ${BLUEWAKE_RECOMPCORE})
set(_dsp ${_donor}/Source/Core/Core/DSP)
add_library(bluewake_dsp_donor STATIC
    ${_dsp}/DSPAccelerator.cpp ${_dsp}/DSPAnalyzer.cpp ${_dsp}/DSPAssembler.cpp
    ${_dsp}/DSPCaptureLogger.cpp ${_dsp}/DSPCodeUtil.cpp ${_dsp}/DSPCore.cpp
    ${_dsp}/DSPDisassembler.cpp ${_dsp}/DSPHWInterface.cpp ${_dsp}/DSPMemoryMap.cpp
    ${_dsp}/DSPStacks.cpp ${_dsp}/DSPTables.cpp ${_dsp}/LabelMap.cpp
    ${_dsp}/Interpreter/DSPIntArithmetic.cpp ${_dsp}/Interpreter/DSPIntBranch.cpp
    ${_dsp}/Interpreter/DSPIntExtOps.cpp ${_dsp}/Interpreter/DSPIntLoadStore.cpp
    ${_dsp}/Interpreter/DSPIntMisc.cpp ${_dsp}/Interpreter/DSPIntMultiplier.cpp
    ${_dsp}/Interpreter/DSPIntTables.cpp ${_dsp}/Interpreter/DSPInterpreter.cpp
    ${_dsp}/Jit/DSPEmitterBase.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/DSPHLE.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/MailHandler.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/AESnd.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/ASnd.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/AX.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/AXWii.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/CARD.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/GBA.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/INIT.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/ROM.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/UCodes.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/Zelda.cpp
    ${_donor}/Source/Core/Core/HW/DSPHLE/UCodes/ZeldaUCodesTable.cpp
    ${BLUEWAKE_REPO_ROOT}/apple/ios/src/dsp_common_shim.cpp)
target_include_directories(bluewake_dsp_donor PUBLIC
    ${_donor}/Source/Core ${_donor}/Source ${_donor}/Externals
    ${_donor}/Externals/xxhash/xxHash
    ${BLUEWAKE_REPO_ROOT}/apple/ios/dsp_generated)
# No _M_X86_64: that selects Dolphin's x64 DSP JIT, and BlueWake runs the DSP
# interpreter everywhere (DSPEmitterNull), as on iOS and Windows.
target_compile_definitions(bluewake_dsp_donor PUBLIC
    _ARCH_64=1 FMT_EXCEPTIONS=0
    __STDC_CONSTANT_MACROS __STDC_LIMIT_MACROS)
target_compile_features(bluewake_dsp_donor PUBLIC cxx_std_23)
target_compile_options(bluewake_dsp_donor PRIVATE -fno-strict-aliasing -w)
target_link_libraries(bluewake_dsp_donor PUBLIC fmt::fmt xxhash ZLIB::ZLIB)

add_library(bluewake_dsp_adapter STATIC
    ${BLUEWAKE_HOST_SRC}/dsp_adapter.cpp
    ${BLUEWAKE_HOST_SRC}/dsp_adapter_c.cpp
    ${BLUEWAKE_HOST_SRC}/dsp_adapter_donor_stubs.cpp
    ${BLUEWAKE_HOST_SRC}/dsp_hle_backend.cpp)
target_link_libraries(bluewake_dsp_adapter PUBLIC bluewake_dsp_donor)
target_compile_definitions(bluewake PRIVATE BLUEWAKE_HAS_DSP_ADAPTER=1)
target_link_libraries(bluewake PRIVATE bluewake_dsp_adapter)

# Tools the builder runs on the player's disc.
add_executable(bluewake_disc_extract
    ${BLUEWAKE_REPO_ROOT}/scripts/ios/disc_extract.c
    ${BLUEWAKE_REPO_ROOT}/apple/ios/src/disc_import.c)
target_include_directories(bluewake_disc_extract PRIVATE ${BLUEWAKE_REPO_ROOT}/apple/ios/src)

if(BLUEWAKE_LINUX_REGRESSION_TESTS)
    enable_testing()
    add_executable(bluewake_fast_blocks_test EXCLUDE_FROM_ALL ${BLUEWAKE_REPO_ROOT}/tests/fast_blocks_test.c)
    target_include_directories(bluewake_fast_blocks_test PRIVATE
        ${GXRUNTIME_DIR}/include
        ${BLUEWAKE_RECOMPCORE}/Source/Core/Core/PowerPC/StaticRecomp)
    target_link_libraries(bluewake_fast_blocks_test PRIVATE ${CMAKE_DL_LIBS})

    add_executable(bluewake_cycle_domain_test
        ${BLUEWAKE_REPO_ROOT}/tests/cycle_domain_test.c ${BLUEWAKE_HOST_SRC}/cycle_domain.c)
    add_executable(bluewake_interrupt_sources_test
        ${BLUEWAKE_REPO_ROOT}/tests/interrupt_sources_test.c
        ${BLUEWAKE_HOST_SRC}/cycle_domain.c ${BLUEWAKE_HOST_SRC}/interrupt_sources.c)
    add_executable(bluewake_delivery_digest_test
        ${BLUEWAKE_REPO_ROOT}/tests/delivery_digest_test.c ${BLUEWAKE_HOST_SRC}/delivery_digest.c)
    add_executable(bluewake_composite_dispatch_loop_test
        ${BLUEWAKE_REPO_ROOT}/tests/composite_dispatch_loop_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/dispatch_loop.c)
    add_executable(bluewake_edge_intercepts_test
        ${BLUEWAKE_REPO_ROOT}/tests/edge_intercepts_test.c
        ${BLUEWAKE_HOST_SRC}/edge_intercepts.c ${BLUEWAKE_HOST_SRC}/game_options.c ${BLUEWAKE_HOST_SRC}/card_runtime.c)
    foreach(TEST_NAME cycle_domain interrupt_sources delivery_digest composite_dispatch_loop edge_intercepts)
        set(TEST_TARGET bluewake_${TEST_NAME}_test)
        target_include_directories(${TEST_TARGET} PRIVATE
            ${BLUEWAKE_HOST_SRC} ${BLUEWAKE_REPO_ROOT}/cmake/composite)
        target_link_libraries(${TEST_TARGET} PRIVATE gxruntime)
        set_target_properties(${TEST_TARGET} PROPERTIES LINKER_LANGUAGE CXX)
        add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
    endforeach()

    add_executable(bluewake_linux_disc_test ${BLUEWAKE_REPO_ROOT}/tests/linux_disc_test.c)
    target_include_directories(bluewake_linux_disc_test PRIVATE ${BLUEWAKE_HOST_SRC} ${BLUEWAKE_REPO_ROOT}/apple/ios/src)
    target_link_libraries(bluewake_linux_disc_test PRIVATE gxruntime GXRuntime::aurora xxhash)
    set_target_properties(bluewake_linux_disc_test PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME bluewake_linux_disc_test COMMAND bluewake_linux_disc_test)
    set_tests_properties(bluewake_linux_disc_test PROPERTIES TIMEOUT 30)

    add_executable(bluewake_crash_text_test ${BLUEWAKE_REPO_ROOT}/tests/crash_text_test.c)
    add_test(NAME bluewake_crash_text_test COMMAND bluewake_crash_text_test)

    add_executable(bluewake_launch_marker_test ${BLUEWAKE_REPO_ROOT}/tests/launch_marker_test.c)
    target_include_directories(bluewake_launch_marker_test PRIVATE ${BLUEWAKE_HOST_SRC})
    add_test(NAME bluewake_launch_marker_test COMMAND bluewake_launch_marker_test)

    add_executable(bluewake_settings_state_test ${BLUEWAKE_REPO_ROOT}/tests/settings_state_test.cpp)
    target_compile_features(bluewake_settings_state_test PRIVATE cxx_std_17)
    add_test(NAME bluewake_settings_state_test COMMAND bluewake_settings_state_test)

    add_executable(bluewake_save_state_test ${BLUEWAKE_REPO_ROOT}/tests/save_state_test.c ${BLUEWAKE_HOST_SRC}/save_state.c)
    target_include_directories(bluewake_save_state_test PRIVATE ${BLUEWAKE_HOST_SRC})
    target_link_libraries(bluewake_save_state_test PRIVATE ZLIB::ZLIB)
    add_test(NAME bluewake_save_state_test COMMAND bluewake_save_state_test)

    add_executable(bluewake_restart_request_test ${BLUEWAKE_REPO_ROOT}/tests/restart_request_test.cpp)
    add_test(NAME bluewake_restart_request_test COMMAND bluewake_restart_request_test)

    # Shared host tests that reach Linux just by building from main, the same
    # way they reach Windows: the deferred-DVD-completion queue and the cache
    # flush fallback. dvd_completion needs ZLIB for save_state.c's checksum.
    include("${BLUEWAKE_REPO_ROOT}/cmake/dvd_completion_test.cmake")
    target_include_directories(bluewake_dvd_completion_test PRIVATE ${BLUEWAKE_HOST_SRC} ${GXRUNTIME_DIR}/include)
    target_link_libraries(bluewake_dvd_completion_test PRIVATE gxruntime ZLIB::ZLIB)
    set_target_properties(bluewake_dvd_completion_test PROPERTIES LINKER_LANGUAGE CXX)
    include("${BLUEWAKE_REPO_ROOT}/cmake/cache_flush_fallback_test.cmake")
    target_link_libraries(bluewake_cache_flush_fallback_test PRIVATE gxruntime)
    set_target_properties(bluewake_cache_flush_fallback_test PROPERTIES LINKER_LANGUAGE CXX)
endif()

# Aurora's bundled pipeline cache (the same seed the Windows build ships).
set(BLUEWAKE_PIPELINE_SEED "${BLUEWAKE_REPO_ROOT}/windows/resources/initial_pipeline_cache.db")
add_custom_command(TARGET bluewake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${BLUEWAKE_PIPELINE_SEED}" $<TARGET_FILE_DIR:bluewake>
    VERBATIM)
