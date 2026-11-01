// See disc_import.h.
#include "disc_import.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#elif defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#endif
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if !defined(_WIN32)
#include <sys/statvfs.h>
#endif

#define BW_GC_MAGIC 0xC2339F3Du
// main.dol of GZLE01 USA rev 0, as checked by scripts/prepare.py.
static const char* const kExpectedDolSha1 =
    "8d28bab68bb5078c38e43f29206f0bd01f7e7a67";

static int fail(char* error, size_t size, const char* fmt, ...) {
    if (error != NULL && size > 0) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(error, size, fmt, ap);
        va_end(ap);
    }
    return -1;
}

// Free space in bytes on the filesystem holding `path`, or -1 when the platform
// cannot say (the caller then makes no claim instead of refusing the import).
static int64_t free_bytes(const char* path) {
#if defined(_WIN32)
    ULARGE_INTEGER caller_free;
    if (GetDiskFreeSpaceExA(path, &caller_free, NULL, NULL) == 0)
        return -1;
    return (int64_t)caller_free.QuadPart;
#else
    struct statvfs vfs;
    if (statvfs(path, &vfs) != 0)
        return -1;
    return (int64_t)vfs.f_bavail * (int64_t)vfs.f_frsize;
#endif
}

// "10 MB", "1.4 GB": a size the player can compare with what their file manager
// shows for the folder.
static void format_size(int64_t bytes, char* out, size_t size) {
    if (bytes >= ((int64_t)1 << 30))
        snprintf(out, size, "%.1f GB",
                 (double)bytes / (double)((int64_t)1 << 30));
    else if (bytes >= ((int64_t)1 << 20))
        snprintf(out, size, "%.0f MB",
                 (double)bytes / (double)((int64_t)1 << 20));
    else
        snprintf(out, size, "%lld KB", (long long)((bytes + 1023) / 1024));
}

int bluewake_disc_space_verdict(const char* out_dir, int64_t needed,
                               int64_t available, char* error,
                               size_t error_size) {
    if (available < 0 || available >= needed)
        return 0;
    char need_text[32], free_text[32];
    format_size(needed, need_text, sizeof need_text);
    format_size(available, free_text, sizeof free_text);
    return fail(error, error_size,
                "BlueWake needs about %s of free space in %s to unpack the "
                "game's files, and only %s is free. Free some space there, or "
                "give BlueWake a folder on another drive.",
                need_text, out_dir, free_text);
}

// The free-space check an import makes before it writes anything.
static int check_space(const char* out_dir, int64_t needed, char* error,
                       size_t error_size) {
    return bluewake_disc_space_verdict(out_dir, needed, free_bytes(out_dir),
                                       error, error_size);
}

// A write that failed, naming the folder as well as the file: the bare file
// name read as a damaged disc in the report this fixes (#479).
static int fail_write(const char* dir, const char* name, int err, char* error,
                      size_t error_size) {
    if (err == ENOSPC) {
        char free_text[32];
        const int64_t left = free_bytes(dir);
        if (left >= 0) {
            format_size(left, free_text, sizeof free_text);
            return fail(error, error_size,
                        "Could not write %s into %s: the folder is full (%s "
                        "free).",
                        name, dir, free_text);
        }
        return fail(error, error_size,
                    "Could not write %s into %s: the folder is full.", name, dir);
    }
    return fail(error, error_size, "Could not write %s into %s (%s).", name, dir,
                err != 0 ? strerror(err) : "write failed");
}

static uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint8_t* read_at(FILE* f, uint64_t offset, size_t size) {
    uint8_t* buffer = (uint8_t*)malloc(size > 0 ? size : 1);
    if (buffer == NULL)
        return NULL;
    if (fseeko(f, (off_t)offset, SEEK_SET) != 0 ||
        fread(buffer, 1, size, f) != size) {
        free(buffer);
        return NULL;
    }
    return buffer;
}

