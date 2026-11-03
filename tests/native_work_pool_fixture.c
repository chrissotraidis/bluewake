#include "native_work_pool.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#define BLUEWAKE_FIXTURE_EXPORT __declspec(dllexport)
#else
#define BLUEWAKE_FIXTURE_EXPORT __attribute__((visibility("default")))
#endif

enum { ITEM_COUNT = 4099 };

struct fixture_context {
    atomic_uint visits[ITEM_COUNT];
    uint32_t output[ITEM_COUNT];
    uint32_t salt;
};

static void fixture_range(void *opaque, size_t first, size_t end)
{
    struct fixture_context *context = opaque;
    for (size_t index = first; index < end; ++index) {
        atomic_fetch_add_explicit(&context->visits[index], 1u,
                                  memory_order_relaxed);
        context->output[index] = (uint32_t)index ^ context->salt;
    }
}

BLUEWAKE_FIXTURE_EXPORT int bluewake_pool_fixture_run(uint32_t salt)
{
    struct fixture_context context = {.salt = salt};
    bluewake_parallel_range(ITEM_COUNT, 1, fixture_range, &context);
    if (bluewake_parallel_batches() == 0) return 1;
    for (size_t index = 0; index < ITEM_COUNT; ++index) {
        if (atomic_load_explicit(&context.visits[index], memory_order_relaxed) != 1u ||
            context.output[index] != ((uint32_t)index ^ salt))
            return 2;
    }
    return 0;
}
