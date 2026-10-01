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
#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
static int fail_directory_sync;
static int card_test_fsync(int fd) {
    struct stat status;
    assert(fstat(fd, &status) == 0);
    if (fail_directory_sync && S_ISDIR(status.st_mode)) { errno = EIO; return -1; }
    return fsync(fd);
}
#define fsync card_test_fsync
#endif
static int fail_flush;
static unsigned flush_calls;
static int card_test_fflush(FILE* file) {
    ++flush_calls;
    if (fail_flush) { errno = EIO; return EOF; }
    return fflush(file);
}
#define fflush card_test_fflush
static int fail_rename;
static unsigned rename_calls;
static int card_test_rename(const char* from, const char* to) {
    size_t length = strlen(to);
    int live = length >= 5 && strcmp(to + length - 5, ".card") == 0;
    if (live) { assert(flush_calls > rename_calls); ++rename_calls; }
    if (live && fail_rename) { errno = EACCES; return -1; }
    return rename(from, to);
}
#undef rename
#define rename card_test_rename
#include "../ref/recompcore/GXRuntime/src/memory_card.c"
#undef rename
#undef fflush
#ifndef _WIN32
#undef fsync
static void assert_disk_matches_memory(DolMemoryCard* card) {
    DolMemoryCard disk = {0};
    disk.path = card->path;
    assert(load_container(&disk));
    for (unsigned i = 0; i < DOL_CARD_MAX_FILES; ++i) {
        assert(card->files[i].used == disk.files[i].used);
        if (!card->files[i].used) continue;
        assert(card->files[i].stat.length == disk.files[i].stat.length);
        assert(card->files[i].stat.banner_format == disk.files[i].stat.banner_format);
        assert(card->files[i].stat.time == disk.files[i].stat.time);
        assert(memcmp(card->files[i].data, disk.files[i].data, card->files[i].stat.length) == 0);
    }
    clear_files(&disk);
}
#endif

int main(void) {
    char path[256];
    snprintf(path, sizeof path, "card-io-test-%lu.card", (unsigned long)getpid());
    DolMemoryCardConfig config = {.path = path, .size_mbits = 4,
        .game_code = {'G','Z','L','E'}, .company = {'0','1'}};
    DolMemoryCard* card = dol_card_open(&config);
    assert(card && dol_card_mount(card) == 0);
    assert(dol_card_open(&config) == NULL);
    s32 file_no;
    assert(dol_card_create_file(card, "synthetic", 8192, &file_no) == 0);
    u8 before[8192], after[8192];
    memset(before, 0x21, sizeof before);
    memset(after, 0x43, sizeof after);
    assert(dol_card_write_file(card, file_no, 0, before, sizeof before) == 0);
    char backup_path[300];
    snprintf(backup_path, sizeof backup_path, "%s.bak", path);
    assert(dol_card_validate(backup_path));
    assert(dol_card_write_file(card, file_no, 0, after, sizeof after) == 0);
    DolMemoryCardConfig backup_config = config;
    backup_config.path = backup_path;
    DolMemoryCard* backup_card_handle = dol_card_open(&backup_config);
    assert(backup_card_handle && dol_card_mount(backup_card_handle) == 0);
    u8 backup_bytes[8192];
    assert(dol_card_read_file(backup_card_handle, file_no, 0, backup_bytes, sizeof backup_bytes) == 0);
    assert(memcmp(backup_bytes, before, sizeof before) == 0);
    dol_card_close(backup_card_handle);
    assert(dol_card_write_file(card, file_no, 0, before, sizeof before) == 0);
    fail_flush = 1;
    unsigned flush_before = rename_calls;
    assert(dol_card_write_file(card, file_no, 0, after, sizeof after) == DOL_CARD_RESULT_IO_ERROR);
    assert(rename_calls == flush_before);
    fail_flush = 0;
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
#ifndef _WIN32
    // Every mutating operation must stay consistent after publication, while
    // still returning the directory-sync error to the guest.
    memset(after, 0x43, sizeof after);
    fail_directory_sync = 1;
    assert(dol_card_write_file(card, file_no, 0, after, sizeof after) == DOL_CARD_RESULT_IO_ERROR);
    assert_disk_matches_memory(card);
    fail_directory_sync = 0;
    DolMemoryCardStat metadata;
    assert(dol_card_get_status(card, file_no, &metadata) == 0);
    metadata.banner_format = 1;
    assert(dol_card_set_status(card, file_no, &metadata) == 0);
    assert_disk_matches_memory(card);
    assert(memcmp(card->files[file_no].data, after, sizeof after) == 0);
    fail_directory_sync = 1;
    metadata.banner_format = 2;
    assert(dol_card_set_status(card, file_no, &metadata) == DOL_CARD_RESULT_IO_ERROR);
    assert_disk_matches_memory(card);
    assert(dol_card_create_file(card, "published", 8192, NULL) == DOL_CARD_RESULT_IO_ERROR);
    assert_disk_matches_memory(card);
    assert(dol_card_delete_file(card, "published") == DOL_CARD_RESULT_IO_ERROR);
    assert_disk_matches_memory(card);
    assert(dol_card_format(card) == DOL_CARD_RESULT_IO_ERROR);
    assert_disk_matches_memory(card);
    fail_directory_sync = 0;
    assert(dol_card_create_file(card, "synthetic", 8192, &file_no) == 0);
    assert(dol_card_write_file(card, file_no, 0, before, sizeof before) == 0);
#endif
    dol_card_close(card);
    assert(dol_card_validate(path));
    u8* bytes = NULL;
    size_t size = 0;
    assert(read_whole_file(path, &bytes, &size));
    char bad[300];
    snprintf(bad, sizeof bad, "%s.bad", path);
    for (int mode = 0; mode < 2; ++mode) {
        FILE* file = fopen(bad, "wb");
        assert(file);
        if (mode == 1) bytes[size - 1] ^= 1;
        assert(fwrite(bytes, 1, mode == 0 ? size - 1 : size, file) == (mode == 0 ? size - 1 : size));
        assert(fclose(file) == 0);
        assert(!dol_card_validate(bad));
        config.path = bad;
        assert(dol_card_open(&config) == NULL);
        u8* check = NULL; size_t check_size = 0;
        assert(read_whole_file(bad, &check, &check_size));
        assert(check_size == (mode == 0 ? size - 1 : size));
        assert(memcmp(bytes, check, check_size) == 0);
        free(check);
    }
    free(bytes);
    remove(bad);
    remove(path);
    puts("memory-card I/O regression passed");
    return 0;
}
