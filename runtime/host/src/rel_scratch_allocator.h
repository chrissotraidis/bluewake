#ifndef BLUEWAKE_REL_SCRATCH_ALLOCATOR_H
#define BLUEWAKE_REL_SCRATCH_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct BlueWakeScratchRange {
    uint32_t start;
    uint32_t size;
} BlueWakeScratchRange;

bool bluewake_rel_scratch_first_fit(uint32_t base, uint32_t limit,
                                    uint32_t size,
                                    const BlueWakeScratchRange* occupied,
                                    size_t occupied_count,
                                    uint32_t* address);

#endif
