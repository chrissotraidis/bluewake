/* scripts/windows/fast_blocks.py: the prepaid block copies against the blocks.
 *
 *   fast_blocks_test ORIGINAL.dll TRANSFORMED.dll [CASES]
 *
 * Loads a personal module built without the copies and one built with them
 * (under another file name), sets up the same guest state in both and runs
 * the same function through each: J3DModel::calcWeightEnvelopeMtx (loops, many
 * blocks, memory and paired singles) and the SDK's matrix and vector leaves.
 * The states put the next deadline anywhere from inside the first block to
 * far away, and the turn's budget anywhere from mid-function to unlimited, so
 * the copies' deadline refunds, their return to the original block and the
 * blocks' own stops all run; every byte of the CPU state and of RAM must
 * match after each, including the cycle observation suffix. Modules are
 * generated privately from the player's disc; no native replacement sources.
 * Works with either ordinary pointer-state or optional fixed-state modules.
 * Global-MEM1 modules require a separate fixture bound to their own RAM. */
#include "core/cpu.h"
#include "StaticRecompABI.h"

#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define RAM_SIZE GC_MAIN_RAM_SIZE
#define SELF 0x80100000u
#define MODEL 0x80101000u
#define MIX_COUNTS 0x80102000u
#define INDICES 0x80103000u
#define WEIGHTS 0x80108000u
#define SCALE_FLAGS 0x8010F000u
#define ENVELOPE_FLAGS 0x80110000u
#define NODES 0x80120000u
#define INVERSE 0x80130000u
#define WEIGHTED 0x80140000u
#define VECTORS 0x80150000u
#define STACK 0x80200000u
#define SDA 0x80400000u
#define JOINTS 64u

static u32 seed = 0x0FA57B10u;
static u32 next(void) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static u32 random_float_bits(void) {
    const u32 kind = next() % 64u;
    const u32 sign = next() & 0x80000000u;
    if (kind == 0u) return sign;
    if (kind == 1u) return sign | (next() & 0x007FFFFFu);
    if (kind == 2u) return 0x3F800000u;
    return sign | ((110u + next() % 30u) << 23) | (next() & 0x007FFFFFu);
}

static void put32(u8* ram, u32 a, u32 v) { write_be32(ram + (a - GC_RAM_BASE), v); }
static void put16(u8* ram, u32 a, u16 v) { write_be16(ram + (a - GC_RAM_BASE), v); }
static void put8(u8* ram, u32 a, u8 v) { ram[a - GC_RAM_BASE] = v; }

static const u32 ENTRIES[] = {
    0x802EE67Cu,                                                 /* calcWeightEnvelopeMtx */
    0x8030D0C8u, 0x8030D0FCu, 0x8030DA44u, 0x8030DA98u,          /* PSMTXCopy, Concat, MultVec, MultVecArray */
    0x8030DCE0u, 0x8030DD04u, 0x8030DD28u, 0x8030DE0Cu, 0x8030DE50u,
    0x8030DE68u, 0x8030DEACu, 0x8030DECCu, 0x8030E0B4u,          /* the PSVEC leaves */
};
#define ENTRY_COUNT (sizeof ENTRIES / sizeof ENTRIES[0])

