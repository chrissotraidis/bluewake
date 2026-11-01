#pragma once
/*
 * <execinfo.h> for Android builds below API 33.
 *
 * bionic's backtrace(), backtrace_symbols() and backtrace_symbols_fd() live in
 * libc's LIBC_T version (AOSP's bionic/libc/libc.map.txt, introduced in Android
 * 13, API 33), and <execinfo.h> declares them only when the NDK compiles for
 * that platform or newer. Built for Android 10 a caller does not compile ("use
 * of undeclared identifier 'backtrace'"); built for 13 it imports
 * backtrace@LIBC_T, a symbol Android 10's libc does not have, so the app is
 * refused when it loads.
 *
 * GXRuntime's Aurora backend includes <execinfo.h> (backends/aurora/
 * aurora_graphics.cpp) for DOL_GX_STALL_STACKS, the watchdog that writes the GX
 * worker's and the render worker's call stacks to stderr when a batch has been
 * stalled for 200 ms. RecompCore is pinned by BlueWake, so this header stands in
 * for bionic's on the older platforms: the same three functions, over the
 * unwinder every Android build already links, printing a stack in the shape
 * glibc's does. An API 33 or newer build gets bionic's own header and its own
 * implementation. android/CMakeLists.txt puts this directory first on the
 * include path.
 */
#include <android/api-level.h>

#if !defined(__ANDROID__) || __ANDROID_API__ >= 33

#include_next <execinfo.h>

#else

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unwind.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bw_backtrace_state {
    void** frames;
    int size;
    int count;
} bw_backtrace_state;

static inline _Unwind_Reason_Code bw_backtrace_step(struct _Unwind_Context* context, void* arg) {
    bw_backtrace_state* state = (bw_backtrace_state*)arg;
    if (state->count >= state->size) {
        return _URC_END_OF_STACK;
    }
    /* The caller's frame: _Unwind_GetIP returns the return address, which is
       already inside the calling function (as bionic's backtrace has it). */
    const uintptr_t pc = (uintptr_t)_Unwind_GetIP(context);
    if (pc != 0) {
        state->frames[state->count++] = (void*)pc;
    }
    return _URC_NO_REASON;
}

/* One frame, as "module(symbol+0xoffset) [address]", into a fixed buffer: the
   watchdog runs this from a signal handler. */
static inline void bw_backtrace_frame(void* address, char* out, size_t size) {
    Dl_info info;
    if (address != NULL && dladdr(address, &info) != 0) {
        if (info.dli_sname != NULL) {
            snprintf(out, size, "%s(%s+0x%lx) [%p]", info.dli_fname != NULL ? info.dli_fname : "?",
                     info.dli_sname, (unsigned long)((uintptr_t)address - (uintptr_t)info.dli_saddr),
                     address);
            return;
        }
        if (info.dli_fname != NULL) {
            snprintf(out, size, "%s(+0x%lx) [%p]", info.dli_fname,
                     (unsigned long)((uintptr_t)address - (uintptr_t)info.dli_fbase), address);
            return;
        }
    }
    snprintf(out, size, "[%p]", address);
}

static inline int backtrace(void** buffer, int size) {
    if (buffer == NULL || size <= 0) {
        return 0;
    }
    bw_backtrace_state state = {buffer, size, 0};
    _Unwind_Backtrace(bw_backtrace_step, &state);
    return state.count;
}

static inline char** backtrace_symbols(void* const* buffer, int size) {
    static const size_t kLine = 256;
    if (buffer == NULL || size <= 0) {
        return NULL;
    }
    char** list = (char**)malloc((size_t)size * (sizeof(char*) + kLine));
    if (list == NULL) {
        return NULL;
    }
    char* text = (char*)(list + size);
    for (int i = 0; i < size; ++i) {
        list[i] = text + (size_t)i * kLine;
        bw_backtrace_frame(buffer[i], list[i], kLine);
    }
    return list;
}

static inline void backtrace_symbols_fd(void* const* buffer, int size, int fd) {
    if (buffer == NULL || size <= 0) {
        return;
    }
    for (int i = 0; i < size; ++i) {
        char frame[256];
        char line[288];
        bw_backtrace_frame(buffer[i], frame, sizeof frame);
        const int n = snprintf(line, sizeof line, "  #%02d %s\n", i, frame);
        if (n <= 0) {
            continue;
        }
        size_t written = 0;
        const size_t total = (size_t)n < sizeof line ? (size_t)n : sizeof line - 1;
        while (written < total) {
            const ssize_t w = write(fd, line + written, total - written);
            if (w <= 0) {
                return;
            }
            written += (size_t)w;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif
