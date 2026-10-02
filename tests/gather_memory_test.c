/* Run the maintained memory contract through the module's inline wrappers,
 * then check FIFO order and the boundaries observable without a renderer. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>
#include "gather_pipe.h"

#define main ordinary_memory_contract
#include "guest_memory_bounds_test.c"
#undef main

void bluewake_composite_set_gather_pipe(BwGatherPipeWrite write);
void bluewake_composite_set_gather_pipe_bytes(BwGatherPipeBytes bytes);

static u8 captured[2048];
static u32 captured_size, word_calls, byte_calls, external_calls;
static void capture_bytes(const u8* bytes, u32 size) {
    assert(captured_size + size <= sizeof captured);
    memcpy(captured + captured_size, bytes, size);
    captured_size += size;
    byte_calls++;
}
static void capture_word(u64 value, u8 size) {
    u8 bytes[8];
    for (u8 i = 0; i < size; ++i)
        bytes[i] = (u8)(value >> (8u * (size - i - 1u)));
    capture_bytes(bytes, size);
    word_calls++;
}
static u64 external_read(CPUState* cpu, u32 addr, u8 size) {
    (void)cpu; (void)addr; (void)size;
    assert(bw_gather_pipe_length == 0u);
    external_calls++;
    return 0x123456789ABCDEF0ull;
}
static void external_write(CPUState* cpu, u32 addr, u64 value, u8 size) {
    (void)cpu; (void)addr; (void)value; (void)size;
    assert(bw_gather_pipe_length == 0u);
    external_calls++;
}
static void instruction_fallback(CPUState* cpu, u32 raw, u32 cia) {
    assert(raw == 0x90640000u && cia == 0x80001000u);
    assert(bw_gather_pipe_length == 0);
    cpu->external_write(cpu, cpu->gpr[4], cpu->gpr[3], 4);
}

int main(void) {
    assert(ordinary_memory_contract() == 0);
    CPUState cpu = {0};
    cpu.ram = bw_test_mem1;
    cpu.ram_size = sizeof bw_test_mem1;
    cpu.external_read = external_read;
    cpu.external_write = external_write;

#ifndef BW_GUEST_MEM1
    /* A subtraction underflow must never turn undersized RAM into a hit. */
    u8 tiny[1] = {0};
    cpu.ram = tiny;
    for (u32 size = 0; size <= sizeof tiny; ++size) {
        cpu.ram_size = size;
        assert(mem_read64(&cpu, GC_RAM_BASE) == 0x123456789ABCDEF0ull);
        mem_write64(&cpu, GC_RAM_BASE, UINT64_MAX);
        assert(tiny[0] == 0);
    }
    cpu.ram = bw_test_mem1;
    cpu.ram_size = sizeof bw_test_mem1;
