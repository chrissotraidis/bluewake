#ifndef BLUEWAKE_COLD_FALLBACK_H
#define BLUEWAKE_COLD_FALLBACK_H

#include "core/cpu.h"

// Execute one ordinary scalar instruction reached through an LLVM cold entry.
// The caller remains responsible for privileged/cache instructions.
bool bluewake_cold_fallback_execute(CPUState* cpu, u32 raw, u32 cia);

#endif
