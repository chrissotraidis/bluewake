/* getenv, remembered per call site, for GXRuntime's C runtime on Windows.
 *
 * The Windows C runtime's getenv takes a lock and scans the whole environment
 * block. GXRuntime reads trace switches on hot paths: ppc_take_exception checks
 * one on every guest exception, which was 4 percent of the game thread in a
 * profile at Outset, where macOS's getenv costs next to nothing.
 *
 * BlueWake sets its variables before the game starts, so each call site keeps
 * its first answer (copied, so a later change to the environment cannot leave
 * it dangling). That is only right where every call names a literal variable:
 * force-include this into GXRuntime's src/ and the game module's runtime
 * sources, never into code that passes a variable name.
 */
#ifndef BW_GETENV_CACHE_H
#define BW_GETENV_CACHE_H

#if defined(_WIN32)
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BW_GETENV_UNSET ((char*)(~(uintptr_t)0))
/* Clang's atomics work in both the C runtime and translated C sources. A
 * volatile pointer was not sufficient: concurrent first reads could publish
 * different allocations (and race with readers). Release/acquire publishes
 * the copied string, with one cached answer and no CRT lock after first use.
 */
#define getenv(name)                                                        \
    (__extension__({                                                        \
        static char* bw_getenv_value_ = BW_GETENV_UNSET;                     \
        char* bw_getenv_result_ =                                           \
            __atomic_load_n(&bw_getenv_value_, __ATOMIC_ACQUIRE);            \
        if (bw_getenv_result_ == BW_GETENV_UNSET) {                          \
            const char* bw_getenv_source_ = (getenv)(name);                  \
            bw_getenv_result_ = bw_getenv_source_ == NULL                    \
                ? NULL : _strdup(bw_getenv_source_);                         \
            /* Do not permanently cache an allocation failure as unset. */ \
            if (bw_getenv_source_ == NULL || bw_getenv_result_ != NULL) {    \
                char* bw_getenv_expected_ = BW_GETENV_UNSET;                 \
                if (!__atomic_compare_exchange_n(                          \
                        &bw_getenv_value_, &bw_getenv_expected_,            \
                        bw_getenv_result_, 0,                              \
                        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {              \
                    free(bw_getenv_result_);                                \
                    bw_getenv_result_ = bw_getenv_expected_;                \
                }                                                          \
            }                                                              \
        }                                                                  \
        bw_getenv_result_;                                                  \
    }))
#endif

#endif /* BW_GETENV_CACHE_H */
