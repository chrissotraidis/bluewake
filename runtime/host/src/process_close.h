// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BLUEWAKE_PROCESS_CLOSE_H
#define BLUEWAKE_PROCESS_CLOSE_H
#include <stdio.h>
#include <unistd.h>
// UIKit close action: synchronous card writes are completed by close_card.
// Bypass static destruction of renderer threads still owned by the host.
static inline void bw_process_close(void (*close_card)(void), int status) {
    close_card();
    fflush(stdout);
    fflush(stderr);
    _exit(status);
}
#endif
