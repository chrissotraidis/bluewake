#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "rel_scratch_allocator.h"

static void test_reuses_first_hole(void) {
    const BlueWakeScratchRange occupied[] = {
        {0x1800u, 0x400u},
        {0x1000u, 0x200u},
    };
    uint32_t address = 0u;
    assert(bluewake_rel_scratch_first_fit(0x1000u, 0x2000u, 0x300u,
                                         occupied, 2u, &address));
    assert(address == 0x1200u);
}

static void test_skips_fragmented_overlaps(void) {
    const BlueWakeScratchRange occupied[] = {
        {0x1240u, 0x100u},
        {0x1000u, 0x180u},
        {0x1180u, 0xC0u},
    };
    uint32_t address = 0u;
    assert(bluewake_rel_scratch_first_fit(0x1000u, 0x1800u, 0x100u,
                                         occupied, 3u, &address));
    assert(address == 0x1340u);
}

static void test_honors_exclusive_limit(void) {
    const BlueWakeScratchRange occupied[] = {{0x1000u, 0x700u}};
    uint32_t address = 0u;
    assert(bluewake_rel_scratch_first_fit(0x1000u, 0x1800u, 0x100u,
                                         occupied, 1u, &address));
    assert(address == 0x1700u);
    assert(!bluewake_rel_scratch_first_fit(0x1000u, 0x1800u, 0x120u,
                                          occupied, 1u, &address));
}

int main(void) {
    test_reuses_first_hole();
    test_skips_fragmented_overlaps();
    test_honors_exclusive_limit();
    puts("rel scratch allocator tests passed");
    return 0;
}
