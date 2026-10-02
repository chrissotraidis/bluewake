/* Implementations for bw_posix_compat.h and the shim headers in include/.
 * The only BlueWake compat source that sees <windows.h>. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "bw_posix_compat.h"
#include "include/dirent.h"
#include "include/dlfcn.h"
#include "include/pthread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* bw_realpath(const char* path, char* resolved) {
    char* out = _fullpath(resolved, path, PATH_MAX);
    if (out == NULL)
        return NULL;
    if (_access(out, 0) != 0) {
        if (resolved == NULL)
            free(out);
        errno = ENOENT;
        return NULL;
    }
    return out;
}

void* memmem(const void* haystack, size_t haystack_size, const void* needle, size_t needle_size) {
    if (needle_size == 0)
        return (void*)haystack;
    if (haystack_size < needle_size)
        return NULL;
    const unsigned char* h = (const unsigned char*)haystack;
    const unsigned char first = *(const unsigned char*)needle;
    const size_t last = haystack_size - needle_size;
    for (size_t i = 0; i <= last; i++) {
        const unsigned char* hit = (const unsigned char*)memchr(h + i, first, last - i + 1);
        if (hit == NULL)
            return NULL;
        i = (size_t)(hit - h);
        if (memcmp(hit, needle, needle_size) == 0)
            return (void*)hit;
    }
    return NULL;
}

int bw_rename(const char* from, const char* to) {
    if (MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return 0;
    errno = GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND
                ? ENOENT
                : EACCES;
    return -1;
}

const char* bw_executable_dir(void) {
    static char dir[MAX_PATH * 4];
    if (dir[0] == '\0') {
        wchar_t wide[MAX_PATH * 2];
        DWORD n = GetModuleFileNameW(NULL, wide, (DWORD)(sizeof wide / sizeof wide[0]));
        if (n == 0 || n >= sizeof wide / sizeof wide[0])
            return "";
        if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, dir, (int)sizeof dir, NULL, NULL) == 0) {
            dir[0] = '\0';
            return "";
        }
        char* slash = strrchr(dir, '\\');
        if (slash != NULL)
            slash[1] = '\0';
    }
    return dir;
}

int bw_setenv(const char* name, const char* value, int overwrite) {
    if (!overwrite && getenv(name) != NULL)
        return 0;
    return _putenv_s(name, value) == 0 ? 0 : -1;
}

int bw_unsetenv(const char* name) {
    return _putenv_s(name, "") == 0 ? 0 : -1;
}

static void filetime_to_timespec(unsigned long long ticks100ns, struct timespec* ts) {
    ts->tv_sec = (time_t)(ticks100ns / 10000000ull);
    ts->tv_nsec = (long)((ticks100ns % 10000000ull) * 100ull);
}

static unsigned long long filetime_ticks(const FILETIME* ft) {
    return ((unsigned long long)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
}

int bw_clock_gettime(clockid_t clock, struct timespec* ts) {
    if (ts == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (clock == CLOCK_MONOTONIC) {
        /* QPF is fixed at boot and identical across processors. Concurrent
         * first readers may query it, but all cache accesses must be atomic;
         * the hot path needs neither a mutex nor another QPF call. There is no
         * other state to publish, so relaxed ordering is sufficient. */
        static long long cached_frequency;
        long long frequency = __atomic_load_n(&cached_frequency, __ATOMIC_RELAXED);
        if (frequency == 0) {
            LARGE_INTEGER measured;
            if (!QueryPerformanceFrequency(&measured) || measured.QuadPart <= 0) {
                errno = EINVAL;
                return -1;
            }
            frequency = measured.QuadPart;
            __atomic_store_n(&cached_frequency, frequency, __ATOMIC_RELAXED);
        }
        LARGE_INTEGER now;
        if (!QueryPerformanceCounter(&now)) {
            errno = EINVAL;
            return -1;
        }
        ts->tv_sec = (time_t)(now.QuadPart / frequency);
        ts->tv_nsec = (long)(((now.QuadPart % frequency) * 1000000000ll) /
                             frequency);
        return 0;
    }
    if (clock == CLOCK_PROCESS_CPUTIME_ID || clock == CLOCK_THREAD_CPUTIME_ID) {
        FILETIME created, exited, kernel, user;
        BOOL ok = clock == CLOCK_PROCESS_CPUTIME_ID
                      ? GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)
                      : GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user);
        if (!ok) {
            errno = EINVAL;
            return -1;
        }
        filetime_to_timespec(filetime_ticks(&kernel) + filetime_ticks(&user), ts);
        return 0;
    }
    if (clock != CLOCK_REALTIME) {
        errno = EINVAL;
        return -1;
    }
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    filetime_to_timespec(filetime_ticks(&ft) - 116444736000000000ull, ts);
    return 0;
}

int bw_nanosleep(const struct timespec* request, struct timespec* remaining) {
    if (remaining != NULL) {
        remaining->tv_sec = 0;
        remaining->tv_nsec = 0;
    }
    if (request == NULL) {
        errno = EINVAL;
        return -1;
    }
    long long ns = (long long)request->tv_sec * 1000000000ll + request->tv_nsec;
    if (ns <= 0)
        return 0;
    /* One high-resolution timer per thread, reused. */
    static _Thread_local HANDLE timer;
    if (timer == NULL)
        timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                       TIMER_ALL_ACCESS);
    if (timer == NULL) {
        Sleep((DWORD)((ns + 999999ll) / 1000000ll));
        return 0;
    }
    LARGE_INTEGER due;
    due.QuadPart = -((ns + 99ll) / 100ll);
    if (!SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE)) {
        Sleep((DWORD)((ns + 999999ll) / 1000000ll));
        return 0;
    }
    WaitForSingleObject(timer, INFINITE);
    return 0;
}

