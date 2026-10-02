/* cmake/composite/native_vec.c against the translations it stands in for.
 *
 *   native_vec_test ORIGINAL_MODULE [CASES] [ROUTED_MODULE]
 *
 * For each vector leaf: random vectors (zeros, denormals, large values,
 * sometimes past the bound), operands that alias the output or overlap it,
 * random registers, FPSCR and cycle state; the leaf through the personal
 * module's translation and through bluewake_native_vec; every byte of the CPU
 * state (including its observation suffix) and the 256-byte RAM test area must match - or, where the native declines, nothing may
 * have changed. Uses BlueWake's declared module storage ABIs on Windows and POSIX. */
#include "native_vec.h"
#include "StaticRecompABI.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module_cpu_contract.h"
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define RAM_SIZE GC_MAIN_RAM_SIZE
#define AREA 0x80100000u

static u32 seed = 0x2468ACE1u;
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
    if (kind == 2u) return sign | ((180u + next() % 8u) << 23) | (next() & 0x007FFFFFu);
    if (kind == 3u) return sign | ((60u + next() % 10u) << 23) | (next() & 0x007FFFFFu);
    if (kind == 4u) return 0x3F800000u;
    if (kind == 5u && next() % 8u == 0u) return 0x7F800000u | (next() & 1u); /* not finite: declines */
    if (kind == 6u && next() % 8u == 0u) return sign | (200u << 23);          /* past the bound: declines */
    return sign | ((100u + next() % 50u) << 23) | (next() & 0x007FFFFFu);
}

static const u32 LEAVES[] = {BLUEWAKE_PSVEC_ADD,         BLUEWAKE_PSVEC_SUBTRACT,     BLUEWAKE_PSVEC_SCALE,
                             BLUEWAKE_PSVEC_SQUARE_MAG,  BLUEWAKE_PSVEC_DOT_PRODUCT,  BLUEWAKE_PSVEC_CROSS_PRODUCT,
                             BLUEWAKE_PSVEC_SQUARE_DISTANCE, BLUEWAKE_PSVEC_NORMALIZE,
                             BLUEWAKE_PSVEC_MAG};
#define LEAF_COUNT (sizeof LEAVES / sizeof LEAVES[0])

