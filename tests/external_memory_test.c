#include "external_memory.h"

#include <stdio.h>

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            fprintf(stderr, "check failed: %s:%d: %s\n", __FILE__, __LINE__, \
                    #condition);                                                \
            return 1;                                                           \
        }                                                                       \
    } while (0)

int main(void) {
    CPUState cpu;
    CHECK(cpu_init(&cpu));

    const u32 linked = 0x81F80178u;
    const u32 tagged = linked | 0x40000000u;
    const u8 initial[32] = {
        [20] = 0x80u,
        [21] = 0x39u,
        [22] = 0x40u,
        [23] = 0x4Cu,
    };
    CHECK(ppc_guest_alias_add(linked, sizeof(initial), initial));

    u64 value = 0u;
    CHECK(bluewake_external_memory_read(&cpu, tagged + 20u, 4u, &value));
    CHECK(value == 0x8039404Cu);

    CHECK(bluewake_external_memory_write(&cpu, tagged + 24u, 0x12345678u,
                                         4u));
    CHECK(mem_read32(&cpu, linked + 24u) == 0x12345678u);

    CHECK(!bluewake_external_memory_read(&cpu, 0xCC005000u, 2u, &value));
    CHECK(!bluewake_external_memory_write(&cpu, 0xCC005000u, 0u, 2u));

    ppc_guest_alias_clear();
    cpu_free(&cpu);
    puts("external memory tests passed");
    return 0;
}
