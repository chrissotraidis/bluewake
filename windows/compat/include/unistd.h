/* <unistd.h> for BlueWake on Windows: see ../bw_posix_compat.h. */
#ifndef BW_COMPAT_UNISTD_H
#define BW_COMPAT_UNISTD_H
#include "../bw_posix_compat.h"
#include <direct.h>
#include <io.h>
#include <process.h>
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif
#endif
