// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BLUEWAKE_HOST_STOP_H
#define BLUEWAKE_HOST_STOP_H
#include <string.h>
static inline int bw_host_stop_status(const char* reason) {
    return reason == NULL || strcmp(reason, "quit") == 0 ? 0 : 1;
}
#endif
