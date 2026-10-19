#include "native_work_pool.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* POSIX callbacks can update the visitation map concurrently. The Windows
 * backend is serial and uses plain counters so the same source remains usable
 * with Windows C toolchains that do not provide C11 atomics. */
#if defined(_WIN32)
typedef unsigned visit_count;
#define STORE_VISIT(value, count) (*(value) = (count))
#define LOAD_VISIT(value) (*(value))
#define INCREMENT_VISIT(value) (++*(value))
#else
#include <pthread.h>
#include <stdatomic.h>
typedef atomic_uint visit_count;
#define STORE_VISIT(value, count) \
    atomic_store_explicit((value), (count), memory_order_relaxed)
#define LOAD_VISIT(value) \
    atomic_load_explicit((value), memory_order_relaxed)
#define INCREMENT_VISIT(value) \
    atomic_fetch_add_explicit((value), 1u, memory_order_relaxed)
#endif

enum { MAX_ITEMS = 4099, MAX_PARTICIPANTS = 9, ITERATIONS = 1000 };

#if !defined(_WIN32)
struct range_observation {
    pthread_t thread;
    size_t first;
    size_t end;
};
#endif

struct work_context {
    visit_count visits[MAX_ITEMS];
    uint32_t output[MAX_ITEMS];
    uint32_t salt;
#if !defined(_WIN32)
    pthread_mutex_t observations_mutex;
    pthread_t submitting_thread;
    struct range_observation observations[MAX_PARTICIPANTS];
    unsigned observation_count;
    unsigned observation_overflow;
#endif
};

static void fill_range(void *opaque, size_t first, size_t end)
{
    struct work_context *context = opaque;
#if !defined(_WIN32)
    pthread_mutex_lock(&context->observations_mutex);
    if (context->observation_count < MAX_PARTICIPANTS) {
        struct range_observation *observation =
            &context->observations[context->observation_count++];
        observation->thread = pthread_self();
        observation->first = first;
        observation->end = end;
    } else {
        context->observation_overflow = 1;
    }
    pthread_mutex_unlock(&context->observations_mutex);
#endif
    for (size_t index = first; index < end; ++index) {
        INCREMENT_VISIT(&context->visits[index]);
        context->output[index] = (uint32_t)index ^ context->salt;
    }
}

#if !defined(_WIN32)
static int compare_ranges(const void *left, const void *right)
{
    const struct range_observation *a = left;
    const struct range_observation *b = right;
    return a->first < b->first ? -1 : a->first > b->first;
}

static int verify_participants(struct work_context *context, size_t count,
                               unsigned expected_workers)
{
    const unsigned expected_participants = expected_workers + 1;
    if (context->observation_overflow ||
        context->observation_count != expected_participants) {
        fprintf(stderr, "participants: expected=%u observed=%u overflow=%u\n",
                expected_participants, context->observation_count,
                context->observation_overflow);
        return 1;
    }

    struct range_observation ranges[MAX_PARTICIPANTS];
    for (unsigned i = 0; i < context->observation_count; ++i)
        ranges[i] = context->observations[i];
    qsort(ranges, expected_participants, sizeof(*ranges), compare_ranges);

    unsigned caller_ranges = 0;
    for (unsigned i = 0; i < expected_participants; ++i) {
        size_t expected_first = count * i / expected_participants;
        size_t expected_end = count * (i + 1) / expected_participants;
        if (ranges[i].first != expected_first || ranges[i].end != expected_end) {
            fprintf(stderr,
                    "range %u: expected=[%zu,%zu) observed=[%zu,%zu)\n",
                    i, expected_first, expected_end,
                    ranges[i].first, ranges[i].end);
            return 1;
        }
        if (pthread_equal(ranges[i].thread, context->submitting_thread)) {
            ++caller_ranges;
            if (i != expected_workers) {
                fprintf(stderr, "caller handled range %u, expected %u\n",
                        i, expected_workers);
                return 1;
            }
        }
        for (unsigned earlier = 0; earlier < i; ++earlier) {
            if (pthread_equal(ranges[i].thread, ranges[earlier].thread)) {
                fprintf(stderr, "ranges %u and %u used the same participant\n",
                        earlier, i);
                return 1;
            }
        }
    }
    if (caller_ranges != 1) {
        fprintf(stderr, "caller ranges: expected=1 observed=%u\n", caller_ranges);
        return 1;
    }
    return 0;
}
#endif

