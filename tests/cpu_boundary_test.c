#include "core/cpu.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

typedef void (*PPCMemWriteJournal)(u32 offset, u32 size, void* user);
extern void ppc_set_mem_write_journal(PPCMemWriteJournal fn, void* user);

static u32 g_offsets[4];
static u32 g_preimages[4];
static unsigned g_events;

typedef struct RuntimeShadow {
    f64 ps1[32];
    u32 dar;
    u32 dsisr;
    u32 ear;
    u32 hid2;
    u32 sr[16];
    u32 exception;
    u32 program_exception;
    u32 reserve_addr;
    bool reserve_valid;
} RuntimeShadow;

static void capture_runtime_shadow(RuntimeShadow* shadow, const CPUState* cpu) {
    memcpy(shadow->ps1, cpu->ps1, sizeof shadow->ps1);
    shadow->dar = cpu->dar;
    shadow->dsisr = cpu->dsisr;
    shadow->ear = cpu->ear;
    shadow->hid2 = cpu->hid2;
    memcpy(shadow->sr, cpu->sr, sizeof shadow->sr);
    shadow->exception = cpu->exception;
    shadow->program_exception = cpu->program_exception;
    shadow->reserve_addr = cpu->reserve_addr;
    shadow->reserve_valid = cpu->reserve_valid;
}

static void restore_runtime_shadow(CPUState* cpu, const RuntimeShadow* shadow) {
    memcpy(cpu->ps1, shadow->ps1, sizeof shadow->ps1);
    cpu->dar = shadow->dar;
    cpu->dsisr = shadow->dsisr;
    cpu->ear = shadow->ear;
    cpu->hid2 = shadow->hid2;
    memcpy(cpu->sr, shadow->sr, sizeof shadow->sr);
    cpu->exception = shadow->exception;
    cpu->program_exception = shadow->program_exception;
    cpu->reserve_addr = shadow->reserve_addr;
    cpu->reserve_valid = shadow->reserve_valid;
}

static void journal(u32 offset, u32 size, void* user) {
    CPUState* cpu = (CPUState*)user;
    assert(size == 4u);
    assert(g_events < 4u);
    g_offsets[g_events] = offset;
    g_preimages[g_events] = mem_read32(cpu, GC_RAM_BASE + offset);
    g_events++;
}

// These stand in for two generated basic blocks crossing a real PPC fence.
static void translated_block_a(CPUState* cpu) {
    cpu->downcount -= 3;
    mem_write32(cpu, GC_RAM_BASE + 0x100u, cpu->gpr[3]);
}

static void translated_block_b(CPUState* cpu) {
    cpu->downcount -= 5;
    ppc_memory_fence();
    mem_write32(cpu, GC_RAM_BASE + 0x104u, cpu->gpr[4]);
}

static void save_exception_context(CPUState* cpu, u32 context) {
    for (u32 reg = 0; reg < 32u; reg++)
        mem_write32(cpu, context + reg * 4u, cpu->gpr[reg]);
    mem_write32(cpu, context + 0x80u, cpu->cr);
    mem_write32(cpu, context + 0x84u, cpu->lr);
    mem_write32(cpu, context + 0x88u, cpu->ctr);
    mem_write32(cpu, context + 0x8Cu, cpu->xer);
    for (u32 reg = 0; reg < 32u; reg++) {
        u64 bits;
        memcpy(&bits, &cpu->fpr[reg], sizeof(bits));
        mem_write64(cpu, context + 0x90u + reg * 8u, bits);
    }
    mem_write32(cpu, context + 0x194u, cpu->fpscr);
    mem_write32(cpu, context + 0x198u, cpu->pc);
    mem_write32(cpu, context + 0x19Cu, cpu->msr);
    for (u32 reg = 0; reg < 8u; reg++)
        mem_write32(cpu, context + 0x1A4u + reg * 4u, cpu->gqr[reg]);
}