static CPUState build(u8* ram, u32 entry, unsigned scenario) {
    memset(ram + (0x80100000u - GC_RAM_BASE), 0, 0x60000);
    const u32 count = 1u + next() % 12u;
    put32(ram, SELF + 4u, MODEL);
    put32(ram, SELF + 132u, SCALE_FLAGS);
    put32(ram, SELF + 136u, ENVELOPE_FLAGS);
    put32(ram, SELF + 140u, NODES);
    put32(ram, SELF + 144u, WEIGHTED);
    put16(ram, MODEL + 48u, (u16)count);
    put32(ram, MODEL + 52u, MIX_COUNTS);
    put32(ram, MODEL + 56u, INDICES);
    put32(ram, MODEL + 60u, WEIGHTS);
    put32(ram, MODEL + 64u, INVERSE);
    u32 joints = 0;
    for (u32 i = 0; i < count; ++i) {
        const u32 mix = 1u + next() % 3u;
        put8(ram, MIX_COUNTS + i, (u8)mix);
        joints += mix;
    }
    for (u32 j = 0; j < joints; ++j) {
        put16(ram, INDICES + 2u * j, (u16)(next() % JOINTS));
        put32(ram, WEIGHTS + 4u * j, random_float_bits());
    }
    for (u32 k = 0; k < JOINTS * 12u; ++k) {
        put32(ram, NODES + 4u * k, random_float_bits());
        put32(ram, INVERSE + 4u * k, random_float_bits());
    }
    for (u32 k = 0; k < JOINTS; ++k) put8(ram, SCALE_FLAGS + k, (u8)next());
    for (u32 k = 0; k < 0x1000u; k += 4u) put32(ram, VECTORS + k, random_float_bits());
    put32(ram, SDA - 31288u, 0u);
    put32(ram, SDA - 31284u, 0x3F800000u);

    CPUState c;
    memset(&c, 0, sizeof c);
    c.ram = ram;
    c.ram_size = RAM_SIZE;
    for (unsigned r = 0; r < 32; ++r) {
        c.gpr[r] = next();
        c.fpr[r] = f64_value(convert_to_double(random_float_bits()));
        c.ps1[r] = f64_value(convert_to_double(random_float_bits()));
    }
    c.gpr[1] = STACK;
    c.gpr[2] = VECTORS + 0x800u + 12884u; /* the SDA2 constants PSVECNormalize and Mag read */
    put32(ram, VECTORS + 0x800u, 0x3F000000u);
    put32(ram, VECTORS + 0x804u, 0x40400000u);
    c.gpr[13] = SDA;
    if (entry == 0x802EE67Cu) {
        c.gpr[3] = SELF;
    } else {
        c.gpr[3] = VECTORS + 16u * (next() % 32u);
        c.gpr[4] = VECTORS + 16u * (next() % 32u);
        c.gpr[5] = scenario % 3u == 0u ? c.gpr[3] : VECTORS + 0x600u + 16u * (next() % 16u);
        c.gpr[6] = 1u + next() % 8u; /* PSMTXMultVecArray's count */
    }
    c.lr = 0xFFFFFFFCu;
    c.pc = entry;
    c.ctr = next();
    c.cr = next();
    c.xer = next();
    c.msr = PPC_MSR_FP;
    c.hid2 = PPC_HID2_LSQE;
    c.fpscr = next() & 0xFFFFF000u;
    if (scenario % 3u == 1u) c.fpscr |= 0x4u;
    /* The deadline: none, inside the function (a block cannot prepay, or a
     * refund), or far; the turn's budget: from mid-function to large. */
    switch (scenario % 5u) {
    case 0: c.cycle_deadline_budget = 0; break;
    case 1: c.cycle_deadline_budget = 1 + (s64)(next() % 40u); break;
    case 2: c.cycle_deadline_budget = 20 + (s64)(next() % 400u); break;
    case 3: c.cycle_deadline_budget = 100000; break;
    default: c.cycle_deadline_budget = 1 + (s64)(next() % 3000u); break;
    }
    c.cycle_budget = scenario % 4u == 2u ? 20 + (s64)(next() % 600u) : 16384;
    c.downcount = -(s64)(next() % 64u);
    if (scenario % 7u == 3u) c.downcount = (s64)(next() % 64u); /* ahead of the turn */
    c.reserve_valid = (scenario & 1u) != 0u;
    c.reserve_addr = WEIGHTED + 16u * (next() % 8u);
    return c;
}

typedef struct Module {
    const StaticRecompModuleDesc* desc;
    CPUState* cpu;
    CPUState storage;
} Module;

static int open_module(const char* path, Module* out) {
#if defined(_WIN32)
    HMODULE lib = LoadLibraryA(path);
#define MODULE_SYMBOL(name) ((void*)GetProcAddress(lib, name))
#else
    void* lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#define MODULE_SYMBOL(name) dlsym(lib, name)
#endif
    if (lib == NULL) {
        fprintf(stderr, "cannot load %s\n", path);
        return 0;
    }
    StaticRecompGetModuleFn get = (StaticRecompGetModuleFn)MODULE_SYMBOL(STATICRECOMP_GET_MODULE_SYMBOL);
    CPUState* (*guest_cpu)(void) = (CPUState * (*)(void))MODULE_SYMBOL("bluewake_composite_guest_cpu");
    if (MODULE_SYMBOL("bluewake_composite_guest_mem1") != NULL) {
        fprintf(stderr, "%s: global MEM1 needs a separate RAM comparison fixture\n", path);
        return 0;
    }
    if (get == NULL) {
        fprintf(stderr, "%s: not a BlueWake module\n", path);
        return 0;
    }
    out->desc = get();
    out->cpu = guest_cpu != NULL ? guest_cpu() : &out->storage;
    if (out->desc == NULL || out->cpu == NULL) return 0;
    if (out->desc->abi_version != STATICRECOMP_ABI_VERSION ||
        out->desc->cpu_abi_version != GXRUNTIME_CPU_ABI_VERSION ||
        out->desc->cpu_state_size != sizeof(CPUState) ||
        memcmp(out->desc->game_id, "GZLE01\0", 7) != 0 ||
        out->desc->dispatch == NULL) {
        fprintf(stderr, "%s: incompatible module descriptor\n", path);
        return 0;
    }
    return 1;
#undef MODULE_SYMBOL
}

