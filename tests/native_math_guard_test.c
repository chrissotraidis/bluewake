/* Synthetic matrices/vectors only; no disc or translated functions. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_math.h"
#include "native_work_pool.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE 0x400000u
static u8* ram;
static void put(u32 offset, float value) {
    u32 bits; memcpy(&bits, &value, sizeof bits); write_be32(ram + offset, bits);
}
static CPUState state(u32 entry) {
    memset(ram, 0, RAM_SIZE);
    for (unsigned i = 0; i < 12; ++i) {
        put(0x1000 + 4 * i, i == 0 || i == 5 || i == 10 ? 1 : 0);
        put(0x2000 + 4 * i, i == 0 || i == 5 || i == 10 ? 1 : 0);
    }
    if (entry == 0x8030DA44 || entry == 0x8030DA98)
        for (unsigned i = 0; i < (entry == 0x8030DA44 ? 3 : 4096 * 3); ++i)
            put(0x2000 + 4 * i, 1 + i % 10);
    put(0x3F66F4, 1);
    CPUState c = {0}; c.ram = ram; c.ram_size = RAM_SIZE;
    c.gpr[1] = GC_RAM_BASE + 0x40100;
    c.gpr[3] = GC_RAM_BASE + 0x1000;
    c.gpr[4] = GC_RAM_BASE + 0x2000;
    c.gpr[5] = GC_RAM_BASE + 0x10000;
    c.gpr[6] = 4096;
    c.pc = entry; c.lr = 0x80001003;
    c.msr = PPC_MSR_FP; c.hid2 = PPC_HID2_LSQE;
    c.cycle_budget = 100000; c.cycle_deadline_budget = 100000; c.downcount = -17;
    ppc_fpscr_updated(&c);
    return c;
}
static void decline(CPUState c, u32 entry) {
    CPUState saved = c;
    u8* before = malloc(RAM_SIZE); assert(before); memcpy(before, ram, RAM_SIZE);
    assert(!bluewake_native_math(&c, entry));
    assert(!memcmp(&c, &saved, sizeof c));
    assert(!memcmp(before, ram, RAM_SIZE)); free(before);
}
static void journal(u32 a, u32 n, void* user) { (void)a; (void)n; (void)user; assert(0); }
static bool ready(void* user, const CPUState* cpu, u32 entry) {
    assert(cpu != NULL); (void)entry; return *(bool*)user;
}
int main(void) {
    ram = calloc(1, RAM_SIZE); assert(ram);
    const u32 entries[] = {0x8030D0C8, 0x8030D0FC, 0x8030DA44, 0x8030DA98};
    for (unsigned i = 0; i < 4; ++i) {
        u32 entry = entries[i]; CPUState c = state(entry);
        CPUState before = c;
        assert(!bluewake_native_math_try(&c, entry));
        assert(!memcmp(&c, &before, sizeof c));
        assert(!bluewake_composite_native_math_v1(true, NULL, NULL));
        bool allowed = false;
        assert(bluewake_composite_native_math_v1(true, ready, &allowed));
        assert(!bluewake_native_math_try(&c, entry));
        assert(!memcmp(&c, &before, sizeof c));
        allowed = true; assert(bluewake_native_math_try(&c, entry));
        assert(!bluewake_composite_native_math_v1(false, ready, &allowed));
        c = state(entry); assert(!bluewake_native_math_try(&c, entry));
        assert(bluewake_native_math(&c, entry)); assert(c.pc == 0x80001000u);
        if (i < 2) assert(!memcmp(ram + 0x1000, ram + (i == 0 ? 0x2000 : 0x10000), 48));
        else assert(!memcmp(ram + 0x2000, ram + 0x10000, i == 2 ? 12 : 4096 * 12));
        c = state(entry); c.ram_size = 1; decline(c, entry);
        c = state(entry); c.exception = 1; decline(c, entry);
        c = state(entry); c.msr = 0; decline(c, entry);
        c = state(entry); c.hid2 = 0; decline(c, entry);
        c = state(entry); c.gqr[0] = 1; decline(c, entry);
        c = state(entry); c.fpscr = 1; decline(c, entry);
        c = state(entry); c.cycle_budget = 0; decline(c, entry);
        c = state(entry); c.downcount = -100000; decline(c, entry);
        c = state(entry); c.cycle_deadline_budget = 1; decline(c, entry);
        c = state(entry); c.gpr[3] = 0xCC008000; decline(c, entry);
        c = state(entry); g_mem_write_journal = journal; decline(c, entry); g_mem_write_journal = NULL;
        u8 alias[4] = {0}; c = state(entry);
        assert(ppc_guest_alias_add_shared(GC_RAM_BASE + 0x1000, sizeof alias, alias));
        decline(c, entry); assert(ppc_guest_alias_remove(GC_RAM_BASE + 0x1000, sizeof alias));
        c = state(entry); decline(c, 0); assert(!bluewake_native_math(NULL, entry));
        c = state(entry); c.ram = NULL; decline(c, entry);
    }
    printf("Matrix guards: identity, unchanged fallback and %llu parallel batches pass\n", bluewake_parallel_batches());
#if !defined(_WIN32)
    const char* workers = getenv("BLUEWAKE_NATIVE_WORKERS");
    if (workers && atoi(workers) > 0) assert(bluewake_parallel_batches() > 0);
#endif
    free(ram); return 0;
}
