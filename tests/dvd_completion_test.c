#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "dvd_completion.h"
#include "save_state.h"

int main(void) {
    BluewakeDvdCompletions queue = {0};
    BluewakeDvdCompletion result;
    BluewakeDvdCompletion first = {.ready_cycle = 100, .callback = 0x80001234,
        .file_info = 0x80400000, .length = 5760};
    BluewakeDvdCompletion second = {.ready_cycle = 120, .callback = 0,
        .file_info = 0x80400100, .length = 32};
    assert(bluewake_dvd_enqueue(&queue, first));
    assert(bluewake_dvd_enqueue(&queue, second));
    assert(!bluewake_dvd_take_ready(&queue, 99, true, &result));
    assert(!bluewake_dvd_take_ready(&queue, 100, false, &result));

    // Save accepted work, consume it, then restore: work from the future must
    // disappear and each saved completion must be delivered exactly once.
    const BwStateField fields[] = {{"g_dvd_completions", &queue, sizeof(queue)}};
    uint8_t* blob = NULL;
    uint64_t size = 0;
    assert(bw_state_fields_pack(fields, 1, &blob, &size));
    assert(bluewake_dvd_take_ready(&queue, 100, true, &result));
    assert(result.callback == first.callback && result.length == 5760);
    assert(bluewake_dvd_enqueue(&queue, first));
    memset(&queue, 0, sizeof(queue));
    assert(bw_state_fields_unpack(fields, 1, blob, size, NULL, NULL, NULL));
    free(blob);
    assert(queue.count == 2);
    assert(bluewake_dvd_take_ready(&queue, 100, true, &result));
    assert(result.file_info == first.file_info);
    assert(!bluewake_dvd_take_ready(&queue, 119, true, &result));
    assert(bluewake_dvd_take_ready(&queue, 120, true, &result));
    assert(result.callback == 0 && result.file_info == second.file_info);
    assert(!bluewake_dvd_take_ready(&queue, 999, true, &result));

    // A full queue never overwrites previously accepted callbacks.
    for (unsigned i = 0; i < BLUEWAKE_DVD_COMPLETION_CAPACITY; ++i)
        assert(bluewake_dvd_enqueue(&queue, first));
    assert(!bluewake_dvd_enqueue(&queue, second));
    assert(queue.count == BLUEWAKE_DVD_COMPLETION_CAPACITY);
    for (unsigned i = 0; i < BLUEWAKE_DVD_COMPLETION_CAPACITY; ++i) {
        assert(bluewake_dvd_take_ready(&queue, 100, true, &result));
        assert(result.file_info == first.file_info);
    }
    puts("Deferred DVD completion and saved-queue checks passed.");
    return 0;
}
