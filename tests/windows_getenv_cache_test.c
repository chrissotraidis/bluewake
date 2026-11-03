/* Synthetic-only: no disc, preferences, saves or translated game code. */
#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
/* Match the runtime's CRT policy: this test intentionally calls getenv. */
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define NATIVE_WINDOWS 1
#else
#include <pthread.h>
#include <sched.h>
#define NATIVE_WINDOWS 0
#endif

enum { THREADS = 16 };
static int race_mode, copies, fail_copy;

static void yield_thread(void) {
#if NATIVE_WINDOWS
    Sleep(0);
#else
    sched_yield();
#endif
}

static char* test_strdup(const char* source) {
    if (fail_copy)
        return NULL;
    const size_t bytes = strlen(source) + 1;
    char* copy = malloc(bytes);
    assert(copy != NULL);
    memcpy(copy, source, bytes);
    if (race_mode) {
        __atomic_add_fetch(&copies, 1, __ATOMIC_RELEASE);
        /* Every contender allocates before any may publish its answer. */
        while (__atomic_load_n(&copies, __ATOMIC_ACQUIRE) < THREADS)
            yield_thread();
    }
    return copy;
}

/* Exercise the exact Windows macro on macOS as well as native Windows. */
#if !NATIVE_WINDOWS
#define _WIN32 1
#endif
#define _strdup test_strdup
#include "../windows/compat/bw_getenv_cache.h"

static void set_variable(const char* name, const char* value) {
#if NATIVE_WINDOWS
    assert(_putenv_s(name, value == NULL ? "" : value) == 0);
#else
    if (value == NULL)
        assert(unsetenv(name) == 0);
    else
        assert(setenv(name, value, 1) == 0);
#endif
}

static char* missing(void) { return getenv("BW_CACHE_TEST_MISSING"); }
static char* stable(void) { return getenv("BW_CACHE_TEST_VALUE"); }
static char* independent(void) { return getenv("BW_CACHE_TEST_VALUE"); }
static char* retry(void) { return getenv("BW_CACHE_TEST_RETRY"); }
static char* concurrent(void) { return getenv("BW_CACHE_TEST_RACE"); }
static char* results[THREADS];

#if NATIVE_WINDOWS
static DWORD WINAPI read_concurrently(LPVOID index) {
#else
static void* read_concurrently(void* index) {
#endif
    results[(size_t)index] = concurrent();
    return 0;
}

int main(void) {
    set_variable("BW_CACHE_TEST_MISSING", NULL);
    assert(missing() == NULL);
    set_variable("BW_CACHE_TEST_MISSING", "later");
    assert(missing() == NULL); // cache an absent value, too
    set_variable("BW_CACHE_TEST_VALUE", "original");
    char* first = stable();
    assert(first != NULL && strcmp(first, "original") == 0);
    set_variable("BW_CACHE_TEST_VALUE", "changed");
    assert(stable() == first && strcmp(first, "original") == 0);
    assert(strcmp(independent(), "changed") == 0); // independent call site

    set_variable("BW_CACHE_TEST_RETRY", "retry");
    fail_copy = 1;
    assert(retry() == NULL);
    fail_copy = 0;
    assert(strcmp(retry(), "retry") == 0); // OOM did not poison the cache

    set_variable("BW_CACHE_TEST_RACE", "shared");
    race_mode = 1;
#if NATIVE_WINDOWS
    HANDLE threads[THREADS];
    for (size_t i = 0; i < THREADS; ++i) {
        threads[i] = CreateThread(NULL, 0, read_concurrently, (LPVOID)i, 0, NULL);
        assert(threads[i] != NULL);
    }
    assert(WaitForMultipleObjects(THREADS, threads, TRUE, 10000) == WAIT_OBJECT_0);
    for (size_t i = 0; i < THREADS; ++i)
        CloseHandle(threads[i]);
#else
    pthread_t threads[THREADS];
    for (size_t i = 0; i < THREADS; ++i)
        assert(pthread_create(&threads[i], NULL, read_concurrently, (void*)i) == 0);
    for (size_t i = 0; i < THREADS; ++i)
        assert(pthread_join(threads[i], NULL) == 0);
#endif
    for (size_t i = 0; i < THREADS; ++i) {
        assert(results[i] == results[0]); // one published allocation, no race
        assert(strcmp(results[i], "shared") == 0);
    }
    assert(copies == THREADS);
    assert(concurrent() == results[0]); // no extra getenv/strdup on the hot path
    puts("Windows getenv cache tests passed (16 simultaneous first reads).");
    return 0;
}
