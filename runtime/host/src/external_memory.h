#ifndef BLUEWAKE_EXTERNAL_MEMORY_H
#define BLUEWAKE_EXTERNAL_MEMORY_H

#include "core/cpu.h"

#include <stdbool.h>

bool bluewake_external_memory_read(CPUState* cpu, u32 address, u8 size,
                                   u64* value);
bool bluewake_external_memory_write(CPUState* cpu, u32 address, u64 value,
                                    u8 size);

#endif
