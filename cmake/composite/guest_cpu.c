// The guest CPU's state, kept in the module at a fixed address.
//
// Translated code reaches every register and flag through its CPUState. With
// a pointer parameter the pointer needs a host register for the whole of a
// chunk, and a chunk is one function of 4,096 instructions with a label per
// instruction: on x86-64's 16 registers clang keeps spilling it, so most
// fields are read through a reload of the pointer first (about 32,000
// reloads in one chunk). The Windows builder rewrites the chunks to address
// this object directly (scripts/windows/global_guest_cpu.py), which removes
// those reloads in the donor measurements; BlueWake qualification is separate.
// The host runs the guest on this object when the
// module offers it (bluewake_composite_guest_cpu), so there is one state.
#include "core/cpu.h"

#if defined(_WIN32)
#define BW_GUEST_CPU_EXPORT __declspec(dllexport)
#else
#define BW_GUEST_CPU_EXPORT __attribute__((visibility("default")))
#endif

CPUState bw_guest_cpu;

BW_GUEST_CPU_EXPORT CPUState* bluewake_composite_guest_cpu(void) { return &bw_guest_cpu; }

#if defined(BW_GUEST_MEM1)
// MEM1 at a fixed address too (core/cpu.h's BW_GUEST_MEM1), the size the host
// gives the guest (linked REL data above the GameCube's 24 MiB). The host
// runs the guest on it, as on bw_guest_cpu.
__attribute__((aligned(4096))) u8 BW_GUEST_MEM1[BW_GUEST_MEM1_SIZE];

BW_GUEST_CPU_EXPORT u8* bluewake_composite_guest_mem1(u32* size) {
    *size = BW_GUEST_MEM1_SIZE;
    return BW_GUEST_MEM1;
}
#endif
