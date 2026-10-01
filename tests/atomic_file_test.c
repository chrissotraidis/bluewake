// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
static int failing;
static int test_rename(const char* from, const char* to) {
    if (failing) { errno = EACCES; return -1; }
    return rename(from, to);
}
#define rename test_rename
#include "atomic_file.h"
#undef rename
#include "../runtime/host/src/ipl_sram.c"
int main(void) {
    char path[256];
    snprintf(path, sizeof path, "atomic-test-%lu", (unsigned long)getpid());
    BluewakeIplSram d;
    bluewake_ipl_sram_init(&d, path);
    u8 old[64], bytes[64];
    memcpy(old, d.sram, sizeof old);
    d.sram[19] ^= 4;
    failing = 1;
    persist(&d);
    FILE* file = fopen(path, "rb");
    assert(file && fread(bytes, 1, sizeof bytes, file) == sizeof bytes);
    fclose(file);
    assert(memcmp(old, bytes, sizeof bytes) == 0);
    failing = 0;
    persist(&d);
    file = fopen(path, "rb");
    assert(file && fread(bytes, 1, sizeof bytes, file) == sizeof bytes);
    fclose(file);
    assert(memcmp(d.sram, bytes, sizeof bytes) == 0);
    remove(path);
    puts("atomic file failure/retry regression passed");
}