// How many bytes one disc member occupies once written: a Yaz0 header's total,
// else its stored size. Reads only the eight bytes it needs.
static int64_t unpacked_size(const uint8_t* data, size_t size) {
    if (size >= 8 && memcmp(data, "Yaz0", 4) == 0) {
        const uint32_t total = be32(data + 4);
        if (total > 0)
            return (int64_t)total;
    }
    return (int64_t)size;
}

// The same for a member still on disc, which is read through `f` at `off`.
static int64_t unpacked_size_at(FILE* f, uint32_t off, uint32_t size) {
    uint8_t header[8];
    if (size >= 8 && fseeko(f, (off_t)off, SEEK_SET) == 0 &&
        fread(header, 1, 8, f) == 8)
        return unpacked_size(header, sizeof header);
    return (int64_t)size;
}

// Yaz0: returns a malloc'd buffer of *out_size bytes, or NULL if malformed.
static uint8_t* yaz0_decode(const uint8_t* in, size_t in_size,
                            size_t* out_size) {
    if (in_size < 16 || memcmp(in, "Yaz0", 4) != 0)
        return NULL;
    const size_t total = be32(in + 4);
    if (total == 0 || total > (64u << 20))
        return NULL;
    uint8_t* out = (uint8_t*)malloc(total);
    if (out == NULL)
        return NULL;
    size_t s = 16, d = 0;
    while (d < total) {
        if (s >= in_size)
            goto bad;
        uint8_t code = in[s++];
        for (int bit = 0; bit < 8 && d < total; ++bit, code <<= 1) {
            if (code & 0x80) {
                if (s >= in_size)
                    goto bad;
                out[d++] = in[s++];
                continue;
            }
            if (s + 1 >= in_size)
                goto bad;
            const uint8_t b1 = in[s], b2 = in[s + 1];
            s += 2;
            const size_t dist = ((size_t)(b1 & 0x0F) << 8 | b2) + 1;
            size_t count = b1 >> 4;
            if (count == 0) {
                if (s >= in_size)
                    goto bad;
                count = (size_t)in[s++] + 0x12;
            } else {
                count += 2;
            }
            if (dist > d || d + count > total)
                goto bad;
            for (size_t i = 0; i < count; ++i, ++d)
                out[d] = out[d - dist];
        }
    }
    *out_size = total;
    return out;
bad:
    free(out);
    return NULL;
}

