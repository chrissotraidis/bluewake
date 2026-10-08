#include "StaticRecompABI.h"
#include "callback_delivery.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>

#define OS_CURRENT_CONTEXT 0x800000D4u
#define OS_CURRENT_THREAD 0x800000E4u
#define OS_RUN_QUEUE 0x803F02B8u
#define OS_RUN_QUEUE_BITS 0x803F7A30u
#define OS_RUN_QUEUE_HINT 0x803F7A34u
#define OS_RESCHEDULE 0x803F7A38u
#define OS_SWITCH_CALLBACK 0x803F66D8u
#define OS_RECEIVE_MESSAGE 0x803059D0u
#define JKR_DVD_DONE_PROCESS 0x802BCC78u

#define THREAD_STATE 0x2C8u
#define THREAD_SUSPEND 0x2CCu
#define THREAD_EFFECTIVE_PRIORITY 0x2D0u
#define THREAD_BASE_PRIORITY 0x2D4u
#define THREAD_QUEUE 0x2DCu
#define THREAD_LINK_NEXT 0x2E0u
#define THREAD_LINK_PREV 0x2E4u

typedef const StaticRecompModuleDesc* (*GetModuleFn)(void);

static void prepare_callback_dispatch(CPUState* cpu, void* user) {
    unsigned* calls = user;
    (*calls)++;
    cpu->downcount = 0;
    cpu->cycle_budget = 37;
}

static void dispatch_until(const StaticRecompModuleDesc* module, CPUState* cpu,
                           u32 stop_pc) {
    unsigned dispatches = 0u;
    while (cpu->pc != stop_pc && dispatches < 10000u) {
        cpu->downcount = 0;
        cpu->cycle_budget = 10000;
        if (module->dispatch(cpu, cpu->pc) != 1) {
            fprintf(stderr,
                    "uncovered pc=0x%08X dispatches=%u lr=0x%08X r1=0x%08X "
                    "r3=0x%08X current=0x%08X context=0x%08X\n",
                    cpu->pc, dispatches, cpu->lr, cpu->gpr[1], cpu->gpr[3],
                    mem_read32(cpu, OS_CURRENT_THREAD),
                    mem_read32(cpu, OS_CURRENT_CONTEXT));
            assert(0);
        }
        dispatches++;
    }
    if (cpu->pc != stop_pc)
        fprintf(stderr, "callback test expected 0x%08X, stopped at 0x%08X after %u dispatches\n",
                stop_pc, cpu->pc, dispatches);
    assert(cpu->pc == stop_pc);
}

static void init_thread(CPUState* cpu, u32 thread, u16 state, u32 priority,
                        u32 stack, u32 resume_pc) {
    mem_write32(cpu, thread + 0x04u, stack);
    mem_write32(cpu, thread + 13u * 4u, 0x803FE0E0u);
    mem_write32(cpu, thread + 0x198u, resume_pc);
    mem_write32(cpu, thread + 0x19Cu, PPC_MSR_EE | PPC_MSR_RI);
    mem_write16(cpu, thread + 0x1A2u, 0u);
    mem_write16(cpu, thread + THREAD_STATE, state);
    mem_write32(cpu, thread + THREAD_SUSPEND, 0u);
    mem_write32(cpu, thread + THREAD_EFFECTIVE_PRIORITY, priority);
    mem_write32(cpu, thread + THREAD_BASE_PRIORITY, priority);
    mem_write32(cpu, thread + THREAD_QUEUE, 0u);
    mem_write32(cpu, thread + THREAD_LINK_NEXT, 0u);
    mem_write32(cpu, thread + THREAD_LINK_PREV, 0u);
}

