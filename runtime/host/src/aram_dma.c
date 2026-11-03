#include "aram_dma.h"

#include "gxruntime/aram.h"

#include <string.h>

#define BLUEWAKE_MEM1_PHYSICAL_SIZE 0x01800000u
#define BLUEWAKE_MEM1_CACHED_BASE 0x80000000u

static bool valid_access(u32 address, u8 size) {
    return (size == 2u || size == 4u) &&
           address >= BLUEWAKE_ARAM_DMA_BASE &&
           address + size <= BLUEWAKE_ARAM_DMA_BASE + BLUEWAKE_ARAM_DMA_BYTES &&
           (address & 1u) == 0u;
}

static u16 register_read(const BluewakeAramDma* dma, u32 address) {
    return dma->regs[(address - BLUEWAKE_ARAM_DMA_BASE) / 2u];
}

static void register_write(BluewakeAramDma* dma, u32 address, u16 value) {
    dma->regs[(address - BLUEWAKE_ARAM_DMA_BASE) / 2u] = value;
}

static bool commit_transfer(BluewakeAramDma* dma, CPUState* cpu) {
    const u32 main_physical = ((u32)(dma->regs[0] & 0x03FFu) << 16) |
                              (u32)(dma->regs[1] & 0xFFE0u);
    const u32 aram_address = ((u32)(dma->regs[2] & 0x03FFu) << 16) |
                             (u32)(dma->regs[3] & 0xFFE0u);
    const u32 direction = (dma->regs[4] >> 15) & 1u;
    const u32 length = ((u32)(dma->regs[4] & 0x03FFu) << 16) |
                       (u32)(dma->regs[5] & 0xFFE0u);

    if (cpu == NULL || dma->interrupt_pending || length == 0u ||
        (main_physical & 0x1Fu) != 0u || (aram_address & 0x1Fu) != 0u ||
        main_physical >= BLUEWAKE_MEM1_PHYSICAL_SIZE ||
        length > BLUEWAKE_MEM1_PHYSICAL_SIZE - main_physical ||
        length > ARAM_SIZE) {
        dma->rejected_count++;
        return false;
    }

    const u32 main_cached = BLUEWAKE_MEM1_CACHED_BASE | main_physical;
    if (direction == 0u)
        aram_dma_to_aram(cpu->ram, main_cached, aram_address, length);
    else
        aram_dma_to_ram(cpu->ram, main_cached, aram_address, length);

    dma->last_main_address = main_cached;
    dma->last_aram_address = aram_address;
    dma->last_length = length;
    dma->last_direction = direction;
    dma->transfer_count++;
    dma->interrupt_pending = true;
    return true;
}

void bluewake_aram_dma_init(BluewakeAramDma* dma) {
    if (dma != NULL)
        memset(dma, 0, sizeof(*dma));
}

bool bluewake_aram_dma_contains(u32 address) {
    return address >= BLUEWAKE_ARAM_DMA_BASE &&
           address < BLUEWAKE_ARAM_DMA_BASE + BLUEWAKE_ARAM_DMA_BYTES;
}

u64 bluewake_aram_dma_read(const BluewakeAramDma* dma, u32 address, u8 size) {
    if (dma == NULL || !valid_access(address, size))
        return 0u;
    if (size == 2u)
        return register_read(dma, address);
    return ((u64)register_read(dma, address) << 16) |
           register_read(dma, address + 2u);
}

bool bluewake_aram_dma_write(BluewakeAramDma* dma, CPUState* cpu, u32 address,
                             u8 size, u64 value) {
    if (dma == NULL || !valid_access(address, size))
        return false;

    if (size == 2u) {
        register_write(dma, address, (u16)value);
    } else {
        register_write(dma, address, (u16)(value >> 16));
        register_write(dma, address + 2u, (u16)value);
    }

    if (address + size == BLUEWAKE_ARAM_DMA_BASE + BLUEWAKE_ARAM_DMA_BYTES)
        return commit_transfer(dma, cpu);
    return true;
}

bool bluewake_aram_dma_interrupt_pending(const BluewakeAramDma* dma) {
    return dma != NULL && dma->interrupt_pending;
}

void bluewake_aram_dma_acknowledge(BluewakeAramDma* dma) {
    if (dma != NULL)
        dma->interrupt_pending = false;
}