#if !defined(_WIN32) && !defined(__APPLE__)
// Self-contained SHA-1 (FIPS 180-1), so the Linux build needs no crypto
// library for the one hash this importer computes (main.dol's revision-0
// SHA-1). macOS uses CommonCrypto above; Windows uses BCrypt above.
static uint32_t rol32(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

static void sha1_transform(uint32_t state[5], const uint8_t block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)block[i * 4] << 24) | ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) | (uint32_t)block[i * 4 + 3];
    for (int i = 16; i < 80; i++)
        w[i] = rol32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999u;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }
        uint32_t t = rol32(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rol32(b, 30);
        b = a;
        a = t;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void sha1(const uint8_t* data, size_t size, uint8_t digest[20]) {
    uint32_t state[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
    uint64_t bitlen = (uint64_t)size * 8u;
    uint8_t block[64];
    size_t i = 0;
    while (size - i >= 64) {
        sha1_transform(state, data + i);
        i += 64;
    }
    memset(block, 0, sizeof block);
    size_t rem = size - i;
    memcpy(block, data + i, rem);
    block[rem] = 0x80u;
    if (rem >= 56) {
        sha1_transform(state, block);
        memset(block, 0, sizeof block);
    }
    for (int j = 0; j < 8; j++)
        block[56 + j] = (uint8_t)(bitlen >> (56 - j * 8));
    sha1_transform(state, block);
    for (int j = 0; j < 5; j++) {
        digest[j * 4] = (uint8_t)(state[j] >> 24);
        digest[j * 4 + 1] = (uint8_t)(state[j] >> 16);
        digest[j * 4 + 2] = (uint8_t)(state[j] >> 8);
        digest[j * 4 + 3] = (uint8_t)state[j];
    }
}
#endif

static void sha1_hex(const uint8_t* data, size_t size, char out[41]) {
#if defined(_WIN32)
    uint8_t digest[20];
    if (!BCRYPT_SUCCESS(BCryptHash(BCRYPT_SHA1_ALG_HANDLE, NULL, 0, (PUCHAR)data,
                                   (ULONG)size, digest, (ULONG)sizeof digest)))
        memset(digest, 0, sizeof digest);
    for (int i = 0; i < (int)sizeof digest; ++i)
        snprintf(out + i * 2, 3, "%02x", digest[i]);
#elif defined(__APPLE__)
    uint8_t digest[CC_SHA1_DIGEST_LENGTH];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CC_SHA1(data, (CC_LONG)size, digest);
#pragma clang diagnostic pop
    for (int i = 0; i < CC_SHA1_DIGEST_LENGTH; ++i)
        snprintf(out + i * 2, 3, "%02x", digest[i]);
#else
    uint8_t digest[20];
    sha1(data, size, digest);
    for (int i = 0; i < 20; ++i)
        snprintf(out + i * 2, 3, "%02x", digest[i]);
#endif
}

static int write_file(const char* path, const uint8_t* data, size_t size) {
    FILE* f = fopen(path, "wb");
    if (f == NULL)
        return -1;
    const int ok = fwrite(data, 1, size, f) == size;
    return (fclose(f) == 0 && ok) ? 0 : -1;
}

// The prepared RELs carry alignment 4 in the header word at 0x40 where the
// disc has 8 or 0x20 (docs/archive/status/GATES.md: "REL alignment field patched from
// 8 to 4 in extracted copies"); the composite was translated from those
// copies, all 415 of which hold 4.
static int write_rel(const char* dir, const char* name, uint8_t* data,
                     size_t size) {
    if (size >= 0x44) {
        data[0x40] = 0;
        data[0x41] = 0;
        data[0x42] = 0;
        data[0x43] = 4;
    }
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    return write_file(path, data, size);
}

static int has_rel_suffix(const char* name) {
    const size_t n = strlen(name);
    return n > 4 && strcmp(name + n - 4, ".rel") == 0;
}

// Removes a flat directory of files, then the directory.
static void remove_flat_dir(const char* path) {
    DIR* dir = opendir(path);
    if (dir != NULL) {
        struct dirent* e;
        while ((e = readdir(dir)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
                continue;
            char child[1024];
            snprintf(child, sizeof(child), "%s/%s", path, e->d_name);
            unlink(child);
        }
        closedir(dir);
    }
    rmdir(path);
}

int bluewake_disc_check(const char* iso_path, char* error, size_t error_size) {
    FILE* f = fopen(iso_path, "rb");
    if (f == NULL)
        return fail(error, error_size, "The disc image could not be opened.");
    uint8_t* h = read_at(f, 0, 0x440);
    fclose(f);
    if (h == NULL)
        return fail(error, error_size,
                    "This file is too small to be a GameCube disc image.");
    int rc = 0;
    if (be32(h + 0x1C) != BW_GC_MAGIC)
        rc = fail(error, error_size,
                  "This file is not a GameCube disc image. BlueWake needs an "
                  "uncompressed .iso or .gcm dump of your disc.");
    else if (memcmp(h, "GZLE01", 6) != 0)
        rc = fail(error, error_size,
                  "This is a GameCube disc, but not The Wind Waker (USA, "
                  "GZLE01). Its id is %.6s.", (const char*)h);
    free(h);
    return rc;
}

// Walks the .rel members of RELS.arc (a RARC archive, possibly inside a Yaz0
// wrapper, whose members are Yaz0 RELs). With `out` it writes each one into that
// directory and adds the bytes written to *bytes; with `out` NULL it only adds
// the bytes they would take, which is how the importer knows how much room the
// unpacking needs before it writes anything. Returns the member count or -1.
static int unpack_rels_arc(uint8_t* arc, size_t arc_size, const char* out,
                           int64_t* bytes, BlueWakeImportProgress progress,
                           void* context, char* error, size_t error_size) {
    if (arc_size < 0x40 || memcmp(arc, "RARC", 4) != 0)
        return -1;
    const size_t data_start = (size_t)be32(arc + 0x0C) + 0x20;
    const uint8_t* info = arc + 0x20;
    const uint32_t nfiles = be32(info + 8);
    const size_t file_off = (size_t)be32(info + 12) + 0x20;
    const size_t str_off = (size_t)be32(info + 20) + 0x20;
    if (file_off + (size_t)nfiles * 0x14 > arc_size || str_off >= arc_size)
        return -1;
    int found = 0;
    for (uint32_t i = 0; i < nfiles; ++i) {
        const uint8_t* e = arc + file_off + (size_t)i * 0x14;
        const uint32_t type_name = be32(e + 4);
        if (((type_name >> 24) & 1u) == 0)
            continue;
        const size_t name_off = str_off + (type_name & 0xFFFFu);
        if (name_off >= arc_size ||
            memchr(arc + name_off, 0, arc_size - name_off) == NULL)
            return -1;
        const char* name = (const char*)arc + name_off;
        if (!has_rel_suffix(name))
            continue;
        const size_t doff = data_start + be32(e + 8), dlen = be32(e + 12);
        if (doff + dlen > arc_size)
            return -1;
        *bytes += unpacked_size(arc + doff, dlen);
        ++found;
        if (out == NULL)
            continue;
        uint8_t* data;
        size_t data_size = dlen;
        if (dlen >= 4 && memcmp(arc + doff, "Yaz0", 4) == 0) {
            data = yaz0_decode(arc + doff, dlen, &data_size);
        } else {
            data = (uint8_t*)malloc(dlen > 0 ? dlen : 1);
            if (data != NULL)
                memcpy(data, arc + doff, dlen);
        }
        if (data == NULL) {
            fail(error, error_size, "The disc's %s could not be unpacked.",
                 name);
            return -1;
        }
        if (write_rel(out, name, data, data_size) != 0) {
            const int err = errno;
            free(data);
            return fail_write(out, name, err, error, error_size);
        }
        free(data);
        if (progress != NULL && (i & 31u) == 0)
            progress(context, 0.3 + 0.65 * (double)i / nfiles,
                     "Unpacking game modules");
    }
    return found;
}

// One walk of the disc's file table. With `out` it writes the loose rels/*.rel
// entries as it goes; with `out` NULL it only adds what they would take.
// *arc_off and *arc_len receive RELS.arc's location. Returns 0, or -1 with a
// sentence in `error` (a damaged file table, or a write that failed).
static int scan_rel_entries(FILE* f, const uint8_t* fst, uint32_t fst_size,
                            const char* out, int64_t* need, int* written,
                            uint32_t* arc_off, uint32_t* arc_len, char* error,
                            size_t error_size) {
    // Directory entries hold the index one past their last child.
    const uint32_t count = be32(fst + 8);
    const uint32_t strings = count * 12;
    uint32_t dir_end[16];
    const char* dir_name[16];
    int depth = 0;
    for (uint32_t i = 1; i < count; ++i) {
        while (depth > 0 && i >= dir_end[depth - 1])
            --depth;
        const uint8_t* e = fst + i * 12;
        const uint32_t name_off = strings + (be32(e) & 0xFFFFFFu);
        if (name_off >= fst_size ||
            memchr(fst + name_off, 0, fst_size - name_off) == NULL)
            return fail(error, error_size, "The disc's file table is malformed.");
        const char* name = (const char*)fst + name_off;
        if (e[0] != 0) {
            if (depth < 16) {
                dir_end[depth] = be32(e + 8);
                dir_name[depth] = name;
                ++depth;
            }
            continue;
        }
        const uint32_t off = be32(e + 4), size = be32(e + 8);
        if (depth == 0 && strcmp(name, "RELS.arc") == 0) {
            *arc_off = off;
            *arc_len = size;
            continue;
        }
        if (depth != 1 || strcmp(dir_name[0], "rels") != 0 ||
            !has_rel_suffix(name))
            continue;
        *need += unpacked_size_at(f, off, size);
        if (out == NULL) {
            ++*written;
            continue;
        }
        uint8_t* data = read_at(f, off, size);
        if (data == NULL)
            return fail(error, error_size,
                        "The disc image could not be read at %s; the image "
                        "looks damaged.",
                        name);
        size_t data_size = size;
        if (size >= 4 && memcmp(data, "Yaz0", 4) == 0) {
            uint8_t* plain = yaz0_decode(data, size, &data_size);
            free(data);
            data = plain;
        }
        if (data == NULL)
            return fail(error, error_size,
                        "The disc's %s could not be unpacked.", name);
        if (write_rel(out, name, data, data_size) != 0) {
            const int err = errno;
            free(data);
            return fail_write(out, name, err, error, error_size);
        }
        free(data);
        ++*written;
    }
    return 0;
}

int bluewake_disc_prepare(const char* iso_path, const char* out_dir,
                          BlueWakeImportProgress progress, void* context,
                          char* error, size_t error_size) {
    if (bluewake_disc_check(iso_path, error, error_size) != 0)
        return -1;
    FILE* f = fopen(iso_path, "rb");
    if (f == NULL)
        return fail(error, error_size, "The disc image could not be opened.");
    int rc = -1;
    uint8_t *h = NULL, *dol_hdr = NULL, *dol = NULL, *fst = NULL, *arc = NULL;
    char tmp_dir[1024], rels_tmp[1024], rels_dir[1024], dol_tmp[1024],
        dol_path[1024];
    snprintf(tmp_dir, sizeof(tmp_dir), "%s/.import", out_dir);
    snprintf(rels_tmp, sizeof(rels_tmp), "%s/rels", tmp_dir);
    snprintf(dol_tmp, sizeof(dol_tmp), "%s/main.dol", tmp_dir);
    snprintf(rels_dir, sizeof(rels_dir), "%s/rels", out_dir);
    snprintf(dol_path, sizeof(dol_path), "%s/main.dol", out_dir);

    if (progress != NULL)
        progress(context, 0.0, "Reading the disc");
    h = read_at(f, 0, 0x440);
    if (h == NULL) {
        fail(error, error_size, "The disc header could not be read.");
        goto done;
    }
    const uint32_t dol_off = be32(h + 0x420), fst_off = be32(h + 0x424),
                   fst_size = be32(h + 0x428);

    // main.dol: its header plus the furthest section end.
    dol_hdr = read_at(f, dol_off, 0x100);
    if (dol_hdr == NULL) {
        fail(error, error_size, "The game executable could not be read.");
        goto done;
    }
    uint32_t dol_size = 0x100;
    for (int i = 0; i < 18; ++i) {
        const uint32_t o = be32(dol_hdr + i * 4);
        const uint32_t s = be32(dol_hdr + 0x90 + i * 4);
        if (s != 0 && o + s > dol_size)
            dol_size = o + s;
    }
    if (dol_size > (16u << 20)) {
        fail(error, error_size, "The game executable is malformed.");
        goto done;
    }
    dol = read_at(f, dol_off, dol_size);
    if (dol == NULL) {
        fail(error, error_size, "The game executable could not be read.");
        goto done;
    }
    char sha[41];
    sha1_hex(dol, dol_size, sha);
    // BLUEWAKE_ACCEPT_DOL_SHA1 accepts one other executable, for a disc a mod
    // tool patched from USA revision 0 (Better Wind Waker): its code comes
    // from the composite's variant chunks, built from that same executable.
    const char* also = getenv("BLUEWAKE_ACCEPT_DOL_SHA1");
    if (strcmp(sha, kExpectedDolSha1) != 0 && (also == NULL || strcmp(sha, also) != 0)) {
        fail(error, error_size,
             "This GZLE01 disc is not the revision BlueWake supports (USA "
             "revision 0), or the image is damaged.");
        goto done;
    }

    // File system table: loose rels/*.rel and the RELS.arc archive. It is read
    // and walked once to count before anything is written, so the import can be
    // refused for want of room instead of failing on whichever file came first
    // (#479).
    fst = fst_size >= 12 ? read_at(f, fst_off, fst_size) : NULL;
    const uint32_t count = fst != NULL ? be32(fst + 8) : 0;
    if (fst == NULL || count == 0 || (uint64_t)count * 12 > fst_size) {
        fail(error, error_size, "The disc's file table could not be read.");
        goto done;
    }
    uint32_t arc_off = 0, arc_len = 0;
    int64_t need = (int64_t)dol_size;
    int written = 0;
    if (scan_rel_entries(f, fst, fst_size, NULL, &need, &written, &arc_off,
                         &arc_len, error, error_size) != 0)
        goto done;
    if (arc_len == 0) {
        fail(error, error_size,
             "The disc has no RELS.arc; it is not a complete GZLE01 image.");
        goto done;
    }
    arc = read_at(f, arc_off, arc_len);
    size_t arc_size = arc_len;
    if (arc != NULL && arc_size >= 4 && memcmp(arc, "Yaz0", 4) == 0) {
        uint8_t* plain = yaz0_decode(arc, arc_size, &arc_size);
        free(arc);
        arc = plain;
    }
    int64_t arc_need = 0;
    const int arc_members =
        arc != NULL
            ? unpack_rels_arc(arc, arc_size, NULL, &arc_need, NULL, NULL, error,
                              error_size)
            : -1;
    if (arc_members < 0) {
        fail(error, error_size, "The disc's RELS.arc could not be unpacked.");
        goto done;
    }
    need += arc_need;
    if (check_space(out_dir, need, error, error_size) != 0)
        goto done;

    remove_flat_dir(rels_tmp);
    unlink(dol_tmp);
    rmdir(tmp_dir);
    if (mkdir(tmp_dir, 0755) != 0 || mkdir(rels_tmp, 0755) != 0) {
        fail(error, error_size, "Could not create the data folder (%s).",
             strerror(errno));
        goto done;
    }
    if (write_file(dol_tmp, dol, dol_size) != 0) {
        fail_write(tmp_dir, "main.dol", errno, error, error_size);
        goto done;
    }

    written = 0;
    int64_t wrote = 0;
    if (scan_rel_entries(f, fst, fst_size, rels_tmp, &wrote, &written, &arc_off,
                         &arc_len, error, error_size) != 0)
        goto done;
    if (progress != NULL)
        progress(context, 0.3, "Unpacking game modules");
    int64_t from_arc_bytes = 0;
    const int from_arc = unpack_rels_arc(arc, arc_size, rels_tmp, &from_arc_bytes,
                                         progress, context, error, error_size);
    if (from_arc < 0) {
        if (error != NULL && error_size > 0 && error[0] == '\0')
            fail(error, error_size,
                 "The disc's RELS.arc could not be unpacked.");
        goto done;
    }
    written += from_arc;
    if (written < 400) {
        fail(error, error_size,
             "Only %d game modules were found; the image looks incomplete.",
             written);
        goto done;
    }

    // Swap the finished outputs in.
    remove_flat_dir(rels_dir);
    unlink(dol_path);
    if (rename(rels_tmp, rels_dir) != 0 || rename(dol_tmp, dol_path) != 0) {
        fail(error, error_size,
             "Could not move the prepared files into place (%s).",
             strerror(errno));
        goto done;
    }
    rmdir(tmp_dir);
    if (progress != NULL)
        progress(context, 1.0, "Ready");
    rc = 0;
done:
    if (rc != 0) {
        remove_flat_dir(rels_tmp);
        unlink(dol_tmp);
        rmdir(tmp_dir);
    }
    free(h);
    free(dol_hdr);
    free(dol);
    free(fst);
    free(arc);
    fclose(f);
    return rc;
}