static CPUState build(u8* ram, u32 leaf, unsigned scenario) {
    for (u32 k = 0; k < 256u; k += 4u)
        write_be32(ram + (AREA - GC_RAM_BASE) + k, random_float_bits());
    CPUState c;
    memset(&c, 0, sizeof c);
    c.ram = ram;
    c.ram_size = RAM_SIZE;
    for (unsigned r = 0; r < 32; ++r) {
        c.gpr[r] = next();
        c.fpr[r] = f64_value(convert_to_double(random_float_bits()));
        c.ps1[r] = f64_value(convert_to_double(random_float_bits()));
    }
    c.ps1[7] = 1.0 / 3.0; /* not a single */
    /* Operands: separate, the output on an input, or overlapping it. */
    const u32 a = AREA + 4u * (next() % 8u), b = AREA + 64u + 4u * (next() % 8u);
    u32 out = AREA + 128u + 4u * (next() % 8u);
    const u32 shape = scenario % 6u;
    if (shape == 1u) out = a;
    if (shape == 2u) out = b;
    if (shape == 3u) out = a + 4u;
    if (shape == 4u) out = b + 8u;
    c.gpr[3] = a;
    c.gpr[4] = leaf == BLUEWAKE_PSVEC_SCALE ? out : b;
    c.gpr[5] = out;
    /* PSVECNormalize's SDA constants, 0.5 and 3.0, at r2-12884 (sometimes others). */
    c.gpr[2] = AREA + 192u + 12884u;
    write_be32(ram + (AREA - GC_RAM_BASE) + 192u, scenario % 9u == 4u ? random_float_bits() : 0x3F000000u);
    write_be32(ram + (AREA - GC_RAM_BASE) + 196u, scenario % 9u == 4u ? random_float_bits() : 0x40400000u);
    if ((leaf == BLUEWAKE_PSVEC_NORMALIZE || leaf == BLUEWAKE_PSVEC_MAG) && scenario % 13u == 5u)
        memset(ram + (a - GC_RAM_BASE), 0, 12); /* a zero vector: frsqrte of zero */
    if (leaf == BLUEWAKE_PSVEC_SCALE && scenario % 5u == 2u) c.fpr[1] = 1e30; /* past the bound: declines */
    if (scenario % 41u == 7u) c.gpr[3] = 0xCC000000u;                          /* not RAM: declines */
    c.lr = 0xFFFFFFFCu;
    c.pc = leaf;
    c.ctr = next();
    c.cr = next();
    c.xer = next();
    c.msr = PPC_MSR_FP;
    c.hid2 = PPC_HID2_LSQE;
    c.gqr[0] = scenario % 5u == 1u ? 0x3F003F00u : 0u;
    if (scenario % 37u == 9u) c.gqr[0] = 0x00070007u;
    c.fpscr = next(); /* Include rounding modes and exception-enable flags. */
    if (scenario % 3u == 0u) c.fpscr |= 0x4u;
    c.cycle_budget = 16384;
    c.downcount = -(s64)(next() % 64u);
    c.cycle_deadline_budget = scenario % 4u == 0u ? 0 : 100000;
    if (scenario % 17u == 2u) c.cycle_deadline_budget = 70 + (s64)(next() % 16u); /* near the block */
    if (scenario % 19u == 3u) c.downcount = -16384;                             /* budget spent: declines */
    if (scenario % 6u == 4u) {
        c.reserve_valid = true;
        c.reserve_addr = out & ~31u;
    }
    c.cycle_observation_suffix = next();
    switch (scenario % 47u) {
    case 1: c.ram = NULL; break;
    case 2: c.ram_size = 1; break;
    case 3: c.exception = 1; break;
    case 4: c.msr = 0; break;
    case 5: c.hid2 = 0; break;
    case 6: c.cycle_budget = 0; break;
    case 7: c.cycle_budget = 1; c.downcount = 0; break;
    case 8: c.cycle_deadline_budget = 1; break;
    default: break;
    }
    return c;
}

