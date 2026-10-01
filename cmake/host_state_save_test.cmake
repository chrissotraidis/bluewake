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
add_executable(bluewake_host_state_save_failure_test
    "${BLUEWAKE_REPO_ROOT}/tests/host_state_save_failure_test.c" "${BLUEWAKE_HOST_SRC}/save_state.c")
target_include_directories(bluewake_host_state_save_failure_test PRIVATE
    "${BLUEWAKE_HOST_SRC}" "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME bluewake_host_state_save_failure_test COMMAND bluewake_host_state_save_failure_test)
