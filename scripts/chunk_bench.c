// Standalone microbenchmark for one DolRecomp 16 KB chunk.
//
// Why this exists: every planning number in this project has been a code-size
// proxy, and code-size proxies have measured wrong repeatedly. This harness
// links one already-built chunk object against the GXRuntime objects it needs,
// builds a CPUState, drives the chunk the way the host does (a fresh downcount
// budget per dispatch), and reports retired host instructions per guest cycle.
//
// The ratio falls out without an interpreter: the emitted code charges each
// guest instruction's Gekko cycle cost into ctx->downcount, so the cycles a
// dispatch consumed are exactly -downcount when it returns.
//
// Three modes, because they answer different questions:
//
//   (default)              entry sweep over a synthetic CPUState. Exercises
//                          every basic block in the chunk, so the aggregate
//                          describes the emitted shape rather than one path.
//   --state-sweep FILE     the same sweep, but the registers and the memory
//                          image come from a real in-game state captured by
//                          scripts/dump_chunk_state.sh. This is the primary
//                          instrument: full block coverage plus a real state,
//                          so the branches inside each block are the game's.
//   --snapshot FILE        one real entry replayed. Measures only what the
//                          chunk does before the guest's first cross-chunk
//                          call, which at a call site is a single instruction,
//                          so treat it as a spot check rather than a result.
//
// Build and run through scripts/bench_chunk.sh, which compiles the object and
// links the runtime.

#include "generated.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Selected with -DBENCH_FUNC / -DBENCH_ENTRY / -DBENCH_SLOTS / -DBENCH_NAME.
// generated.h already declares every func_*.
#ifndef BENCH_FUNC
#define BENCH_FUNC func_802416E0
#endif
#ifndef BENCH_ENTRY
#define BENCH_ENTRY 0x802416E0u
#endif
#ifndef BENCH_SLOTS
#define BENCH_SLOTS 4096u
#endif
#ifndef BENCH_NAME
#define BENCH_NAME "chunk"
#endif

extern void BENCH_FUNC(CPUState* ctx);

#define BENCH_DUMP_MAGIC 0x31574442u

static u64 bench_external_read(CPUState* cpu, u32 ea, u8 size) {
    (void)cpu; (void)ea; (void)size;
    return 0;
}

static void bench_external_write(CPUState* cpu, u32 ea, u64 value, u8 size) {
    (void)cpu; (void)ea; (void)value; (void)size;
}

// Deterministic filler so loads return varied values instead of a zero page.
static u32 bench_fill(u32 i) {
    u32 x = i * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;
    return x;
}

static double bench_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

struct BenchResult {
    long entries;
    long stalls;
    s64 cycles;
    double seconds;
};

// Sweep every instruction slot in the chunk as a dispatch entry point, using
// base as the entry state for each one. lr points outside the chunk, so an
// entry runs until the guest's first return rather than looping on a
// synthetic return address.
static struct BenchResult bench_sweep(CPUState* ctx, const CPUState* base,
                                      u32 lo, u32 slots, u32 lr, long entries) {
    struct BenchResult result;
    result.entries = 0;
    result.stalls = 0;
    result.cycles = 0;
    const double t0 = bench_now();
    while (result.entries < entries) {
        for (u32 k = 0; k < slots && result.entries < entries; ++k) {
            memcpy(ctx, base, sizeof(CPUState));
            ctx->pc = lo + 4u * k;
            ctx->lr = lr;
            ctx->downcount = 0;
            BENCH_FUNC(ctx);
            const s64 charged = -ctx->downcount;
            if (charged <= 0)
                ++result.stalls;
            result.cycles += charged;
            ++result.entries;
        }
    }
    result.seconds = bench_now() - t0;
    return result;
}

static void bench_report(const char* label, const char* detail,
                         const struct BenchResult* r) {
    printf("chunk              %s\n", BENCH_NAME);
    printf("mode               %s\n", label);
    if (detail != NULL)
        printf("state              %s\n", detail);
    printf("entries            %ld\n", r->entries);
    printf("entry stalls       %ld\n", r->stalls);
    printf("guest cycles       %lld\n", (long long)r->cycles);
    printf("cycles/entry       %.2f\n",
           r->entries ? (double)r->cycles / (double)r->entries : 0.0);
    printf("wall seconds       %.4f\n", r->seconds);
    printf("ns per entry       %.1f\n",
           r->entries ? 1e9 * r->seconds / (double)r->entries : 0.0);
}

// Every pointer in a dumped CPUState belongs to the host process that wrote
// it, so they are all reset before use. The cycle deadline is cleared as well:
// with a live deadline the generated code refunds charged cycles through the
// observation suffix, which makes -downcount a poor measure of the guest
// cycles a dispatch consumed.
static int bench_load_snapshot(const char* path, CPUState** out_ctx) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "chunk_bench: cannot open snapshot %s\n", path);
        return 1;
    }
    u32 header[6];
    if (fread(header, sizeof(header), 1, f) != 1 || header[0] != BENCH_DUMP_MAGIC) {
        fprintf(stderr, "chunk_bench: %s is not a chunk-state snapshot\n", path);
        fclose(f);
        return 1;
    }
    if (header[2] != (u32)sizeof(CPUState)) {
        fprintf(stderr, "chunk_bench: snapshot CPUState is %u bytes, this build wants %u\n",
                header[2], (u32)sizeof(CPUState));
        fclose(f);
        return 1;
    }
    const u32 ram_size = header[3];
    const u32 entry = header[4];

    CPUState* ctx = (CPUState*)calloc(1, sizeof(CPUState));
    u8* ram = (u8*)calloc(1, ram_size ? ram_size : 1u);
    if (ctx == NULL || ram == NULL) {
        fprintf(stderr, "chunk_bench: out of memory\n");
        fclose(f);
        return 1;
    }
    if (fread(ctx, sizeof(CPUState), 1, f) != 1 ||
        (ram_size != 0u && fread(ram, 1, ram_size, f) != ram_size)) {
        fprintf(stderr, "chunk_bench: %s is truncated\n", path);
        fclose(f);
        return 1;
    }
    fclose(f);

    ctx->ram = ram;
    ctx->ram_size = ram_size;
    ctx->exram = NULL;
    ctx->exram_size = 0u;
    ctx->external_read = bench_external_read;
    ctx->external_write = bench_external_write;
    ctx->external_read32 = NULL;
    ctx->external_write32 = NULL;
    ctx->external_pointer = NULL;
    ctx->external_user_data = NULL;
    ctx->instruction_fallback = NULL;
    ctx->host_call = NULL;
    ctx->spr_read = NULL;
    ctx->spr_write = NULL;
    ctx->cache_control = NULL;
    ctx->cycle_deadline_active = 0u;
    ctx->cycle_deadline_budget = 0;
    ctx->cycle_observation_suffix = 0u;
    ctx->pc = entry;

    *out_ctx = ctx;
    return 0;
}

static void bench_make_synthetic(CPUState* ctx, u8* ram) {
    u32* words = (u32*)ram;
    for (u32 i = 0; i < GC_MAIN_RAM_SIZE / 4u; ++i)
        words[i] = bench_fill(i);
    for (u32 i = 0; i < 32; ++i)
        ctx->gpr[i] = (u32)(GC_RAM_BASE + 0x1000u * (i + 1u));
    ctx->gpr[1] = GC_RAM_BASE + 0x800000u;
    ctx->gpr[2] = GC_RAM_BASE + 0x100000u;
    ctx->gpr[3] = GC_RAM_BASE + 0x200000u;
    ctx->pc = (u32)BENCH_ENTRY;
    ctx->lr = (u32)BENCH_ENTRY;
    ctx->ctr = 16u;
    ctx->msr = PPC_MSR_FP | PPC_MSR_ME;
    ctx->ram = ram;
    ctx->ram_size = GC_MAIN_RAM_SIZE;
    ctx->cycle_budget = 16384;
    ctx->cycle_deadline_budget = 0;
    ctx->external_read = bench_external_read;
    ctx->external_write = bench_external_write;
}

int main(int argc, char** argv) {
    const u32 lo = (u32)BENCH_ENTRY;
    const u32 slots = (u32)BENCH_SLOTS;
    const u32 lr_outside = 0x80400000u;

    const int state_sweep = (argc > 2 && strcmp(argv[1], "--state-sweep") == 0);
    const int single = (argc > 2 && strcmp(argv[1], "--snapshot") == 0);
    const long entries = (state_sweep || single)
        ? (argc > 3 ? atol(argv[3]) : 200000)
        : (argc > 1 ? atol(argv[1]) : 200000);

    CPUState* base = NULL;
    u8* ram = NULL;
    char detail[640];
    detail[0] = 0;

    if (state_sweep || single) {
        if (bench_load_snapshot(argv[2], &base) != 0)
            return 1;
        snprintf(detail, sizeof(detail), "real in-game entry %08X from %s",
                 base->pc, argv[2]);
        ram = base->ram;
    } else {
        base = (CPUState*)calloc(1, sizeof(CPUState));
        ram = (u8*)calloc(1, GC_MAIN_RAM_SIZE);
        if (base == NULL || ram == NULL) {
            fprintf(stderr, "chunk_bench: out of memory\n");
            return 1;
        }
        bench_make_synthetic(base, ram);
    }

    CPUState* ctx = (CPUState*)calloc(1, sizeof(CPUState));
    if (ctx == NULL) {
        fprintf(stderr, "chunk_bench: out of memory\n");
        return 1;
    }

    struct BenchResult result;
    if (single) {
        // One real entry replayed. The state is restored each dispatch and
        // MEM1 is left evolving, so the stream is deterministic.
        long done = 0, stalls = 0;
        s64 cycles = 0;
        const double t0 = bench_now();
        while (done < entries) {
            memcpy(ctx, base, sizeof(CPUState));
            ctx->downcount = 0;
            BENCH_FUNC(ctx);
            const s64 charged = -ctx->downcount;
            if (charged <= 0)
                ++stalls;
            cycles += charged;
            ++done;
        }
        result.entries = done;
        result.stalls = stalls;
        result.cycles = cycles;
        result.seconds = bench_now() - t0;
        bench_report("single real entry replay", detail, &result);
    } else {
        result = bench_sweep(ctx, base, lo, slots, lr_outside, entries);
        bench_report(state_sweep ? "entry sweep, real state" : "entry sweep, synthetic state",
                     state_sweep ? detail : NULL, &result);
    }

    free(ctx);
    free(base);
    free(ram);
    return 0;
}
