/* BlueWake native test harness: POSIX shim for the Windows loader API.
 *
 * The native_* tests (native_search_test.c, native_mtxcalc_test.c, ...) were
 * written against the Windows game module (gGZLE01_recomp.dll) and use the
 * Windows loader + memory-protection surface. This header provides the same
 * surface on POSIX so the exact same test sources run against a Linux
 * gGZLE01_recomp.so.
 *
 * The tests do `#include <windows.h>`; a windows.h shim on the include path
 * resolves to this file, so no test source edits are required.
 */
#ifndef BW_POSIX_TEST_SHIM_H
#define BW_POSIX_TEST_SHIM_H

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HMODULE;
typedef uint32_t DWORD;
typedef uint32_t UINT;
typedef uint16_t WORD;
typedef uint8_t BYTE;

typedef struct { long long QuadPart; } LARGE_INTEGER;

static __thread char s_bw_last_error[256];

static inline HMODULE LoadLibraryA(const char* path) {
    HMODULE h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (h == NULL) {
        const char* e = dlerror();
        snprintf(s_bw_last_error, sizeof s_bw_last_error, "%s", e ? e : "dlopen failed");
    }
    return h;
}
static inline void* GetProcAddress(HMODULE h, const char* name) {
    return dlsym(h, name);
}
static inline int FreeLibrary(HMODULE h) {
    return dlclose(h) == 0;
}
static inline DWORD GetLastError(void) {
    (void)0;
    return (DWORD)0;
}
static inline const char* bw_last_error(void) {
    return s_bw_last_error;
}

#define MEM_RESERVE 0x00002000u
#define MEM_COMMIT 0x00001000u
#define MEM_RELEASE 0x00008000u
#define PAGE_READONLY 0x02u
#define PAGE_READWRITE 0x04u

static inline void* VirtualAlloc(void* addr, size_t size, DWORD alloc_type, DWORD protect) {
    (void)alloc_type; (void)protect;
    void* p = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return p == MAP_FAILED ? NULL : p;
}
static inline int VirtualFree(void* addr, size_t size, DWORD free_type) {
    (void)free_type;
    return munmap(addr, size) == 0;
}
static inline int VirtualProtect(void* addr, size_t size, DWORD protect, DWORD* old) {
    (void)old;
    int prot = PROT_NONE;
    if (protect == PAGE_READONLY) prot = PROT_READ;
    else if (protect == PAGE_READWRITE) prot = PROT_READ | PROT_WRITE;
    return mprotect(addr, size, prot) == 0;
}

static inline void QueryPerformanceFrequency(LARGE_INTEGER* freq) {
    freq->QuadPart = 1000000000LL;
}
static inline void QueryPerformanceCounter(LARGE_INTEGER* t) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    t->QuadPart = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static inline int _putenv_s(const char* name, const char* value) {
    return setenv(name, value, 1);
}

#ifdef __cplusplus
}
#endif

#endif /* BW_POSIX_TEST_SHIM_H */
