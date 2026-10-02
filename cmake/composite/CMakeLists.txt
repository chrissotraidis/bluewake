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
      ppc_guest_alias_remove ppc_guest_alias_resolve)
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
target_compile_options(${MODULE_NAME} PRIVATE -O${COMPOSITE_OPTIMIZATION_LEVEL} -ffp-contract=off)

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
