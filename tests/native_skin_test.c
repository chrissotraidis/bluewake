/* Elliott's randomized skinning fixture, adapted to declared BlueWake module
 * storage on Windows and POSIX. Compares every CPU byte, including the cycle
 * observation suffix, and every byte of MEM1. Personal modules remain local.
 * Usage: native_skin_test ORIGINAL_MODULE [CASES] [ROUTED_MODULE]
 */
#include "native_skin.h"
#include "StaticRecompABI.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "module_cpu_contract.h"
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
#define STACK 0x80200000u
#define SDA 0x80400000u
#define JOINTS 64u

static u32 seed = 0x5EED1234u;
static u32 next(void) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static u32 random_float_bits(void) {
    const u32 kind = next() % 64u;
    const u32 sign = next() & 0x80000000u;
    if (kind == 0u) return sign;                          /* signed zeros */
    if (kind == 1u) return sign | (next() & 0x007FFFFFu); /* denormals */
    if (kind == 2u) return sign | ((150u + next() % 8u) << 23) | (next() & 0x007FFFFFu); /* near the bound */
    if (kind == 3u) return 0x3F800000u;                   /* one */
    /* A broad ordinary range, with the cancellations sums of products give. */
    return sign | ((110u + next() % 30u) << 23) | (next() & 0x007FFFFFu);
}

static void put32(u8* ram, u32 address, u32 value) { write_be32(ram + (address - GC_RAM_BASE), value); }
static void put16(u8* ram, u32 address, u16 value) { write_be16(ram + (address - GC_RAM_BASE), value); }
static void put8(u8* ram, u32 address, u8 value) { ram[address - GC_RAM_BASE] = value; }

/* The model, and a CPU about to call the function. */
static CPUState build(u8* ram, unsigned scenario) {
    const u32 count = scenario % 11u == 0u ? 0u : 1u + next() % 24u;
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
        const u32 mix = next() % 13u == 0u ? 0u : 1u + next() % 4u;
        put8(ram, MIX_COUNTS + i, (u8)mix);
        joints += mix != 0u ? mix : 1u;
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
    for (u32 k = 0; k < 64u; ++k) put8(ram, ENVELOPE_FLAGS + k, (u8)next());
    for (u32 k = 0; k < 48u * 32u; k += 4u) put32(ram, WEIGHTED + k, next());
    for (u32 k = 0; k < 256u; k += 4u) put32(ram, STACK - 128u + k, next());
    const u32 unit = SDA + (u32)(s32)-31288;
    put32(ram, unit, scenario % 7u == 3u ? random_float_bits() : 0u);
    put32(ram, unit + 4u, scenario % 7u == 3u ? random_float_bits() : 0x3F800000u);
    /* Rarely, a value past the bound or not finite: the native declines. */
    if (scenario % 29u == 5u && joints != 0u) put32(ram, NODES + 4u * (next() % (JOINTS * 12u)), 0x7F800000u);
    if (scenario % 31u == 6u && joints != 0u) put32(ram, WEIGHTS, 0x5F000000u);

    CPUState c;
    memset(&c, 0, sizeof c);
    c.ram = ram;
    c.ram_size = RAM_SIZE;
    for (unsigned r = 0; r < 32; ++r) {
        c.gpr[r] = next();
        u32 a = random_float_bits(), b = random_float_bits();
        c.fpr[r] = f64_value(convert_to_double(a));
        c.ps1[r] = f64_value(convert_to_double(b));
    }
    /* Second halves that are not singles: the epilogue rounds them. */
    c.ps1[29] = 1.0 / 3.0;
    c.fpr[28] = 1.0 / 7.0;
    c.gpr[1] = STACK;
    c.gpr[3] = SELF;
    c.gpr[13] = SDA;
    c.lr = 0xFFFFFFFCu; /* outside every chunk: the dispatch returns there */
    c.pc = BLUEWAKE_NATIVE_SKIN_ENTRY;
    c.ctr = next();
    c.cr = next();
    c.xer = next();
    c.msr = PPC_MSR_FP;
    c.hid2 = PPC_HID2_LSQE;
    c.gqr[0] = scenario % 5u == 1u ? 0x3F003F00u : 0u; /* scales with type 0 are fine */
    if (scenario % 37u == 9u) c.gqr[0] = 0x00040004u;   /* a quantised type: declines */
    c.fpscr = next() & 0xFFFFF000u;
    if (scenario % 3u == 0u) c.fpscr |= 0x4u; /* NI: denormal results flush */
    c.cycle_budget = 16384;
    c.downcount = -(s64)(next() % 64u);
    c.cycle_deadline_budget = scenario % 4u == 0u ? 0 : 100000;
    if (scenario % 17u == 2u) c.cycle_deadline_budget = 40;    /* too near: declines */
    if (scenario % 19u == 3u) c.cycle_budget = 200;            /* the turn would end inside: declines */
    if (scenario % 6u == 4u) {
        c.reserve_valid = true;
        c.reserve_addr = WEIGHTED + 48u * (next() % (count ? count : 1u));
    }
    return c;
}

typedef struct Module {
    const StaticRecompModuleDesc* desc;
    CPUState fallback;
    BlueWakeModuleStorage storage;
    int (*native_skin)(bool, BluewakeNativeSkinReady, void*);
    void (*report)(void);
} Module;

