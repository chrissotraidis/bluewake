// SPDX-License-Identifier: GPL-3.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <exception>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "win_crash.h"
#include "crash_text.h"

static HANDLE crash_file = INVALID_HANDLE_VALUE;
static volatile LONG reporting;
static void write_bytes(const char* bytes, unsigned size) {
    if (crash_file != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(crash_file, bytes, size, &written, nullptr);
    }
}
static void literal(const char* value) {
    unsigned size = 0; while (value[size]) ++size;
    write_bytes(value, size);
}
static void hex(ULONG64 value) {
    char bytes[18]; unsigned size = bw_crash_hex(bytes, value);
    write_bytes(bytes, size);
}
static void report_context(const char* reason, DWORD code, CONTEXT context) {
    if (InterlockedExchange(&reporting, 1)) return;
    literal("[crash] "); literal(reason); literal(" code="); hex(code);
    literal(" rip="); hex(context.Rip); literal(" rsp="); hex(context.Rsp); literal("\r\n");
    ULONG_PTR lo, hi; GetCurrentThreadStackLimits(&lo, &hi);
    __try {
        // A call through null leaves its return address on the stack.
        if (context.Rip == 0 && context.Rsp >= lo && context.Rsp <= hi - 8) {
            context.Rip = *reinterpret_cast<DWORD64*>(context.Rsp);
            context.Rsp += 8;
            literal("[crash] null-call caller="); hex(context.Rip); literal("\r\n");
        }
        for (unsigned frame = 0; frame < 16 && context.Rip; ++frame) {
            literal("[crash] frame="); hex(frame); literal(" pc="); hex(context.Rip);
            DWORD64 image = 0;
            PRUNTIME_FUNCTION function = RtlLookupFunctionEntry(context.Rip, &image, nullptr);
            literal(" image="); hex(image); literal(" offset="); hex(context.Rip - image); literal("\r\n");
            DWORD64 previous = context.Rsp;
            if (context.Rsp < lo || context.Rsp > hi - 8) break;
            if (!function) {
                context.Rip = *reinterpret_cast<DWORD64*>(context.Rsp); context.Rsp += 8;
            } else {
                PVOID handler_data; DWORD64 establisher;
                RtlVirtualUnwind(UNW_FLAG_NHANDLER, image, context.Rip, function, &context,
                    &handler_data, &establisher, nullptr);
            }
            if (context.Rsp <= previous || context.Rsp > hi) break;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        literal("[crash] stack unavailable\r\n");
    }
    if (crash_file != INVALID_HANDLE_VALUE) FlushFileBuffers(crash_file);
}
static LONG WINAPI unhandled(EXCEPTION_POINTERS* info) {
    report_context("exception", info->ExceptionRecord->ExceptionCode, *info->ContextRecord);
    TerminateProcess(GetCurrentProcess(), info->ExceptionRecord->ExceptionCode);
    return EXCEPTION_EXECUTE_HANDLER;
}
static void fatal(const char* reason, DWORD code) {
    CONTEXT context{}; RtlCaptureContext(&context);
    report_context(reason, code, context);
    TerminateProcess(GetCurrentProcess(), code);
}
extern "C" void bw_crash_install(const char* data_dir) {
    char path[MAX_PATH * 4];
    std::snprintf(path, sizeof path, "%slogs\\crash-%lu.log", data_dir, GetCurrentProcessId());
    crash_file = CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    ULONG guarantee = 32768;
    if (!SetThreadStackGuarantee(&guarantee)) std::fprintf(stderr, "[crash] stack guarantee unavailable: %lu\n", GetLastError());
    if (crash_file == INVALID_HANDLE_VALUE) std::fprintf(stderr, "[crash] report file unavailable: %lu\n", GetLastError());
    else std::fprintf(stderr, "[crash] reports=%s\n", path);
    SetUnhandledExceptionFilter(unhandled);
    std::set_terminate([] { fatal("std::terminate", 0xE0420001); });
    std::signal(SIGABRT, [](int) { fatal("SIGABRT", 0xE0420002); });
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
}
extern "C" void bw_crash_test(void) {
#if BLUEWAKE_ENABLE_DEVELOPER_TRACING
    const char* test = std::getenv("BLUEWAKE_CRASH_TEST");
    if (!test) return;
    if (std::strcmp(test, "null") == 0) {
        // Volatile keeps this an actual indirect call, not UB folded by the compiler.
        void (*volatile call)() = nullptr; call();
    } else if (std::strcmp(test, "abort") == 0) std::abort();
    else if (std::strcmp(test, "terminate") == 0) std::terminate();
#endif
}
