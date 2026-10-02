#ifndef BLUEWAKE_NATIVE_J3D_H
#define BLUEWAKE_NATIVE_J3D_H

#include "core/cpu.h"

#define BLUEWAKE_J3D_TRANSFORM_INFO 0x802DA64Cu
#define BLUEWAKE_J3D_TRANSFORM_ANGLES 0x802DA724u

typedef bool (*BluewakeNativeJ3DReady)(void*, const CPUState*, u32);
/* Older hosts have no handshake, so prepared chunks retain translated bodies. */
int bluewake_composite_native_j3d_v1(bool enabled, BluewakeNativeJ3DReady ready, void* user);
int bluewake_native_j3d_try(CPUState* cpu, u32 address);
/* Declines without changing CPU state or memory when exact replacement is unavailable. */
int bluewake_native_j3d_transform(CPUState* cpu, u32 address);
void bluewake_native_j3d_report(void);

#endif
