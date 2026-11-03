/* Elliott's J3D differential fixture, adapted for BlueWake's declared module
 * storage ABIs on Windows and POSIX. Compare the complete CPU (including the
 * observation suffix) and the RAM test area. Personal modules stay local.
 * Usage: native_j3d_test ORIGINAL_MODULE [CASES] [ROUTED_MODULE]
 */
#include "native_j3d.h"
#include "StaticRecompABI.h"

#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module_cpu_contract.h"
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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
    case 12: c.ram = NULL; break;
    case 13: c.ram_size = 1; break;
    case 14: c.ram_size = 0; break;
    default: break;
    }
    return c;
}

static void journal(u32 address, u32 size, void* opaque) {
    (void)address; (void)size; (void)opaque;
}

typedef struct Module {
    const StaticRecompModuleDesc* desc;
    CPUState fallback;
    BlueWakeModuleStorage storage;
    int (*native_j3d)(bool, BluewakeNativeJ3DReady, void*);
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
    module->native_j3d = (int (*)(bool, BluewakeNativeJ3DReady, void*))SYMBOL("bluewake_composite_native_j3d_v1");
    module->report = (void (*)(void))SYMBOL("bluewake_native_j3d_report");
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
        fprintf(stderr, "usage: native_j3d_test ORIGINAL_MODULE [CASES] [ROUTED_MODULE]\n");
        return 2;
    }
#if defined(_WIN32)
    _putenv_s("BLUEWAKE_NATIVE_MATH", "0");
    _putenv_s("BLUEWAKE_NATIVE_J3D", "0");
#else
    setenv("BLUEWAKE_NATIVE_MATH", "0", 1);
    setenv("BLUEWAKE_NATIVE_J3D", "0", 1);
#endif
    const unsigned cases = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 100000u;
    Module original = {0}, candidate = {0};
    if (!load_module(argv[1], &original)) return 1;
    const StaticRecompModuleDesc* mod = original.desc;
    const StaticRecompModuleDesc* routed = NULL;
    u8* routed_ram = NULL;
    if (argc > 3) {
        if (!load_module(argv[3], &candidate)) return 1;
        if (!candidate.native_j3d || !candidate.native_j3d(true, routed_ready, NULL)) {
            fprintf(stderr, "candidate does not support native J3D handshake\n"); return 1;
        }
        routed = candidate.desc;
        routed_ram = candidate.storage.mem1;
    }
    u8* native_ram = calloc(1, GC_MAIN_RAM_SIZE);
    u8* reference_ram = original.storage.mem1;
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
        CPUState* g = original.storage.cpu;
        *g = c;
        g->ram = reference_ram;
        mod->on_state_loaded(g);
        if (!mod->dispatch(g, leaf)) { fprintf(stderr, "translation did not run\n"); return 1; }
        CPUState reference = *g;
        reference.ram = native.ram;
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
            CPUState* h = candidate.storage.cpu;
            *h = c; h->ram = routed_ram; routed->on_state_loaded(h);
            if (!routed->dispatch(h, leaf)) return 1;
            CPUState routed_result = *h;
            routed_result.ram = reference.ram;
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
    if (routed) {
        if (routed_queries == 0) { fprintf(stderr, "native J3D routing was inactive\n"); return 1; }
        printf("native J3D routing: %u readiness queries\n", routed_queries);
        if (candidate.report) candidate.report();
    }
    return 0;
}
