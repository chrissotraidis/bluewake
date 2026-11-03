#include "cold_fallback.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

typedef struct FallbackInstruction {
    u32 pc;
    u32 raw;
} FallbackInstruction;

static const FallbackInstruction kOpeningColdSuffix[] = {
    {0x80233D68u, 0xC03E02A4u}, {0x80233D6Cu, 0xC002BC5Cu},
    {0x80233D70u, 0xEC010028u}, {0x80233D74u, 0xD01E02A4u},
    {0x80233D78u, 0xC03E02A8u}, {0x80233D7Cu, 0xC002BC60u},
    {0x80233D80u, 0xEC01002Au}, {0x80233D84u, 0xD01E02A8u},
    {0x80233D88u, 0xC03E02A8u}, {0x80233D8Cu, 0xC002BC3Cu},
    {0x80233D90u, 0xFC010040u}, {0x80233D94u, 0x4C411382u},
    {0x80233D98u, 0x40820010u}, {0x80233D9Cu, 0xD01E02A8u},
    {0x80233DA0u, 0x38000008u}, {0x80233DA4u, 0x901E02B0u},
    {0x80233DA8u, 0x387E003Cu}, {0x80233DACu, 0xC022BC30u},
    {0x80233DB0u, 0xC05E02A4u}, {0x80233DB4u, 0x4BE07ED5u},
};

static u32 opening_raw(u32 pc) {
    for (size_t index = 0;
         index < sizeof kOpeningColdSuffix / sizeof kOpeningColdSuffix[0];
         index++) {
        if (kOpeningColdSuffix[index].pc == pc)
            return kOpeningColdSuffix[index].raw;
    }
    return 0u;
}

int main(void) {
    CPUState cpu;
    assert(cpu_init(&cpu));

    const u32 object = GC_RAM_BASE + 0x1000u;
    cpu.gpr[30] = object;
    mem_write32(&cpu, object + 684u, 90u);
    assert(bluewake_cold_fallback_execute(&cpu, 0x807E02ACu,
                                          0x80233C4Cu));
    assert(cpu.gpr[3] == 90u);
    assert(cpu.pc == 0x80233C50u);

    assert(bluewake_cold_fallback_execute(&cpu, 0x2C03005Au,
                                          0x80233C50u));
    assert((cpu.cr >> 28) == 2u);
    assert(cpu.pc == 0x80233C54u);

    assert(bluewake_cold_fallback_execute(&cpu, 0x41820010u,
                                          0x80233C54u));
    assert(cpu.pc == 0x80233C64u);

    cpu.cr = 0u;
    assert(bluewake_cold_fallback_execute(&cpu, 0x41820010u,
                                          0x80233C54u));
    assert(cpu.pc == 0x80233C58u);

    cpu.msr |= PPC_MSR_FP;
    mem_write32(&cpu, object + 676u, 0x43960000u);
    assert(bluewake_cold_fallback_execute(&cpu, 0xC03E02A4u,
                                          0x80233D68u));
    assert(f64_bits(cpu.fpr[1]) == f64_bits(300.0));
    assert(f64_bits(cpu.ps1[1]) == f64_bits(300.0));
    assert(cpu.pc == 0x80233D6Cu);

    cpu.fpr[0] = 0.5;
    assert(bluewake_cold_fallback_execute(&cpu, 0xEC010028u,
                                          0x80233D70u));
    assert(cpu.fpr[0] == 299.5);
    assert(cpu.ps1[0] == 299.5);
    assert(cpu.pc == 0x80233D74u);

    cpu.fpr[1] = 2.0;
    cpu.fpr[0] = 1.0;
    assert(bluewake_cold_fallback_execute(&cpu, 0xFC010040u,
                                          0x80233D90u));
    assert((cpu.cr >> 28) == 4u);
    assert(cpu.pc == 0x80233D94u);

    const u32 cror_2_1_2 =
        (19u << 26) | (2u << 21) | (1u << 16) | (2u << 11) | (449u << 1);
    assert(bluewake_cold_fallback_execute(&cpu, cror_2_1_2,
                                          0x80233D94u));
    assert((cpu.cr & (0x80000000u >> 2)) != 0u);
    assert(cpu.pc == 0x80233D98u);

    cpu.gpr[2] = GC_RAM_BASE + 0x20000u;
    cpu.gpr[30] = object;
    mem_write32(&cpu, object + 676u, 0x43960000u);
    mem_write32(&cpu, object + 680u, 0x40000000u);
    mem_write32(&cpu, cpu.gpr[2] - 17316u, 0x3F800000u);
    mem_write32(&cpu, cpu.gpr[2] - 17312u, 0x3F000000u);
    mem_write32(&cpu, cpu.gpr[2] - 17348u, 0x447A0000u);
    mem_write32(&cpu, cpu.gpr[2] - 17360u, 0x3F800000u);
    cpu.pc = 0x80233D68u;
    unsigned suffix_steps = 0u;
    while (cpu.pc >= 0x80233D68u && cpu.pc <= 0x80233DB4u) {
        const u32 raw = opening_raw(cpu.pc);
        assert(raw != 0u);
        assert(bluewake_cold_fallback_execute(&cpu, raw, cpu.pc));
        assert(cpu.exception == 0u);
        assert(++suffix_steps <= 20u);
    }
    assert(cpu.pc == 0x8003BC88u);

    assert(!bluewake_cold_fallback_execute(&cpu, 0x00000000u,
                                           0x802AFBF0u));
    cpu_free(&cpu);
    puts("LLVM cold-entry fallback contract test passed.");
    return 0;
}
