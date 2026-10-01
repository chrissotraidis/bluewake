#ifndef BLUEWAKE_SAVE_STATE_H
#define BLUEWAKE_SAVE_STATE_H

// Debug save states: the running game written to a file and read back.
//
// A state file is a gzip stream of tagged chunks (BwStateChunk: a 16-byte
// NUL-padded tag, a 64-bit little-endian size, then the bytes), opened by a
// magic and a version and closed by an "END" chunk. Chunks are independent:
// the reader indexes them all and each subsystem asks for its own by tag, so a
// state written with more chunks than a reader knows still loads, and a
// missing chunk is reported by whoever needed it.
//
// Host globals go through a table of named fields (BwStateField): one chunk
// holds (name, size, bytes) records and the reader restores each field whose
// name and size still match, so reordering or adding globals does not break an
// older state; a field whose size changed is skipped and counted.
//
// This module is only the container. What goes into a state, and when it is
// safe to take one, is main.c's business (host_save_state/host_load_state).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BW_STATE_MAGIC "BWSTATE1"
#define BW_STATE_VERSION 1u
#define BW_STATE_TAG_LEN 16u
#define BW_STATE_MAX_BYTES (256u * 1024u * 1024u)
#define BW_STATE_MAX_CHUNKS 4096u

typedef struct BwStateWriter BwStateWriter;

// Opens `path` for writing (gzip level 1: a 32 MiB MEM1 compresses in a few
// hundred milliseconds). NULL on failure.
BwStateWriter* bw_state_writer_open(const char* path);
bool bw_state_write_chunk(BwStateWriter* writer, const char* tag,
                          const void* data, uint64_t size);
// Writes the END chunk and closes the file; false if anything failed.
bool bw_state_writer_close(BwStateWriter* writer);
// Close a producer's attempt. An incomplete attempt never replaces the old file.
bool bw_state_writer_finish(BwStateWriter* writer, bool complete);

typedef struct BwStateChunk {
    char tag[BW_STATE_TAG_LEN + 1u];
    const uint8_t* data;
    uint64_t size;
} BwStateChunk;

typedef struct BwStateReader {
    uint8_t* buffer;      // the whole decompressed file
    size_t buffer_size;
    uint32_t version;
    BwStateChunk* chunks;
    uint32_t chunk_count;
} BwStateReader;

// Reads and indexes the whole file. False (with a message on stderr) on I/O
// errors, a bad magic, an unknown version or a truncated chunk.
bool bw_state_reader_open(BwStateReader* reader, const char* path);
const BwStateChunk* bw_state_find(const BwStateReader* reader, const char* tag);
void bw_state_reader_close(BwStateReader* reader);

// Named host fields.
typedef struct BwStateField {
    const char* name;
    void* data;
    uint32_t size;
} BwStateField;

// Serializes the fields into one malloc'd blob for a chunk.
bool bw_state_fields_pack(const BwStateField* fields, uint32_t count,
                          uint8_t** out, uint64_t* out_size);
// Restores every field in `fields` found in the blob with the same size.
// `restored`, `missing` (in the table, not in the blob) and `mismatched`
// (same name, other size) may be NULL.
bool bw_state_fields_unpack(const BwStateField* fields, uint32_t count,
                            const uint8_t* blob, uint64_t blob_size,
                            uint32_t* restored, uint32_t* missing,
                            uint32_t* mismatched);
// Validate the complete field stream without changing any running state.
bool bw_state_fields_valid(const uint8_t* blob, uint64_t blob_size);

// FNV-1a 64 over bytes, for identities and quick equality checks in logs.
uint64_t bw_state_hash(const void* data, size_t size, uint64_t seed);

// The window's F5 (save) and F9 (load the last state) keys (main.c acts on
// them at the next retrace).
void bluewake_save_state_hotkey(bool load);

#ifdef __cplusplus
}
#endif

#endif
