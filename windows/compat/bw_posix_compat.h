/* POSIX compatibility for BlueWake's host and tools on Windows (MSVC ABI).
 *
 * Force-included (-include) into BlueWake's own C and C++ sources when building
 * for Windows, with the shim headers in include/ (unistd.h, dirent.h,
 * strings.h, pthread.h, dlfcn.h). It covers only what those sources use: files
 * and directories, clocks and sleeps, the environment, threads and loading the
 * game module. Third-party code (Aurora, Dawn, SDL, Dolphin's DSP) builds with
 * its own Windows support instead.
 *
 * Deliberately free of <windows.h>: its macros would leak into every source
 * file. The implementations live in bw_posix_compat.c.
 */
#ifndef BW_POSIX_COMPAT_H
#define BW_POSIX_COMPAT_H

#if defined(_WIN32)

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <direct.h>
#include <errno.h>
#include <io.h>
#include <process.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Types. MSVC's off_t is a 32-bit long; file offsets are 64-bit here. */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#endif
typedef long long bw_off_t;
#define off_t bw_off_t
#define fseeko(f, o, w) _fseeki64((f), (long long)(o), (w))
#define ftello(f) _ftelli64(f)

/* Files and directories. */
#ifndef F_OK
#define F_OK 0
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef X_OK
#define X_OK 0 /* Windows has no execute bit; existence is the test */
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#define mkdir(path, mode) _mkdir(path)

char* bw_realpath(const char* path, char* resolved);
#define realpath(path, resolved) bw_realpath((path), (resolved))

/* POSIX rename replaces an existing file atomically; the CRT's fails instead.
 * The memory card is saved to GZLE01.card.tmp and renamed over the card, so
 * this keeps a save atomic on Windows too. */
int bw_rename(const char* from, const char* to);
#define rename(from, to) bw_rename((from), (to))

/* memmem, the GNU and BSD search for bytes within bytes. */
void* memmem(const void* haystack, size_t haystack_size, const void* needle, size_t needle_size);

/* The directory holding the running executable, with a trailing separator. */
const char* bw_executable_dir(void);

/* Environment. */
int bw_setenv(const char* name, const char* value, int overwrite);
int bw_unsetenv(const char* name);
#define setenv(name, value, overwrite) bw_setenv((name), (value), (overwrite))
#define unsetenv(name) bw_unsetenv(name)

/* Clocks. CLOCK_MONOTONIC is the performance counter; CLOCK_REALTIME is the
 * time since 1970; the CPU-time clocks come from the process or thread times. */
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
#ifndef CLOCK_PROCESS_CPUTIME_ID
#define CLOCK_PROCESS_CPUTIME_ID 2
#endif
#ifndef CLOCK_THREAD_CPUTIME_ID
#define CLOCK_THREAD_CPUTIME_ID 3
#endif
#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#endif
#ifndef CLOCK_UPTIME_RAW
#define CLOCK_UPTIME_RAW CLOCK_MONOTONIC
#endif
typedef int clockid_t;
int bw_clock_gettime(clockid_t clock, struct timespec* ts);
#define clock_gettime(clock, ts) bw_clock_gettime((clock), (ts))

/* Sleeps use a high-resolution waitable timer: Sleep() rounds up to the
 * system tick (often 15.6 ms), far too coarse for 30 FPS pacing. */
int bw_nanosleep(const struct timespec* request, struct timespec* remaining);
int bw_usleep(unsigned long long usec);
#define nanosleep(request, remaining) bw_nanosleep((request), (remaining))
#define usleep(usec) bw_usleep(usec)

struct tm* bw_localtime_r(const time_t* t, struct tm* out);
struct tm* bw_gmtime_r(const time_t* t, struct tm* out);
#define localtime_r(t, out) bw_localtime_r((t), (out))
#define gmtime_r(t, out) bw_gmtime_r((t), (out))

#ifdef __cplusplus
}
#endif

#endif /* _WIN32 */
#endif /* BW_POSIX_COMPAT_H */
