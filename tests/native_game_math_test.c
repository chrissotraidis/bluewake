/* Full CPU and all 24 MiB RAM comparison, plus per-call microbenchmarks.
 * Run from E:\Github\Wind-Waker-Recomp-natives in an x64 VS tools shell:
 *
 * mkdir .native-study
 * "E:\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang.exe" -O2 -march=x86-64-v3 -ffp-contract=off -Icmake/composite -IE:\Github\Wind-Waker-Recomp\ref\recompcore\GXRuntime\include -IE:\Github\Wind-Waker-Recomp\ref\recompcore\Source\Core\Core\PowerPC\StaticRecomp tests/native_game_math_test.c cmake/composite/native_game_math.c E:\Github\Wind-Waker-Recomp\build\windows\app\gxruntime_build\gxruntime.lib -o .native-study/native_game_math_test.exe
 * .native-study\native_game_math_test.exe E:\Github\Wind-Waker-Recomp\build\windows\BlueWake-test\gGZLE01_recomp.dll 100000
 *
 * The case count is PER entry, not divided among entries. Disable the DLL's
 * natives before loading it. Declines compare the entire untouched state and
 * RAM without running unsafe translation inputs (e.g. MMIO/watched memory).
 * Both 24 MiB images start identical; all pages outside the randomized data
 * area are read-only, so an unexpected write fails the test immediately.
 * Compare every writable byte per case and the full images after each entry:
 * this verifies all RAM bytes without rescanning 24 MiB of immutable pages.
 * Accepted cases dispatch the original DLL and compare every byte. Benchmarks
 * include identical CPU reset overhead on both paths; no BlueWake.exe runs.
 */
#include "StaticRecompABI.h"
#include "native_game_math.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define AREA 0x80100000u
#define STACK (AREA + 0x1000u)
#define TABLE (AREA + 0x2000u)
#define DATA_BYTES 0x6000u
static const u32 entries[] = {0x80245674u, 0x802456C4u, 0x80245714u, 0x80245760u, 0x8024A8E0u,
                              0x8000CD28u, 0x8000CDC8u, 0x8000CE68u, 0x8024AE3Cu, 0x80256888u,
                              0x802569D0u, 0x802F072Cu, 0x802F0954u};