#endif
    external_calls = 0;
    /* Without host opt-in, pipe writes remain ordinary MMIO. */
    mem_write32(&cpu, 0xCC008000u, 0x12345678u);
    assert(external_calls == 1 && captured_size == 0);
    bluewake_composite_set_gather_pipe(capture_word);
    mem_write32(&cpu, 0xCC008004u, 0x12345678u);
    assert(word_calls == 1 && captured_size == 4);
    assert(memcmp(captured, "\x12\x34\x56\x78", 4) == 0);

    captured_size = byte_calls = word_calls = 0;
    bluewake_composite_set_gather_pipe_bytes(capture_bytes);
    mem_write8(&cpu, 0xCC008000u, 0x01);
    mem_write16(&cpu, 0xCC008002u, 0x2345);
    mem_write32(&cpu, 0xCC008004u, 0x6789ABCDu);
    mem_write64(&cpu, 0xCC008008u, 0xEF0123456789ABCDull);
    assert(captured_size == 0 && bw_gather_pipe_length == 15);
    assert(mem_read32(&cpu, 0xCC000020u) == 0x9ABCDEF0u);
    const u8 expected[] = {1, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD,
                          0xEF, 1, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD};
    assert(captured_size == sizeof expected && byte_calls == 1);
    assert(memcmp(captured, expected, sizeof expected) == 0);

    /* Hardware writes and writer changes also drain earlier FIFO data. */
    mem_write8(&cpu, 0xCC008000u, 0x55);
    mem_write16(&cpu, 0xCC002000u, 0x1234);
    assert(captured_size == 16 && bw_gather_pipe_length == 0);
    mem_write8(&cpu, 0xCC008000u, 0x66);
    bluewake_composite_set_gather_pipe_bytes(NULL);
    assert(captured_size == 17 && bw_gather_pipe_length == 0);
    mem_write8(&cpu, 0xCC008000u, 0x77);
    assert(captured_size == 18 && word_calls == 1);
    bluewake_composite_set_gather_pipe_bytes(capture_bytes);
    mem_write8(&cpu, 0xCC008000u, 0x88);
    bluewake_composite_set_gather_pipe(NULL);
    assert(captured_size == 19 && bw_gather_pipe_length == 0);
    mem_write8(&cpu, 0xCC008000u, 0x99);
    assert(captured_size == 19);
    bluewake_composite_set_gather_pipe(capture_word);

    /* Interpreter paths must expose all earlier FIFO bytes before MMIO. */
    const unsigned prior_external = external_calls;
    cpu.hid2 = PPC_HID2_LSQE | PPC_HID2_LCE;
    cpu.instruction_fallback = instruction_fallback;
    mem_write8(&cpu, 0xCC008000u, 0xAA);
    cpu.gpr[3] = 0x11223344u;
    cpu.gpr[4] = 0xCC002000u;
    ppc_fallback_instruction(&cpu, 0x90640000u, 0x80001000u); /* stw r3,0(r4) */
    assert(bw_gather_pipe_length == 0 && captured_size == 20);
    mem_write8(&cpu, 0xCC008000u, 0xBB);
    assert(ppc_psq_load(&cpu, 0, 0xCC002000u, true, 0, false, 0x80001004u));
    assert(bw_gather_pipe_length == 0 && captured_size == 21);
    mem_write8(&cpu, 0xCC008000u, 0xCC);
    assert(ppc_psq_store(&cpu, 0, 0xCC002000u, true, 0, false, 0x80001008u));
    assert(bw_gather_pipe_length == 0 && captured_size == 22);
    assert(external_calls == prior_external + 3);
    mem_write8(&cpu, 0xCC008000u, 0xDD);
    ppc_dcbz_l(&cpu, GC_RAM_BASE, 0x8000100Cu);
    assert(bw_gather_pipe_length == 0 && captured_size == 23);

    /* Spill at capacity, including an eight-byte store across the threshold. */
    captured_size = byte_calls = 0;
    for (unsigned i = 0; i < 255; ++i)
        mem_write8(&cpu, 0xCC008000u, (u8)i);
    assert(captured_size == 0 && bw_gather_pipe_length == 255);
    mem_write64(&cpu, 0xCC008000u, 0x0123456789ABCDEFull);
    assert(captured_size == 263 && byte_calls == 1 && bw_gather_pipe_length == 0);
    for (unsigned i = 0; i < 255; ++i)
        assert(captured[i] == (u8)i);
    assert(memcmp(captured + 255, "\x01\x23\x45\x67\x89\xAB\xCD\xEF", 8) == 0);

    /* Registered storage has precedence over MMIO, including pipe addresses. */
    u8 pipe_alias[32] = {0};
    for (unsigned mirror = 0; mirror < 2; ++mirror) {
        const u32 linked = mirror ? 0x8C008000u : 0xCC008000u;
        assert(ppc_guest_alias_add_shared(linked, sizeof pipe_alias, pipe_alias));
        mem_write8(&cpu, 0xCC00801Fu, 0xEF);
        assert(pipe_alias[31] == 0xEF && captured_size == 263);
        assert(bw_gather_pipe_length == 0);
        ppc_guest_alias_clear();
        memset(pipe_alias, 0, sizeof pipe_alias);
    }
    bluewake_composite_set_gather_pipe(NULL);
    bluewake_composite_set_gather_pipe_bytes(NULL);
    puts("gather and inline-memory contracts passed");
    return 0;
}
