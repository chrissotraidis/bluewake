#ifndef BLUEWAKE_CALLBACK_DELIVERY_H
#define BLUEWAKE_CALLBACK_DELIVERY_H

#include "StaticRecompABI.h"

typedef struct BluewakeCallbackDeliveryResult {
    bool completed;
    unsigned dispatches;
    u32 terminal_pc;
    u32 exception;
} BluewakeCallbackDeliveryResult;

typedef void (*BluewakePrepareGuestDispatchFn)(CPUState* cpu, void* user);

// Run a hardware-originated guest callback under the retail interrupt-handler
// scheduler lock with EE masked. Retain memory effects and restore registers.
BluewakeCallbackDeliveryResult bluewake_deliver_guest_callback(
    CPUState* cpu, const StaticRecompModuleDesc* module, u32 callback,
    u32 arg0, u32 arg1, unsigned max_dispatches,
    BluewakePrepareGuestDispatchFn prepare_dispatch, void* prepare_user);

#endif
