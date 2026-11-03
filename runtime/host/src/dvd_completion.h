#ifndef BLUEWAKE_DVD_COMPLETION_H
#define BLUEWAKE_DVD_COMPLETION_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define BLUEWAKE_DVD_COMPLETION_CAPACITY 32u

typedef struct BluewakeDvdCompletion {
    uint64_t ready_cycle;
    uint32_t callback;
    uint32_t file_info;
    uint32_t length;
    uint32_t reserved;
} BluewakeDvdCompletion;

// Plain data so accepted reads survive save/restore with the guest clock.
typedef struct BluewakeDvdCompletions {
    BluewakeDvdCompletion entries[BLUEWAKE_DVD_COMPLETION_CAPACITY];
    uint32_t count;
    uint32_t reserved;
} BluewakeDvdCompletions;

static inline bool bluewake_dvd_enqueue(BluewakeDvdCompletions* queue,
                                       BluewakeDvdCompletion completion) {
    if (queue->count >= BLUEWAKE_DVD_COMPLETION_CAPACITY)
        return false;
    queue->entries[queue->count++] = completion;
    return true;
}

static inline bool bluewake_dvd_take_ready(BluewakeDvdCompletions* queue,
                                          uint64_t cycle, bool interrupt_safe,
                                          BluewakeDvdCompletion* out) {
    if (!interrupt_safe || queue->count == 0u ||
        queue->count > BLUEWAKE_DVD_COMPLETION_CAPACITY ||
        cycle < queue->entries[0].ready_cycle)
        return false;
    *out = queue->entries[0];
    --queue->count;
    memmove(queue->entries, queue->entries + 1,
            queue->count * sizeof(queue->entries[0]));
    memset(&queue->entries[queue->count], 0, sizeof(queue->entries[0]));
    return true;
}

#endif
