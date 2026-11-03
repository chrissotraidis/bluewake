/* Synthetic matrices/vectors only; no disc or translated functions. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "native_math.h"
#include "native_work_pool.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE 0x400000u
static u8* ram;
static void put(u32 offset, float value) {
    u32 bits; memcpy(&bits, &value, sizeof bits); write_be32(ram + offset, bits);
}
static float get(u32 offset) {
    u32 bits = read_be32(ram + offset); float value;
    memcpy(&value, &bits, sizeof value); return value;
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
static void prepare_array(CPUState* c, u32 count) {
    static const float matrix[12] = {
        1.25f, -0.5f, 0.75f, 2.0f,
        -1.5f, 0.25f, 2.0f, -3.0f,
        0.5f, 1.75f, -0.25f, 4.0f,
    };
    c->gpr[3] = GC_RAM_BASE + 0x1000;
    c->gpr[4] = GC_RAM_BASE + 0x2000;
    c->gpr[5] = GC_RAM_BASE + 0x10000;
    c->gpr[6] = count;
    c->cycle_budget = 1000000;
    c->cycle_deadline_budget = 1000000;
    for (unsigned i = 0; i < 12; ++i) put(0x1000 + 4 * i, matrix[i]);
    for (u32 i = 0; i < count; ++i) {
        put(0x2000 + 12 * i, (float)i * 0.25f - 17.0f);
        put(0x2004 + 12 * i, (float)(i % 29) - 11.0f);
        put(0x2008 + 12 * i, (float)(i % 13) * -0.5f + 3.0f);
    }
    write_be32(ram + 0x10000 - 4, UINT32_C(0xa5a5a5a5));
    write_be32(ram + 0x10000 + 12 * count, UINT32_C(0x5a5a5a5a));
}
static void check_array_output(u32 count) {
    float a[12];
    for (unsigned i = 0; i < 12; ++i) a[i] = get(0x1000 + 4 * i);
    for (u32 i = 0; i < count; ++i) {
        float x = get(0x2000 + 12 * i);
        float y = get(0x2004 + 12 * i);
        float z = get(0x2008 + 12 * i);
        float expected[3] = {
            fmaf(a[2], z, fmaf(a[1], y, fmaf(a[0], x, a[3]))),
            fmaf(a[6], z, fmaf(a[5], y, fmaf(a[4], x, a[7]))),
            fmaf(a[10], z, a[8] * x) + fmaf(a[11], 1.f, a[9] * y),
        };
        for (unsigned component = 0; component < 3; ++component) {
            float actual = get(0x10000 + 12 * i + 4 * component);
            assert(!memcmp(&actual, &expected[component], sizeof actual));
        }
    }
    assert(read_be32(ram + 0x10000 - 4) == UINT32_C(0xa5a5a5a5));
    assert(read_be32(ram + 0x10000 + 12 * count) == UINT32_C(0x5a5a5a5a));
}
static void array_boundaries(void) {
    const u32 entry = 0x8030DA98;
    const u32 counts[] = {767, 768, 769, 4096};
    const char* workers = getenv("BLUEWAKE_NATIVE_WORKERS");
    char* end = NULL;
    long requested = workers ? strtol(workers, &end, 10) : 0;
    unsigned configured_workers = workers && end != workers && !*end && requested > 0
        ? (unsigned)requested : 0;
    if (configured_workers > 8) configured_workers = 8;
#if defined(_WIN32)
    configured_workers = 0;
#endif
    for (unsigned i = 0; i < sizeof counts / sizeof counts[0]; ++i) {
        CPUState c = state(entry); prepare_array(&c, counts[i]);
        unsigned long long before = bluewake_parallel_batches();
        assert(bluewake_native_math(&c, entry));
        unsigned long long after = bluewake_parallel_batches();
        bool parallel = configured_workers &&
            counts[i] / (configured_workers + 1) >= 256;
        assert(after - before == (parallel ? 1u : 0u));
        check_array_output(counts[i]);
    }

    /* Invalid data at the beginning, middle, or end must decline without
     * changing guest state or memory. */
    const u32 invalid_indices[] = {0, 8, 15};
    for (unsigned i = 0; i < sizeof invalid_indices / sizeof invalid_indices[0]; ++i) {
        CPUState c = state(entry); prepare_array(&c, 16);
        put(0x2000 + 12 * invalid_indices[i], NAN);
        decline(c, entry);
    }

    /* Partial overlap is unsupported in both directions; in-place input and
     * output remains covered by the private differential oracle. */
    CPUState c = state(entry); prepare_array(&c, 16);
    c.gpr[5] = c.gpr[4] + 4; decline(c, entry);
    c = state(entry); prepare_array(&c, 16);
    c.gpr[4] = c.gpr[5] + 4; decline(c, entry);

    /* With downcount=-17, 16 vectors require exactly 190 available cycles. */
    c = state(entry); prepare_array(&c, 16);
    c.cycle_budget = 206; decline(c, entry);
    c = state(entry); prepare_array(&c, 16);
    c.cycle_budget = 207; assert(bluewake_native_math(&c, entry));
    c = state(entry); prepare_array(&c, 16);
    c.cycle_deadline_budget = 206; decline(c, entry);
    c = state(entry); prepare_array(&c, 16);
    c.cycle_deadline_budget = 207; assert(bluewake_native_math(&c, entry));
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
    array_boundaries();
    BluewakeNativeMathStatsV1 stats = {0};
    assert(!bluewake_composite_native_math_stats_v1(NULL, sizeof stats));
    assert(!bluewake_composite_native_math_stats_v1(&stats, sizeof stats - 1));
    assert(bluewake_composite_native_math_stats_v1(&stats, sizeof stats));
    assert(stats.version == 1 && stats.size == sizeof stats);
    assert(stats.native_hits[3] > 0 && stats.native_fallbacks[3] > 0);
    assert(stats.array_vectors > 0 && stats.array_max_vectors == 4096);
    assert(stats.completed_batches == bluewake_parallel_batches());
#if !defined(_WIN32)
    const char* configured = getenv("BLUEWAKE_NATIVE_WORKERS");
    if (configured && atoi(configured) > 0) {
        assert(stats.requested_workers > 0 && stats.effective_workers > 0);
        assert(stats.worker_items > 0);
    }
#endif
    printf("Matrix guards: identity/non-identity, boundary fallback and %llu parallel batches pass\n", bluewake_parallel_batches());
#if !defined(_WIN32)
    const char* workers = getenv("BLUEWAKE_NATIVE_WORKERS");
    if (workers && atoi(workers) > 0) assert(bluewake_parallel_batches() > 0);
#endif
    free(ram); return 0;
}
