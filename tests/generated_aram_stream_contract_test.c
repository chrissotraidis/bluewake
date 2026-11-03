#include "StaticRecompABI.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>

#define JKR_ARAM_STREAM_WRITE_TO_ARAM 0x802B637Cu
#define JKR_ARAM_PCS 0x802B5ED4u
#define PREPARE_ARAM_PCS 0x802B64E0u
#define STREAM_READ_SENTINEL 0x81800100u
#define STREAM_SEEK_SENTINEL 0x81800104u

typedef const StaticRecompModuleDesc* (*GetModuleFn)(void);

typedef struct ContractState {
    u32 stream;
    u32 buffer;
    u32 destination;
    u32 total_size;
    u32 offset;
    u32 seek_calls;
    u32 read_calls;
    u32 dma_calls;
    u32 dma_bytes;
} ContractState;

static int service_boundary(CPUState* cpu, ContractState* state) {
    if (cpu->pc == STREAM_SEEK_SENTINEL) {
        assert(cpu->gpr[3] == state->stream);
        assert(cpu->gpr[4] == state->offset);
        assert(cpu->gpr[5] == 0u);
        state->seek_calls++;
        cpu->gpr[3] = state->offset;
        cpu->pc = cpu->lr;
        return 1;
    }
    if (cpu->pc == STREAM_READ_SENTINEL) {
        assert(cpu->gpr[3] == state->stream);
        assert(cpu->gpr[4] == state->buffer);
        assert(cpu->gpr[5] != 0u);
        state->read_calls++;
        cpu->gpr[3] = cpu->gpr[5];
        cpu->pc = cpu->lr;
        return 1;
    }
    if (cpu->pc == JKR_ARAM_PCS) {
        const u32 remaining = state->total_size - state->dma_bytes;
        const u32 expected_length = remaining > 0x2000u ? 0x2000u : remaining;
        assert(cpu->gpr[3] == 0u);
        assert(cpu->gpr[4] == state->buffer);
        assert(cpu->gpr[5] == state->destination + state->dma_bytes);
        assert(cpu->gpr[6] == expected_length);
        assert(cpu->gpr[7] == 0u);
        state->dma_calls++;
        state->dma_bytes += expected_length;
        cpu->pc = cpu->lr;
        return 1;
    }
    return 0;
}

static void dispatch_until(const StaticRecompModuleDesc* module, CPUState* cpu,
                           ContractState* state, u32 stop_pc) {
    unsigned dispatches = 0u;
    while (cpu->pc != stop_pc && dispatches < 10000u) {
        if (service_boundary(cpu, state)) {
            continue;
        }

        /* Force the same-chunk JKRAramPcs call to yield at its public boundary. */
        cpu->downcount = cpu->pc == PREPARE_ARAM_PCS ? -256 : 0;
        if (module->dispatch(cpu, cpu->pc) != 1) {
            fprintf(stderr,
                    "uncovered pc=0x%08X dispatches=%u lr=0x%08X r1=0x%08X "
                    "r3=0x%08X\n",
                    cpu->pc, dispatches, cpu->lr, cpu->gpr[1], cpu->gpr[3]);
            assert(0);
        }
        dispatches++;
    }
    assert(cpu->pc == stop_pc);
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
    const u32 command = 0x80410000u;
    const u32 stream = 0x80411000u;
    const u32 vtable = 0x80411100u;
    const u32 buffer = 0x80412000u;
    const u32 destination = 0x00A04000u;
    const u32 total_size = 0x5000u;
    const u32 offset = 0x1AC0u;
    const u32 return_pc = 0x81800108u;
    const u32 queue = command + 0x2Cu;
    const u32 message_slot = command + 0x4Cu;

    mem_write32(&cpu, command + 0x04u, destination);
    mem_write32(&cpu, command + 0x08u, total_size);
    mem_write32(&cpu, command + 0x10u, stream);
    mem_write32(&cpu, command + 0x14u, offset);
    mem_write32(&cpu, command + 0x18u, buffer);
    mem_write32(&cpu, command + 0x1Cu, 0x2000u);
    mem_write32(&cpu, command + 0x20u, 0u);
    mem_write8(&cpu, command + 0x24u, 0u);

    mem_write32(&cpu, queue + 0x00u, 0u);
    mem_write32(&cpu, queue + 0x04u, 0u);
    mem_write32(&cpu, queue + 0x08u, 0u);
    mem_write32(&cpu, queue + 0x0Cu, 0u);
    mem_write32(&cpu, queue + 0x10u, message_slot);
    mem_write32(&cpu, queue + 0x14u, 1u);
    mem_write32(&cpu, queue + 0x18u, 0u);
    mem_write32(&cpu, queue + 0x1Cu, 0u);

    mem_write32(&cpu, stream, vtable);
    mem_write32(&cpu, vtable + 0x14u, STREAM_READ_SENTINEL);
    mem_write32(&cpu, vtable + 0x20u, STREAM_SEEK_SENTINEL);

    ContractState state = {
        .stream = stream,
        .buffer = buffer,
        .destination = destination,
        .total_size = total_size,
        .offset = offset,
    };
    cpu.gpr[1] = 0x81700000u;
    cpu.gpr[3] = command;
    cpu.gpr[13] = 0x803FE0E0u;
    cpu.lr = return_pc;
    cpu.pc = JKR_ARAM_STREAM_WRITE_TO_ARAM;
    cpu.msr = PPC_MSR_EE | PPC_MSR_RI;

    dispatch_until(module, &cpu, &state, return_pc);
    assert(state.seek_calls == 1u);
    assert(state.read_calls == 3u);
    assert(state.dma_calls == 3u);
    assert(state.dma_bytes == total_size);
    assert(cpu.gpr[3] == total_size);
    assert(mem_read32(&cpu, queue + 0x18u) == 0u);
    assert(mem_read32(&cpu, queue + 0x1Cu) == 1u);
    assert(mem_read32(&cpu, message_slot) == total_size);
    assert(mem_read8(&cpu, command + 0x24u) == 0u);

    cpu_free(&cpu);
    dlclose(library);
    puts("Generated JKRAramStream write contract test passed.");
    return 0;
}