static u32 seed = 0x63E59A71u;
static u32 next(void) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}
static u32 fpbits(void) {
    u32 b = next();
    if ((b & 15u) == 0)
        return b & 0x80000000u;
    return (b & 0x807FFFFFu) | ((70u + next() % 87u) << 23);
}
static void put(u8* ram, u32 a, u32 b) { write_be32(ram + a - GC_RAM_BASE, b); }
static void journal(u32 a, u32 n, void* user) {
    (void)a;
    (void)n;
    (void)user;
    abort();
}
static int edge_service(void* user, CPUState* cpu, u32 at) {
    (void)user;
    (void)cpu;
    return at == 0xFFFFFFFCu;
}
static u8* protect_image(u8* p) {
    DWORD old;
    if (!p || !VirtualProtect(p, GC_MAIN_RAM_SIZE, PAGE_READONLY, &old) ||
        !VirtualProtect(p + AREA - GC_RAM_BASE, DATA_BYTES, PAGE_READWRITE, &old))
        return NULL;
    return p;
}
static u8* image(void) {
    return protect_image(VirtualAlloc(NULL, GC_MAIN_RAM_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
}
/* A module built with BW_GUEST_MEM1 (the Windows builder's) runs its
 * translated code on its own MEM1 array (page aligned), whatever a state's
 * ram says: the translated side's image has to be that array. */
static u8* module_image(HMODULE lib) {
    u8* (*mem1)(u32*) = (u8* (*)(u32*))(void*)GetProcAddress(lib, "bluewake_composite_guest_mem1");
    u32 size = 0;
    u8* ram = mem1 != NULL ? mem1(&size) : NULL;
    if (ram == NULL)
        return image();
    if (size < GC_MAIN_RAM_SIZE)
        return NULL;
    memset(ram, 0, GC_MAIN_RAM_SIZE);
    return protect_image(ram);
}
static int ram_diff(const u8* a, const u8* b) {
    if (!memcmp(a + AREA - GC_RAM_BASE, b + AREA - GC_RAM_BASE, DATA_BYTES))
        return 0;
    for (u32 i = 0; i < GC_MAIN_RAM_SIZE; ++i)
        if (a[i] != b[i]) {
            fprintf(stderr, "RAM %08X: %02X != %02X\n", GC_RAM_BASE + i, a[i], b[i]);
            break;
        }
    return 1;
}
static CPUState build(u8* ram, unsigned which, unsigned scenario, bool edges) {
    for (unsigned k = 0; k < DATA_BYTES; k += 4)
        put(ram, AREA + k, fpbits());
    CPUState c = {0};
    c.ram = ram;
    c.ram_size = GC_MAIN_RAM_SIZE;
    for (unsigned r = 0; r < 32; ++r) {
        c.gpr[r] = next();
        c.fpr[r] = f64_value(((u64)next() << 32) | next());
        c.ps1[r] = f64_value(((u64)next() << 32) | next());
    }
    c.fpr[1] = f64_value(convert_to_double(fpbits()));
    if (scenario % 11u == 0u)
        c.fpr[1] = f64_value(((u64)(1003u + next() % 41u) << 52) | ((u64)next() << 20) |
                            (next() & 0xFFFFFu)); /* non-single finite double */
    c.gpr[1] = STACK;
    c.gpr[3] = AREA + 0x100u;
    c.gpr[4] = AREA + 0x200u;
    c.gpr[5] = AREA + 0x300u;
    c.gpr[13] = AREA + 0x500u + 26460u;
    c.gpr[2] = AREA + 0x600u + 32584u;
    put(ram, AREA + 0x500u, next() % 40u);
    put(ram, AREA + 0x504u, TABLE);
    put(ram, AREA + 0x508u, TABLE + 0x2000u);
    put(ram, AREA + 0x600u, 0x3F800000u);
    put(ram, AREA + 0x604u, 0u);
    if (which >= 5 && which <= 7) {
        c.gpr[4] = next();
        /* Keep the index inside our synthetic tables; shifts >= 32 tested. */
        put(ram, AREA + 0x500u, 3u + next() % 37u);
        if (scenario % 17u == 0u) {
            put(ram, AREA + 0x600u, fpbits());
            put(ram, AREA + 0x604u, fpbits());
        }
    }
    if (which == 4) {
        /* Ordered boxes and cylinder: drive all six return paths. */
        for (unsigned k = 0; k < 3; ++k) {
            put(ram, c.gpr[3] + 4 * k, 0xC1200000u);
            put(ram, c.gpr[3] + 12 + 4 * k, 0x41200000u);
            put(ram, c.gpr[4] + 4 * k, 0u);
        }
        put(ram, c.gpr[4] + 12, 0x3F800000u);
        put(ram, c.gpr[4] + 16, 0x3F800000u);
        const unsigned path = scenario % 7u;
        if (path < 6) {
            unsigned axis = path < 2 ? 0 : path < 4 ? 2 : 1;
            put(ram, c.gpr[4] + 4 * axis, path % 2 ? 0xC1A00000u : 0x41A00000u);
        }
        if (scenario % 8u == 0u)
            for (unsigned k = 0; k < 6; ++k)
                put(ram, c.gpr[3] + 4 * k, fpbits());
    }
    if (which == 8) {
        c.gpr[6] = AREA + 0x400u;
        const u32 masks[] = {1, 2, 16, 32, 4, 8};
        for (unsigned k = 0; k < 6; ++k)
            put(ram, c.gpr[2] - 16512u + 4 * k, masks[k]);
        for (unsigned k = 0; k < 3; ++k) {
            put(ram, c.gpr[3] + 4 * k, 0xC1200000u);
            put(ram, c.gpr[4] + 4 * k, 0x41200000u);
            put(ram, c.gpr[5] + 4 * k, 0);
            put(ram, c.gpr[6] + 4 * k, 0);
        }
        unsigned path = scenario % 12u;
        if (path < 6) {
            const u32 v = path % 2 ? 0xC1A00000u : 0x41A00000u;
            put(ram, c.gpr[5] + 4 * (path / 2), v);
            put(ram, c.gpr[6] + 4 * (path / 2), v);
        }
        if (path == 6) {
            put(ram, c.gpr[5], 0x41A00000u);
            put(ram, c.gpr[6], 0xC1A00000u);
        }
        if (path == 7)
            put(ram, c.gpr[5], 0x41A00000u);
        if (path == 8)
            put(ram, c.gpr[6], 0xC1A00000u);
        if (path >= 10) {
            for (unsigned k = 0; k < 12; ++k)
                put(ram, AREA + 0x100u * (1 + k / 3) + 4 * (k % 3), fpbits());
            if (scenario % 5u == 0)
                for (unsigned k = 0; k < 6; ++k)
                    put(ram, c.gpr[2] - 16512u + 4 * k, next());
        }
    }
    if (which == 9 || which == 10) {
        c.gpr[6] = AREA + 0x400u;
        put(ram, c.gpr[2] - 16184u, 0);
        c.fpr[1] = f64_value(convert_to_double(fpbits()));
        /* Occasionally identity matrix and zero planes: reach every corner
         * and the unculled path, with randomized near/far and bounds. */
        if (scenario % 3u != 0) {
            for (unsigned k = 0; k < 12; ++k)
                put(ram, c.gpr[4] + 4 * k, (k == 0 || k == 5 || k == 10) ? 0x3F800000u : 0);
            for (unsigned k = 0; k < 12; ++k)
                put(ram, c.gpr[3] + 4 + 4 * k, 0);
            put(ram, c.gpr[3] + 84, 0xC1200000u);
            put(ram, c.gpr[3] + 88, 0x41200000u);
        }
    }
    c.lr = 0xFFFFFFFCu;
    c.pc = entries[which];
    c.cr = next();
    c.xer = next();
    c.ctr = next();
    c.msr = PPC_MSR_FP;
    c.hid2 = PPC_HID2_LSQE;
    c.fpscr = (next() & 0xFFFFF000u) | (scenario % 3u == 0 ? 4u : 0u);
    c.gqr[0] = scenario % 5u == 0 ? 0x3F003F00u : 0u;
    c.gqr[5] = 0x00070007u;
    if (which == 11) {
        u32 count = 1u + next() % (scenario % 11u == 0 ? 256u : 32u), type = scenario % 2u,
            stride = type ? 8 : 6;
        write_be16(ram + c.gpr[3] - GC_RAM_BASE, (u16)count);
        write_be16(ram + c.gpr[3] + 4 - GC_RAM_BASE, (u16)type);
        s32 first = -2000 + (s32)(next() % 1000u), spacing = 1 + (s32)(next() % 100u);
        for (u32 j = 0; j < count; ++j) {
            write_be16(ram + c.gpr[4] + j * stride - GC_RAM_BASE, (u16)(first + spacing * (s32)j));
            for (u32 k = 2; k < stride; k += 2)
                write_be16(ram + c.gpr[4] + j * stride + k - GC_RAM_BASE, (u16)next());
        }
        write_be64(ram + c.gpr[2] - 13160u - GC_RAM_BASE, 0x4330000080000000ull);
        unsigned path = scenario % 6u;
        c.fpr[1] = path == 0   ? first - 1.0
                   : path == 1 ? first + (count - 1) * spacing + 1.0
                   : path == 2 ? first
                   : path == 3 ? first + (count - 1) * spacing
                               : first + (count - 1) * spacing * (double)(next() % 10000u) / 10000.0;
        if (edges && scenario % 79u == 1)
            c.gqr[5] = 0x01070007u;
        if (edges && scenario % 79u == 2)
            c.gqr[5] = 0x00060007u;
        if (edges && scenario % 79u == 3)
            write_be16(ram + c.gpr[3] - GC_RAM_BASE, 0);
        if (edges && scenario % 79u == 4)
            write_be16(ram + c.gpr[3] - GC_RAM_BASE, 257);
        if (edges && scenario % 79u == 5 && count > 1)
            write_be16(ram + c.gpr[4] + stride - GC_RAM_BASE, (u16)first);
    }
    if (which == 12) {
        c.gpr[4] = next() % 8u;
        c.gpr[5] = AREA + 0x700u;
        const u32 table = AREA + 0x800u + c.gpr[4] * 54u;
        put(ram, c.gpr[3] + 40u, AREA + 0x800u);
        put(ram, c.gpr[3] + 16u, AREA + 0x200u);
        put(ram, c.gpr[3] + 20u, AREA + 0x300u);
        put(ram, c.gpr[3] + 24u, AREA + 0x400u);
        put(ram, c.gpr[3] + 36u, next() % 100u);
        put(ram, c.gpr[2] - 13176u, scenario % 17u == 0 ? fpbits() : 0x3F800000u);
        put(ram, c.gpr[2] - 13172u, scenario % 17u == 0 ? fpbits() : 0);
        for (unsigned k = 0; k < 9; ++k) {
            write_be16(ram + table + 6u * k - GC_RAM_BASE, (u16)(next() % 2u));
            write_be16(ram + table + 6u * k + 2 - GC_RAM_BASE, (u16)(next() % 16u));
        }
        if (edges && scenario % 13u == 0)
            write_be16(ram + table + 6u * (next() % 9u) - GC_RAM_BASE, 2);
    }
    c.cycle_budget = 16384;
    c.downcount = -(s64)(next() % 64u);
    c.cycle_deadline_budget = scenario % 4u == 0 ? 0 : 100000;
    c.cycle_observation_suffix = next();
    c.reserve_valid = scenario % 2u != 0;
    c.reserve_addr = (scenario % 4u == 1 ? c.gpr[3] : STACK - 32u) | (scenario % 6u == 0 ? 0x40000000u : 0u);
    if (!edges)
        return c;
    unsigned edge = scenario % 31u;
    if (edge == 0)
        c.gpr[3] |= 1u;
    if (edge == 1)
        c.gpr[3] = 0xCC000000u;
    if (edge == 2)
        c.gpr[3] = 0xC0100100u;
    if (edge == 3)
        c.gpr[3] = 0x817FFFFCu;
    if (edge == 4)
        c.downcount = -16384;
    if (edge == 5)
        c.cycle_budget = 1u + next() % 2000u;
    if (edge == 6)
        c.cycle_deadline_budget = 1u + next() % 2000u;
    if (edge == 7)
        c.msr = 0;
    if (edge == 8)
        c.fpscr |= 1u + next() % 3u;
    if (edge == 9)
        c.gqr[0] = 0x00040004u;
    if (edge == 10)
        c.hid2 = 0;
    if (edge == 11)
        c.exception = 1;
    if (edge == 12 && which < 4)
        c.gpr[3] = c.gpr[4] + 4u;
    if (edge == 13 && which < 4)
        c.gpr[4] = STACK - 24u;
    if (edge == 14 && which < 4)
        c.gpr[3] = STACK - 20u;
    if (which == 12 && edge == 12)
        c.gpr[5] = AREA + 0x200u;
    if (which == 12 && edge == 13)
        c.gpr[5] = STACK - 24u;
    if (edge == 15) {
        u32 bad[] = {1u,          0x807FFFFFu, 0x7F800000u, 0xFF800000u,
                     0x7FC12345u, 0x7F812345u, 0x5F000000u, 0x00800000u};
        const u32 bits = bad[(scenario / 31u) % 8u];
        if (which == 2 || which == 11 || which == 12)
            c.fpr[1] = f64_value(convert_to_double(bits));
        else if (which >= 5 && which <= 7) {
            put(ram, AREA + 0x600u, bits);
            put(ram, TABLE, bits);
            put(ram, TABLE + 0x2000u, bits);
        } else
            put(ram, c.gpr[4], bits);
    }
    if (edge == 16 && which >= 5 && which <= 7)
        put(ram, AREA + 0x504u, 0xCC000000u);
    if (edge == 17)
        c.gpr[1] = 0x80000004u;
    return c;
}
static void mismatch(const CPUState* got, const CPUState* want, unsigned i, u32 entry) {
    fprintf(stderr, "case %u entry %08X seed %08X mismatch\n", i, entry, seed);
    for (unsigned b = 0; b < sizeof *got; ++b)
        if (((const u8*)got)[b] != ((const u8*)want)[b])
            fprintf(stderr, "CPU byte %u: %02X != %02X\n", b, ((const u8*)got)[b], ((const u8*)want)[b]);
}
static double bench(const StaticRecompModuleDesc* mod, CPUState* guest, CPUState initial, unsigned which,
                    bool native) {
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    ppc_fpscr_updated(&initial);
    QueryPerformanceCounter(&start);
    for (unsigned i = 0; i < 200000u; ++i) {
        *guest = initial;
        if (native) {
            if (!bluewake_native_game_math(guest, entries[which]))
                abort();
        } else if (!mod->dispatch(guest, entries[which]))
            abort();
    }
    QueryPerformanceCounter(&end);
    return (double)(end.QuadPart - start.QuadPart) * 1e9 / (double)freq.QuadPart / 200000.0;
}
/* The cycle observation suffix is dead after an access to RAM: since
 * scripts/windows/lean_memory.py the translation stores it only on the way
 * to an MMIO or timebase handler, its only readers. It is not compared. */
int main(int argc, char** argv) {
    if (argc < 2)
        return 2;
    _putenv_s("BLUEWAKE_NATIVE_MATH", "0");
    unsigned cases = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 100000u;
    HMODULE lib = LoadLibraryA(argv[1]);
    if (!lib) {
        fprintf(stderr, "LoadLibrary error %lu\n", GetLastError());
        return 1;
    }
    StaticRecompGetModuleFn get =
        (StaticRecompGetModuleFn)(void*)GetProcAddress(lib, STATICRECOMP_GET_MODULE_SYMBOL);
    CPUState* (*guest_cpu)(void) =
        (CPUState * (*)(void))(void*)GetProcAddress(lib, "bluewake_composite_guest_cpu");
    if (!get || !guest_cpu)
        return 1;
    const StaticRecompModuleDesc* mod = get();
    if (mod->cpu_state_size != sizeof(CPUState) || strcmp(mod->game_id, "GZLE01"))
        return 1;
    void (*set_edge)(int (*)(void*, CPUState*, u32), void*) =
        (void (*)(int (*)(void*, CPUState*, u32), void*))(void*)GetProcAddress(lib,
                                                                               "bluewake_set_edge_service");
    int (*direct)(bool, const bool*, const bool*, const u32*, const u32*) =
        (int (*)(bool, const bool*, const bool*, const u32*, const u32*))(void*)GetProcAddress(
            lib, "bluewake_composite_direct_calls");
    static const bool clear = false;
    static const u32 zero = 0;
    if (!set_edge || !direct || !direct(true, &clear, &clear, &zero, &zero))
        return 1;
    set_edge(edge_service, NULL);
    u8* a = image();
    u8* b = module_image(lib);
    if (!a || !b)
        return 1;
    for (unsigned which = 0; which < sizeof entries / sizeof entries[0]; ++which) {
        unsigned ran = 0, declined = 0;
        for (unsigned i = 0; i < cases; ++i) {
            CPUState c = build(a, which, i, true);
            memcpy(b + AREA - GC_RAM_BASE, a + AREA - GC_RAM_BASE, DATA_BYTES);
            ppc_fpscr_updated(&c);
            CPUState untouched = c;
            if (i % 101u == 19u)
                g_mem_write_journal = journal;
            if (i % 103u == 20u)
                g_ppc_guest_aliases_overlap_mem1 = true;
            int ok = bluewake_native_game_math(&c, entries[which]);
            g_mem_write_journal = NULL;
            g_ppc_guest_aliases_overlap_mem1 = false;
            if (!ok) {
                ++declined;
                if (ram_diff(a, b) || memcmp(&c, &untouched, sizeof c)) {
                    mismatch(&c, &untouched, i, entries[which]);
                    return 1;
                }
                continue;
            }
            ++ran;
            CPUState* g = guest_cpu();
            *g = untouched;
            g->ram = b;
            mod->on_state_loaded(g);
            if (!mod->dispatch(g, entries[which]))
                return 1;
            CPUState ref = *g;
            ref.ram = a;
            ref.cycle_observation_suffix = c.cycle_observation_suffix;
            if (ram_diff(a, b) || memcmp(&c, &ref, sizeof c)) {
                mismatch(&c, &ref, i, entries[which]);
                return 1;
            }
        }
        printf("%08X: %u cases, %u identical, %u declined unchanged, zero mismatches\n", entries[which],
               cases, ran, declined);
        fflush(stdout);
        if (memcmp(a, b, GC_MAIN_RAM_SIZE))
            return 1;
        if (which == 3) {
            if (ran)
                return 1;
            continue;
        } /* dropped component multiply is unsupported */
        CPUState c = build(a, which, which == 8 ? 0 : which == 11 ? 5 : which >= 9 ? 1 : 6, false);
        c.ram = b;
        memcpy(b + AREA - GC_RAM_BASE, a + AREA - GC_RAM_BASE, DATA_BYTES);
        double ts[5], ns[5];
        for (unsigned j = 0; j < 5; ++j) {
            if (j & 1) {
                ns[j] = bench(mod, guest_cpu(), c, which, true);
                ts[j] = bench(mod, guest_cpu(), c, which, false);
            } else {
                ts[j] = bench(mod, guest_cpu(), c, which, false);
                ns[j] = bench(mod, guest_cpu(), c, which, true);
            }
        }
        for (unsigned j = 0; j < 5; ++j)
            for (unsigned k = j + 1; k < 5; ++k) {
                if (ts[k] < ts[j]) { double swap = ts[j]; ts[j] = ts[k]; ts[k] = swap; }
                if (ns[k] < ns[j]) { double swap = ns[j]; ns[j] = ns[k]; ns[k] = swap; }
            }
        const double t = ts[2], n = ns[2];
        printf("%08X: translation %.2f ns, native %.2f ns, %.2fx\n", entries[which], t, n, t / n);
        fflush(stdout);
        if (cases >= 60000 && ran < 30000)
            return 1;
    }
    return 0;
}
