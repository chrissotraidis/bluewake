/* cmake/composite/native_vec.c against the translations it stands in for.
 *
 *   native_vec_test MODULE.dll [CASES]
 *
 * For each vector leaf: random vectors (zeros, denormals, large values,
 * sometimes past the bound), operands that alias the output or overlap it,
 * random registers, FPSCR and cycle state; the leaf through the personal
 * module's translation and through bluewake_native_vec; every byte of the CPU
 * state and of RAM must match - or, where the native declines, nothing may
 * have changed. Windows, built like tests/native_skin_test.c with
 * cmake/composite/native_vec.c. */
#include "native_vec.h"
#include "StaticRecompABI.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

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
    c.fpscr = next() & 0xFFFFF000u;
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
    return c;
}

/* A module built with BW_GUEST_MEM1 (the Windows builder's) runs its
 * translated code on its own MEM1 array, whatever a state's ram says: the
 * translated side's RAM has to be that array. Zeroed and returned, or NULL
 * for a module without one. */
static u8* module_mem1(HMODULE lib) {
    u8* (*mem1)(u32*) = (u8* (*)(u32*))(void*)GetProcAddress(lib, "bluewake_composite_guest_mem1");
    u32 size = 0;
    u8* ram = mem1 != NULL ? mem1(&size) : NULL;
    if (ram == NULL)
        return NULL;
    if (size < GC_MAIN_RAM_SIZE) {
        fprintf(stderr, "the module's MEM1 is 0x%X bytes\n", size);
        exit(1);
    }
    memset(ram, 0, GC_MAIN_RAM_SIZE);
    return ram;
}

/* The cycle observation suffix is dead after an access to RAM: since
 * scripts/windows/lean_memory.py the translation stores it only on the way
 * to an MMIO or timebase handler, its only readers. It is not compared. */
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: native_vec_test MODULE.dll [CASES]\n");
        return 2;
    }
    const unsigned cases = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 20000u;
    HMODULE lib = LoadLibraryA(argv[1]);
    if (lib == NULL) {
        fprintf(stderr, "cannot load %s\n", argv[1]);
        return 1;
    }
    StaticRecompGetModuleFn get = (StaticRecompGetModuleFn)(void*)GetProcAddress(lib, STATICRECOMP_GET_MODULE_SYMBOL);
    CPUState* (*guest_cpu)(void) = (CPUState * (*)(void))(void*)GetProcAddress(lib, "bluewake_composite_guest_cpu");
    if (get == NULL || guest_cpu == NULL) {
        fprintf(stderr, "not a BlueWake Windows module\n");
        return 1;
    }
    const StaticRecompModuleDesc* mod = get();
    if (mod->cpu_state_size != sizeof(CPUState) || strcmp(mod->game_id, "GZLE01") != 0) {
        fprintf(stderr, "CPU state size %u, expected %u\n", mod->cpu_state_size, (unsigned)sizeof(CPUState));
        return 1;
    }
    u8* reference_ram = module_mem1(lib);
    if (reference_ram == NULL) reference_ram = calloc(1, RAM_SIZE);
    u8* native_ram = calloc(1, RAM_SIZE);
    if (!reference_ram || !native_ram) return 1;
    unsigned ran[LEAF_COUNT] = {0}, declined[LEAF_COUNT] = {0};
    for (unsigned i = 0; i < cases; ++i) {
        const unsigned which = i % (unsigned)LEAF_COUNT;
        const u32 leaf = LEAVES[which];
        CPUState c = build(native_ram, leaf, i / (unsigned)LEAF_COUNT);
        memcpy(reference_ram + (AREA - GC_RAM_BASE), native_ram + (AREA - GC_RAM_BASE), 256);

        CPUState* g = guest_cpu();
        *g = c;
        g->ram = reference_ram;
        mod->on_state_loaded(g);
        if (!mod->dispatch(g, g->pc)) {
            fprintf(stderr, "case %u: the translation did not run\n", i);
            return 1;
        }
        CPUState reference = *g;

        CPUState native = c;
        ppc_fpscr_updated(&native);
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
        reference.ram = native.ram;
        reference.cycle_observation_suffix = native.cycle_observation_suffix;
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
    }
    unsigned total = 0;
    for (unsigned k = 0; k < LEAF_COUNT; ++k) {
        printf("%08X: %u identical, %u declined unchanged\n", LEAVES[k], ran[k], declined[k]);
        total += ran[k];
    }
    return total != 0u ? 0 : 1;
}
