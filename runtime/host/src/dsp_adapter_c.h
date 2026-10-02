#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BluewakeDspAdapter BluewakeDspAdapter;

typedef uint8_t (*BluewakeDspReadMemoryFn)(void* user, uint32_t address);
typedef void (*BluewakeDspWriteMemoryFn)(void* user, uint32_t address,
                                         uint8_t value);
typedef uint8_t (*BluewakeDspReadAramFn)(void* user, uint32_t address);
typedef void (*BluewakeDspWriteAramFn)(void* user, uint32_t address,
                                      uint8_t value);
typedef void (*BluewakeDspDmaWriteFn)(void* user, uint32_t address,
                                      uint32_t size);
typedef void (*BluewakeDspInterruptFn)(void* user);
// High-level backend services: an alias-aware host pointer for a guest range
// (NULL if unmapped) and the guest timebase in bus ticks.
typedef uint8_t* (*BluewakeDspGuestPointerFn)(void* user, uint32_t address,
                                              uint32_t size);
typedef uint64_t (*BluewakeDspTimebaseFn)(void* user);

BluewakeDspAdapter* bluewake_dsp_adapter_create(
    const char* irom_path, const char* coef_path, void* user,
    BluewakeDspReadMemoryFn read_memory,
    BluewakeDspWriteMemoryFn write_memory,
    BluewakeDspReadAramFn read_aram,
    BluewakeDspWriteAramFn write_aram,
    BluewakeDspDmaWriteFn dma_write,
    BluewakeDspInterruptFn interrupt);
// Dolphin's high-level DSP (Zelda ucode) behind the same interface. The
// remaining functions work on either kind of adapter; IFX access is an
// LLE-only concept and is ignored by the high-level backend.
BluewakeDspAdapter* bluewake_dsp_adapter_create_hle(
    void* user, BluewakeDspGuestPointerFn guest_pointer, uint8_t* aram,
    uint32_t aram_size, BluewakeDspTimebaseFn timebase,
    BluewakeDspInterruptFn interrupt);
int bluewake_dsp_adapter_is_hle(const BluewakeDspAdapter* adapter);
void bluewake_dsp_adapter_destroy(BluewakeDspAdapter* adapter);
int bluewake_dsp_adapter_run_cycles(BluewakeDspAdapter* adapter, int cycles);
void bluewake_dsp_adapter_write_control(BluewakeDspAdapter* adapter,
                                         uint16_t value);
uint16_t bluewake_dsp_adapter_read_control(BluewakeDspAdapter* adapter);
void bluewake_dsp_adapter_write_cpu_mailbox(BluewakeDspAdapter* adapter,
                                             uint32_t value);
uint32_t bluewake_dsp_adapter_peek_cpu_mailbox(
    const BluewakeDspAdapter* adapter);
uint32_t bluewake_dsp_adapter_peek_dsp_mailbox(
    const BluewakeDspAdapter* adapter);
uint16_t bluewake_dsp_adapter_read_dsp_mailbox_low(
    BluewakeDspAdapter* adapter);
void bluewake_dsp_adapter_write_ifx(BluewakeDspAdapter* adapter,
                                     uint16_t address, uint16_t value);
uint16_t bluewake_dsp_adapter_read_ifx(BluewakeDspAdapter* adapter,
                                       uint16_t address);
// Save states (HLE only; the LLE interpreter answers 0/false). save returns
// the size of a malloc'd blob in *out (free it), 0 on failure.
size_t bluewake_dsp_adapter_save_state(BluewakeDspAdapter* adapter,
                                       uint8_t** out);
int bluewake_dsp_adapter_load_state(BluewakeDspAdapter* adapter,
                                    const uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif
