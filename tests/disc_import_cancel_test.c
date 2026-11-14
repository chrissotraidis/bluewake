// SPDX-License-Identifier: GPL-3.0-or-later
// The importer's cancel hook, against a generated disc image: a cancel lands
// between files, removes everything the import wrote, and leaves the folder as
// it found it. The image is synthetic (a real GZLE01 disc is the player's own),
// so main.dol is accepted through BLUEWAKE_ACCEPT_DOL_SHA1, as the builder's
// import does for a patched disc.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../apple/ios/src/disc_import.c"

// Enough members inside RELS.arc to clear the importer's "only N game modules
// were found" floor, so a run that is not cancelled really succeeds: the cancel
// cases below then prove a cancel stopped a working import, not a broken fixture.
#define kMembers 600
#define kMemberBytes 16

static void put_be32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void put_be16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

// A RARC holding kMembers Yaz0-less RELs, laid out the way unpack_rels_arc()
// reads one: INFO at 0x20, file entries and the string table after it, member
// data last, every offset relative to 0x20.
static uint8_t* build_arc(size_t* out_size) {
    const size_t header = 0x20, info = 0x20, entries_at = header + info;
    const size_t entry_bytes = kMembers * 0x14;
    char names[kMembers][13];
    size_t strings_at = entries_at + entry_bytes;
    size_t strings_size = 0;
    size_t name_off[kMembers];
    for (int i = 0; i < kMembers; ++i) {
        snprintf(names[i], sizeof names[i], "m%03d.rel", i);
        name_off[i] = strings_size;
        strings_size += strlen(names[i]) + 1;
    }
    const size_t data_at = strings_at + strings_size;
    const size_t size = data_at + (size_t)kMembers * kMemberBytes;
    uint8_t* arc = (uint8_t*)calloc(1, size);
    assert(arc != NULL);
    memcpy(arc, "RARC", 4);
    put_be32(arc + 0x04, (uint32_t)size);
    put_be32(arc + 0x08, (uint32_t)header);
    put_be32(arc + 0x0C, (uint32_t)(data_at - header));       // data_start = 0x20 + this
    put_be32(arc + 0x10, (uint32_t)(size - data_at));
    memcpy(arc + header, "INFO", 4);
    put_be32(arc + header + 0x04, (uint32_t)(info + entry_bytes + strings_size));
    put_be32(arc + header + 0x08, (uint32_t)kMembers);
    put_be32(arc + header + 0x0C, (uint32_t)(entries_at - header));
    put_be32(arc + header + 0x10, (uint32_t)(entries_at - header));
    put_be32(arc + header + 0x14, (uint32_t)(strings_at - header));
    for (int i = 0; i < kMembers; ++i) {
        // [id u16][hash u16][flags u8][? u8][name offset u16][data offset u32]
        // [size u32][zero u32] — the layout unpack_rels_arc() reads, where the
        // flags byte's bit 0 marks a file and low 16 bits index the string table.
        uint8_t* e = arc + entries_at + (size_t)i * 0x14;
        put_be16(e + 0, 0);
        put_be16(e + 2, 0);
        e[4] = 0x01;
        e[5] = 0;
        put_be16(e + 6, (uint16_t)name_off[i]);
        put_be32(e + 8, (uint32_t)((size_t)i * kMemberBytes));
        put_be32(e + 12, kMemberBytes);
        memcpy(arc + strings_at + name_off[i], names[i], strlen(names[i]) + 1);
    }
    for (int i = 0; i < kMembers; ++i) {
        memset(arc + data_at + (size_t)i * kMemberBytes, 'a' + i % 26, kMemberBytes);
    }
    *out_size = size;
    return arc;
}

static int exists(const char* path) {
    struct stat info;
    return stat(path, &info) == 0;
}

static int count_files(const char* dir) {
    DIR* d = opendir(dir);
    if (d == NULL) return -1;
    int n = 0;
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        ++n;
    }
    closedir(d);
    return n;
}

// A GZLE01 image with the generated executable, a three-entry file table (root,
// an empty rels/ directory, RELS.arc) and the archive.
static void write_image(const char* path, const uint8_t* arc, size_t arc_size,
                        char sha[41]) {
    uint8_t header[0x440];
    memset(header, 0, sizeof header);
    memcpy(header, "GZLE01", 6);
    put_be32(header + 0x1C, BW_GC_MAGIC);
    const uint32_t dol_off = 0x440, dol_size = 0x100;
    const uint32_t fst_off = dol_off + dol_size;
    const uint32_t count = 3;
    const uint32_t strings_at = count * 12;
    uint8_t fst[12 * 3 + 15];
    memset(fst, 0, sizeof fst);
    put_be32(fst + 8, count);
    put_be32(fst + 12, 0x01000000u | 1u);            // "rels", a directory
    put_be32(fst + 20, 2);                           // its children end at entry 2
    put_be32(fst + 24, 0u | 6u);                     // "RELS.arc", a file
    const uint32_t arc_off = fst_off + (uint32_t)sizeof fst;
    put_be32(fst + 28, arc_off);
    put_be32(fst + 32, (uint32_t)arc_size);
    memcpy(fst + strings_at, "\0rels\0RELS.arc", 14);
    put_be32(header + 0x420, dol_off);
    put_be32(header + 0x424, fst_off);
    put_be32(header + 0x428, (uint32_t)sizeof fst);

    // A zeroed executable header (no section offsets or sizes, so main.dol is
    // just these 0x100 bytes) with content in the gap between the two tables.
    uint8_t dol[0x100];
    memset(dol, 0, sizeof dol);
    memset(dol + 0x48, 0x2A, 0x90 - 0x48);
    sha1_hex(dol, sizeof dol, sha);

    FILE* f = fopen(path, "wb");
    assert(f != NULL);
    assert(fwrite(header, 1, sizeof header, f) == sizeof header);
    assert(fwrite(dol, 1, sizeof dol, f) == sizeof dol);
    assert(fwrite(fst, 1, sizeof fst, f) == sizeof fst);
    assert(fwrite(arc, 1, arc_size, f) == arc_size);
    assert(fclose(f) == 0);
}

