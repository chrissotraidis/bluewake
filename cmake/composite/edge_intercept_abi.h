#ifndef BLUEWAKE_EDGE_INTERCEPT_ABI_H
#define BLUEWAKE_EDGE_INTERCEPT_ABI_H

#include "core/cpu.h"

#include <stdbool.h>

// Called after a generated edge and before the next dispatch. The host may
// flush in-flight cycles, publish due device state, and rebudget the CPU before
// returning whether execution must leave the composite loop.
typedef bool (*BluewakeEdgeServiceFn)(void* user, CPUState* cpu, u32 address);

#endif