static void restore_exception_context(CPUState* cpu, u32 context) {
    for (u32 reg = 0; reg < 32u; reg++)
        cpu->gpr[reg] = mem_read32(cpu, context + reg * 4u);
    cpu->cr = mem_read32(cpu, context + 0x80u);
    cpu->lr = mem_read32(cpu, context + 0x84u);
    cpu->ctr = mem_read32(cpu, context + 0x88u);
    cpu->xer = mem_read32(cpu, context + 0x8Cu);
    for (u32 reg = 0; reg < 32u; reg++) {
        u64 bits = mem_read64(cpu, context + 0x90u + reg * 8u);
        memcpy(&cpu->fpr[reg], &bits, sizeof(bits));
    }
    cpu->fpscr = mem_read32(cpu, context + 0x194u);
    for (u32 reg = 0; reg < 8u; reg++)
        cpu->gqr[reg] = mem_read32(cpu, context + 0x1A4u + reg * 4u);
    cpu->srr0 = mem_read32(cpu, context + 0x198u);
    cpu->srr1 = mem_read32(cpu, context + 0x19Cu);
}

static void run_callback_context_handoff(CPUState* cpu) {
    const u32 interrupted = GC_RAM_BASE + 0x400u;
    const u32 exception = GC_RAM_BASE + 0x800u;
    RuntimeShadow interrupted_shadow;

    cpu->gpr[3] = 0xA3000003u;
    cpu->gpr[4] = 0xA4000004u;
    cpu->gpr[31] = 0xAF00001Fu;
    cpu->pc = 0x80307EF4u;
    cpu->msr = PPC_MSR_EE | PPC_MSR_RI;
    cpu->lr = 0x80255E54u;
    cpu->fpr[7] = 7.25;
    cpu->ps1[7] = -7.5;
    cpu->dar = 0xDADA0001u;
    cpu->dsisr = 0xD5150001u;
    cpu->ear = 0xEA000001u;
    cpu->hid2 = 0xB2000001u;
    cpu->sr[3] = 0x53000003u;
    cpu->exception = PPC_EXC_DSI;
    cpu->program_exception = PPC_PROGRAM_TRAP;
    cpu->reserve_addr = 0x80400020u;
    cpu->reserve_valid = true;
    capture_runtime_shadow(&interrupted_shadow, cpu);
    cpu->fpscr = 0x07070707u;
    cpu->gqr[2] = 0x02020002u;
    save_exception_context(cpu, interrupted);

    // Model the retail callback's OSClear/OSSet temporary context. The
    // handler is allowed to clobber live state, but must not alter A.
    cpu->gpr[3] = 0xB3000003u;
    cpu->gpr[4] = 0xB4000004u;
    cpu->gpr[31] = 0xBF00001Fu;
    cpu->pc = 0x803023A4u;
    cpu->msr = 0u;
    cpu->lr = 0x80302594u;
    cpu->fpr[7] = 1.5;
    cpu->ps1[7] = 1.25;
    cpu->dar = 0xDADA0002u;
    cpu->dsisr = 0xD5150002u;
    cpu->ear = 0xEA000002u;
    cpu->hid2 = 0xB2000002u;
    cpu->sr[3] = 0x53000004u;
    cpu->exception = PPC_EXC_ALIGNMENT;
    cpu->program_exception = PPC_PROGRAM_ILLEGAL;
    cpu->reserve_addr = 0x80400040u;
    cpu->reserve_valid = false;
    cpu->fpscr = 0xB0B0B0B0u;
    cpu->gqr[2] = 0x0B0B0002u;
    save_exception_context(cpu, exception);

    // Handler-side mutation and restore of the interrupted context.
    cpu->gpr[3] = 0u;
    cpu->gpr[4] = 0u;
    cpu->gpr[31] = 0u;
    cpu->pc = 0u;
    cpu->msr = 0u;
    cpu->fpr[7] = 0.0;
    cpu->ps1[7] = 0.0;
    restore_runtime_shadow(cpu, &interrupted_shadow);
    cpu->fpscr = 0u;
    cpu->gqr[2] = 0u;
    restore_exception_context(cpu, interrupted);
    ppc_rfi(cpu, interrupted);

    assert(cpu->gpr[3] == 0xA3000003u);
    assert(cpu->gpr[4] == 0xA4000004u);
    assert(cpu->gpr[31] == 0xAF00001Fu);
    assert(cpu->pc == 0x80307EF4u);
    assert(cpu->msr == (PPC_MSR_EE | PPC_MSR_RI));
    assert(cpu->lr == 0x80255E54u);
    assert(cpu->fpr[7] == 7.25 && cpu->fpscr == 0x07070707u);
    assert(cpu->ps1[7] == -7.5);
    assert(cpu->dar == 0xDADA0001u && cpu->dsisr == 0xD5150001u);
    assert(cpu->ear == 0xEA000001u && cpu->hid2 == 0xB2000001u);
    assert(cpu->sr[3] == 0x53000003u);
    assert(cpu->exception == PPC_EXC_DSI &&
           cpu->program_exception == PPC_PROGRAM_TRAP);
    assert(cpu->reserve_addr == 0x80400020u && cpu->reserve_valid);
    assert(cpu->gqr[2] == 0x02020002u);

    // Context B remains the callback's private state rather than a partial
    // overwrite of the interrupted context.
    assert(mem_read32(cpu, exception + 0x0Cu) == 0xB3000003u);
    assert(mem_read32(cpu, exception + 0x10u) == 0xB4000004u);
    assert(mem_read32(cpu, exception + 0x7Cu) == 0xBF00001Fu);
    assert(mem_read32(cpu, exception + 0x198u) == 0x803023A4u);
    assert(mem_read32(cpu, exception + 0x19Cu) == 0u);
    assert(mem_read32(cpu, interrupted + 0x0Cu) == 0xA3000003u);
    assert(mem_read32(cpu, interrupted + 0x10u) == 0xA4000004u);
}