int main(int argc, char** argv) {
    assert(argc == 2);
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    assert(library != NULL);
    GetModuleFn get_module =
        (GetModuleFn)dlsym(library, STATICRECOMP_GET_MODULE_SYMBOL);
    assert(get_module != NULL);
    const StaticRecompModuleDesc* module = get_module();
    assert(module != NULL && module->dispatch != NULL);

    CPUState cpu;
    assert(cpu_init(&cpu));
    const u32 dvd_file = 0x80400000u;
    const u32 queue = dvd_file + 0xC0u;
    const u32 messages = 0x80400200u;
    const u32 file_info = 0x80400400u;
    const u32 receiver = 0x80401000u;
    const u32 callback_thread = 0x80402000u;
    const u32 receiver_return = 0x81800000u;
    const u32 callback_resume = 0x81800008u;
    const u32 output = 0x80400300u;
    const u32 result = 0x00002000u;

    init_thread(&cpu, receiver, 2u, 5u, 0x81700000u, 0u);
    init_thread(&cpu, callback_thread, 1u, 10u, 0x81710000u,
                callback_resume);
    mem_write32(&cpu, queue + 0x10u, messages);
    mem_write32(&cpu, queue + 0x14u, 1u);
    mem_write32(&cpu, queue + 0x18u, 0u);
    mem_write32(&cpu, queue + 0x1Cu, 0u);
    mem_write32(&cpu, file_info + 0x3Cu, dvd_file);
    mem_write32(&cpu, callback_thread + THREAD_QUEUE,
                OS_RUN_QUEUE + 10u * 8u);
    mem_write32(&cpu, OS_RUN_QUEUE + 10u * 8u, callback_thread);
    mem_write32(&cpu, OS_RUN_QUEUE + 10u * 8u + 4u, callback_thread);
    mem_write32(&cpu, OS_CURRENT_CONTEXT, receiver);
    mem_write32(&cpu, OS_CURRENT_THREAD, receiver);
    mem_write32(&cpu, OS_RUN_QUEUE_BITS, 1u << (31u - 10u));
    mem_write32(&cpu, OS_RUN_QUEUE_HINT, 0u);
    mem_write32(&cpu, OS_RESCHEDULE, 0u);
    mem_write32(&cpu, OS_SWITCH_CALLBACK, 0x80305904u);

    cpu.gpr[1] = 0x81700000u;
    cpu.gpr[3] = queue;
    cpu.gpr[4] = output;
    cpu.gpr[5] = 1u;
    cpu.gpr[13] = 0x803FE0E0u;
    cpu.lr = receiver_return;
    cpu.pc = OS_RECEIVE_MESSAGE;
    cpu.msr = PPC_MSR_EE | PPC_MSR_RI;

    dispatch_until(module, &cpu, callback_resume);
    assert(mem_read16(&cpu, receiver + THREAD_STATE) == 4u);
    assert(mem_read32(&cpu, receiver + THREAD_QUEUE) == queue + 0x08u);
    assert(mem_read32(&cpu, queue + 0x08u) == receiver);

    const u32 interrupted_pc = cpu.pc;
    const u32 interrupted_sp = cpu.gpr[1];
    const u32 interrupted_result = cpu.gpr[3];
    const u32 interrupted_lr = cpu.lr;
    const u32 interrupted_msr = cpu.msr;
    const u32 scheduler_depth = mem_read32(&cpu, OS_RESCHEDULE);
    unsigned prepare_calls = 0u;
    const BluewakeCallbackDeliveryResult delivery =
        bluewake_deliver_guest_callback(&cpu, module, JKR_DVD_DONE_PROCESS,
                                        result, file_info, 4096u,
                                        prepare_callback_dispatch,
                                        &prepare_calls);
    if (!delivery.completed)
        fprintf(stderr,
                "delivery incomplete pc=0x%08X exception=0x%08X steps=%u\n",
                delivery.terminal_pc, delivery.exception,
                delivery.dispatches);
    assert(delivery.completed);
    assert(prepare_calls == delivery.dispatches);
    assert(delivery.exception == 0u);
    assert(cpu.pc == interrupted_pc);
    assert(cpu.gpr[1] == interrupted_sp);
    assert(cpu.gpr[3] == interrupted_result);
    assert(cpu.lr == interrupted_lr);
    assert(cpu.msr == interrupted_msr);
    assert(mem_read32(&cpu, OS_RESCHEDULE) == scheduler_depth);
    assert(mem_read32(&cpu, queue + 0x08u) == 0u);
    assert(mem_read32(&cpu, queue + 0x0Cu) == 0u);
    assert(mem_read32(&cpu, queue + 0x1Cu) == 1u);
    assert(mem_read32(&cpu, messages) == result);
    assert(mem_read32(&cpu, output) == 0u);
    assert(mem_read32(&cpu, OS_CURRENT_THREAD) == callback_thread);
    assert(mem_read32(&cpu, OS_CURRENT_CONTEXT) == callback_thread);
    assert(mem_read16(&cpu, receiver + THREAD_STATE) == 1u);
    assert(mem_read32(&cpu, receiver + THREAD_QUEUE) == OS_RUN_QUEUE + 5u * 8u);
    assert((mem_read32(&cpu, OS_RUN_QUEUE_BITS) &
            (1u << (31u - 5u))) != 0u);
    assert(mem_read16(&cpu, callback_thread + THREAD_STATE) == 2u);

    cpu_free(&cpu);
    dlclose(library);
    puts("Generated JKRDvdFile completion callback contract test passed.");
    return 0;
}
