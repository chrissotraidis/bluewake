#include "external_memory.h"

bool bluewake_external_memory_read(CPUState* cpu, u32 address, u8 size,
                                   u64* value) {
    if (value == NULL || get_ram_ptr(cpu, address, size, NULL) == NULL)
        return false;

    switch (size) {
    case 1u:
        *value = mem_read8(cpu, address);
        return true;
    case 2u:
        *value = mem_read16(cpu, address);
        return true;
    case 4u:
        *value = mem_read32(cpu, address);
        return true;
    case 8u:
        *value = mem_read64(cpu, address);
        return true;
    default:
        return false;
    }
}

bool bluewake_external_memory_write(CPUState* cpu, u32 address, u64 value,
                                    u8 size) {
    if (get_ram_ptr(cpu, address, size, NULL) == NULL)
        return false;

    switch (size) {
    case 1u:
        mem_write8(cpu, address, (u8)value);
        return true;
    case 2u:
        mem_write16(cpu, address, (u16)value);
        return true;
    case 4u:
        mem_write32(cpu, address, (u32)value);
        return true;
    case 8u:
        mem_write64(cpu, address, value);
        return true;
    default:
        return false;
    }
}
