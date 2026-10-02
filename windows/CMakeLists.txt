# BlueWake for Windows (x86-64): the unchanged host, GXRuntime/Aurora (Dawn on
# Direct3D 12 or Vulkan, SDL3) and the donor DSP, as apple/ios/CMakeLists.txt
# builds them for iOS. The only Windows-specific sources are the entry shim,
# which fills in default paths beside the executable, and the POSIX
# compatibility layer in compat/.
#
# Built by scripts/windows/build.py with clang (GNU driver, MSVC ABI); see
# docs/WINDOWS.md.
cmake_minimum_required(VERSION 3.25)
project(bluewake_windows LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "windows/ builds BlueWake for Windows only")
endif()

get_filename_component(BLUEWAKE_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(GXRUNTIME_DIR ${BLUEWAKE_REPO_ROOT}/ref/recompcore/GXRuntime)
set(BLUEWAKE_HOST_SRC ${BLUEWAKE_REPO_ROOT}/runtime/host/src)
set(BLUEWAKE_COMPAT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/compat)

option(BLUEWAKE_WINDOWS_CONSOLE "Build a console-subsystem executable (logs on the terminal)" OFF)
option(BLUEWAKE_WINDOWS_DEVELOPER_TRACING "Compile BLUEWAKE_TRACE_* probes into the host" OFF)
option(BLUEWAKE_WINDOWS_REGRESSION_TESTS "Build source-only Windows host regressions" OFF)

# Source paths compiled into the app (assert and log messages) are written
# relative to the checkout, so a build does not carry the builder's home folder.
add_compile_options("-ffile-prefix-map=${BLUEWAKE_REPO_ROOT}/=")
add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE _USE_MATH_DEFINES _WIN32_WINNT=0x0A00)

add_subdirectory(${GXRUNTIME_DIR} gxruntime_build EXCLUDE_FROM_ALL)

# POSIX calls the host makes (threads, clocks, dlopen, dirent): force-included
# into BlueWake's own sources only, never into third-party code.
add_library(bluewake_posix_compat STATIC ${BLUEWAKE_COMPAT_DIR}/bw_posix_compat.c)
target_include_directories(bluewake_posix_compat PRIVATE ${BLUEWAKE_COMPAT_DIR})
set(BLUEWAKE_COMPAT_INCLUDES ${BLUEWAKE_COMPAT_DIR}/include ${BLUEWAKE_COMPAT_DIR})
set(BLUEWAKE_COMPAT_FORCE_INCLUDE "SHELL:-include ${BLUEWAKE_COMPAT_DIR}/bw_posix_compat.h")
# GXRuntime's own C runtime (the disc reader uses fseeko and off_t) needs the
# same few declarations, and remembers its getenv answers (the Windows C
# runtime's getenv is slow on hot paths); its C++ graphics code builds
# unchanged.
target_compile_options(gxruntime PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE}
    "SHELL:-include ${BLUEWAKE_COMPAT_DIR}/bw_getenv_cache.h")

set(BLUEWAKE_HOST_SOURCES
    ${BLUEWAKE_HOST_SRC}/main.c
    ${BLUEWAKE_HOST_SRC}/aram_dma.c
    ${BLUEWAKE_HOST_SRC}/audio_capture.c
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
    ${BLUEWAKE_HOST_SRC}/draw_tags.c
    ${BLUEWAKE_HOST_SRC}/fps_watch.c
    ${BLUEWAKE_HOST_SRC}/game_options.c
    ${BLUEWAKE_HOST_SRC}/mouse_camera.c)

# The manifest (DPI awareness, long paths, UTF-8 code page) is merged into the
# one the linker embeds.
add_executable(BlueWake
    src/win_entry.c
    src/win_disc.c
    ${BLUEWAKE_REPO_ROOT}/apple/ios/src/disc_import.c
    src/win_settings.cpp
    src/win_crash.cpp
    src/bluewake.rc
    resources/BlueWake.manifest
    ${BLUEWAKE_HOST_SOURCES})
# The unchanged host keeps its own entry point under another name; the shim
# owns main() and hands over after setting the defaults.
set_source_files_properties(${BLUEWAKE_HOST_SRC}/main.c PROPERTIES
    COMPILE_DEFINITIONS "main=bluewake_host_main")
target_link_libraries(BlueWake PRIVATE gxruntime GXRuntime::aurora xxhash bluewake_posix_compat winmm bcrypt comctl32 comdlg32)
if(TARGET imgui)
    target_link_libraries(BlueWake PRIVATE imgui)
