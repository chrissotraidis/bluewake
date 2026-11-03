# Compile the actual host save function with synthetic subsystem inputs.
# Keep its body in one place, and regenerate when the host changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${BLUEWAKE_HOST_SRC}/main.c")
file(READ "${BLUEWAKE_HOST_SRC}/main.c" _state_host)
string(FIND "${_state_host}" "static bool host_state_save(" _state_start)
string(FIND "${_state_host}" "static bool host_state_load(" _state_end)
if(_state_start LESS 0 OR _state_end LESS _state_start)
    message(FATAL_ERROR "Could not locate the host state save function for its regression")
endif()
math(EXPR _state_length "${_state_end} - ${_state_start}")
string(SUBSTRING "${_state_host}" ${_state_start} ${_state_length} _state_function)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/host_state_save_under_test.inc" "${_state_function}")
# Exercise the real loose-REL continuation and save/load gate with synthetic
# guest memory. Include their declarations too, so the test cannot drift from
# the slot layout, capacity or request bookkeeping.
set(_state_guard "${CMAKE_CURRENT_BINARY_DIR}/host_state_guard_under_test.inc")
file(WRITE "${_state_guard}" "")
foreach(_bounds IN ITEMS
        "#define BLUEWAKE_DO_LOAD|static void host_loose_rel_configure("
        "static bool host_loose_rel_wait(|static void host_zero_rel_bss("
        "#define HOST_STATE_DRAW_DONE_PC|typedef struct HostStateHeader"
        "static u64 g_state_test_load_retrace|// BLUEWAKE_SAVE_STATE=path"
        "void bluewake_save_state_hotkey(bool load)|int main(int argc, char** argv)")
    string(REPLACE "|" ";" _markers "${_bounds}")
    list(GET _markers 0 _begin)
    list(GET _markers 1 _end)
    string(FIND "${_state_host}" "${_begin}" _start)
    string(FIND "${_state_host}" "${_end}" _stop)
    if(_start LESS 0 OR _stop LESS _start)
        message(FATAL_ERROR "Could not locate host state guard section: ${_begin}")
    endif()
    math(EXPR _length "${_stop} - ${_start}")
    string(SUBSTRING "${_state_host}" ${_start} ${_length} _section)
    file(APPEND "${_state_guard}" "${_section}\n")
endforeach()
add_executable(bluewake_host_state_save_failure_test
    "${BLUEWAKE_REPO_ROOT}/tests/host_state_save_failure_test.c"
    "${BLUEWAKE_REPO_ROOT}/tests/host_state_guard_test.c" "${BLUEWAKE_HOST_SRC}/save_state.c")
target_include_directories(bluewake_host_state_save_failure_test PRIVATE
    "${BLUEWAKE_HOST_SRC}" "${CMAKE_CURRENT_BINARY_DIR}" "${GXRUNTIME_DIR}/include")
add_test(NAME bluewake_host_state_save_failure_test COMMAND bluewake_host_state_save_failure_test)
