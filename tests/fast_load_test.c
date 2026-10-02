#include "fast_load.h"
#include "gxruntime/aurora_backend.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>

static bool forwarding;
void dol_aurora_set_fast_forward(bool on) { forwarding = on; }
bool bluewake_quick_doors_covered(void) { return false; }

int main(void) {
    CPUState cpu;
    assert(cpu_init(&cpu));
    setenv("BLUEWAKE_FADE_FRAMES", "6", 1);
    setenv("BLUEWAKE_FAST_FORWARD", "1", 1);
    bluewake_fast_load_attach(&cpu);
    const u32 request = 0x80400000, task = 0x80400100, fader = 0x80400200;
    mem_write32(&cpu, 0x803F6898, fader);

    // A black title/menu without a scene request must keep normal timing.
    for (unsigned i = 0; i < 10; ++i) bluewake_fast_load_retrace(0);
    assert(!forwarding);

    mem_write32(&cpu, 0x803F6160, request);
    mem_write32(&cpu, request + 0x20, task);
    mem_write16(&cpu, task + 0x08, 0);
    mem_write32(&cpu, fader + 0x04, 3);
    mem_write16(&cpu, fader + 0x08, 26);
    mem_write16(&cpu, fader + 0x0A, 25);
    mem_write32(&cpu, task + 0xD0, 26);
    bluewake_fast_load_retrace(0);
    assert(mem_read16(&cpu, fader + 0x08) == 6);
    assert(mem_read16(&cpu, fader + 0x0A) < 6);
    assert(mem_read32(&cpu, task + 0xD0) > 0);
    assert(!forwarding); // never accelerate before the fade covers the picture

    mem_write32(&cpu, fader + 0x04, 0);
    for (unsigned i = 0; i < 5; ++i) bluewake_fast_load_retrace(0);
    assert(!forwarding);
    bluewake_fast_load_retrace(0);
    assert(forwarding);
    mem_write32(&cpu, fader + 0x04, 2);
    bluewake_fast_load_retrace(0);
    assert(!forwarding); // restore rendering as soon as the picture returns

    // A stuck load cannot run unpaced indefinitely.
    mem_write32(&cpu, fader + 0x04, 0);
    for (unsigned i = 0; i < 606; ++i) bluewake_fast_load_retrace(0);
    assert(!forwarding);

    setenv("BLUEWAKE_FADE_FRAMES", "0", 1);
    setenv("BLUEWAKE_FAST_FORWARD", "0", 1);
    bluewake_fast_load_reload();
    mem_write32(&cpu, 0x803F6160, 0);
    bluewake_fast_load_retrace(0);
    mem_write32(&cpu, 0x803F6160, request);
    mem_write32(&cpu, fader + 0x04, 3);
    mem_write16(&cpu, fader + 0x08, 26);
    bluewake_fast_load_retrace(0);
    assert(mem_read16(&cpu, fader + 0x08) == 26);
    mem_write32(&cpu, fader + 0x04, 0);
    for (unsigned i = 0; i < 10; ++i) bluewake_fast_load_retrace(0);
    assert(!forwarding);
    cpu_free(&cpu);
}
