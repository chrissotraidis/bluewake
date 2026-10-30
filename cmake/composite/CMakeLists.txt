cmake_minimum_required(VERSION 3.16)
project(bluewake_composite C)

set(CMAKE_C_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(GAME_ID "GZLE01" CACHE STRING "Six-character disc ID")
set(COMPOSITE_DIR "" CACHE PATH "Composite output dir (generated.h, module_tables.inc, rel_modules.inc)")
set(GXRUNTIME_DIR "" CACHE PATH "GXRuntime checkout")
set(ABI_DIR "" CACHE PATH "Directory containing StaticRecompABI.h")
option(BLUEWAKE_INLINE_FP "Use explicitly prepared inline floating-point chunks" OFF)
option(BLUEWAKE_GATHER_PIPE "Use explicitly prepared gather/inline-memory chunks" OFF)
option(BLUEWAKE_GATHER_ALIAS_CACHE "Cache gather-pipe alias-window overlap by registry generation" OFF)
option(BLUEWAKE_NATIVE_J3D "Use certified native J3D matrix functions" OFF)
option(BLUEWAKE_NATIVE_GAME_MATH "Use certified native game math" OFF)
option(BLUEWAKE_NATIVE_SKIN "Use certified native skinning" OFF)
option(BLUEWAKE_NATIVE_VEC "Use certified native vector functions" OFF)
option(BLUEWAKE_NATIVE_MATH "Use certified native matrix functions" OFF)
option(BLUEWAKE_NATIVE_ENTRIES "Use Wind Waker Recomp's certified native entries (scripts/windows/native_entries.py)" OFF)
option(BLUEWAKE_NATIVE_GX "Use Wind Waker Recomp's GX FIFO writer natives (the fifth set)" OFF)
option(BLUEWAKE_NATIVE_SET4 "Use Wind Waker Recomp's fourth native set (colour, animation, collision setup)" OFF)
option(BLUEWAKE_DIRECT_CALLS "Use explicitly prepared direct-call chunks" OFF)
option(BLUEWAKE_FIXED_CPU "Use explicitly prepared fixed-address guest CPU chunks" OFF)
option(BLUEWAKE_FIXED_MEM1 "Use module-owned guest RAM (requires fixed CPU)" OFF)
if(BLUEWAKE_FIXED_MEM1 AND NOT BLUEWAKE_FIXED_CPU)
  message(FATAL_ERROR "Fixed MEM1 requires BLUEWAKE_FIXED_CPU")
endif()
set(COMPOSITE_OPTIMIZATION_LEVEL "2" CACHE STRING "Generated composite optimization level (0, 1, or 2)")
set_property(CACHE COMPOSITE_OPTIMIZATION_LEVEL PROPERTY STRINGS 0 1 2)
if(NOT COMPOSITE_OPTIMIZATION_LEVEL MATCHES "^[012]$")
  message(FATAL_ERROR "COMPOSITE_OPTIMIZATION_LEVEL must be 0, 1, or 2")
endif()
set(COMPOSITE_O1_FALLBACK_SOURCES
  "chunks_d_a_npc_so_148/chunk_0000_rel1_C10800F4.c;chunks_dol/chunk_0140_text1_802316E0.c;chunks_dol/chunk_0202_text1_803296E0.c"
  CACHE STRING "Composite-relative generated sources compiled at O1 when the target uses O2")

foreach(REQUIRED_FILE generated.h generated_composite.h module_tables.inc rel_modules.inc rel_data.inc)
  if(NOT EXISTS "${COMPOSITE_DIR}/${REQUIRED_FILE}")
    message(FATAL_ERROR "Missing ${REQUIRED_FILE} in COMPOSITE_DIR=${COMPOSITE_DIR}")
  endif()
endforeach()
if(NOT EXISTS "${GXRUNTIME_DIR}")
  message(FATAL_ERROR "GXRuntime not found at ${GXRUNTIME_DIR}")
endif()
if(NOT EXISTS "${ABI_DIR}/StaticRecompABI.h")
  message(FATAL_ERROR "StaticRecompABI.h not found in ${ABI_DIR}")
endif()

set(MODULE_NAME "g${GAME_ID}_recomp")

# Collect the DOL's C or native-object chunks plus C REL chunks.
file(GLOB DOL_CHUNK_SOURCES "${COMPOSITE_DIR}/chunks_dol/*.c")
file(GLOB DOL_CHUNK_OBJECTS "${COMPOSITE_DIR}/chunks_dol/*.o")
if(DOL_CHUNK_SOURCES AND DOL_CHUNK_OBJECTS)
  message(FATAL_ERROR "DOL composite contains both C and native object chunks")
endif()
if(NOT DOL_CHUNK_SOURCES AND NOT DOL_CHUNK_OBJECTS)
  message(FATAL_ERROR "DOL composite contains no translated chunks")
endif()
set_source_files_properties(${DOL_CHUNK_OBJECTS}
  PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
file(GLOB REL_CHUNK_GLOBS LIST_DIRECTORIES TRUE "${COMPOSITE_DIR}/chunks_*")
set(CHUNK_INPUTS "")
foreach(CHUNK_DIR ${REL_CHUNK_GLOBS})
  if(IS_DIRECTORY "${CHUNK_DIR}" AND NOT CHUNK_DIR MATCHES "chunks_dol")
    file(GLOB RC "${CHUNK_DIR}/*.c")
    list(APPEND CHUNK_INPUTS ${RC})
  endif()
endforeach()
list(APPEND CHUNK_INPUTS ${DOL_CHUNK_SOURCES})
list(APPEND CHUNK_INPUTS ${DOL_CHUNK_OBJECTS})
list(LENGTH CHUNK_INPUTS NUM_CHUNKS)
message(STATUS "${NUM_CHUNKS} chunk translation units")

add_library(${MODULE_NAME} SHARED
  module_export.c
  dispatch_loop.c
  ${GXRUNTIME_DIR}/src/core/cpu.c
  ${GXRUNTIME_DIR}/src/core/cpu_exception.c
  ${GXRUNTIME_DIR}/src/core/cpu_interpreter.c
  ${GXRUNTIME_DIR}/src/core/cpu_interpreter_table.c
  ${GXRUNTIME_DIR}/src/core/cpu_interpreter_float.c
  ${GXRUNTIME_DIR}/src/core/cpu_interpreter_integer.c
  ${CHUNK_INPUTS})

if(BLUEWAKE_INLINE_FP)
  foreach(CHUNK_SOURCE IN LISTS CHUNK_INPUTS)
    if(NOT CHUNK_SOURCE MATCHES "\\.c$")
      message(FATAL_ERROR "Inline FP needs prepared C chunks, not native objects")
    endif()
    file(STRINGS "${CHUNK_SOURCE}" INLINE_FP_MARKER LIMIT_COUNT 1
      REGEX "^#include \"inline_fp.h\"$")
    if(NOT INLINE_FP_MARKER)
      message(FATAL_ERROR "Inline FP chunk is unprepared: ${CHUNK_SOURCE}")
    endif()
  endforeach()
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(BLUEWAKE_GATHER_PIPE)
  foreach(CHUNK_SOURCE IN LISTS CHUNK_INPUTS)
    if(NOT CHUNK_SOURCE MATCHES "\\.c$")
      message(FATAL_ERROR "Gather memory needs prepared C chunks, not native objects")
    endif()
    file(STRINGS "${CHUNK_SOURCE}" GATHER_MARKER LIMIT_COUNT 1
      REGEX "^#include \"gather_pipe.h\"$")
    if(NOT GATHER_MARKER)
      message(FATAL_ERROR "Gather memory chunk is unprepared: ${CHUNK_SOURCE}")
    endif()
  endforeach()
  target_sources(${MODULE_NAME} PRIVATE gather_pipe.c)
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(${MODULE_NAME} PRIVATE BLUEWAKE_GATHER_PIPE=1)
endif()
if(BLUEWAKE_GATHER_ALIAS_CACHE)
  if(NOT BLUEWAKE_GATHER_PIPE)
    message(FATAL_ERROR "Gather alias cache requires BLUEWAKE_GATHER_PIPE")
  endif()
  target_compile_definitions(${MODULE_NAME} PRIVATE BLUEWAKE_GATHER_ALIAS_CACHE=1)
endif()

if(BLUEWAKE_NATIVE_J3D)
  file(STRINGS "${COMPOSITE_DIR}/generated.h" J3D_MARKER LIMIT_COUNT 1
    REGEX "^#define BLUEWAKE_NATIVE_J3D_PREPARED 1$")
  if(NOT J3D_MARKER OR NOT EXISTS "${COMPOSITE_DIR}/native_j3d.json" OR DOL_CHUNK_OBJECTS)
    message(FATAL_ERROR "Native J3D requires certified C chunks and its preparation manifest")
  endif()
  target_sources(${MODULE_NAME} PRIVATE native_j3d.c)
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(BLUEWAKE_NATIVE_GAME_MATH)
  file(STRINGS "${COMPOSITE_DIR}/generated.h" GAME_MATH_MARKER LIMIT_COUNT 1
    REGEX "^#define BLUEWAKE_NATIVE_GAME_MATH_PREPARED 1$")
  if(NOT GAME_MATH_MARKER OR NOT EXISTS "${COMPOSITE_DIR}/native_game_math.json" OR DOL_CHUNK_OBJECTS)
    message(FATAL_ERROR "Native game math requires certified C chunks and its preparation manifest")
  endif()
  target_sources(${MODULE_NAME} PRIVATE native_game_math.c)
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(BLUEWAKE_NATIVE_SKIN)
  file(STRINGS "${COMPOSITE_DIR}/generated.h" SKIN_MARKER LIMIT_COUNT 1
    REGEX "^#define BLUEWAKE_NATIVE_SKIN_PREPARED 1$")
  if(NOT SKIN_MARKER OR NOT EXISTS "${COMPOSITE_DIR}/native_skin.json" OR DOL_CHUNK_OBJECTS)
    message(FATAL_ERROR "Native skinning requires certified C chunks and its preparation manifest")
  endif()
  target_sources(${MODULE_NAME} PRIVATE native_skin.c)
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(BLUEWAKE_NATIVE_VEC)
  file(STRINGS "${COMPOSITE_DIR}/generated.h" VEC_MARKER LIMIT_COUNT 1
    REGEX "^#define BLUEWAKE_NATIVE_VEC_PREPARED 1$")
  if(NOT VEC_MARKER OR NOT EXISTS "${COMPOSITE_DIR}/native_vec.json" OR DOL_CHUNK_OBJECTS)
    message(FATAL_ERROR "Native vectors requires certified C chunks and its preparation manifest")
  endif()
  target_sources(${MODULE_NAME} PRIVATE native_vec.c)
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

if(BLUEWAKE_NATIVE_MATH)
  file(STRINGS "${COMPOSITE_DIR}/generated_composite.h" MATH_MARKER LIMIT_COUNT 1
    REGEX "^#define BLUEWAKE_NATIVE_MATH_CACHED 1$")
  if(NOT MATH_MARKER OR NOT EXISTS "${COMPOSITE_DIR}/native_math.json" OR DOL_CHUNK_OBJECTS)
    message(FATAL_ERROR "Native matrix functions require certified C chunks and their preparation manifest")
  endif()
  target_sources(${MODULE_NAME} PRIVATE native_math.c native_work_pool.c)
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(${MODULE_NAME} PRIVATE Threads::Threads)
  endif()
endif()

if(BLUEWAKE_NATIVE_ENTRIES)
  # The hooks native_entries.py inserts call these; its certification decides
  # which are hooked. The natives use the direct-call, gather-pipe and vector code.
  # The seventh set's replay (native_*_gen.inc) is generated from this build's
  # composite. It is translated game code, so it is not in the repository.
  if(NOT BLUEWAKE_DIRECT_CALLS OR NOT BLUEWAKE_GATHER_PIPE OR NOT BLUEWAKE_NATIVE_VEC)
    message(FATAL_ERROR "Native entries need BLUEWAKE_DIRECT_CALLS, BLUEWAKE_GATHER_PIPE and BLUEWAKE_NATIVE_VEC")
  endif()
  foreach(GROUP libm bgblk rot calc geom jas)
    if(NOT EXISTS "${BLUEWAKE_NATIVE_GEN_DIR}/native_${GROUP}_gen.inc")
      message(FATAL_ERROR
        "Native entries need ${BLUEWAKE_NATIVE_GEN_DIR}/native_${GROUP}_gen.inc from native_leaf_gen.py")
    endif()
  endforeach()
  target_sources(${MODULE_NAME} PRIVATE native_fifo.c native_bg.c native_mtxcalc.c native_search.c
    native_libm.c native_bgblk.c native_rot.c native_calc.c native_geom.c native_jas.c)
  target_include_directories(${MODULE_NAME} BEFORE PRIVATE "${BLUEWAKE_NATIVE_GEN_DIR}")
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(${MODULE_NAME} PRIVATE BLUEWAKE_NATIVE_ENTRIES=1)
endif()

if(BLUEWAKE_NATIVE_GX)
  # The fifth set (native_gx.c). Hooks stay out of the translation until
  # native_entries.py records hashes for this tree, so an uncertified build
  # links the objects and never calls them. The replay is generated from this
  # build's composite, same folder as the seventh set.
  if(NOT BLUEWAKE_DIRECT_CALLS OR NOT BLUEWAKE_GATHER_PIPE OR NOT BLUEWAKE_NATIVE_VEC)
    message(FATAL_ERROR "Native GX needs BLUEWAKE_DIRECT_CALLS, BLUEWAKE_GATHER_PIPE and BLUEWAKE_NATIVE_VEC")
  endif()
  if(NOT EXISTS "${BLUEWAKE_NATIVE_GEN_DIR}/native_gx_gen.inc")
    message(FATAL_ERROR
      "Native GX needs ${BLUEWAKE_NATIVE_GEN_DIR}/native_gx_gen.inc from native_gx_gen.py")
  endif()
  target_sources(${MODULE_NAME} PRIVATE native_gx.c)
  target_include_directories(${MODULE_NAME} BEFORE PRIVATE "${BLUEWAKE_NATIVE_GEN_DIR}")
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(${MODULE_NAME} PRIVATE BLUEWAKE_NATIVE_GX=1)
endif()

if(BLUEWAKE_NATIVE_SET4)
  # The fourth set. Hooks stay out until hashes are recorded for this tree.
  if(NOT BLUEWAKE_DIRECT_CALLS OR NOT BLUEWAKE_GATHER_PIPE OR NOT BLUEWAKE_NATIVE_VEC)
    message(FATAL_ERROR "The fourth native set needs BLUEWAKE_DIRECT_CALLS, BLUEWAKE_GATHER_PIPE and BLUEWAKE_NATIVE_VEC")
  endif()
  target_sources(${MODULE_NAME} PRIVATE native_kankyo.c native_anim.c native_cc.c)
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(${MODULE_NAME} PRIVATE BLUEWAKE_NATIVE_SET4=1)
endif()

if(BLUEWAKE_DIRECT_CALLS)
  file(STRINGS "${COMPOSITE_DIR}/generated.h" DIRECT_MARKER LIMIT_COUNT 1
    REGEX "^#define BLUEWAKE_DIRECT_CALLS_PREPARED 2$")
  if(NOT DIRECT_MARKER OR NOT EXISTS "${COMPOSITE_DIR}/bw_edge_watch.inc")
    message(FATAL_ERROR "Direct calls need a prepared composite and host watch list")
  endif()
  foreach(CHUNK_SOURCE IN LISTS CHUNK_INPUTS)
    if(NOT CHUNK_SOURCE MATCHES "\\.c$")
      message(FATAL_ERROR "Direct calls need prepared C chunks, not native objects")
    endif()
  endforeach()
  target_sources(${MODULE_NAME} PRIVATE direct_calls.c)
  target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_definitions(${MODULE_NAME} PRIVATE BLUEWAKE_DIRECT_CALLS=1 BLUEWAKE_EDGE_FILTER=1)
endif()

if(BLUEWAKE_FIXED_CPU)
  foreach(CHUNK_SOURCE IN LISTS CHUNK_INPUTS)
    if(NOT CHUNK_SOURCE MATCHES "\\.c$")
      message(FATAL_ERROR "Fixed CPU needs prepared C chunks, not native objects")
    endif()
    file(STRINGS "${CHUNK_SOURCE}" FIXED_CPU_MARKER LIMIT_COUNT 1
      REGEX "^extern CPUState bw_guest_cpu;$")
    if(NOT FIXED_CPU_MARKER)
      message(FATAL_ERROR "Fixed CPU chunk is unprepared: ${CHUNK_SOURCE}")
    endif()
  endforeach()
  target_sources(${MODULE_NAME} PRIVATE guest_cpu.c)
  target_compile_definitions(${MODULE_NAME} PRIVATE BLUEWAKE_FIXED_CPU=1)
endif()
if(BLUEWAKE_FIXED_MEM1)
  file(STRINGS "${GXRUNTIME_DIR}/include/core/cpu.h" FIXED_MEM1_SUPPORT
    LIMIT_COUNT 1 REGEX "^extern u8 BW_GUEST_MEM1\\[\\];$")
  if(NOT FIXED_MEM1_SUPPORT)
    message(FATAL_ERROR "Fixed MEM1 requires the supporting pinned runtime")
  endif()
  target_compile_definitions(${MODULE_NAME} PRIVATE
    BW_GUEST_MEM1=bw_guest_mem1 BW_GUEST_MEM1_SIZE=0x02000000u)
endif()

set_target_properties(${MODULE_NAME} PROPERTIES PREFIX "" OUTPUT_NAME "${MODULE_NAME}" UNITY_BUILD OFF C_VISIBILITY_PRESET hidden)
# Extra sources a builder adds (the Android builder's training-module profile
# flush, android/src/profile_flush.c); empty otherwise.
set(COMPOSITE_EXTRA_SOURCES "" CACHE STRING "Extra sources compiled into the module")
if(COMPOSITE_EXTRA_SOURCES)
  target_sources(${MODULE_NAME} PRIVATE ${COMPOSITE_EXTRA_SOURCES})
endif()
target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_GAME_ID="${GAME_ID}" DOLRECOMP_CPU_HEADER="core/cpu.h")
target_include_directories(${MODULE_NAME} PRIVATE "${COMPOSITE_DIR}" "${GXRUNTIME_DIR}/include" "${ABI_DIR}")
if(APPLE)
  set_target_properties(${MODULE_NAME} PROPERTIES SUFFIX ".dylib")
endif()
if(WIN32)
  # A DLL exports only what is marked dllexport. module_export.c marks its own
  # entry points; the guest-alias and write-journal hooks the host resolves in
  # the runtime embedded here (GXRuntime cpu.c) are named for the linker.
  foreach(EXPORTED_SYMBOL
      ppc_set_mem_write_journal ppc_guest_alias_clear ppc_guest_alias_add
      ppc_guest_alias_add_shared ppc_guest_alias_get_storage
      ppc_guest_alias_remove ppc_guest_alias_resolve
      ppc_guest_alias_range_overlaps)
    target_link_options(${MODULE_NAME} PRIVATE "LINKER:/EXPORT:${EXPORTED_SYMBOL}")
  endforeach()
  target_compile_definitions(${MODULE_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
  # The Windows C runtime's getenv is slow, and the embedded runtime checks a
  # trace switch on every guest exception: remember each call site's answer
  # (windows/compat/bw_getenv_cache.h). Only these sources, whose every getenv
  # names a literal; the translated chunks never call it.
  set_property(SOURCE module_export.c dispatch_loop.c
      ${GXRUNTIME_DIR}/src/core/cpu.c ${GXRUNTIME_DIR}/src/core/cpu_exception.c
      ${GXRUNTIME_DIR}/src/core/cpu_interpreter.c ${GXRUNTIME_DIR}/src/core/cpu_interpreter_table.c
      ${GXRUNTIME_DIR}/src/core/cpu_interpreter_float.c ${GXRUNTIME_DIR}/src/core/cpu_interpreter_integer.c
      APPEND PROPERTY COMPILE_OPTIONS
      -include "${CMAKE_CURRENT_SOURCE_DIR}/../../windows/compat/bw_getenv_cache.h")
else()
  target_link_libraries(${MODULE_NAME} PRIVATE m)
endif()
# ELF (Linux, Steam Deck): the module is a shared object built -fPIC,
# and the chunks declared each other's symbols with default visibility, so every
# reference was an address loaded from the GOT that then stayed live in a
# register across the chunk's one large function. That doubled the register
# allocator's work: a chunk took 11.7 s here against 5.9 s for the same source
# compiled for Windows, whose DLLs have no GOT. hidden_externs.h declares the
# module's own symbols hidden, so each reference is a direct PC-relative operand.
# Android leaves it out: on ARM64 the game thread then ran about 5% more
# instructions at Outset's pier (172 M a retrace against 163 M, #93), and players
# feel that every frame while they wait for the compile once.
if(NOT WIN32 AND NOT APPLE AND NOT ANDROID)
  set(CHUNK_C_SOURCES ${CHUNK_INPUTS})
  list(FILTER CHUNK_C_SOURCES INCLUDE REGEX "\\.c$")
  set_property(SOURCE ${CHUNK_C_SOURCES} APPEND PROPERTY COMPILE_OPTIONS
      -include "${CMAKE_CURRENT_SOURCE_DIR}/hidden_externs.h")
endif()

# ELF: the module's pointer tables need about 2.2 million relative relocations,
# 24 bytes each as RELA (about 51 MB, and as many fix-ups at every launch).
# RELR stores runs of them as bitmaps. The loader needs glibc 2.36 for it, and
# -z pack-relative-relocs records that requirement in the module, so an older
# glibc refuses to load it rather than running it unrelocated. Only when the
# glibc it is built against has it: the module is built on the machine that
# runs it, and a release built on an older baseline (#214) leaves it off.
if(NOT WIN32 AND NOT APPLE)
  include(CheckCSourceCompiles)
  include(CheckLinkerFlag)
  check_c_source_compiles("
#include <features.h>
#if !defined(__GLIBC__) || __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 36)
#error no DT_RELR
#endif
int main(void) { return 0; }" BLUEWAKE_GLIBC_HAS_DT_RELR)
  check_linker_flag(C "LINKER:-z,pack-relative-relocs" BLUEWAKE_LINKER_PACKS_RELR)
  if(BLUEWAKE_GLIBC_HAS_DT_RELR AND BLUEWAKE_LINKER_PACKS_RELR)
    target_link_options(${MODULE_NAME} PRIVATE "LINKER:-z,pack-relative-relocs")
  endif()
endif()
target_compile_options(${MODULE_NAME} PRIVATE -O${COMPOSITE_OPTIMIZATION_LEVEL} -ffp-contract=off)

# The builder's --tiered: chunks the optimization training never ran, one
# composite-relative path a line, compiled at -O1 without GVN's memory
# dependence analysis instead of -O2 (scripts/windows/build.py cold_sources).
set(COMPOSITE_COLD_SOURCES_FILE "" CACHE FILEPATH "Chunks compiled at O1 when the target uses O2")
if(COMPOSITE_OPTIMIZATION_LEVEL STREQUAL "2" AND COMPOSITE_COLD_SOURCES_FILE)
  file(STRINGS "${COMPOSITE_COLD_SOURCES_FILE}" COLD_SOURCES)
  list(LENGTH COLD_SOURCES COLD_COUNT)
  foreach(COLD_SOURCE IN LISTS COLD_SOURCES)
    set(COLD_PATH "${COMPOSITE_DIR}/${COLD_SOURCE}")
    if(EXISTS "${COLD_PATH}")
      set_property(SOURCE "${COLD_PATH}" APPEND PROPERTY COMPILE_OPTIONS -O1 -mllvm -enable-gvn-memdep=false)
    endif()
  endforeach()
  message(STATUS "Tiered: ${COLD_COUNT} cold chunks at O1")
endif()

if(COMPOSITE_OPTIMIZATION_LEVEL STREQUAL "2")
  foreach(FALLBACK_SOURCE IN LISTS COMPOSITE_O1_FALLBACK_SOURCES)
    set(FALLBACK_PATH "${COMPOSITE_DIR}/${FALLBACK_SOURCE}")
    if(NOT EXISTS "${FALLBACK_PATH}")
      message(FATAL_ERROR "O1 fallback source not found: ${FALLBACK_PATH}")
    endif()
    list(FIND CHUNK_INPUTS "${FALLBACK_PATH}" FALLBACK_INDEX)
    if(FALLBACK_INDEX EQUAL -1)
      message(FATAL_ERROR "O1 fallback is not a generated chunk: ${FALLBACK_PATH}")
    endif()
    set_property(SOURCE "${FALLBACK_PATH}" APPEND PROPERTY COMPILE_OPTIONS -O1)
    message(STATUS "O1 optimizer fallback: ${FALLBACK_SOURCE}")
  endforeach()
endif()
