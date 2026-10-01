// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <pthread.h>
static unsigned locks;
static int counted_lock(pthread_mutex_t* mutex) {
    ++locks;
    return pthread_mutex_lock(mutex);
}
#undef pthread_mutex_lock
#define pthread_mutex_lock counted_lock
#include "../runtime/host/src/card_runtime.c"
#undef pthread_mutex_lock
int main(void) {
    CPUState cpu = {0};
    for (unsigned i = 0; i < 10000; ++i) {
        cpu.pc = 0x80000000u + 4u * i;
        assert(!bluewake_card_runtime_dispatch(&cpu));
    }
    assert(locks == 0); // Ordinary guest blocks must not take the card lock.
    cpu.pc = 0x8031D400u;
    assert(!bluewake_card_runtime_dispatch(&cpu)); // No open card yet.
    assert(locks == 1);
    bluewake_card_runtime_suspend_writes(true);
    cpu.pc = 0x8031F45Cu; cpu.lr = 0x80003140u;
    assert(bluewake_card_runtime_dispatch(&cpu));
    assert((s32)cpu.gpr[3] == DOL_CARD_RESULT_BUSY && cpu.pc == cpu.lr);
    assert(locks == 3);
    bluewake_card_runtime_suspend_writes(false);
}
