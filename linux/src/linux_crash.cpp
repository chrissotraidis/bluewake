// SPDX-License-Identifier: GPL-3.0-or-later
// BlueWake for Linux: the crash reporter (the POSIX counterpart of
// windows/src/win_crash.cpp). A fatal signal or std::terminate writes a
// [crash] report with the faulting address and a short stack walk into
// <data_dir>/logs/crash-<pid>.log, then re-raises the signal so the default
// action (core dump, the shell's report) still happens.
#define _GNU_SOURCE
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <unistd.h>
#include <ucontext.h>

#if defined(__GLIBC__)
#include <execinfo.h>
#define BW_HAVE_BACKTRACE 1
#else
#define BW_HAVE_BACKTRACE 0
#endif

#include "linux_crash.h"
#include "crash_text.h"

static char g_crash_path[4096];

static void write_bytes(const char* bytes, unsigned size) {
    FILE* f = fopen(g_crash_path, "a");
    if (f == NULL) return;
    fwrite(bytes, 1, size, f);
    fflush(f);
    fclose(f);
}

static void literal(const char* value) {
    write_bytes(value, (unsigned)strlen(value));
}

static void hex(uint64_t value) {
    char bytes[18];
    write_bytes(bytes, bw_crash_hex(bytes, value));
}

static void report(const char* reason, int sig, void* pc) {
    literal("[crash] "); literal(reason); literal(" signal=");
    char num[32]; snprintf(num, sizeof num, "%d", sig); literal(num);
    literal(" pc="); hex((uint64_t)(uintptr_t)pc); literal("\n");
#if BW_HAVE_BACKTRACE
    void* frames[32];
    int count = backtrace(frames, 32);
    char** names = backtrace_symbols(frames, count);
    for (int i = 0; i < count; ++i) {
        literal("[crash] frame="); snprintf(num, sizeof num, "%d", i); literal(num);
        literal(" "); literal(names != NULL && names[i] != NULL ? names[i] : "?"); literal("\n");
    }
    free(names);
#endif
}

static void* fault_pc(ucontext_t* context) {
#if defined(__x86_64__)
    return (void*)context->uc_mcontext.gregs[REG_RIP];
#elif defined(__aarch64__)
    return (void*)context->uc_mcontext.pc;
#else
    (void)context;
    return NULL;
#endif
}

static void handle_fatal(int sig, siginfo_t*, void* context) {
    report("signal", sig, context != NULL ? fault_pc((ucontext_t*)context) : NULL);
    struct sigaction dfl;
    memset(&dfl, 0, sizeof dfl);
    dfl.sa_handler = SIG_DFL;
    sigaction(sig, &dfl, NULL);
    raise(sig);
}

static void terminate_handler() {
    report("terminate", 0, NULL);
    std::abort();
}

extern "C" void bw_crash_install(const char* data_dir) {
    snprintf(g_crash_path, sizeof g_crash_path, "%slogs/crash-%ld.log", data_dir, (long)getpid());
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_fatal;
    const int signals[] = {SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGFPE};
    for (int sig : signals) sigaction(sig, &sa, NULL);
    std::set_terminate(terminate_handler);
    fprintf(stderr, "[crash] reports=%s\n", g_crash_path);
}

extern "C" void bw_crash_test(void) {
    const char* test = getenv("BLUEWAKE_CRASH_TEST");
    if (test == NULL) return;
    if (strcmp(test, "null") == 0) {
        void (*volatile call)() = NULL; call();
    } else if (strcmp(test, "abort") == 0) {
        std::abort();
    }
}