typedef struct {
    int reports;
    int cancel_at;      // -1: never; N: once N progress reports have arrived
    double last_fraction;
    const char* out;
    int files_at_cancel;
} Watch;

static void progress_cb(void* context, double fraction, const char* stage) {
    (void)stage;
    Watch* watch = (Watch*)context;
    ++watch->reports;
    watch->last_fraction = fraction;
}

// What the player's Cancel looks like to the importer. It records how many
// modules were already unpacked at the moment it answered, so the test can prove
// the import was interrupted in the middle of writing rather than before it.
static int cancel_cb(void* context) {
    Watch* watch = (Watch*)context;
    if (watch->cancel_at < 0 || watch->reports < watch->cancel_at) return 0;
    if (watch->files_at_cancel == 0) {
        char rels[1024];
        snprintf(rels, sizeof rels, "%s/.import/rels", watch->out);
        const int written = count_files(rels);
        watch->files_at_cancel = written > 0 ? written : -1;
    }
    return 1;
}

int main(void) {
    char root[] = "/tmp/bluewake-cancel-test-XXXXXX";
    assert(mkdtemp(root) != NULL);
    char iso[4096], out[4096], second[4096], third[4096], path[4096];
    char error[1024];
    snprintf(iso, sizeof iso, "%s/GZLE01.iso", root);
    size_t arc_size = 0;
    uint8_t* arc = build_arc(&arc_size);
    char sha[41];
    write_image(iso, arc, arc_size, sha);
    free(arc);
    // The synthetic executable is not USA rev 0's, which is the importer's
    // documented door for a disc a mod tool patched.
    setenv("BLUEWAKE_ACCEPT_DOL_SHA1", sha, 1);

    // 1. No cancel: the fixture really imports, so the cases below are about the
    // hook and not about a disc the importer would have refused anyway.
    snprintf(out, sizeof out, "%s/out", root);
    assert(mkdir(out, 0755) == 0);
    Watch watch = {-1, -1, 0.0, out, 0};
    error[0] = '\0';
    int rc = bluewake_disc_prepare(iso, out, progress_cb, &watch, NULL, NULL,
                                   error, sizeof error);
    if (rc != 0) fprintf(stderr, "the fixture did not import: %d %s\n", rc, error);
    assert(rc == 0);
    snprintf(path, sizeof path, "%s/main.dol", out);
    assert(exists(path));
    snprintf(path, sizeof path, "%s/rels", out);
    assert(count_files(path) == kMembers);
    snprintf(path, sizeof path, "%s/.import", out);
    assert(!exists(path));
    assert(watch.last_fraction == 1.0);

    // 2. Cancel in the middle of the archive: what was unpacked so far is gone,
    // no output remains, and the importer reports a cancel rather than a broken
    // disc (so the window can go back to the form with no failure sentence).
    snprintf(second, sizeof second, "%s/second", root);
    assert(mkdir(second, 0755) == 0);
    Watch midway = {0, 12, 0.0, second, 0};
    error[0] = '\0';
    assert(bluewake_disc_prepare(iso, second, progress_cb, &midway, cancel_cb,
                                 &midway, error, sizeof error) == 1);
    assert(error[0] == '\0');
    assert(midway.files_at_cancel > 100);
    assert(midway.files_at_cancel < kMembers);
    assert(midway.last_fraction > 0.3 && midway.last_fraction < 1.0);
    snprintf(path, sizeof path, "%s/main.dol", second);
    assert(!exists(path));
    snprintf(path, sizeof path, "%s/rels", second);
    assert(!exists(path));
    snprintf(path, sizeof path, "%s/.import", second);
    assert(!exists(path));
    assert(count_files(second) == 0);

    // 3. Cancel before the first file is written: same result, nothing touched.
    snprintf(third, sizeof third, "%s/third", root);
    assert(mkdir(third, 0755) == 0);
    Watch early = {0, 1, 0.0, third, 0};
    error[0] = '\0';
    assert(bluewake_disc_prepare(iso, third, progress_cb, &early, cancel_cb,
                                 &early, error, sizeof error) == 1);
    assert(error[0] == '\0');
    assert(count_files(third) == 0);

    printf("disc_import_cancel_test: 3 cases pass (%d modules in the fixture, "
           "%d unpacked before the mid-import cancel was honoured)\n",
           kMembers, midway.files_at_cancel);
    return 0;
}
