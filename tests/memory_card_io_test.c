// SPDX-License-Identifier: GPL-3.0-or-later
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include "bw_posix_compat.h"
#else
#include <unistd.h>
#endif
static int fail_rename;
static unsigned rename_calls;
static int card_test_rename(const char* from, const char* to) {
    ++rename_calls;
    if (fail_rename) { errno = EACCES; return -1; }
    return rename(from, to);
}
#undef rename
#define rename card_test_rename
#include "../ref/recompcore/GXRuntime/src/memory_card.c"
#undef rename

int main(void) {
    char path[256];
    snprintf(path, sizeof path, "card-io-test-%lu.card", (unsigned long)getpid());
    DolMemoryCardConfig config = {.path = path, .size_mbits = 4,
        .game_code = {'G','Z','L','E'}, .company = {'0','1'}};
    DolMemoryCard* card = dol_card_open(&config);
    assert(card && dol_card_mount(card) == 0);
    s32 file_no;
    assert(dol_card_create_file(card, "synthetic", 8192, &file_no) == 0);
    u8 before[8192], after[8192];
    memset(before, 0x21, sizeof before);
    memset(after, 0x43, sizeof after);
    assert(dol_card_write_file(card, file_no, 0, before, sizeof before) == 0);
    fail_rename = 1;
    unsigned calls = rename_calls;
    assert(dol_card_write_file(card, file_no, 0, after, sizeof after) == DOL_CARD_RESULT_IO_ERROR);
    assert(rename_calls == calls + 1);
    dol_card_close(card);
    fail_rename = 0;
    card = dol_card_open(&config);
    assert(card && dol_card_mount(card) == 0);
    assert(dol_card_open_file(card, "synthetic", &file_no, NULL) == 0);
    assert(dol_card_read_file(card, file_no, 0, after, sizeof after) == 0);
    assert(memcmp(before, after, sizeof before) == 0);
    dol_card_close(card);
    remove(path);
    puts("memory-card I/O regression passed");
    return 0;
}