static int run_case(struct work_context *context, size_t count,
                    size_t grain, uint32_t salt)
{
    /* Reset the full allocation so entries outside the submitted range serve
     * as sentinels for overrun and incorrect partition boundaries. */
    for (size_t index = 0; index < MAX_ITEMS; ++index) {
        STORE_VISIT(&context->visits[index], 0u);
        context->output[index] = UINT32_C(0xdeadbeef);
    }
    context->salt = salt;
#if !defined(_WIN32)
    context->submitting_thread = pthread_self();
    context->observation_count = 0;
    context->observation_overflow = 0;
#endif

    bluewake_parallel_range(count, grain, fill_range, context);

    /* Returning from bluewake_parallel_range is the publication boundary: all
     * submitted entries must be visible exactly once, with sentinels intact. */
    for (size_t index = 0; index < count; ++index) {
        unsigned visits = LOAD_VISIT(&context->visits[index]);
        if (visits != 1u || context->output[index] != ((uint32_t)index ^ salt)) {
            fprintf(stderr, "index %zu: visits=%u output=%08x\n",
                    index, visits, context->output[index]);
            return 1;
        }
    }
    for (size_t index = count; index < MAX_ITEMS; ++index) {
        if (LOAD_VISIT(&context->visits[index]) != 0u ||
            context->output[index] != UINT32_C(0xdeadbeef)) {
            fprintf(stderr, "sentinel changed at index %zu\n", index);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    unsigned expected_workers = argc == 2 ? (unsigned)strtoul(argv[1], NULL, 10) : 0;
    struct work_context *context = calloc(1, sizeof(*context));
    if (!context) {
        fputs("allocation failed\n", stderr);
        return 1;
    }
#if !defined(_WIN32)
    if (pthread_mutex_init(&context->observations_mutex, NULL) != 0) {
        fputs("observation mutex initialization failed\n", stderr);
        free(context);
        return 1;
    }
#endif
    if (bluewake_parallel_workers() != expected_workers) {
        fprintf(stderr, "effective workers: got %u want %u\n",
                bluewake_parallel_workers(), expected_workers);
#if !defined(_WIN32)
        pthread_mutex_destroy(&context->observations_mutex);
#endif
        free(context);
        return 1;
    }

    /* Exercise empty, below-grain serial, and uneven parallel partitions before
     * repeatedly replacing the context contents and submitted range. */
    unsigned long long batches_before = bluewake_parallel_batches();
    if (run_case(context, 0, 1, 1) ||
        run_case(context, 17, 256, 2) ||
        run_case(context, MAX_ITEMS, 1, 3)) {
#if !defined(_WIN32)
        pthread_mutex_destroy(&context->observations_mutex);
#endif
        free(context);
        return 1;
    }
#if !defined(_WIN32)
    if (verify_participants(context, MAX_ITEMS, expected_workers)) {
        pthread_mutex_destroy(&context->observations_mutex);
        free(context);
        return 1;
    }
#endif
    for (unsigned iteration = 0; iteration < ITERATIONS; ++iteration) {
        size_t count = 257u + iteration % (MAX_ITEMS - 256u);
        if (run_case(context, count, 1, UINT32_C(0x9e3779b9) ^ iteration)) {
#if !defined(_WIN32)
            pthread_mutex_destroy(&context->observations_mutex);
#endif
            free(context);
            return 1;
        }
    }

    /* The public batch counter proves that worker-enabled test processes took
     * the parallel path, while invalid and zero configurations stayed serial. */
    unsigned long long batches_after = bluewake_parallel_batches();
    if ((expected_workers == 0 && batches_after != batches_before) ||
        (expected_workers != 0 && batches_after <= batches_before)) {
        fprintf(stderr, "unexpected batch count: %llu -> %llu\n",
                batches_before, batches_after);
#if !defined(_WIN32)
        pthread_mutex_destroy(&context->observations_mutex);
#endif
        free(context);
        return 1;
    }

#if !defined(_WIN32)
    pthread_mutex_destroy(&context->observations_mutex);
#endif
    free(context);
    return 0;
}
