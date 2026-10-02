#ifndef BLUEWAKE_NATIVE_GAME_MATH_H
#define BLUEWAKE_NATIVE_GAME_MATH_H

#include "core/cpu.h"

/* Certified game math entries, from entry with LR set through blr. Success
 * preserves the translation's complete CPU/RAM result. Failure changes
 * nothing. scripts/windows/native_game_math.py checks each translated body
 * before inserting the entry hook. No identifier here may be `ctx`. */
typedef bool (*BluewakeNativeGameMathReady)(void*, const CPUState*, u32);
int bluewake_composite_native_game_math_v1(bool enabled, BluewakeNativeGameMathReady ready, void* user);
int bluewake_native_game_math_try(CPUState* cpu, u32 address);
int bluewake_native_game_math(CPUState* cpu, u32 address);
void bluewake_native_game_math_report(void);

#endif
