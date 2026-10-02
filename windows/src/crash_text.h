// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
// No heap, stdio or locale in the fault path.
static inline unsigned bw_crash_hex(char output[18], uint64_t value) {
    output[0] = '0'; output[1] = 'x';
    for (unsigned i = 0; i < 16; ++i) output[2 + i] = "0123456789abcdef"[(value >> ((15 - i) * 4)) & 15];
    return 18;
}
