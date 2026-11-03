#include "fpu_context.h"

#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,     \
                    #condition);                                             \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#define CURRENT_CONTEXT 0x800000D4u
#define CURRENT_FPU_CONTEXT 0x800000D8u
#define FPU_EXCEPTION_HANDLER 0x80303EB8u
#define CONTEXT_STATE 0x1A2u
#define CONTEXT_GQR 0x1A4u

static void seed_exception(CPUState* cpu, u32 current, u32 previous,
                           u16 state) {
    mem_write32(cpu, CURRENT_CONTEXT, current);
    mem_write32(cpu, CURRENT_FPU_CONTEXT, previous);
    mem_write16(cpu, current + CONTEXT_STATE, state);
    for (u32 reg = 0; reg < 32u; reg++)
        cpu->gpr[reg] = 0x11000000u + reg;
    cpu->cr = 0x22334455u;
    cpu->lr = 0x33445566u;
    cpu->ctr = 0x44556677u;
    cpu->xer = 0x55667788u;
    cpu->srr0 = 0x8007811Cu;
    cpu->srr1 = 0x66778899u;
    for (u32 reg = 0; reg < 8u; reg++)
        cpu->gqr[reg] = 0x77000000u + reg;
    cpu->pc = 0x80000100u;
    cpu->exception = PPC_EXC_FP_UNAVAILABLE;
}

static int check_delivery_image(CPUState* cpu, u32 context, u16 state) {
    for (u32 reg = 0; reg < 32u; reg++)
        CHECK(mem_read32(cpu, context + reg * 4u) == 0x11000000u + reg);
    CHECK(mem_read32(cpu, context + 0x080u) == 0x22334455u);
    CHECK(mem_read32(cpu, context + 0x084u) == 0x33445566u);
    CHECK(mem_read32(cpu, context + 0x088u) == 0x44556677u);
    CHECK(mem_read32(cpu, context + 0x08Cu) == 0x55667788u);
    CHECK(mem_read32(cpu, context + 0x198u) == 0x8007811Cu);
    CHECK(mem_read32(cpu, context + 0x19Cu) == 0x66778899u);
    CHECK(mem_read16(cpu, context + CONTEXT_STATE) == (u16)(state | 0x0002u));
    for (u32 reg = 0; reg < 8u; reg++)
        CHECK(mem_read32(cpu, context + CONTEXT_GQR + reg * 4u) ==
              0x77000000u + reg);
    CHECK(cpu->exception == 0u);
    CHECK(cpu->gpr[4] == context);
    CHECK(cpu->pc == FPU_EXCEPTION_HANDLER);
    return 0;
}

int main(void) {
    CPUState cpu;
    CHECK(cpu_init(&cpu));

    const u32 first = 0x80004000u;
    const u32 second = 0x80005000u;

    seed_exception(&cpu, second, first, 0x0001u);
    CHECK(bluewake_fpu_exception_deliver(&cpu) ==
          BLUEWAKE_FPU_SWITCH_RESTORED);
    CHECK(check_delivery_image(&cpu, second, 0x0001u) == 0);
    CHECK(mem_read32(&cpu, CURRENT_FPU_CONTEXT) == first);
    CHECK(!bluewake_fpu_registers_materialized(&cpu));

    cpu.msr |= PPC_MSR_FP;
    mem_write32(&cpu, CURRENT_FPU_CONTEXT, second);
    CHECK(bluewake_fpu_registers_materialized(&cpu));
    mem_write32(&cpu, CURRENT_FPU_CONTEXT, first);
    CHECK(!bluewake_fpu_registers_materialized(&cpu));
    cpu.msr &= ~PPC_MSR_FP;

    seed_exception(&cpu, second, second, 0x0001u);
    CHECK(bluewake_fpu_exception_deliver(&cpu) ==
          BLUEWAKE_FPU_SWITCH_SAME_OWNER);
    CHECK(check_delivery_image(&cpu, second, 0x0001u) == 0);
    CHECK(mem_read32(&cpu, CURRENT_FPU_CONTEXT) == second);

    const u32 fresh = 0x80006000u;
    seed_exception(&cpu, fresh, second, 0x0000u);
    CHECK(bluewake_fpu_exception_deliver(&cpu) ==
          BLUEWAKE_FPU_SWITCH_FRESH);
    CHECK(check_delivery_image(&cpu, fresh, 0x0000u) == 0);
    CHECK(mem_read32(&cpu, CURRENT_FPU_CONTEXT) == second);

    seed_exception(&cpu, second, first, 0x0001u);
    mem_write32(&cpu, CURRENT_CONTEXT, 0x70000000u);
    CHECK(bluewake_fpu_exception_deliver(&cpu) ==
          BLUEWAKE_FPU_SWITCH_INVALID);
    CHECK(cpu.exception == PPC_EXC_FP_UNAVAILABLE);
    CHECK(cpu.pc == 0x80000100u);

    seed_exception(&cpu, second, 0x70000000u, 0x0001u);
    CHECK(bluewake_fpu_exception_deliver(&cpu) ==
          BLUEWAKE_FPU_SWITCH_INVALID);
    CHECK(cpu.exception == PPC_EXC_FP_UNAVAILABLE);
    CHECK(cpu.pc == 0x80000100u);
    CHECK(mem_read32(&cpu, CURRENT_FPU_CONTEXT) == 0x70000000u);

    cpu_free(&cpu);
    puts("Retail FPU exception delivery contract test passed.");
    return 0;
}
