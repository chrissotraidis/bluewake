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
    src/win_settings.cpp
    src/bluewake.rc
    resources/BlueWake.manifest
    ${BLUEWAKE_HOST_SOURCES})
# The unchanged host keeps its own entry point under another name; the shim
# owns main() and hands over after setting the defaults.
set_source_files_properties(${BLUEWAKE_HOST_SRC}/main.c PROPERTIES
    COMPILE_DEFINITIONS "main=bluewake_host_main")
target_link_libraries(BlueWake PRIVATE gxruntime GXRuntime::aurora bluewake_posix_compat winmm)
if(TARGET imgui)
    target_link_libraries(BlueWake PRIVATE imgui)
endif()
set_target_properties(BlueWake PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME BlueWake)
target_include_directories(BlueWake PRIVATE
    ${BLUEWAKE_COMPAT_INCLUDES}
    ${BLUEWAKE_HOST_SRC}
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
    enable_testing()

add_executable(bluewake_restart_request_test ${BLUEWAKE_REPO_ROOT}/tests/restart_request_test.cpp)
add_test(NAME bluewake_restart_request_test COMMAND bluewake_restart_request_test)


add_executable(bluewake_memory_card_io_test ${BLUEWAKE_REPO_ROOT}/tests/memory_card_io_test.c)
target_include_directories(bluewake_memory_card_io_test PRIVATE ${GXRUNTIME_DIR}/include)
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
