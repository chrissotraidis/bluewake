/* Synthetic J3D inputs only; no disc or translated functions. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_j3d.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static u8 ram[512];
static CPUState state(u32 entry) {
    memset(ram, 0, sizeof ram);
    write_be32(ram + 16, 4);
    write_be32(ram + 20, GC_RAM_BASE + 128);
    write_be32(ram + 24, GC_RAM_BASE + 132);
    write_be32(ram + 132, 0x3F800000u);
    CPUState cpu = {0};
    cpu.ram = ram; cpu.ram_size = sizeof ram;
    cpu.gpr[13] = GC_RAM_BASE + 16 + 26460;
    cpu.gpr[3] = entry == BLUEWAKE_J3D_TRANSFORM_INFO ? GC_RAM_BASE + 64 : 0;
    cpu.gpr[4] = entry == BLUEWAKE_J3D_TRANSFORM_INFO ? GC_RAM_BASE + 160 : 0;
    cpu.gpr[6] = GC_RAM_BASE + 160;
    cpu.pc = entry; cpu.lr = 0x80001000;
    cpu.msr = PPC_MSR_FP; cpu.cycle_budget = 100; cpu.cycle_deadline_budget = 100;
    return cpu;
}
static void decline(CPUState cpu, u32 entry) {
    CPUState before = cpu;
    u8 saved[sizeof ram]; memcpy(saved, ram, sizeof ram);
    assert(!bluewake_native_j3d_transform(&cpu, entry));
    assert(!memcmp(&cpu, &before, sizeof cpu));
    assert(!memcmp(saved, ram, sizeof ram));
}
static void journal(u32 a, u32 n, void* user) { (void)a; (void)n; (void)user; assert(0); }
int main(void) {
    const u32 entries[] = {BLUEWAKE_J3D_TRANSFORM_INFO, BLUEWAKE_J3D_TRANSFORM_ANGLES};
    for (unsigned i = 0; i < 2; ++i) {
        const u32 entry = entries[i];
        CPUState cpu = state(entry);
        assert(bluewake_native_j3d_transform(&cpu, entry));
        assert(read_be32(ram + 160) == 0x3F800000u);
        assert(read_be32(ram + 180) == 0x3F800000u);
        assert(read_be32(ram + 200) == 0x3F800000u);
        assert(cpu.pc == 0x80001000u);
        cpu = state(entry); cpu.ram = NULL; decline(cpu, entry);
        cpu = state(entry); cpu.ram_size = 1; decline(cpu, entry);
        cpu = state(entry); cpu.exception = 1; decline(cpu, entry);
        cpu = state(entry); cpu.msr = 0; decline(cpu, entry);
        cpu = state(entry); cpu.fpscr = 1; decline(cpu, entry);
        cpu = state(entry); cpu.cycle_budget = 0; decline(cpu, entry);
        cpu = state(entry); cpu.cycle_deadline_budget = 47; decline(cpu, entry);
        cpu = state(entry); cpu.gpr[i == 0 ? 4 : 6] = 0xCC008000u; decline(cpu, entry);
        cpu = state(entry); g_mem_write_journal = journal; decline(cpu, entry); g_mem_write_journal = NULL;
        u8 alias[4] = {0}; cpu = state(entry);
        assert(ppc_guest_alias_add_shared(GC_RAM_BASE + 164, sizeof alias, alias));
        decline(cpu, entry);
        assert(ppc_guest_alias_remove(GC_RAM_BASE + 164, sizeof alias));
        cpu = state(entry); decline(cpu, 0);
        assert(!bluewake_native_j3d_transform(NULL, entry));
    }
    puts("J3D guards: identity matrices and unchanged fallback state pass");
    return 0;
}
