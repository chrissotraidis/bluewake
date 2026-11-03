#ifndef BLUEWAKE_FPU_CONTEXT_H
#define BLUEWAKE_FPU_CONTEXT_H

#include "core/cpu.h"

typedef enum BluewakeFpuSwitchResult {
    BLUEWAKE_FPU_SWITCH_INVALID,
    BLUEWAKE_FPU_SWITCH_SAME_OWNER,
    BLUEWAKE_FPU_SWITCH_FRESH,
    BLUEWAKE_FPU_SWITCH_RESTORED,
} BluewakeFpuSwitchResult;

BluewakeFpuSwitchResult bluewake_fpu_exception_deliver(CPUState* cpu);
bool bluewake_fpu_registers_materialized(CPUState* cpu);

#endif
