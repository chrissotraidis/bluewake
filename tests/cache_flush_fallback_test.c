#include "cache_flush_fallback.h"
#include "gxruntime/guest_memory_dirty.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

static unsigned cache_calls;
static u8 cache_operation;
static u32 cache_address;
static void observe_cache(CPUState* cpu, u8 operation, u32 address, u32 cia) {
    (void)cpu;
    assert(cia == 0x80303180u);
    cache_calls++;
    cache_operation = operation;
    cache_address = address;
    dol_guest_memory_dirty_mark(address & ~31u, 32u);
}

static void check_cache_writeback(CPUState* cpu) {
    const u32 pc = 0x80303180u;
    cpu->cache_control = observe_cache;
    cpu->gpr[0] = 0xDEADBEEFu; // rA=0 means zero, not the contents of r0.
    cpu->gpr[3] = 0x80A8B6A7u;
    cpu->pc = pc;
    dol_guest_memory_dirty_reset();
    u64 before, after;
    assert(dol_guest_memory_dirty_epoch(0x00A8B6A0u, 7904u, &before));
    assert(bluewake_cache_flush_fallback(cpu, 0x7C00186Cu, pc)); // dcbst 0,r3
    assert(cache_calls == 1 && cache_operation == PPC_CACHE_DCBST);
    assert(cache_address == cpu->gpr[3] && cpu->pc == pc + 4u);
    assert(cpu->gpr[0] == 0xDEADBEEFu && cpu->gpr[3] == 0x80A8B6A7u);
    assert(dol_guest_memory_dirty_epoch(0xC0A8B6A0u, 7904u, &after));
    assert(after > before); // Physical and cached/uncached aliases agree.

    cpu->gpr[4] = 0xC0000000u;
    cpu->gpr[3] = 0x00A8B6A7u;
    assert(bluewake_cache_flush_fallback(cpu, 0x7C0418ACu, pc)); // dcbf r4,r3
    assert(cache_calls == 2 && cache_operation == PPC_CACHE_DCBF);
    assert(cache_address == 0xC0A8B6A7u);
    cpu->pc = pc;
    assert(!bluewake_cache_flush_fallback(cpu, 0x7C001BACu, pc)); // dcbi
    assert(!bluewake_cache_flush_fallback(cpu, 0x0000006Cu, pc)); // wrong opcode
    assert(cache_calls == 2 && cpu->pc == pc);
    cpu->cache_control = NULL;
}

int main(void) {
    CPUState cpu;
    assert(cpu_init(&cpu));
    check_cache_writeback(&cpu);
    cpu_free(&cpu);
    puts("Cache writeback fallback contract test passed.");
    return 0;
}
