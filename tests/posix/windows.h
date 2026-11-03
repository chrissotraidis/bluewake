/* Shim: the native_* tests include <windows.h>; on Linux, resolve it to the
 * POSIX loader shim so the identical test source compiles unmodified. */
#ifndef BW_POSIX_WINDOWS_SHIM_H
#define BW_POSIX_WINDOWS_SHIM_H
#include "bw_posix_test_shim.h"
#endif