static int load_module(const char* path, Module* module) {
#if defined(_WIN32)
    HMODULE lib = LoadLibraryA(path);
#define SYMBOL(name) ((void*)GetProcAddress(lib, name))
#else
    void* lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
#define SYMBOL(name) dlsym(lib, name)
#endif
    if (!lib) { fprintf(stderr, "cannot load %s\n", path); return 0; }
    StaticRecompGetModuleFn get = (StaticRecompGetModuleFn)SYMBOL(STATICRECOMP_GET_MODULE_SYMBOL);
    if (!get) return 0;
    module->desc = get();
    module->native_skin = (int (*)(bool, BluewakeNativeSkinReady, void*))SYMBOL("bluewake_composite_native_skin_v1");
    module->report = (void (*)(void))SYMBOL("bluewake_native_skin_report");
    const char* error = bw_module_select_storage(module->desc,
        (BlueWakeModuleCPUFn)SYMBOL("bluewake_composite_guest_cpu"),
        (BlueWakeModuleMEM1Fn)SYMBOL("bluewake_composite_guest_mem1"),
        &module->fallback, &module->storage);
    if (error) { fprintf(stderr, "%s\n", error); return 0; }
    if (module->storage.mem1 == NULL) {
        module->storage.mem1 = calloc(1, GC_MAIN_RAM_SIZE);
        module->storage.mem1_size = GC_MAIN_RAM_SIZE;
    }
    return module->storage.mem1 != NULL;
#undef SYMBOL
}

static unsigned routed_queries;
static bool routed_ready(void* user, const CPUState* cpu, u32 entry) {
    (void)user;
    if (!cpu || entry != BLUEWAKE_NATIVE_SKIN_ENTRY) return false;
    ++routed_queries; return true;
}
static int run_window(Module* module, CPUState initial, u8* ram) {
    CPUState* cpu = module->storage.cpu;
    *cpu = initial; cpu->ram = ram; module->desc->on_state_loaded(cpu);
    unsigned turns = 0;
    while (cpu->pc != 0xFFFFFFFCu && cpu->downcount > -cpu->cycle_budget && !cpu->exception) {
        if (++turns > 10000 || !module->desc->dispatch(cpu, cpu->pc)) return 0;
    }
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "usage: native_skin_test ORIGINAL_MODULE [CASES] [ROUTED_MODULE]\n");
        return 2;
    }
    const unsigned cases = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 4000u;
    Module original = {0}, candidate = {0};
    if (!load_module(argv[1], &original)) return 1;
    const StaticRecompModuleDesc* mod = original.desc;
    if (strcmp(mod->game_id, "GZLE01") != 0) return 1;
    if (argc > 3 && (!load_module(argv[3], &candidate) || !candidate.native_skin)) return 1;
    u8* reference_ram = original.storage.mem1;
    u8* native_ram = calloc(1, RAM_SIZE);
    u8* before_ram = calloc(1, RAM_SIZE);
    if (!reference_ram || !native_ram || !before_ram) return 1;
    unsigned ran = 0, declined = 0;
    for (unsigned i = 0; i < cases; ++i) {
        CPUState c = build(native_ram, i);
        memcpy(reference_ram, native_ram, RAM_SIZE);
        memcpy(before_ram, native_ram, RAM_SIZE);

        if (!run_window(&original, c, reference_ram)) return 1;
        CPUState reference = *original.storage.cpu;
        if (candidate.desc) {
            const bool enabled = i % 31 != 0;
            if (candidate.native_skin(enabled, routed_ready, NULL) != enabled) return 1;
            u8* routed_ram = candidate.storage.mem1;
            memcpy(routed_ram, before_ram, RAM_SIZE);
            if (!run_window(&candidate, c, routed_ram)) return 1;
            CPUState actual = *candidate.storage.cpu; actual.ram = reference.ram;
            if (memcmp(&actual, &reference, sizeof reference) || memcmp(routed_ram, reference_ram, RAM_SIZE)) {
                fprintf(stderr, "case %u: routed skin module differs from translated reference\n", i);
                return 1;
            }
        }

        CPUState native = c;
        ppc_fpscr_updated(&native);
        CPUState untouched = native;
        if (!bluewake_native_skin(&native)) {
            declined++;
            if (memcmp(&native, &untouched, sizeof native) != 0 || memcmp(native_ram, before_ram, RAM_SIZE) != 0) {
                fprintf(stderr, "case %u: declined but changed state\n", i);
                return 1;
            }
            continue;
        }
        ran++;
        reference.ram = native.ram;
        if (memcmp(&native, &reference, sizeof native) != 0 || memcmp(native_ram, reference_ram, RAM_SIZE) != 0) {
            fprintf(stderr, "case %u (seed %08X): mismatch\n", i, seed);
            for (unsigned b = 0; b < sizeof native; ++b)
                if (((u8*)&native)[b] != ((u8*)&reference)[b])
                    fprintf(stderr, "  CPU byte %u (offset of gpr %u, fpr %u, ps1 %u): got %02X want %02X\n", b,
                            (unsigned)offsetof(CPUState, gpr), (unsigned)offsetof(CPUState, fpr),
                            (unsigned)offsetof(CPUState, ps1), ((u8*)&native)[b], ((u8*)&reference)[b]);
            unsigned shown = 0;
            for (u32 a = 0; a < RAM_SIZE && shown < 32u; ++a)
                if (native_ram[a] != reference_ram[a]) {
                    fprintf(stderr, "  RAM %08X: got %02X want %02X\n", GC_RAM_BASE + a, native_ram[a], reference_ram[a]);
                    shown++;
                }
            return 1;
        }
    }
    printf("native skin: %u cases identical to the translation, %u declined unchanged\n", ran, declined);
    if (candidate.desc) {
        if (candidate.report) candidate.report();
        printf("routed skin: %u readiness queries, %u complete CPU/MEM1 comparisons\n", routed_queries, cases);
        if (!routed_queries) return 1;
    }
    return ran != 0u ? 0 : 1;
}
