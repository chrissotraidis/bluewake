/* Recovered J3D transforms against the untouched GZLE01 translations.
 *
 * clang -O2 -march=x86-64-v3 -ffp-contract=off -Icmake/composite
 *   -Iref/recompcore/GXRuntime/include
 *   -Iref/recompcore/Source/Core/Core/PowerPC/StaticRecomp
 *   tests/native_j3d_test.c cmake/composite/native_j3d.c
 *   build/windows/app/gxruntime_build/gxruntime.lib -o native_j3d_test.exe
 * native_j3d_test MODULE.dll [CASES=100000] [BENCH_CALLS=1000000] [ROUTED.dll]
 *
 * Compare every CPU byte and the complete RAM test area. Declines must leave
 * both untouched. The microbenchmark includes module dispatch overhead; it
 * measures a leaf call, not a whole-game FPS improvement.
 */
#include "native_j3d.h"
#include "StaticRecompABI.h"

#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define AREA 0x80100000u
#define AREA_SIZE 0xA000u
#define SIN_TABLE AREA
#define COS_TABLE (AREA + 0x4000u)
#define INFO (AREA + 0x9000u)
#define OUTPUT (AREA + 0x9100u)
#define GLOBALS (AREA + 0x9200u)

static u32 seed = 0x731245ABu;
static u32 next(void) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static u32 trig_bits(void) {
    const u32 sign = next() & 0x80000000u;
    switch (next() % 16u) {
    case 0: return sign;
    case 1: return sign | (next() & 0x007FFFFFu);
    case 2: return sign | 0x00800000u;
    case 3: return sign | 0x3F800000u;
    case 4: return sign | 0x44800000u;
    default: return sign | ((1u + next() % 137u) << 23) | (next() & 0x007FFFFFu);
    }
}

static void put(u8* ram, u32 address, u32 bits) {
    write_be32(ram + address - GC_RAM_BASE, bits);
}

static CPUState build(u8* ram, u32 leaf, unsigned scenario) {
    CPUState c;
    memset(&c, 0, sizeof c);
    c.ram = ram;
    c.ram_size = GC_MAIN_RAM_SIZE;
    for (unsigned r = 0; r < 32; ++r) {
        c.gpr[r] = next();
        c.fpr[r] = f64_value(((u64)next() << 32) | next());
        c.ps1[r] = f64_value(((u64)next() << 32) | next());
    }
    for (unsigned k = 0; k < 256; k += 4)
        put(ram, INFO + k, next());
    const u32 shift = scenario % 7u == 0u ? next() & 63u : 4u;
    const unsigned effective_shift = shift < 32u ? shift : 32u;
    const u32 mask = shift < 4u ? (0xFFFu << shift) : 0xFFFFu;
    u32 angles[3];
    for (unsigned k = 0; k < 3; ++k) {
        angles[k] = next() & mask;
        const u32 offset = effective_shift == 32u ? 0u : (angles[k] >> effective_shift) * 4u;
        put(ram, SIN_TABLE + offset, trig_bits());
        put(ram, COS_TABLE + offset, trig_bits());
        write_be16(ram + INFO - GC_RAM_BASE + 12u + k * 2u, (u16)angles[k]);
    }
    put(ram, GLOBALS, shift);
    put(ram, GLOBALS + 4, SIN_TABLE);
    put(ram, GLOBALS + 8, COS_TABLE);
    const bool info = leaf == BLUEWAKE_J3D_TRANSFORM_INFO;
    c.gpr[3] = info ? INFO : angles[0] | (next() & 0xFFFF0000u);
    c.gpr[4] = info ? OUTPUT : angles[1] | (next() & 0xFFFF0000u);
    c.gpr[5] = info ? next() : angles[2] | (next() & 0xFFFF0000u);
    c.gpr[6] = OUTPUT;
    c.gpr[13] = GLOBALS + 26460u;
    c.lr = 0xFFFFFFFCu | (next() & 3u);
    c.pc = leaf;
    c.ctr = next();
    c.cr = next();
    c.xer = next();
    c.msr = PPC_MSR_FP;
    c.hid2 = next();
    for (unsigned k = 0; k < 8; ++k) c.gqr[k] = next();
    c.fpscr = next() & ~3u;
    c.cycle_budget = scenario % 11u == 0u ? 1 : 16384;
    c.downcount = scenario % 11u == 0u ? 0 : -(s64)(next() % 64u);
    c.cycle_deadline_budget = scenario % 4u == 0u ? 0 : 100000;
    const unsigned cycles = info ? 54 : 48;
    if (scenario % 13u == 1u)
        c.cycle_deadline_budget = cycles - c.downcount + (s64)(scenario % 3u) - 1;
    if (scenario % 6u == 0u) {
        c.reserve_valid = true;
        c.reserve_addr = OUTPUT & ~31u;
    }
    /* Declining cases and memory shapes that must still be exact if accepted. */
    switch (scenario % 41u) {
    case 1: c.gpr[info ? 4 : 6] = INFO + 16u; break;
    case 2: c.gpr[info ? 4 : 6] = OUTPUT + 1u; break;
    case 3: c.gpr[info ? 4 : 6] = 0xCC000000u; break;
    case 4: c.gpr[13] = 0xCC000000u + 26460u; break;
    case 5: c.msr = 0; break;
    case 6: c.fpscr |= 1; break;
    case 7: c.downcount = -c.cycle_budget; break;
    case 8: c.exception = 1; break;
    case 9: put(ram, GLOBALS + 4, 0xCC000000u); break;
    case 10: put(ram, SIN_TABLE + ((angles[0] >> (shift < 32 ? shift : 31)) * 4u), 0x7F800001u); break;
    case 11: c.gpr[info ? 4 : 6] = SIN_TABLE; break;
    default: break;
    }
    return c;
}

