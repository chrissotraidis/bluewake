#include "dpad_menus.h"

#include <stdatomic.h>
#include <stdlib.h>

static CPUState* g_cpu;
static atomic_bool g_on;

void bluewake_dpad_menus_reload(void) {
    const char* value = getenv("BLUEWAKE_DPAD_MENUS");
    atomic_store_explicit(&g_on, value != NULL && value[0] == '1', memory_order_relaxed);
}

void bluewake_dpad_menus_attach(CPUState* cpu) {
    g_cpu = cpu;
    bluewake_dpad_menus_reload();
}

void bluewake_dpad_menus_apply(DolPadState* pad) {
    CPUState* cpu = g_cpu;
    if (cpu == NULL || pad == NULL || !atomic_load_explicit(&g_on, memory_order_relaxed) ||
        (pad->button & kBwPadDpad) == 0u)
        return;
    (void)bw_dpad_menus_map((uint8_t)mem_read8(cpu, kBwDpadMenusOpen),
                            (uint8_t)mem_read8(cpu, kBwDpadMenusStatus), &pad->button,
                            &pad->stick_x, &pad->stick_y);
}