endif()
set_target_properties(BlueWake PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME BlueWake)
target_include_directories(BlueWake PRIVATE
    ${BLUEWAKE_COMPAT_INCLUDES}
    ${BLUEWAKE_HOST_SRC}
    ${BLUEWAKE_REPO_ROOT}/apple/ios/src
    ${GXRUNTIME_DIR}/include
    ${BLUEWAKE_REPO_ROOT}/ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp)
target_compile_options(BlueWake PRIVATE
    $<$<COMPILE_LANGUAGE:C,CXX>:${BLUEWAKE_COMPAT_FORCE_INCLUDE}>)
target_compile_definitions(BlueWake PRIVATE
    DOLRECOMP_CPU_HEADER="core/cpu.h"
    BLUEWAKE_ENABLE_DEVELOPER_TRACING=$<BOOL:${BLUEWAKE_WINDOWS_DEVELOPER_TRACING}>
    BLUEWAKE_EDGE_CENSUS=0
    BLUEWAKE_WINDOWS=1)
# The game's translated code recurses on the host stack as the guest does on
# the GameCube's; macOS gives the main thread 8 MB, Windows only 1 MB.
target_link_options(BlueWake PRIVATE "LINKER:/STACK:67108864")
if(BLUEWAKE_WINDOWS_CONSOLE)
    target_compile_definitions(BlueWake PRIVATE BLUEWAKE_WINDOWS_CONSOLE=1)
else()
    # A windowed game: no console of its own. It still prints to the terminal
    # it was started from, and always to the session log (src/win_entry.c).
    target_link_options(BlueWake PRIVATE "LINKER:/SUBSYSTEM:WINDOWS" "LINKER:/ENTRY:mainCRTStartup")
endif()