int main(void) {
    CPUState cpu;
    if (!cpu_init(&cpu))
        return 1;
    cpu.gpr[3] = 0x11223344u;
    cpu.gpr[4] = 0x55667788u;
    cpu.downcount = 0;
    ppc_set_mem_write_journal(journal, &cpu);

    translated_block_a(&cpu);
    translated_block_b(&cpu);

    assert(cpu.downcount == -8);
    assert(g_events == 2u);
    assert(g_offsets[0] == 0x100u && g_offsets[1] == 0x104u);
    assert(g_preimages[0] == 0u && g_preimages[1] == 0u);
    assert(mem_read32(&cpu, GC_RAM_BASE + 0x100u) == 0x11223344u);
    assert(mem_read32(&cpu, GC_RAM_BASE + 0x104u) == 0x55667788u);
    mem_write32(&cpu, GC_RAM_BASE + 0x120u, 0xA1B2C3D4u);
    assert(mem_read32(&cpu, GC_RAM_UNCACHED + 0x120u) == 0xA1B2C3D4u);
    mem_write32(&cpu, GC_RAM_UNCACHED + 0x124u, 0xD4C3B2A1u);
    assert(mem_read32(&cpu, GC_RAM_BASE + 0x124u) == 0xD4C3B2A1u);
    ppc_set_mem_write_journal(NULL, NULL);

    for (u32 reg = 0; reg < 32u; reg++)
        cpu.gpr[reg] = 0x81000000u + reg * 4u;
    cpu.pc = 0x80301234u;
    cpu.msr = PPC_MSR_EE | PPC_MSR_RI;
    cpu.lr = 0x8020ABCDu;
    cpu.ctr = 0x80405678u;
    cpu.cr = 0x12345678u;
    cpu.xer = 0x87654321u;
    cpu.fpr[7] = 3.5;
    cpu.ps1[7] = -3.75;
    RuntimeShadow context_shadow;
    capture_runtime_shadow(&context_shadow, &cpu);
    cpu.fpscr = 0xA5A5A5A5u;
    cpu.gqr[2] = 0x12340056u;
    const u32 context = GC_RAM_BASE + 0x200u;
    save_exception_context(&cpu, context);
    cpu.gpr[3] = 0u;
    cpu.gpr[31] = 0u;
    cpu.pc = 0u;
    cpu.msr = 0u;
    cpu.fpr[7] = 0.0;
    cpu.ps1[7] = 0.0;
    cpu.fpscr = 0u;
    cpu.gqr[2] = 0u;
    restore_exception_context(&cpu, context);
    restore_runtime_shadow(&cpu, &context_shadow);
    ppc_rfi(&cpu, context);
    assert(cpu.gpr[3] == 0x8100000Cu);
    assert(cpu.gpr[31] == 0x8100007Cu);
    assert(cpu.pc == 0x80301234u);
    assert(cpu.msr == (PPC_MSR_EE | PPC_MSR_RI));
    assert(cpu.lr == 0x8020ABCDu && cpu.ctr == 0x80405678u);
    assert(cpu.fpr[7] == 3.5 && cpu.fpscr == 0xA5A5A5A5u);
    assert(cpu.ps1[7] == -3.75);
    assert(cpu.gqr[2] == 0x12340056u);

    run_callback_context_handoff(&cpu);

    cpu_free(&cpu);
    puts("CPU boundary store-order/context-handoff test passed.");
    return 0;
}