int bw_usleep(unsigned long long usec) {
    struct timespec ts;
    ts.tv_sec = (time_t)(usec / 1000000ull);
    ts.tv_nsec = (long)((usec % 1000000ull) * 1000ull);
    return bw_nanosleep(&ts, NULL);
}

struct tm* bw_localtime_r(const time_t* t, struct tm* out) {
    return localtime_s(out, t) == 0 ? out : NULL;
}

struct tm* bw_gmtime_r(const time_t* t, struct tm* out) {
    return gmtime_s(out, t) == 0 ? out : NULL;
}

/* dirent */
struct BwDir {
    HANDLE find;
    WIN32_FIND_DATAA data;
    int first;
    struct dirent entry;
};

DIR* opendir(const char* path) {
    if (path == NULL || path[0] == '\0') {
        errno = ENOENT;
        return NULL;
    }
    char pattern[MAX_PATH * 4];
    size_t len = strlen(path);
    const char* sep = (path[len - 1] == '/' || path[len - 1] == '\\') ? "" : "\\";
    if (snprintf(pattern, sizeof pattern, "%s%s*", path, sep) >= (int)sizeof pattern) {
        errno = ENAMETOOLONG;
        return NULL;
    }
    DIR* dir = (DIR*)calloc(1, sizeof *dir);
    if (dir == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    dir->find = FindFirstFileA(pattern, &dir->data);
    if (dir->find == INVALID_HANDLE_VALUE) {
        free(dir);
        errno = ENOENT;
        return NULL;
    }
    dir->first = 1;
    return dir;
}

struct dirent* readdir(DIR* dir) {
    if (dir == NULL)
        return NULL;
    if (!dir->first && !FindNextFileA(dir->find, &dir->data))
        return NULL;
    dir->first = 0;
    snprintf(dir->entry.d_name, sizeof dir->entry.d_name, "%s", dir->data.cFileName);
    dir->entry.d_type = (dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
    return &dir->entry;
}

int closedir(DIR* dir) {
    if (dir == NULL)
        return -1;
    FindClose(dir->find);
    free(dir);
    return 0;
}

/* dlfcn */
static _Thread_local char g_dl_error[512];
static _Thread_local int g_dl_error_set;

static void dl_set_error(const char* what, const char* name) {
    DWORD code = GetLastError();
    char message[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0,
                   message, (DWORD)sizeof message, NULL);
    size_t n = strlen(message);
    while (n > 0 && (message[n - 1] == '\n' || message[n - 1] == '\r'))
        message[--n] = '\0';
    snprintf(g_dl_error, sizeof g_dl_error, "%s %s: %s (error %lu)", what, name ? name : "(null)",
             message, (unsigned long)code);
    g_dl_error_set = 1;
}

void* dlopen(const char* path, int flags) {
    (void)flags;
    if (path == NULL)
        return (void*)GetModuleHandleW(NULL);
    wchar_t wide[MAX_PATH * 4];
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, (int)(sizeof wide / sizeof wide[0])) == 0) {
        dl_set_error("cannot convert path", path);
        return NULL;
    }
    /* Resolve the module's own dependencies beside it, not from the cwd. */
    HMODULE module = LoadLibraryExW(wide, NULL,
                                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                        LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (module == NULL)
        module = LoadLibraryW(wide);
    if (module == NULL) {
        dl_set_error("cannot load", path);
        return NULL;
    }
    return (void*)module;
}

void* dlsym(void* handle, const char* name) {
    FARPROC proc = GetProcAddress((HMODULE)handle, name);
    if (proc == NULL) {
        dl_set_error("missing symbol", name);
        return NULL;
    }
    return (void*)proc;
}

int dlclose(void* handle) {
    return FreeLibrary((HMODULE)handle) ? 0 : -1;
}

char* dlerror(void) {
    if (!g_dl_error_set)
        return NULL;
    g_dl_error_set = 0;
    return g_dl_error;
}

/* pthread */
typedef struct BwThreadStart {
    void* (*fn)(void*);
    void* arg;
} BwThreadStart;

static unsigned __stdcall thread_trampoline(void* raw) {
    BwThreadStart start = *(BwThreadStart*)raw;
    free(raw);
    start.fn(start.arg);
    return 0;
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*fn)(void*), void* arg) {
    (void)attr;
    BwThreadStart* start = (BwThreadStart*)malloc(sizeof *start);
    if (start == NULL)
        return ENOMEM;
    start->fn = fn;
    start->arg = arg;
    uintptr_t handle = _beginthreadex(NULL, 0, thread_trampoline, start, 0, NULL);
    if (handle == 0) {
        free(start);
        return EAGAIN;
    }
    *thread = (pthread_t)handle;
    return 0;
}

int pthread_join(pthread_t thread, void** result) {
    if (result != NULL)
        *result = NULL;
    if (WaitForSingleObject((HANDLE)thread, INFINITE) != WAIT_OBJECT_0)
        return EINVAL;
    CloseHandle((HANDLE)thread);
    return 0;
}

int pthread_detach(pthread_t thread) {
    return CloseHandle((HANDLE)thread) ? 0 : EINVAL;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const void* attr) {
    (void)attr;
    InitializeSRWLock((PSRWLOCK)&mutex->lock);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    (void)mutex;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    AcquireSRWLockExclusive((PSRWLOCK)&mutex->lock);
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t* mutex) {
    return TryAcquireSRWLockExclusive((PSRWLOCK)&mutex->lock) ? 0 : EBUSY;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    ReleaseSRWLockExclusive((PSRWLOCK)&mutex->lock);
    return 0;
}
