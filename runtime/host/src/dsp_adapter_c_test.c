#include "dsp_adapter_c.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct TestMemory {
    uint8_t bytes[0x200000];
    uint8_t aram[0x1000000];
    unsigned dma_writes;
    unsigned interrupts;
};

static uint8_t read_memory(void* user, uint32_t address) {
    struct TestMemory* memory = (struct TestMemory*)user;
    const uint32_t index = address >= 0x81000000u ? address - 0x81000000u : address;
    assert(index < sizeof(memory->bytes));
    return memory->bytes[index];
}

static uint8_t read_aram(void* user, uint32_t address) {
    struct TestMemory* memory = (struct TestMemory*)user;
    return memory->aram[address & (sizeof(memory->aram) - 1u)];
}

static void write_aram(void* user, uint32_t address, uint8_t value) {
    struct TestMemory* memory = (struct TestMemory*)user;
    memory->aram[address & (sizeof(memory->aram) - 1u)] = value;
}

static void write_memory(void* user, uint32_t address, uint8_t value) {
    struct TestMemory* memory = (struct TestMemory*)user;
    const uint32_t index = address >= 0x81000000u ? address - 0x81000000u : address;
    assert(index < sizeof(memory->bytes));
    memory->bytes[index] = value;
}

static void dma_write(void* user, uint32_t address, uint32_t size) {
    struct TestMemory* memory = (struct TestMemory*)user;
    assert(address != 0 || size != 0);
    memory->dma_writes++;
}

static void interrupt(void* user) {
    struct TestMemory* memory = (struct TestMemory*)user;
    memory->interrupts++;
}

int main(int argc, char** argv) {
    assert(argc == 3);
    static struct TestMemory memory;
    BluewakeDspAdapter* empty_adapter = bluewake_dsp_adapter_create(
        argv[1], argv[2], &memory, read_memory, write_memory, read_aram,
        write_aram, dma_write, interrupt);
    assert(empty_adapter != NULL);
    bluewake_dsp_adapter_write_control(empty_adapter, 0);
    assert(bluewake_dsp_adapter_run_cycles(empty_adapter, 12) == 0);
    bluewake_dsp_adapter_destroy(empty_adapter);

    for (uint32_t offset = 0; offset < 0x1000; offset += 2) {
        memory.bytes[offset] = 0;
        memory.bytes[offset + 1] = 0x21;
    }

    BluewakeDspAdapter* adapter = bluewake_dsp_adapter_create(
        argv[1], argv[2], &memory, read_memory, write_memory, read_aram,
        write_aram, dma_write, interrupt);
    assert(adapter != NULL);
    bluewake_dsp_adapter_write_control(adapter, 0);
    assert((bluewake_dsp_adapter_read_control(adapter) & 0x0804u) == 0);
    bluewake_dsp_adapter_write_cpu_mailbox(adapter, 0x12345678u);
    assert((bluewake_dsp_adapter_peek_cpu_mailbox(adapter) & 0x80000000u) != 0);
    bluewake_dsp_adapter_write_ifx(adapter, 0xD1, 0x0040);
    assert(bluewake_dsp_adapter_read_ifx(adapter, 0xD1) == 0x0040);
    bluewake_dsp_adapter_write_ifx(adapter, 0xFC, 0x1234);
    bluewake_dsp_adapter_write_ifx(adapter, 0xFD, 0x5678);
    assert((bluewake_dsp_adapter_peek_dsp_mailbox(adapter) & 0x80000000u) != 0);
    assert(bluewake_dsp_adapter_read_dsp_mailbox_low(adapter) == 0x5678u);
    assert((bluewake_dsp_adapter_peek_dsp_mailbox(adapter) & 0x80000000u) == 0);
    assert(bluewake_dsp_adapter_run_cycles(adapter, 12) >= 0);
    bluewake_dsp_adapter_destroy(adapter);
    return 0;
}