/* One dispatch, then more while the function has not returned and neither
 * run ended on a pending exception: a block's stop for the budget leaves the
 * chunk and the loop would dispatch it again. */
static int run(Module* m, CPUState* state) {
    *m->cpu = *state;
    if (m->desc->on_state_loaded != NULL) m->desc->on_state_loaded(m->cpu);
    for (unsigned turns = 0; turns < 64u; ++turns) {
        if (!m->desc->dispatch(m->cpu, m->cpu->pc)) {
            fprintf(stderr, "unexpected dispatch miss at %08X\n", m->cpu->pc);
            return 0;
        }
        if (m->cpu->pc == 0xFFFFFFFCu || m->cpu->exception != 0u)
            break;
        /* A new turn: the loop's host would reset the budget. */
        m->cpu->downcount = 0;
    }
    *state = *m->cpu;
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: fast_blocks_test ORIGINAL_MODULE TRANSFORMED_MODULE [CASES]\n");
        return 2;
    }
    unsigned cases = 20000u;
    if (argc > 3) {
        char* end;
        errno = 0;
        unsigned long parsed = strtoul(argv[3], &end, 10);
        if (argc != 4 || argv[3][0] == '-' || end == argv[3] || *end != '\0' || errno || parsed > UINT_MAX)
            return 2;
        cases = (unsigned)parsed;
    }
    if (cases < ENTRY_COUNT) { fprintf(stderr, "need at least %u cases\n", (unsigned)ENTRY_COUNT); return 2; }
    Module a, b;
    if (!open_module(argv[1], &a) || !open_module(argv[2], &b))
        return 1;
    if (a.cpu == b.cpu) {
        fprintf(stderr, "the two modules share one guest CPU: load them under different file names\n");
        return 1;
    }
    u8* ram_a = calloc(1, RAM_SIZE);
    u8* ram_b = calloc(1, RAM_SIZE);
    if (!ram_a || !ram_b) return 1;
    unsigned per_entry[ENTRY_COUNT] = {0}, stopped = 0, refunded = 0;
    for (unsigned i = 0; i < cases; ++i) {
        const unsigned which = i % (unsigned)ENTRY_COUNT;
        CPUState c = build(ram_a, ENTRIES[which], i / (unsigned)ENTRY_COUNT);
        memcpy(ram_b, ram_a, RAM_SIZE);
        CPUState sa = c, sb = c;
        sa.ram = ram_a;
        sb.ram = ram_b;
        if (!run(&a, &sa) || !run(&b, &sb)) return 1;
        sb.ram = sa.ram;
        if (memcmp(&sa, &sb, sizeof sa) != 0 || memcmp(ram_a, ram_b, RAM_SIZE) != 0) {
            fprintf(stderr, "case %u (%08X, seed %08X): the copies differ (fpr at %u, ps1 at %u)\n", i,
                    ENTRIES[which], seed, (unsigned)offsetof(CPUState, fpr), (unsigned)offsetof(CPUState, ps1));
            for (unsigned k = 0; k < sizeof sa; ++k)
                if (((u8*)&sa)[k] != ((u8*)&sb)[k])
                    fprintf(stderr, "  CPU byte %u: original %02X, copies %02X\n", k, ((u8*)&sa)[k], ((u8*)&sb)[k]);
            unsigned shown = 0;
            for (u32 k = 0; k < RAM_SIZE && shown < 16u; ++k)
                if (ram_a[k] != ram_b[k]) {
                    fprintf(stderr, "  RAM %08X: original %02X, copies %02X\n", GC_RAM_BASE + k, ram_a[k], ram_b[k]);
                    shown++;
                }
            return 1;
        }
        per_entry[which]++;
        if (sa.pc != 0xFFFFFFFCu) stopped++;
        if (c.cycle_deadline_budget > 0 && c.cycle_deadline_budget < 400) refunded++;
    }
    printf("fast blocks: %u cases identical (%u stopped before returning, %u with the deadline inside)\n", cases,
           stopped, refunded);
    for (unsigned k = 0; k < ENTRY_COUNT; ++k)
        printf("  %08X: %u cases\n", ENTRIES[k], per_entry[k]);
    free(ram_a);
    free(ram_b);
    return 0;
}
