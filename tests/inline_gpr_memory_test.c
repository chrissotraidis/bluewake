#ifdef NDEBUG
#undef NDEBUG
#endif
#include "inline_gpr.h"
#include <assert.h>
#include <stdio.h>

static void journal(u32 offset, u32 size, void* user) {
    (void)offset; (void)size; (void)user;
    assert(!"readiness query must not write memory");
}
int main(void) {
    u8 ram[256] = {0}, alias[4] = {0};
    CPUState cpu = {0};
    cpu.ram = ram; cpu.ram_size = sizeof ram;
    cpu.gpr[11] = 0x80000100u;
    assert(bw_inline_gpr_memory_ready(&cpu, 14));
    assert(bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.reserve_valid = true;
    assert(bw_inline_gpr_memory_ready(&cpu, 14));
    /* An alias in the middle must reject, even though resolving the whole
     * frame would miss it. */
    assert(ppc_guest_alias_add_shared(0x800000ECu, sizeof alias, alias));
    assert(!bw_inline_gpr_memory_ready(&cpu, 14));
    assert(bw_inline_gpr_memory_ready(&cpu, 31));
    assert(ppc_guest_alias_remove(0x800000ECu, sizeof alias));
    cpu.gpr[11] = 0xC0000100u;
    assert(bw_inline_gpr_memory_ready(&cpu, 14));
    g_mem_write_journal = journal;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    g_mem_write_journal = NULL;
    cpu.gpr[11] = 0xCC008004u;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.gpr[11] = 0x80000101u;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.gpr[11] = 0x80000004u;
    cpu.ram_size = 1;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.ram_size = sizeof ram;
    cpu.ram = NULL;
    assert(!bw_inline_gpr_memory_ready(&cpu, 31));
    cpu.ram = ram;
    assert(!bw_inline_gpr_memory_ready(&cpu, 13));
    assert(!bw_inline_gpr_memory_ready(&cpu, 32));
    for (unsigned i = 0; i < sizeof ram; ++i) assert(ram[i] == 0);
    puts("Inline GPR: plain RAM eligibility excludes aliases, MMIO, journals and bounds");
    return 0;
}