# The donor DSP (Dolphin's DSP interpreter and its high-level Zelda ucode, from
# RecompCore) compiled from source, with only the Common services it needs
# (apple/ios/src/dsp_common_shim.cpp), as on iOS.
set(_donor ${BLUEWAKE_REPO_ROOT}/ref/recompcore)
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
# interpreter everywhere (DSPEmitterNull), as on iOS.
target_compile_definitions(bluewake_dsp_donor PUBLIC
    _ARCH_64=1 FMT_EXCEPTIONS=0
    __STDC_CONSTANT_MACROS __STDC_LIMIT_MACROS
    NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
target_compile_features(bluewake_dsp_donor PUBLIC cxx_std_23)
target_compile_options(bluewake_dsp_donor PRIVATE -fno-strict-aliasing -w)
# Dolphin builds with a conforming preprocessor (MSVC /Zc:preprocessor); its
# assert macros forward __VA_ARGS__ through __VA_OPT__, which clang's MSVC
# compatibility mode expands the old MSVC way, miscounting format arguments.
target_compile_options(bluewake_dsp_donor PRIVATE -fno-ms-compatibility)
# Under the Microsoft ABI a member pointer's size depends on whether its class
# is complete where the pointer type is first used; a forward-declared one gave
# the DSP interpreter's op table two layouts and Exact audio a call to address 0
# (patches/recompcore/0114). Refuse such pointers in the donor DSP.
target_compile_options(bluewake_dsp_donor PRIVATE -fcomplete-member-pointers)
target_link_libraries(bluewake_dsp_donor PUBLIC fmt::fmt xxhash ZLIB::ZLIB)

add_library(bluewake_dsp_adapter STATIC
    ${BLUEWAKE_HOST_SRC}/dsp_adapter.cpp
    ${BLUEWAKE_HOST_SRC}/dsp_adapter_c.cpp
    ${BLUEWAKE_HOST_SRC}/dsp_adapter_donor_stubs.cpp
    ${BLUEWAKE_HOST_SRC}/dsp_hle_backend.cpp)
target_include_directories(bluewake_dsp_adapter PRIVATE ${BLUEWAKE_COMPAT_INCLUDES})
target_compile_options(bluewake_dsp_adapter PRIVATE -fno-ms-compatibility)
target_link_libraries(bluewake_dsp_adapter PUBLIC bluewake_dsp_donor bluewake_posix_compat)
target_compile_definitions(BlueWake PRIVATE BLUEWAKE_HAS_DSP_ADAPTER=1)
target_link_libraries(BlueWake PRIVATE bluewake_dsp_adapter)

# Tools the builder runs on the player's disc.
add_executable(bluewake_disc_extract
    ${BLUEWAKE_REPO_ROOT}/scripts/ios/disc_extract.c
    ${BLUEWAKE_REPO_ROOT}/apple/ios/src/disc_import.c)
target_include_directories(bluewake_disc_extract PRIVATE
    ${BLUEWAKE_COMPAT_INCLUDES} ${BLUEWAKE_REPO_ROOT}/apple/ios/src)
target_compile_options(bluewake_disc_extract PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
target_link_libraries(bluewake_disc_extract PRIVATE bluewake_posix_compat bcrypt)

if(BLUEWAKE_WINDOWS_REGRESSION_TESTS)
    # Run manually against two private own-disc modules; no game code in CI.
    add_executable(bluewake_fast_blocks_test EXCLUDE_FROM_ALL ${BLUEWAKE_REPO_ROOT}/tests/fast_blocks_test.c)
    target_include_directories(bluewake_fast_blocks_test PRIVATE
        ${GXRUNTIME_DIR}/include
        ${BLUEWAKE_REPO_ROOT}/ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp)
    target_compile_definitions(bluewake_fast_blocks_test PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Assertion-based scheduler checks also run in Release configurations.
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
            ${BLUEWAKE_HOST_SRC} ${BLUEWAKE_REPO_ROOT}/cmake/composite
            ${BLUEWAKE_COMPAT_INCLUDES})
        target_compile_options(${TEST_TARGET} PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
        target_link_libraries(${TEST_TARGET} PRIVATE gxruntime bluewake_posix_compat)
        set_target_properties(${TEST_TARGET} PROPERTIES LINKER_LANGUAGE CXX)
        add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
    endforeach()

    # Source-only contracts for helpers awaiting whole-module qualification.
    foreach(_variant plain global)
        add_executable(bluewake_gather_memory_${_variant}_test
            ${BLUEWAKE_REPO_ROOT}/tests/gather_memory_test.c
            ${BLUEWAKE_REPO_ROOT}/cmake/composite/gather_pipe.c)
        target_include_directories(bluewake_gather_memory_${_variant}_test PRIVATE
            ${BLUEWAKE_REPO_ROOT}/cmake/composite ${GXRUNTIME_DIR}/tests)
        target_link_libraries(bluewake_gather_memory_${_variant}_test PRIVATE gxruntime)
        if(_variant STREQUAL "global")
            target_compile_definitions(bluewake_gather_memory_${_variant}_test PRIVATE
                BW_GUEST_MEM1=bw_test_mem1 BW_GUEST_MEM1_SIZE=0x02000000u)
        endif()
        add_test(NAME bluewake_gather_memory_${_variant}_test COMMAND bluewake_gather_memory_${_variant}_test)
    endforeach()
    add_executable(bluewake_inline_gpr_memory_test ${BLUEWAKE_REPO_ROOT}/tests/inline_gpr_memory_test.c)
    target_include_directories(bluewake_inline_gpr_memory_test PRIVATE
        ${GXRUNTIME_DIR}/include ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_inline_gpr_memory_test PRIVATE gxruntime)
    add_test(NAME bluewake_inline_gpr_memory_test COMMAND bluewake_inline_gpr_memory_test)

    add_executable(bluewake_direct_calls_test
        ${BLUEWAKE_REPO_ROOT}/tests/direct_calls_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/direct_calls.c)
    target_include_directories(bluewake_direct_calls_test PRIVATE
        ${GXRUNTIME_DIR}/include ${BLUEWAKE_REPO_ROOT}/cmake/composite ${BLUEWAKE_REPO_ROOT}/tests)
    target_compile_definitions(bluewake_direct_calls_test PRIVATE
        BLUEWAKE_EDGE_FILTER=1 BW_EDGE_WATCH_INCLUDE="direct_call_watch.inc")
    add_test(NAME bluewake_direct_calls_test COMMAND bluewake_direct_calls_test)

    add_executable(bluewake_guest_checkpoint_test ${BLUEWAKE_REPO_ROOT}/tests/guest_checkpoint_test.c)
    target_include_directories(bluewake_guest_checkpoint_test PRIVATE
        ${GXRUNTIME_DIR}/include ${BLUEWAKE_REPO_ROOT}/runtime/host/src)
    target_link_libraries(bluewake_guest_checkpoint_test PRIVATE xxhash)
    add_test(NAME bluewake_guest_checkpoint_test COMMAND bluewake_guest_checkpoint_test)

    add_executable(bluewake_gather_host_test
        ${BLUEWAKE_REPO_ROOT}/tests/gather_host_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/gather_pipe.c)
    target_include_directories(bluewake_gather_host_test PRIVATE
        ${BLUEWAKE_REPO_ROOT}/cmake/composite ${BLUEWAKE_REPO_ROOT}/runtime/host/src)
    target_link_libraries(bluewake_gather_host_test PRIVATE gxruntime)
    add_test(NAME bluewake_gather_host_test COMMAND bluewake_gather_host_test)

    add_executable(bluewake_gather_dispatch_test
        ${BLUEWAKE_REPO_ROOT}/tests/gather_dispatch_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/gather_pipe.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/dispatch_loop.c)
    target_include_directories(bluewake_gather_dispatch_test PRIVATE ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_gather_dispatch_test PRIVATE gxruntime)
    target_compile_definitions(bluewake_gather_dispatch_test PRIVATE BLUEWAKE_GATHER_PIPE=1)
    add_test(NAME bluewake_gather_dispatch_test COMMAND bluewake_gather_dispatch_test)
    foreach(_test bluewake_inline_gpr_memory_test bluewake_direct_calls_test bluewake_guest_checkpoint_test bluewake_gather_host_test bluewake_gather_memory_plain_test bluewake_gather_memory_global_test bluewake_gather_dispatch_test)
        target_include_directories(${_test} PRIVATE ${BLUEWAKE_COMPAT_INCLUDES})
        target_compile_options(${_test} PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
        target_link_libraries(${_test} PRIVATE bluewake_posix_compat)
        set_target_properties(${_test} PROPERTIES LINKER_LANGUAGE CXX)
    endforeach()

    add_executable(bluewake_native_math_guard_test
        ${BLUEWAKE_REPO_ROOT}/tests/native_math_guard_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_math.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_work_pool.c)
    target_include_directories(bluewake_native_math_guard_test PRIVATE ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_native_math_guard_test PRIVATE gxruntime)
    target_compile_options(bluewake_native_math_guard_test PRIVATE -ffp-contract=off)
    add_test(NAME bluewake_native_math_guard_test COMMAND bluewake_native_math_guard_test)

    target_compile_options(bluewake_native_math_guard_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_native_math_guard_test PRIVATE bluewake_posix_compat)
    set_target_properties(bluewake_native_math_guard_test PROPERTIES LINKER_LANGUAGE CXX)

    add_executable(bluewake_native_game_math_guard_test
        ${BLUEWAKE_REPO_ROOT}/tests/native_game_math_guard_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_game_math.c)
    target_include_directories(bluewake_native_game_math_guard_test PRIVATE ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_native_game_math_guard_test PRIVATE gxruntime)
    target_compile_options(bluewake_native_game_math_guard_test PRIVATE -ffp-contract=off)
    add_test(NAME bluewake_native_game_math_guard_test COMMAND bluewake_native_game_math_guard_test)

    target_compile_options(bluewake_native_game_math_guard_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_native_game_math_guard_test PRIVATE bluewake_posix_compat)
    set_target_properties(bluewake_native_game_math_guard_test PROPERTIES LINKER_LANGUAGE CXX)

    add_executable(bluewake_native_skin_guard_test
        ${BLUEWAKE_REPO_ROOT}/tests/native_skin_guard_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_skin.c)
    target_include_directories(bluewake_native_skin_guard_test PRIVATE ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_native_skin_guard_test PRIVATE gxruntime)
    target_compile_options(bluewake_native_skin_guard_test PRIVATE -ffp-contract=off)
    add_test(NAME bluewake_native_skin_guard_test COMMAND bluewake_native_skin_guard_test)

    target_compile_options(bluewake_native_skin_guard_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_native_skin_guard_test PRIVATE bluewake_posix_compat)
    set_target_properties(bluewake_native_skin_guard_test PROPERTIES LINKER_LANGUAGE CXX)

    add_executable(bluewake_native_vec_guard_test
        ${BLUEWAKE_REPO_ROOT}/tests/native_vec_guard_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_vec.c)
    target_include_directories(bluewake_native_vec_guard_test PRIVATE ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_native_vec_guard_test PRIVATE gxruntime)
    target_compile_options(bluewake_native_vec_guard_test PRIVATE -ffp-contract=off)
    add_test(NAME bluewake_native_vec_guard_test COMMAND bluewake_native_vec_guard_test)

    target_compile_options(bluewake_native_vec_guard_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_native_vec_guard_test PRIVATE bluewake_posix_compat)
    set_target_properties(bluewake_native_vec_guard_test PROPERTIES LINKER_LANGUAGE CXX)

    add_executable(bluewake_native_j3d_guard_test
        ${BLUEWAKE_REPO_ROOT}/tests/native_j3d_guard_test.c
        ${BLUEWAKE_REPO_ROOT}/cmake/composite/native_j3d.c)
    target_include_directories(bluewake_native_j3d_guard_test PRIVATE ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_link_libraries(bluewake_native_j3d_guard_test PRIVATE gxruntime)
    target_compile_options(bluewake_native_j3d_guard_test PRIVATE -ffp-contract=off)
    add_test(NAME bluewake_native_j3d_guard_test COMMAND bluewake_native_j3d_guard_test)

    target_compile_options(bluewake_native_j3d_guard_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_native_j3d_guard_test PRIVATE bluewake_posix_compat)
    set_target_properties(bluewake_native_j3d_guard_test PROPERTIES LINKER_LANGUAGE CXX)

    add_executable(bluewake_inline_fp_test ${BLUEWAKE_REPO_ROOT}/tests/inline_fp_test.c)
    target_link_libraries(bluewake_inline_fp_test PRIVATE gxruntime)
    target_include_directories(bluewake_inline_fp_test PRIVATE ${BLUEWAKE_REPO_ROOT}/cmake/composite)
    target_compile_options(bluewake_inline_fp_test PRIVATE -ffp-contract=off)
    add_test(NAME bluewake_inline_fp_test COMMAND bluewake_inline_fp_test)
    target_compile_options(bluewake_inline_fp_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_inline_fp_test PRIVATE bluewake_posix_compat)
    set_target_properties(bluewake_inline_fp_test PROPERTIES LINKER_LANGUAGE CXX)

    add_executable(bluewake_module_cpu_contract_test ${BLUEWAKE_REPO_ROOT}/tests/module_cpu_contract_test.c)
    target_compile_options(bluewake_module_cpu_contract_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_module_cpu_contract_test PRIVATE gxruntime bluewake_posix_compat)
    set_target_properties(bluewake_module_cpu_contract_test PROPERTIES LINKER_LANGUAGE CXX)
    target_include_directories(bluewake_module_cpu_contract_test PRIVATE
        ${GXRUNTIME_DIR}/include
        ${BLUEWAKE_REPO_ROOT}/ref/recompcore/Source/Core/Core/PowerPC/StaticRecomp)
    add_test(NAME bluewake_module_cpu_contract_test COMMAND bluewake_module_cpu_contract_test)

    foreach(_variant IN ITEMS plain global)
        add_executable(bluewake_guest_memory_${_variant}_test ${GXRUNTIME_DIR}/tests/guest_memory_bounds_test.c)
        target_include_directories(bluewake_guest_memory_${_variant}_test PRIVATE ${BLUEWAKE_COMPAT_INCLUDES})
        target_compile_options(bluewake_guest_memory_${_variant}_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
        target_link_libraries(bluewake_guest_memory_${_variant}_test PRIVATE gxruntime bluewake_posix_compat)
        set_target_properties(bluewake_guest_memory_${_variant}_test PROPERTIES LINKER_LANGUAGE CXX)
        if(_variant STREQUAL "global")
            target_compile_definitions(bluewake_guest_memory_${_variant}_test PRIVATE BW_GUEST_MEM1=bw_test_mem1 BW_GUEST_MEM1_SIZE=0x02000000u)
        endif()
        add_test(NAME bluewake_guest_memory_${_variant}_test COMMAND bluewake_guest_memory_${_variant}_test)
    endforeach()
    add_executable(bluewake_windows_disc_test ${BLUEWAKE_REPO_ROOT}/tests/windows_disc_test.c)
    target_include_directories(bluewake_windows_disc_test PRIVATE ${BLUEWAKE_HOST_SRC} ${BLUEWAKE_REPO_ROOT}/apple/ios/src)
    target_compile_options(bluewake_windows_disc_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_windows_disc_test PRIVATE bluewake_posix_compat comctl32 comdlg32)
    target_compile_definitions(bluewake_windows_disc_test PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    add_test(NAME bluewake_windows_disc_test COMMAND bluewake_windows_disc_test)
    set_tests_properties(bluewake_windows_disc_test PROPERTIES TIMEOUT 30)

    enable_testing()
    add_executable(bluewake_card_dispatch_lock_test ${BLUEWAKE_REPO_ROOT}/tests/card_dispatch_lock_test.c)
    target_include_directories(bluewake_card_dispatch_lock_test PRIVATE ${BLUEWAKE_HOST_SRC} ${GXRUNTIME_DIR}/include ${BLUEWAKE_COMPAT_INCLUDES})
    target_compile_options(bluewake_card_dispatch_lock_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_card_dispatch_lock_test PRIVATE gxruntime bluewake_posix_compat)
    set_target_properties(bluewake_card_dispatch_lock_test PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME bluewake_card_dispatch_lock_test COMMAND bluewake_card_dispatch_lock_test)
    add_executable(bluewake_render_worker_identity_test ${BLUEWAKE_REPO_ROOT}/tests/render_worker_identity_test.cpp)
    target_include_directories(bluewake_render_worker_identity_test PRIVATE ${GXRUNTIME_DIR}/graphics/aurora/lib)
    target_link_libraries(bluewake_render_worker_identity_test PRIVATE gxruntime GXRuntime::aurora GXRuntime::gxcore)
    add_test(NAME bluewake_render_worker_identity_test COMMAND bluewake_render_worker_identity_test)
    set_tests_properties(bluewake_render_worker_identity_test PROPERTIES TIMEOUT 30)

    add_executable(bluewake_gxcore_test ${GXRUNTIME_DIR}/graphics/gxcore/tests/gxcore_wgsl_test.cpp)
    target_link_libraries(bluewake_gxcore_test PRIVATE GXRuntime::gxcore)
    add_test(NAME bluewake_gxcore_test COMMAND bluewake_gxcore_test)
    set_tests_properties(bluewake_gxcore_test PROPERTIES TIMEOUT 30)

    include("${BLUEWAKE_REPO_ROOT}/cmake/fifo_trace_worker_test.cmake")
    include("${BLUEWAKE_REPO_ROOT}/cmake/mouse_motion_test.cmake")

    add_executable(bluewake_renderer_shared_flags_test ${BLUEWAKE_REPO_ROOT}/tests/renderer_shared_flags_test.cpp)
    target_compile_definitions(bluewake_renderer_shared_flags_test PRIVATE GXRUNTIME_HAS_AURORA_RECOMP=1)
    target_include_directories(bluewake_renderer_shared_flags_test PRIVATE ${GXRUNTIME_DIR}/backends/aurora ${GXRUNTIME_DIR}/graphics/aurora/lib)
    target_link_libraries(bluewake_renderer_shared_flags_test PRIVATE gxruntime GXRuntime::aurora GXRuntime::gxcore)
    add_test(NAME bluewake_renderer_shared_flags_test COMMAND bluewake_renderer_shared_flags_test)
    set_tests_properties(bluewake_renderer_shared_flags_test PROPERTIES TIMEOUT 15)

add_executable(bluewake_gx_flush_metrics_test ${BLUEWAKE_REPO_ROOT}/tests/gx_flush_metrics_test.c)
target_include_directories(bluewake_gx_flush_metrics_test PRIVATE ${BLUEWAKE_HOST_SRC})
add_test(NAME bluewake_gx_flush_metrics_test COMMAND bluewake_gx_flush_metrics_test)


add_executable(bluewake_desktop_controller_test ${BLUEWAKE_REPO_ROOT}/tests/desktop_controller_test.cpp)
target_include_directories(bluewake_desktop_controller_test PRIVATE ${GXRUNTIME_DIR}/graphics/aurora/lib ${BLUEWAKE_HOST_SRC})
target_link_libraries(bluewake_desktop_controller_test PRIVATE gxruntime GXRuntime::aurora GXRuntime::gxcore)
add_test(NAME bluewake_desktop_controller_test COMMAND bluewake_desktop_controller_test)
set_tests_properties(bluewake_desktop_controller_test PROPERTIES TIMEOUT 15)


add_executable(bluewake_face_button_swap_test ${BLUEWAKE_REPO_ROOT}/tests/face_button_swap_test.c)
target_include_directories(bluewake_face_button_swap_test PRIVATE ${BLUEWAKE_HOST_SRC})
add_test(NAME bluewake_face_button_swap_test COMMAND bluewake_face_button_swap_test)


add_executable(bluewake_audio_sink_test ${GXRUNTIME_DIR}/graphics/aurora/tests/audio_sink_test.cpp)
target_include_directories(bluewake_audio_sink_test PRIVATE ${GXRUNTIME_DIR}/graphics/aurora/lib)
target_link_libraries(bluewake_audio_sink_test PRIVATE gxruntime GXRuntime::aurora GXRuntime::gxcore)
add_test(NAME bluewake_audio_sink_test COMMAND bluewake_audio_sink_test)
set_tests_properties(bluewake_audio_sink_test PROPERTIES TIMEOUT 15)

    add_executable(bluewake_windows_crash_driver src/win_crash.cpp ${BLUEWAKE_REPO_ROOT}/tests/windows_crash_driver.cpp)
    target_compile_definitions(bluewake_windows_crash_driver PRIVATE BLUEWAKE_ENABLE_DEVELOPER_TRACING=1)
    add_test(NAME bluewake_windows_crash_test COMMAND powershell -NoProfile -ExecutionPolicy Bypass
        -File ${BLUEWAKE_REPO_ROOT}/tests/windows_crash_test.ps1 -Driver $<TARGET_FILE:bluewake_windows_crash_driver>)
    set_tests_properties(bluewake_windows_crash_test PROPERTIES TIMEOUT 30)


add_executable(bluewake_crash_text_test ${BLUEWAKE_REPO_ROOT}/tests/crash_text_test.c)
add_test(NAME bluewake_crash_text_test COMMAND bluewake_crash_text_test)


add_executable(bluewake_pipeline_shutdown_test ${GXRUNTIME_DIR}/graphics/aurora/tests/pipeline_shutdown_test.cpp)
target_link_libraries(bluewake_pipeline_shutdown_test PRIVATE gxruntime GXRuntime::aurora GXRuntime::gxcore)
add_test(NAME bluewake_pipeline_shutdown_test COMMAND bluewake_pipeline_shutdown_test)
set_tests_properties(bluewake_pipeline_shutdown_test PROPERTIES TIMEOUT 45)


add_executable(bluewake_settings_state_test ${BLUEWAKE_REPO_ROOT}/tests/settings_state_test.cpp)
target_compile_features(bluewake_settings_state_test PRIVATE cxx_std_17)
add_test(NAME bluewake_settings_state_test COMMAND bluewake_settings_state_test)


add_executable(bluewake_launch_marker_test ${BLUEWAKE_REPO_ROOT}/tests/launch_marker_test.c)
target_include_directories(bluewake_launch_marker_test PRIVATE ${BLUEWAKE_HOST_SRC})
add_test(NAME bluewake_launch_marker_test COMMAND bluewake_launch_marker_test)
target_compile_options(bluewake_launch_marker_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
target_link_libraries(bluewake_launch_marker_test PRIVATE bluewake_posix_compat)

add_executable(bluewake_atomic_copy_race_test ${BLUEWAKE_REPO_ROOT}/tests/atomic_copy_race_test.c)
target_include_directories(bluewake_atomic_copy_race_test PRIVATE ${BLUEWAKE_HOST_SRC})
target_compile_options(bluewake_atomic_copy_race_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
target_link_libraries(bluewake_atomic_copy_race_test PRIVATE bluewake_posix_compat)
add_test(NAME bluewake_atomic_copy_race_test COMMAND bluewake_atomic_copy_race_test)

add_executable(bluewake_save_state_test ${BLUEWAKE_REPO_ROOT}/tests/save_state_test.c ${BLUEWAKE_HOST_SRC}/save_state.c)
target_include_directories(bluewake_save_state_test PRIVATE ${BLUEWAKE_HOST_SRC} ${BLUEWAKE_COMPAT_INCLUDES})
target_compile_options(bluewake_save_state_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
target_link_libraries(bluewake_save_state_test PRIVATE ZLIB::ZLIB bluewake_posix_compat)
add_test(NAME bluewake_save_state_test COMMAND bluewake_save_state_test)

include("${BLUEWAKE_REPO_ROOT}/cmake/host_state_save_test.cmake")
target_include_directories(bluewake_host_state_save_failure_test PRIVATE ${BLUEWAKE_COMPAT_INCLUDES})
target_compile_options(bluewake_host_state_save_failure_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
target_link_libraries(bluewake_host_state_save_failure_test PRIVATE ZLIB::ZLIB bluewake_posix_compat)


add_executable(bluewake_restart_request_test ${BLUEWAKE_REPO_ROOT}/tests/restart_request_test.cpp)
add_test(NAME bluewake_restart_request_test COMMAND bluewake_restart_request_test)


add_executable(bluewake_memory_card_io_test ${BLUEWAKE_REPO_ROOT}/tests/memory_card_io_test.c)
target_include_directories(bluewake_memory_card_io_test PRIVATE ${GXRUNTIME_DIR}/include ${BLUEWAKE_COMPAT_INCLUDES})
target_compile_options(bluewake_memory_card_io_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME bluewake_memory_card_io_test COMMAND bluewake_memory_card_io_test)
set_tests_properties(bluewake_memory_card_io_test PROPERTIES TIMEOUT 15)
target_link_libraries(bluewake_memory_card_io_test PRIVATE bluewake_posix_compat)

    add_executable(bluewake_audio_dma_stereo_test ${BLUEWAKE_REPO_ROOT}/tests/audio_dma_stereo_test.c)
    target_include_directories(bluewake_audio_dma_stereo_test PRIVATE ${BLUEWAKE_HOST_SRC})
    target_compile_options(bluewake_audio_dma_stereo_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME bluewake_audio_dma_stereo_test COMMAND bluewake_audio_dma_stereo_test)
    add_executable(bluewake_bmg_patch_test ${BLUEWAKE_REPO_ROOT}/tests/bmg_patch_test.c)
    target_include_directories(bluewake_bmg_patch_test PRIVATE ${BLUEWAKE_HOST_SRC})
    target_compile_options(bluewake_bmg_patch_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME bluewake_bmg_patch_test COMMAND bluewake_bmg_patch_test)
    set_tests_properties(bluewake_bmg_patch_test PROPERTIES TIMEOUT 30)
    add_executable(bluewake_windows_clock_test ${BLUEWAKE_REPO_ROOT}/tests/windows_clock_test.c)
    target_link_libraries(bluewake_windows_clock_test PRIVATE bluewake_posix_compat)
    target_compile_options(bluewake_windows_clock_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME bluewake_windows_clock_test COMMAND bluewake_windows_clock_test)
    set_tests_properties(bluewake_windows_clock_test PROPERTIES TIMEOUT 15)
    add_executable(bluewake_feature_dispatch_test ${BLUEWAKE_REPO_ROOT}/tests/feature_dispatch_test.c)
    target_include_directories(bluewake_feature_dispatch_test PRIVATE
        ${BLUEWAKE_HOST_SRC} ${BLUEWAKE_COMPAT_INCLUDES})
    target_compile_options(bluewake_feature_dispatch_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_feature_dispatch_test PRIVATE gxruntime bluewake_posix_compat)
    set_target_properties(bluewake_feature_dispatch_test PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME bluewake_feature_dispatch_test COMMAND bluewake_feature_dispatch_test)
    set_tests_properties(bluewake_feature_dispatch_test PROPERTIES TIMEOUT 30)
    add_executable(bluewake_windows_getenv_cache_test
        ${BLUEWAKE_REPO_ROOT}/tests/windows_getenv_cache_test.c)
    target_compile_options(bluewake_windows_getenv_cache_test PRIVATE -Wall -Wextra -Werror)
    add_test(NAME bluewake_windows_getenv_cache_test COMMAND bluewake_windows_getenv_cache_test)

    add_executable(bluewake_fps_watch_test
        ${BLUEWAKE_REPO_ROOT}/tests/fps_watch_test.c ${BLUEWAKE_HOST_SRC}/fps_watch.c)
    target_include_directories(bluewake_fps_watch_test PRIVATE
        ${BLUEWAKE_HOST_SRC} ${BLUEWAKE_COMPAT_INCLUDES})
    target_compile_options(bluewake_fps_watch_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_fps_watch_test PRIVATE gxruntime bluewake_posix_compat)
    set_target_properties(bluewake_fps_watch_test PROPERTIES LINKER_LANGUAGE CXX)
    add_test(NAME bluewake_fps_watch_test COMMAND bluewake_fps_watch_test)
    set_tests_properties(bluewake_windows_getenv_cache_test bluewake_fps_watch_test PROPERTIES TIMEOUT 15)

    add_executable(bluewake_frame_interp_test
        ${GXRUNTIME_DIR}/graphics/aurora/tests/frame_interp_test.cpp)
    target_include_directories(bluewake_frame_interp_test PRIVATE ${BLUEWAKE_COMPAT_INCLUDES})
    target_compile_options(bluewake_frame_interp_test PRIVATE ${BLUEWAKE_COMPAT_FORCE_INCLUDE})
    target_link_libraries(bluewake_frame_interp_test PRIVATE gxruntime GXRuntime::aurora GXRuntime::gxcore bluewake_posix_compat)
    add_test(NAME bluewake_frame_interp_test COMMAND bluewake_frame_interp_test)
    set_tests_properties(bluewake_frame_interp_test PROPERTIES TIMEOUT 30)

    add_executable(bluewake_dsp_interpreter_layout_test
        ${BLUEWAKE_REPO_ROOT}/tests/dsp_interpreter_layout_test.cpp
        ${BLUEWAKE_REPO_ROOT}/tests/dsp_interpreter_layout_tables_first.cpp)
    target_include_directories(bluewake_dsp_interpreter_layout_test PRIVATE ${BLUEWAKE_REPO_ROOT}/tests)
    target_compile_options(bluewake_dsp_interpreter_layout_test PRIVATE -fno-ms-compatibility)
    target_link_libraries(bluewake_dsp_interpreter_layout_test PRIVATE bluewake_dsp_donor)
    add_test(NAME bluewake_dsp_interpreter_layout_test COMMAND bluewake_dsp_interpreter_layout_test)
    set_tests_properties(bluewake_dsp_interpreter_layout_test PROPERTIES TIMEOUT 15)
endif()

# The app folder: the executable, its runtime DLLs (SDL3, Dawn and the DXC
# shader compiler Dawn loads for Direct3D 12) and Aurora's bundled pipeline
# cache. The builder adds the game module and the prepared game files.
include(${GXRUNTIME_DIR}/graphics/aurora/cmake/AuroraCopyRuntimeDLLs.cmake)
aurora_copy_runtime_dlls(BlueWake)
set(BLUEWAKE_PIPELINE_SEED "${BLUEWAKE_REPO_ROOT}/apple/ios/resources/initial_pipeline_cache.db")
add_custom_command(TARGET BlueWake POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${BLUEWAKE_PIPELINE_SEED}" $<TARGET_FILE_DIR:BlueWake>
    VERBATIM)
