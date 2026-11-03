#include "aram_dma.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include "gxruntime/aram.h"

#include <assert.h>
#include <stdio.h>

static void program_transfer(BluewakeAramDma* dma, CPUState* cpu,
                             u32 direction, u32 main_address,
                             u32 aram_address, u32 length) {
    assert(bluewake_aram_dma_write(
        dma, cpu, BLUEWAKE_ARAM_DMA_BASE + 0x00u, 2,
        (main_address >> 16) & 0x03FFu));
    assert(bluewake_aram_dma_write(
        dma, cpu, BLUEWAKE_ARAM_DMA_BASE + 0x02u, 2,
        main_address & 0xFFE0u));
    assert(bluewake_aram_dma_write(
        dma, cpu, BLUEWAKE_ARAM_DMA_BASE + 0x04u, 2,
        (aram_address >> 16) & 0x03FFu));
    assert(bluewake_aram_dma_write(
        dma, cpu, BLUEWAKE_ARAM_DMA_BASE + 0x06u, 2,
        aram_address & 0xFFE0u));
    assert(bluewake_aram_dma_write(
        dma, cpu, BLUEWAKE_ARAM_DMA_BASE + 0x08u, 2,
        (direction << 15) | ((length >> 16) & 0x03FFu)));
    assert(bluewake_aram_dma_write(
        dma, cpu, BLUEWAKE_ARAM_DMA_BASE + 0x0Au, 2,
        length & 0xFFE0u));
}

int main(void) {
    CPUState cpu;
    assert(cpu_init(&cpu));
    aram_free();
    aram_init();

    BluewakeAramDma dma;
    bluewake_aram_dma_init(&dma);
    assert(bluewake_aram_dma_contains(BLUEWAKE_ARAM_DMA_BASE));
    assert(!bluewake_aram_dma_contains(BLUEWAKE_ARAM_DMA_BASE - 2u));

    const u32 main_address = 0x80012000u;
    const u32 aram_address = 0x00024000u;
    for (u32 i = 0; i < 0x40u; i += 4u)
        mem_write32(&cpu, main_address + i, 0xA5A50000u | i);

    program_transfer(&dma, &cpu, 0u, main_address, aram_address, 0x40u);
    assert(bluewake_aram_dma_interrupt_pending(&dma));
    assert(dma.transfer_count == 1u);
    assert(dma.last_main_address == main_address);
    assert(dma.last_aram_address == aram_address);
    assert(dma.last_length == 0x40u);
    assert(aram_read(aram_address, 4) == 0xA5A50000u);

    assert(!bluewake_aram_dma_write(
        &dma, &cpu, BLUEWAKE_ARAM_DMA_BASE + 0x0Au, 2, 0x40u));
    assert(dma.transfer_count == 1u);
    assert(dma.rejected_count == 1u);

    bluewake_aram_dma_acknowledge(&dma);
    assert(!bluewake_aram_dma_interrupt_pending(&dma));
    aram_write(aram_address, 0x11223344u, 4);
    program_transfer(&dma, &cpu, 1u, main_address, aram_address, 0x40u);
    assert(mem_read32(&cpu, main_address) == 0x11223344u);
    assert(dma.transfer_count == 2u);
    assert(dma.last_direction == 1u);
    assert(bluewake_aram_dma_read(
               &dma, BLUEWAKE_ARAM_DMA_BASE + 0x08u, 4) == 0x80000040u);

    bluewake_aram_dma_acknowledge(&dma);
    mem_write32(&cpu, main_address, 0x55667788u);
    program_transfer(&dma, &cpu, 0u, main_address, ARAM_SIZE, 0x20u);
    assert(aram_read(0u, 4) == 0x55667788u);
    assert(dma.transfer_count == 3u);

    bluewake_aram_dma_acknowledge(&dma);
    aram_free();
    cpu_free(&cpu);
    puts("ARAM DMA register/completion contract test passed.");
    return 0;
}
