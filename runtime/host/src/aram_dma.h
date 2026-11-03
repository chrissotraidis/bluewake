#ifndef BLUEWAKE_ARAM_DMA_H
#define BLUEWAKE_ARAM_DMA_H

#include "core/cpu.h"

#define BLUEWAKE_ARAM_DMA_BASE 0xCC005020u
#define BLUEWAKE_ARAM_DMA_BYTES 0x0Cu

typedef struct BluewakeAramDma {
    u16 regs[6];
    bool interrupt_pending;
    u64 transfer_count;
    u64 rejected_count;
    u32 last_main_address;
    u32 last_aram_address;
    u32 last_length;
    u32 last_direction;
} BluewakeAramDma;

void bluewake_aram_dma_init(BluewakeAramDma* dma);
bool bluewake_aram_dma_contains(u32 address);
u64 bluewake_aram_dma_read(const BluewakeAramDma* dma, u32 address, u8 size);
bool bluewake_aram_dma_write(BluewakeAramDma* dma, CPUState* cpu, u32 address,
                             u8 size, u64 value);
bool bluewake_aram_dma_interrupt_pending(const BluewakeAramDma* dma);
void bluewake_aram_dma_acknowledge(BluewakeAramDma* dma);

#endif