typedef struct Module {
    const StaticRecompModuleDesc* desc;
    CPUState fallback;
    BlueWakeModuleStorage storage;
    int (*native_vec)(bool, BluewakeNativeVecReady, void*);
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
    module->native_vec = (int (*)(bool, BluewakeNativeVecReady, void*))SYMBOL("bluewake_composite_native_vec_v1");
    module->report = (void (*)(void))SYMBOL("bluewake_native_vec_report");
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
static bool routed_ready(void* user, const CPUState* cpu, u32 address) {
    (void)user; (void)cpu; (void)address;
    ++routed_queries;
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "usage: native_vec_test ORIGINAL_MODULE [CASES] [ROUTED_MODULE]\n");
        return 2;
    }
    const unsigned cases = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 20000u;
    Module original = {0}, candidate = {0};
    if (!load_module(argv[1], &original)) return 1;
    const StaticRecompModuleDesc* mod = original.desc;
    if (strcmp(mod->game_id, "GZLE01") != 0) return 1;
    const StaticRecompModuleDesc* routed = NULL;
    u8* routed_ram = NULL;
    if (argc > 3) {
        if (!load_module(argv[3], &candidate)) return 1;
        if (!candidate.native_vec || !candidate.native_vec(true, routed_ready, NULL)) {
            fprintf(stderr, "candidate lacks native vector handshake\n"); return 1;
        }
        routed = candidate.desc;
        routed_ram = candidate.storage.mem1;
    }
    u8* reference_ram = original.storage.mem1;
    u8* native_ram = calloc(1, RAM_SIZE);
    if (!reference_ram || !native_ram) return 1;
    unsigned ran[LEAF_COUNT] = {0}, declined[LEAF_COUNT] = {0}, routed_ran = 0;
    for (unsigned i = 0; i < cases; ++i) {
        const unsigned which = i % (unsigned)LEAF_COUNT;
        const u32 leaf = LEAVES[which];
        CPUState c = build(native_ram, leaf, i / (unsigned)LEAF_COUNT);
        ppc_fpscr_updated(&c);
        CPUState native = c;
        CPUState untouched = native;
        u8 area_before[256];
        memcpy(area_before, native_ram + (AREA - GC_RAM_BASE), 256);
        if (!bluewake_native_vec(&native, leaf)) {
            declined[which]++;
            if (memcmp(&native, &untouched, sizeof native) != 0 ||
                memcmp(native_ram + (AREA - GC_RAM_BASE), area_before, 256) != 0) {
                fprintf(stderr, "case %u (%08X): declined but changed state\n", i, leaf);
                return 1;
            }
            continue;
        }
        ran[which]++;
        memcpy(reference_ram + (AREA - GC_RAM_BASE), area_before, 256);

        CPUState* g = original.storage.cpu;
        *g = c;
        g->ram = reference_ram;
        mod->on_state_loaded(g);
        if (!mod->dispatch(g, g->pc)) {
            fprintf(stderr, "case %u: the translation did not run\n", i);
            return 1;
        }
        CPUState reference = *g;

        reference.ram = native.ram;
        if (memcmp(&native, &reference, sizeof native) != 0 ||
            memcmp(native_ram + (AREA - GC_RAM_BASE), reference_ram + (AREA - GC_RAM_BASE), 256) != 0) {
            fprintf(stderr, "case %u (%08X, seed %08X): mismatch (fpr at %u, ps1 at %u)\n", i, leaf, seed,
                    (unsigned)offsetof(CPUState, fpr), (unsigned)offsetof(CPUState, ps1));
            for (unsigned b = 0; b < sizeof native; ++b)
                if (((u8*)&native)[b] != ((u8*)&reference)[b])
                    fprintf(stderr, "  CPU byte %u: got %02X want %02X\n", b, ((u8*)&native)[b], ((u8*)&reference)[b]);
            for (u32 k = 0; k < 256u; ++k)
                if (native_ram[AREA - GC_RAM_BASE + k] != reference_ram[AREA - GC_RAM_BASE + k])
                    fprintf(stderr, "  RAM %08X: got %02X want %02X\n", AREA + k, native_ram[AREA - GC_RAM_BASE + k],
                            reference_ram[AREA - GC_RAM_BASE + k]);
            return 1;
        }
        if (routed) {
            memcpy(routed_ram + AREA - GC_RAM_BASE, area_before, 256);
            CPUState* h = candidate.storage.cpu;
            *h = c; h->ram = routed_ram; routed->on_state_loaded(h);
            if (!routed->dispatch(h, leaf)) return 1;
            CPUState routed_result = *h;
            routed_result.ram = reference.ram;
            if (memcmp(&routed_result, &reference, sizeof reference) ||
                memcmp(routed_ram + AREA - GC_RAM_BASE, reference_ram + AREA - GC_RAM_BASE, 256)) {
                fprintf(stderr, "case %u (%08X): routed module differs from original\n", i, leaf); return 1;
            }
            ++routed_ran;
        }
    }
    unsigned total = 0;
    for (unsigned k = 0; k < LEAF_COUNT; ++k) {
        printf("%08X: %u identical, %u declined unchanged\n", LEAVES[k], ran[k], declined[k]);
        if (!ran[k]) return 1;
        total += ran[k];
    }
    if (routed) {
        if (!routed_queries) { fprintf(stderr, "native vector routing was inactive\n"); return 1; }
        printf("routed module: %u identical calls, %u readiness queries\n", routed_ran, routed_queries);
        if (candidate.report) candidate.report();
    }
    return total != 0u ? 0 : 1;
}
