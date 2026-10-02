#ifndef BLUEWAKE_NATIVE_GAME_MATH_H
#define BLUEWAKE_NATIVE_GAME_MATH_H

#include "core/cpu.h"

/* Certified game math entries, from entry with LR set through blr. Success
 * preserves the translation's complete CPU/RAM result. Failure changes
 * nothing. scripts/windows/native_game_math.py checks each translated body
 * before inserting the entry hook. No identifier here may be `ctx`. */
extern int bluewake_native_game_math_enabled;
int bluewake_native_game_math(CPUState* cpu, u32 address);
void bluewake_native_game_math_report(void);

#endif
