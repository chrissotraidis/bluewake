#include "rel_scratch_allocator.h"

static uint32_t align32(uint32_t value) {
    return (value + 31u) & ~31u;
}

bool bluewake_rel_scratch_first_fit(uint32_t base, uint32_t limit,
                                    uint32_t size,
                                    const BlueWakeScratchRange* occupied,
                                    size_t occupied_count,
                                    uint32_t* address) {
    if (address == NULL || size == 0u || base >= limit)
        return false;

    const uint32_t aligned_size = align32(size);
    uint32_t candidate = align32(base);
    while ((uint64_t)candidate + aligned_size <= limit) {
        uint32_t next = candidate;
        const uint64_t candidate_end = (uint64_t)candidate + aligned_size;
        for (size_t i = 0; i < occupied_count; ++i) {
            const BlueWakeScratchRange range = occupied[i];
            if (range.size == 0u)
                continue;
            const uint64_t range_end = (uint64_t)range.start + range.size;
            if ((uint64_t)candidate < range_end &&
                candidate_end > range.start && range_end > next) {
                if (range_end > UINT32_MAX)
                    return false;
                next = (uint32_t)range_end;
            }
        }
        if (next == candidate) {
            *address = candidate;
            return true;
        }
        candidate = align32(next);
    }
    return false;
}
