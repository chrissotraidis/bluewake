/* Synthetic vector inputs; no disc or translated source. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_vec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static u8 ram[256];
static CPUState state(u32 entry) {
    memset(ram, 0, sizeof ram);
    for (unsigned i = 0; i < 3; ++i) {
        write_be32(ram + 16 + 4 * i, 0x3F800000u);
        write_be32(ram + 64 + 4 * i, 0x40000000u);
    }
    write_be32(ram + 192, 0x3F000000u);
    write_be32(ram + 196, 0x40400000u);
    CPUState c = {0};
    c.ram = ram; c.ram_size = sizeof ram;
    c.gpr[2] = GC_RAM_BASE + 192 + 12884;
    c.gpr[3] = GC_RAM_BASE + 16;
    c.gpr[4] = GC_RAM_BASE + 64;
    c.gpr[5] = GC_RAM_BASE + 128;
    c.fpr[1] = 2;
    c.pc = entry; c.lr = 0x80001003;
    c.msr = PPC_MSR_FP; c.hid2 = PPC_HID2_LSQE;
    c.cycle_budget = 100; c.cycle_deadline_budget = 100;
    ppc_fpscr_updated(&c);
    return c;
}
static void decline(CPUState cpu, u32 entry) {
    CPUState before = cpu;
    u8 saved[sizeof ram]; memcpy(saved, ram, sizeof ram);
    assert(!bluewake_native_vec(&cpu, entry));
    assert(!memcmp(&cpu, &before, sizeof cpu));
    assert(!memcmp(saved, ram, sizeof ram));
}
static void journal(u32 a, u32 n, void* user) { (void)a; (void)n; (void)user; assert(0); }
static bool ready(void* user, const CPUState* cpu, u32 entry) {
    assert(cpu != NULL); (void)entry;
    return *(bool*)user;
}
int main(void) {
    const u32 entries[] = {BLUEWAKE_PSVEC_ADD, BLUEWAKE_PSVEC_SUBTRACT, BLUEWAKE_PSVEC_SCALE,
        BLUEWAKE_PSVEC_SQUARE_MAG, BLUEWAKE_PSVEC_DOT_PRODUCT, BLUEWAKE_PSVEC_CROSS_PRODUCT,
        BLUEWAKE_PSVEC_SQUARE_DISTANCE, BLUEWAKE_PSVEC_NORMALIZE, BLUEWAKE_PSVEC_MAG};
    for (unsigned i = 0; i < sizeof entries / sizeof entries[0]; ++i) {
        const u32 entry = entries[i];
        CPUState cpu = state(entry);
        CPUState before = cpu;
        assert(!bluewake_native_vec_try(&cpu, entry));
        assert(!memcmp(&cpu, &before, sizeof cpu));
        assert(!bluewake_composite_native_vec_v1(true, NULL, NULL));
        bool allowed = false;
        assert(bluewake_composite_native_vec_v1(true, ready, &allowed));
        assert(!bluewake_native_vec_try(&cpu, entry));
        assert(!memcmp(&cpu, &before, sizeof cpu));
        allowed = true;
        assert(bluewake_native_vec_try(&cpu, entry));
        assert(!bluewake_composite_native_vec_v1(false, ready, &allowed));
        cpu = state(entry);
        assert(!bluewake_native_vec_try(&cpu, entry));
        assert(bluewake_native_vec(&cpu, entry));
        assert(cpu.pc == 0x80001000u);
        cpu = state(entry); cpu.ram_size = 1; decline(cpu, entry);
        cpu = state(entry); cpu.exception = 1; decline(cpu, entry);
        cpu = state(entry); cpu.msr = 0; decline(cpu, entry);
        cpu = state(entry); cpu.hid2 = 0; decline(cpu, entry);
        cpu = state(entry); cpu.gqr[0] = 0x70007; decline(cpu, entry);
        cpu = state(entry); cpu.cycle_budget = 0; decline(cpu, entry);
        cpu = state(entry); cpu.downcount = -100; decline(cpu, entry);
        cpu = state(entry); cpu.cycle_deadline_budget = 1; decline(cpu, entry);
        cpu = state(entry); cpu.gpr[3] = 0xCC008000u; decline(cpu, entry);
        cpu = state(entry); g_mem_write_journal = journal; decline(cpu, entry); g_mem_write_journal = NULL;
        u8 alias[4] = {0}; cpu = state(entry);
        assert(ppc_guest_alias_add_shared(GC_RAM_BASE + 16, sizeof alias, alias));
        decline(cpu, entry);
        assert(ppc_guest_alias_remove(GC_RAM_BASE + 16, sizeof alias));
        cpu = state(entry); decline(cpu, 0);
        cpu = state(entry); cpu.ram = NULL; decline(cpu, entry);
        assert(!bluewake_native_vec(NULL, entry));
    }
    puts("Vector guards: all nine leaves and unchanged fallback state pass");
    return 0;
}
