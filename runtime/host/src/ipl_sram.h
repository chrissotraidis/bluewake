#ifndef BLUEWAKE_IPL_SRAM_H
#define BLUEWAKE_IPL_SRAM_H

// The console's SRAM behind EXI channel 0, device 1 (the IPL/RTC chip), for
// the SDK's __OSInitSram/WriteSram (ref/tww src/dolphin/os/OSRtc.c): a 4-byte
// immediate command (0x20000100 read, 0xA0000100 + (offset << 6) write), then
// a DMA read of the 64 bytes or immediate writes of the changed tail.
//
// Without it every EXI access read 0, so the game saw an all-zero SRAM, and
// OSGetSoundMode() (flags bit 2) said mono: Wind Waker set mono output at
// every new file and every file load, and its audio was mono (CURRENT.md,
// 2026-09-24). The defaults are Dolphin's (Source/Core/Core/HW/Sram.cpp):
// English, stereo, setup done.
//
// Only channel 0 is modelled, and only device 1 answers: the memory card is
// served at the CARD API (card_runtime.c), so channel 0's EXT bit stays clear
// and a probe of device 0 sees no card, exactly as before.

#include "core/types.h"

#include <stdbool.h>

#define BLUEWAKE_IPL_SRAM_SIZE 64u

typedef struct BluewakeIplSram {
    bool enabled;
    u8 sram[BLUEWAKE_IPL_SRAM_SIZE];
    const char* path;  // persisted after each write when set
    u32 status, dma_address, dma_length, control, data;
    bool command_latched;
    bool write;         // latched command is a write
    u32 cursor;         // SRAM byte offset of the next transfer
    bool sram_region;   // latched command addresses SRAM (else: reads 0)
    u64 writes;
} BluewakeIplSram;

// Guest memory for DMA: read and write one byte at a physical address.
typedef void (*BluewakeIplWrite8)(void* user, u32 physical, u8 value);
typedef u8 (*BluewakeIplRead8)(void* user, u32 physical);

// spec: NULL or "" or "0" leaves the device off (every access reads 0, the
// behaviour the certified route was recorded with); "default" uses Dolphin's
// SRAM in memory; anything else is a file path, loaded when it holds 64 bytes
// and created from the defaults otherwise.
void bluewake_ipl_sram_init(BluewakeIplSram* d, const char* spec);
void bluewake_ipl_sram_defaults(u8 sram[BLUEWAKE_IPL_SRAM_SIZE]);
bool bluewake_ipl_sram_contains(const BluewakeIplSram* d, u32 address);
u32 bluewake_ipl_sram_read(const BluewakeIplSram* d, u32 address);
void bluewake_ipl_sram_write(BluewakeIplSram* d, u32 address, u32 value,
                             BluewakeIplRead8 read8, BluewakeIplWrite8 write8,
                             void* user);

#endif
