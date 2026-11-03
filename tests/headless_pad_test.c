#include "core/cpu.h"
#include "gxruntime/hle.h"
#include "gxruntime/headless_backend.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>

int main(void) {
    DolHeadlessBackend backend;
    dol_headless_backend_init(&backend);
    backend.pad[0].button = 0x0100u | 0x1000u;
    backend.pad[0].stick_x = 12;
    backend.pad_mask = 1u;
    dol_headless_backend_install(&backend);

    CPUState cpu;
    assert(cpu_init(&cpu));
    const u32 output = GC_RAM_BASE + 0x100u;
    cpu.gpr[3] = output;
    dol_hle_PADRead(&cpu);

    assert(cpu.gpr[3] == 1u);
    assert(mem_read16(&cpu, output) == (0x0100u | 0x1000u));
    assert((s8)mem_read8(&cpu, output + 2u) == 12);
    assert(backend.pad_read_count == 1u);
    puts("PASS: headless PAD status reaches guest PADRead");
    return 0;
}