static void journal(u32 address, u32 size, void* opaque) {
    (void)address; (void)size; (void)opaque;
}

static double elapsed(LARGE_INTEGER start, LARGE_INTEGER stop, LARGE_INTEGER freq, unsigned count) {
    return (double)(stop.QuadPart - start.QuadPart) * 1e9 / (double)freq.QuadPart / count;
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
        fprintf(stderr, "usage: native_j3d_test MODULE.dll [CASES] [BENCH_CALLS] [ROUTED.dll]\n");
        return 2;
    }
    _putenv_s("BLUEWAKE_NATIVE_MATH", "0");
    _putenv_s("BLUEWAKE_NATIVE_J3D", "0");
    const unsigned cases = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 100000u;
    const unsigned bench_calls = argc > 3 ? (unsigned)strtoul(argv[3], NULL, 10) : 1000000u;
    HMODULE lib = LoadLibraryA(argv[1]);
    if (!lib) { fprintf(stderr, "cannot load %s (%lu)\n", argv[1], GetLastError()); return 1; }
    StaticRecompGetModuleFn get = (StaticRecompGetModuleFn)(void*)GetProcAddress(lib, STATICRECOMP_GET_MODULE_SYMBOL);
    CPUState* (*guest_cpu)(void) = (CPUState* (*)(void))(void*)GetProcAddress(lib, "bluewake_composite_guest_cpu");
    if (!get || !guest_cpu) { fprintf(stderr, "not a BlueWake module\n"); return 1; }
    const StaticRecompModuleDesc* mod = get();
    if (mod->cpu_state_size != sizeof(CPUState) || strcmp(mod->game_id, "GZLE01")) {
        fprintf(stderr, "incompatible CPU ABI/game ID\n"); return 1;
    }
    const StaticRecompModuleDesc* routed = NULL;
    CPUState* (*routed_cpu)(void) = NULL;
    u8* routed_ram = NULL;
    if (argc > 4) {
        _putenv_s("BLUEWAKE_NATIVE_MATH", "1");
        _putenv_s("BLUEWAKE_NATIVE_J3D", "1");
        HMODULE other = LoadLibraryA(argv[4]);
        if (!other) { fprintf(stderr, "cannot load routed module\n"); return 1; }
        StaticRecompGetModuleFn routed_get = (StaticRecompGetModuleFn)(void*)GetProcAddress(other, STATICRECOMP_GET_MODULE_SYMBOL);
        routed_cpu = (CPUState* (*)(void))(void*)GetProcAddress(other, "bluewake_composite_guest_cpu");
        if (!routed_get || !routed_cpu) return 1;
        routed = routed_get();
        if (routed->cpu_state_size != sizeof(CPUState)) return 1;
        routed_ram = module_mem1(other);
        if (routed_ram == NULL) routed_ram = calloc(1, GC_MAIN_RAM_SIZE);
        if (!routed_ram) return 1;
    }
    u8* native_ram = calloc(1, GC_MAIN_RAM_SIZE);
    u8* reference_ram = module_mem1(lib);
    if (reference_ram == NULL) reference_ram = calloc(1, GC_MAIN_RAM_SIZE);
    u8* before = malloc(AREA_SIZE);
    if (!native_ram || !reference_ram || !before) return 1;
    for (u32 k = 0; k < 0x8000u; k += 4u) put(native_ram, AREA + k, trig_bits());
    const u32 leaves[] = {BLUEWAKE_J3D_TRANSFORM_INFO, BLUEWAKE_J3D_TRANSFORM_ANGLES};
    unsigned ran[2] = {0}, declined[2] = {0}, routed_ran = 0;
    for (unsigned i = 0; i < cases; ++i) {
        const unsigned which = i % 2u;
        const u32 leaf = leaves[which];
        CPUState c = build(native_ram, leaf, i / 2u);
        ppc_fpscr_updated(&c);
        CPUState native = c;
        memcpy(before, native_ram + AREA - GC_RAM_BASE, AREA_SIZE);
        if (i % 127u == 3u) g_mem_write_journal = journal;
        const bool accepted = bluewake_native_j3d_transform(&native, leaf) != 0;
        g_mem_write_journal = NULL;
        if (!accepted) {
            ++declined[which];
            if (memcmp(&native, &c, sizeof c) || memcmp(before, native_ram + AREA - GC_RAM_BASE, AREA_SIZE)) {
                fprintf(stderr, "case %u: declined but mutated CPU/RAM\n", i); return 1;
            }
            continue;
        }
        ++ran[which];
        memcpy(reference_ram + AREA - GC_RAM_BASE, before, AREA_SIZE);
        CPUState* g = guest_cpu();
        *g = c;
        g->ram = reference_ram;
        mod->on_state_loaded(g);
        if (!mod->dispatch(g, leaf)) { fprintf(stderr, "translation did not run\n"); return 1; }
        CPUState reference = *g;
        reference.ram = native.ram;
        reference.cycle_observation_suffix = native.cycle_observation_suffix;
        if (memcmp(&native, &reference, sizeof native) ||
            memcmp(native_ram + AREA - GC_RAM_BASE, reference_ram + AREA - GC_RAM_BASE, AREA_SIZE)) {
            fprintf(stderr, "case %u (%08X, seed %08X): mismatch, FPR offset %u, PS1 offset %u\n",
                    i, leaf, seed, (unsigned)offsetof(CPUState, fpr), (unsigned)offsetof(CPUState, ps1));
            for (unsigned b = 0; b < sizeof native; ++b)
                if (((u8*)&native)[b] != ((u8*)&reference)[b])
                    fprintf(stderr, " CPU byte %u got %02X want %02X\n", b, ((u8*)&native)[b], ((u8*)&reference)[b]);
            for (u32 k = 0; k < AREA_SIZE; ++k)
                if (native_ram[AREA - GC_RAM_BASE + k] != reference_ram[AREA - GC_RAM_BASE + k])
                    fprintf(stderr, " RAM %08X got %02X want %02X\n", AREA + k,
                            native_ram[AREA - GC_RAM_BASE + k], reference_ram[AREA - GC_RAM_BASE + k]);
            return 1;
        }
        if (routed) {
            memcpy(routed_ram + AREA - GC_RAM_BASE, before, AREA_SIZE);
            CPUState* h = routed_cpu();
            *h = c; h->ram = routed_ram; routed->on_state_loaded(h);
            if (!routed->dispatch(h, leaf)) return 1;
            CPUState routed_result = *h;
            routed_result.ram = reference.ram;
            routed_result.cycle_observation_suffix = reference.cycle_observation_suffix;
            if (memcmp(&routed_result, &reference, sizeof reference) ||
                memcmp(routed_ram + AREA - GC_RAM_BASE, reference_ram + AREA - GC_RAM_BASE, AREA_SIZE)) {
                fprintf(stderr, "case %u (%08X): routed module differs from original\n", i, leaf); return 1;
            }
            ++routed_ran;
        }
    }
    for (unsigned k = 0; k < 2; ++k)
        printf("%08X: %u identical, %u declined unchanged\n", leaves[k], ran[k], declined[k]);
    if (routed) printf("routed module: %u identical calls\n", routed_ran);
    if (!ran[0] || !ran[1]) return 1;
    LARGE_INTEGER freq, start, stop;
    QueryPerformanceFrequency(&freq);
    for (unsigned k = 0; k < 2 && bench_calls; ++k) {
        CPUState c = build(native_ram, leaves[k], 12);
        /* Performance uses ordinary trigonometric values. The differential
         * cases above deliberately include many host-slow subnormals. */
        for (u32 j = 0; j < 4096u; ++j) {
            f32 sine = sinf((f32)j * 0.0015339807878856412f);
            f32 cosine = cosf((f32)j * 0.0015339807878856412f);
            u32 bits;
            memcpy(&bits, &sine, 4); put(native_ram, SIN_TABLE + j * 4, bits);
            memcpy(&bits, &cosine, 4); put(native_ram, COS_TABLE + j * 4, bits);
        }
        c.fpscr = 0;
        c.downcount = 0;
        c.cycle_deadline_budget = 0;
        c.cycle_budget = (s64)bench_calls * 60 + 1000;
        ppc_fpscr_updated(&c);
        memcpy(reference_ram + AREA - GC_RAM_BASE, native_ram + AREA - GC_RAM_BASE, AREA_SIZE);
        CPUState* g = guest_cpu();
        *g = c; g->ram = reference_ram; mod->on_state_loaded(g);
        QueryPerformanceCounter(&start);
        for (unsigned i = 0; i < bench_calls; ++i) {
            g->gpr[3] = c.gpr[3]; g->gpr[4] = c.gpr[4]; g->gpr[5] = c.gpr[5]; g->gpr[6] = c.gpr[6];
            g->pc = leaves[k];
            if (!mod->dispatch(g, leaves[k])) return 1;
        }
        QueryPerformanceCounter(&stop);
        const double translated_ns = elapsed(start, stop, freq, bench_calls);
        CPUState native = c;
        QueryPerformanceCounter(&start);
        for (unsigned i = 0; i < bench_calls; ++i) {
            native.gpr[3] = c.gpr[3]; native.gpr[4] = c.gpr[4]; native.gpr[5] = c.gpr[5]; native.gpr[6] = c.gpr[6];
            native.pc = leaves[k];
            if (!bluewake_native_j3d_transform(&native, leaves[k])) return 1;
        }
        QueryPerformanceCounter(&stop);
        const double native_ns = elapsed(start, stop, freq, bench_calls);
        printf("%08X: translated %.1f ns/call, native %.1f ns/call, %.2fx\n",
               leaves[k], translated_ns, native_ns, translated_ns / native_ns);
    }
    return 0;
}
