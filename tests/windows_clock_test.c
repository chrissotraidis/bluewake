/* Native Windows only. No disc, device, preferences or game module. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../windows/compat/bw_posix_compat.h"
#include <assert.h>
#include <windows.h>

enum { THREADS = 16, READS = 10000 };
static HANDLE start_event;
static int ready;
static struct timespec first[THREADS], last[THREADS];

static void valid_time(const struct timespec* value) {
    assert(value->tv_sec >= 0);
    assert(value->tv_nsec >= 0 && value->tv_nsec < 1000000000L);
}

static int compare(const struct timespec* a, const struct timespec* b) {
    if (a->tv_sec != b->tv_sec)
        return a->tv_sec < b->tv_sec ? -1 : 1;
    return (a->tv_nsec > b->tv_nsec) - (a->tv_nsec < b->tv_nsec);
}

static struct timespec counter_time(LARGE_INTEGER ticks, LARGE_INTEGER frequency) {
    struct timespec value;
    value.tv_sec = (time_t)(ticks.QuadPart / frequency.QuadPart);
    value.tv_nsec = (long)((ticks.QuadPart % frequency.QuadPart) * 1000000000LL /
                          frequency.QuadPart);
    return value;
}

static DWORD WINAPI read_clock(LPVOID argument) {
    const size_t index = (size_t)argument;
    __atomic_add_fetch(&ready, 1, __ATOMIC_RELEASE);
    assert(WaitForSingleObject(start_event, 10000) == WAIT_OBJECT_0);
    assert(bw_clock_gettime(CLOCK_MONOTONIC, &first[index]) == 0);
    valid_time(&first[index]);
    struct timespec previous = first[index];
    for (int i = 0; i < READS; ++i) {
        struct timespec current;
        assert(bw_clock_gettime(CLOCK_MONOTONIC, &current) == 0);
        valid_time(&current);
        assert(compare(&current, &previous) >= 0);
        previous = current;
    }
    last[index] = previous;
    return 0;
}

int main(void) {
    LARGE_INTEGER frequency, before, after;
    assert(QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0);
    assert(QueryPerformanceCounter(&before));
    start_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    assert(start_event != NULL);
    HANDLE threads[THREADS];
    for (size_t i = 0; i < THREADS; ++i) {
        threads[i] = CreateThread(NULL, 0, read_clock, (LPVOID)i, 0, NULL);
        assert(threads[i] != NULL);
    }
    while (__atomic_load_n(&ready, __ATOMIC_ACQUIRE) < THREADS)
        Sleep(0);
    assert(SetEvent(start_event));
    assert(WaitForMultipleObjects(THREADS, threads, TRUE, 10000) == WAIT_OBJECT_0);
    assert(QueryPerformanceCounter(&after));
    const struct timespec lower = counter_time(before, frequency);
    const struct timespec upper = counter_time(after, frequency);
    for (size_t i = 0; i < THREADS; ++i) {
        assert(compare(&first[i], &lower) >= 0);
        assert(compare(&last[i], &upper) <= 0);
        assert(CloseHandle(threads[i]));
    }
    assert(CloseHandle(start_event));

    const clockid_t clocks[] = {CLOCK_REALTIME, CLOCK_PROCESS_CPUTIME_ID,
                                CLOCK_THREAD_CPUTIME_ID};
    for (size_t i = 0; i < sizeof clocks / sizeof clocks[0]; ++i) {
        struct timespec value;
        assert(bw_clock_gettime(clocks[i], &value) == 0);
        valid_time(&value);
        if (clocks[i] == CLOCK_REALTIME) {
            const time_t now = time(NULL);
            assert(value.tv_sec >= now - 2 && value.tv_sec <= now + 2);
        }
    }
    struct timespec unchanged = {123, 456};
    errno = 0;
    assert(bw_clock_gettime(-1, &unchanged) == -1 && errno == EINVAL);
    assert(unchanged.tv_sec == 123 && unchanged.tv_nsec == 456);
    errno = 0;
    assert(bw_clock_gettime(CLOCK_MONOTONIC, NULL) == -1 && errno == EINVAL);
    puts("Windows clocks: concurrent first reads, monotonicity, QPC bounds and errors pass");
    return 0;
}
