#include "save_state.h"
#include "atomic_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

struct BwStateWriter {
    gzFile file;
    char* path;
    char* pending;
    bool ok;
};

static bool gz_write_all(BwStateWriter* writer, const void* data, uint64_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    while (writer->ok && size != 0u) {
        const unsigned piece = size > (1u << 30) ? (1u << 30) : (unsigned)size;
        if (gzwrite(writer->file, bytes, piece) != (int)piece)
            writer->ok = false;
        bytes += piece;
        size -= piece;
    }
    return writer->ok;
}

static void put_le64(uint8_t* out, uint64_t value) {
    for (unsigned i = 0; i < 8u; ++i)
        out[i] = (uint8_t)(value >> (8u * i));
}

static uint64_t get_le64(const uint8_t* in) {
    uint64_t value = 0u;
    for (unsigned i = 0; i < 8u; ++i)
        value |= (uint64_t)in[i] << (8u * i);
    return value;
}

static void put_le32(uint8_t* out, uint32_t value) {
    for (unsigned i = 0; i < 4u; ++i)
        out[i] = (uint8_t)(value >> (8u * i));
}

static uint32_t get_le32(const uint8_t* in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) |
           ((uint32_t)in[3] << 24);
}

BwStateWriter* bw_state_writer_open(const char* path) {
    if (path == NULL || path[0] == '\0')
        return NULL;
    BwStateWriter* writer = (BwStateWriter*)calloc(1u, sizeof(*writer));
    if (writer == NULL)
        return NULL;
    writer->path = strdup(path);
    writer->pending = bw_atomic_path(path);
    writer->file = writer->path && writer->pending ? gzopen(writer->pending, "wb1") : NULL;
    if (writer->file == NULL) {
        free(writer->path);
        free(writer->pending);
        free(writer);
        return NULL;
    }
    // A bigger buffer than zlib's 8 KiB default: fewer, larger deflate calls.
    gzbuffer(writer->file, 1u << 20);
    writer->ok = true;
    uint8_t header[12];
    memcpy(header, BW_STATE_MAGIC, 8u);
    put_le32(header + 8u, BW_STATE_VERSION);
    gz_write_all(writer, header, sizeof header);
    return writer;
}

bool bw_state_write_chunk(BwStateWriter* writer, const char* tag,
                          const void* data, uint64_t size) {
    if (writer == NULL) return false;
    if (tag == NULL || (data == NULL && size != 0u)) {
        writer->ok = false;
        return false;
    }
    uint8_t header[BW_STATE_TAG_LEN + 8u];
    memset(header, 0, sizeof header);
    strncpy((char*)header, tag, BW_STATE_TAG_LEN);
    put_le64(header + BW_STATE_TAG_LEN, size);
    gz_write_all(writer, header, sizeof header);
    if (size != 0u)
        gz_write_all(writer, data, size);
    return writer->ok;
}

bool bw_state_writer_close(BwStateWriter* writer) {
    if (writer == NULL)
        return false;
    bw_state_write_chunk(writer, "END", NULL, 0u);
    bool ok = writer->ok;
    if (gzclose(writer->file) != Z_OK)
        ok = false;
    if (ok) {
        FILE* file = fopen(writer->pending, "rb");
        ok = file != NULL && bw_atomic_finish(file, writer->pending, writer->path, true);
    }
    if (!ok) remove(writer->pending);
    free(writer->path);
    free(writer->pending);
    free(writer);
    return ok;
}

bool bw_state_reader_open(BwStateReader* reader, const char* path) {
    if (reader == NULL || path == NULL || path[0] == '\0')
        return false;
    memset(reader, 0, sizeof(*reader));
    gzFile file = gzopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "[state] cannot open %s\n", path);
        return false;
    }
    gzbuffer(file, 1u << 20);
    size_t capacity = 64u << 20;
    size_t size = 0u;
    uint8_t* buffer = (uint8_t*)malloc(capacity);
    for (;;) {
        if (buffer == NULL) {
            gzclose(file);
            fprintf(stderr, "[state] out of memory reading %s\n", path);
            return false;
        }
        if (size == capacity) {
            if (capacity >= BW_STATE_MAX_BYTES) {
                fprintf(stderr, "[state] %s exceeds the state size limit\n", path);
                free(buffer);
                gzclose(file);
                return false;
            }
            capacity *= 2u;
            uint8_t* grown = (uint8_t*)realloc(buffer, capacity);
            if (grown == NULL)
                free(buffer);
            buffer = grown;
            continue;
        }
        const size_t want = capacity - size > (1u << 30) ? (1u << 30) : capacity - size;
        const int got = gzread(file, buffer + size, (unsigned)want);
        if (got < 0) {
            int error = 0;
            fprintf(stderr, "[state] %s: %s\n", path, gzerror(file, &error));
            free(buffer);
            gzclose(file);
            return false;
        }
        if (got == 0)
            break;
        size += (size_t)got;
    }
    int read_error = Z_OK;
    (void)gzerror(file, &read_error);
    if (read_error != Z_OK && read_error != Z_STREAM_END) {
        fprintf(stderr, "[state] %s: incomplete compressed stream\n", path);
        free(buffer);
        gzclose(file);
        return false;
    }
    gzclose(file);
    if (size < 12u || memcmp(buffer, BW_STATE_MAGIC, 8u) != 0) {
        fprintf(stderr, "[state] %s is not a BlueWake state\n", path);
        free(buffer);
        return false;
    }
    reader->version = get_le32(buffer + 8u);
    if (reader->version != BW_STATE_VERSION) {
        fprintf(stderr, "[state] %s is version %u, this host reads %u\n", path,
                reader->version, BW_STATE_VERSION);
        free(buffer);
        return false;
    }
    reader->buffer = buffer;
    reader->buffer_size = size;
    size_t offset = 12u;
    uint32_t chunk_capacity = 32u;
    reader->chunks = (BwStateChunk*)calloc(chunk_capacity, sizeof(BwStateChunk));
    bool ended = false;
    while (reader->chunks != NULL && offset + BW_STATE_TAG_LEN + 8u <= size) {
        BwStateChunk chunk;
        memset(&chunk, 0, sizeof chunk);
        memcpy(chunk.tag, buffer + offset, BW_STATE_TAG_LEN);
        chunk.size = get_le64(buffer + offset + BW_STATE_TAG_LEN);
        offset += BW_STATE_TAG_LEN + 8u;
        if (chunk.size > size - offset)
            break;
        chunk.data = buffer + offset;
        offset += (size_t)chunk.size;
        if (strcmp(chunk.tag, "END") == 0) {
            ended = chunk.size == 0u && offset == size;
            break;
        }
        if (chunk.tag[0] == '\0' || reader->chunk_count >= BW_STATE_MAX_CHUNKS ||
            bw_state_find(reader, chunk.tag) != NULL)
            break;
        if (reader->chunk_count == chunk_capacity) {
            chunk_capacity *= 2u;
            BwStateChunk* grown = (BwStateChunk*)realloc(
                reader->chunks, chunk_capacity * sizeof(BwStateChunk));
            if (grown == NULL)
                break;
            reader->chunks = grown;
        }
        reader->chunks[reader->chunk_count++] = chunk;
    }
    if (!ended) {
        fprintf(stderr, "[state] %s is truncated\n", path);
        bw_state_reader_close(reader);
        return false;
    }
    return true;
}

const BwStateChunk* bw_state_find(const BwStateReader* reader, const char* tag) {
    for (uint32_t i = 0; reader != NULL && i < reader->chunk_count; ++i) {
        if (strncmp(reader->chunks[i].tag, tag, BW_STATE_TAG_LEN) == 0)
            return &reader->chunks[i];
    }
    return NULL;
}

void bw_state_reader_close(BwStateReader* reader) {
    free(reader->chunks);
    free(reader->buffer);
    memset(reader, 0, sizeof(*reader));
}

bool bw_state_fields_pack(const BwStateField* fields, uint32_t count,
                          uint8_t** out, uint64_t* out_size) {
    uint64_t total = 4u;
    for (uint32_t i = 0; i < count; ++i) {
        if (fields[i].name == NULL || strlen(fields[i].name) > UINT16_MAX ||
            (fields[i].data == NULL && fields[i].size != 0u))
            return false;
        total += 2u + strlen(fields[i].name) + 4u + fields[i].size;
        if (total > BW_STATE_MAX_BYTES)
            return false;
    }
    uint8_t* blob = (uint8_t*)malloc((size_t)total);
    if (blob == NULL)
        return false;
    uint8_t* at = blob;
    put_le32(at, count);
    at += 4u;
    for (uint32_t i = 0; i < count; ++i) {
        const size_t name_len = strlen(fields[i].name);
        at[0] = (uint8_t)name_len;
        at[1] = (uint8_t)(name_len >> 8);
        at += 2u;
        memcpy(at, fields[i].name, name_len);
        at += name_len;
        put_le32(at, fields[i].size);
        at += 4u;
        memcpy(at, fields[i].data, fields[i].size);
        at += fields[i].size;
    }
    *out = blob;
    *out_size = total;
    return true;
}

bool bw_state_fields_valid(const uint8_t* blob, uint64_t blob_size) {
    if (blob == NULL || blob_size < 4u || blob_size > BW_STATE_MAX_BYTES)
        return false;
    const uint32_t records = get_le32(blob);
    uint64_t offset = 4u;
    for (uint32_t r = 0; r < records; ++r) {
        if (blob_size - offset < 2u)
            return false;
        const uint32_t length = (uint32_t)blob[offset] | ((uint32_t)blob[offset + 1u] << 8);
        offset += 2u;
        if (length == 0u || blob_size - offset < (uint64_t)length + 4u)
            return false;
        offset += length;
        const uint32_t size = get_le32(blob + offset);
        offset += 4u;
        if (size > blob_size - offset)
            return false;
        offset += size;
    }
    return offset == blob_size;
}

bool bw_state_fields_unpack(const BwStateField* fields, uint32_t count,
                            const uint8_t* blob, uint64_t blob_size,
                            uint32_t* restored, uint32_t* missing,
                            uint32_t* mismatched) {
    if (!bw_state_fields_valid(blob, blob_size))
        return false;
    uint32_t n_restored = 0u, n_mismatched = 0u;
    bool* seen = (bool*)calloc(count != 0u ? count : 1u, sizeof(bool));
    if (seen == NULL || blob_size < 4u) {
        free(seen);
        return false;
    }
    const uint32_t records = get_le32(blob);
    uint64_t offset = 4u;
    bool ok = true;
    for (uint32_t r = 0; r < records; ++r) {
        if (offset + 2u > blob_size) {
            ok = false;
            break;
        }
        const uint32_t name_len = (uint32_t)blob[offset] | ((uint32_t)blob[offset + 1u] << 8);
        offset += 2u;
        if (offset + name_len + 4u > blob_size) {
            ok = false;
            break;
        }
        const char* name = (const char*)blob + offset;
        offset += name_len;
        const uint32_t size = get_le32(blob + offset);
        offset += 4u;
        if (offset + size > blob_size) {
            ok = false;
            break;
        }
        for (uint32_t i = 0; i < count; ++i) {
            if (strlen(fields[i].name) != name_len ||
                memcmp(fields[i].name, name, name_len) != 0)
                continue;
            seen[i] = true;
            if (fields[i].size == size) {
                memcpy(fields[i].data, blob + offset, size);
                n_restored++;
            } else {
                fprintf(stderr, "[state] field %s is %u bytes here, %u in the state: kept\n",
                        fields[i].name, fields[i].size, size);
                n_mismatched++;
            }
            break;
        }
        offset += size;
    }
    uint32_t n_missing = 0u;
    for (uint32_t i = 0; i < count; ++i) {
        if (!seen[i]) {
            n_missing++;
            fprintf(stderr, "[state] field %s is not in the state: kept\n", fields[i].name);
        }
    }
    free(seen);
    if (restored != NULL)
        *restored = n_restored;
    if (missing != NULL)
        *missing = n_missing;
    if (mismatched != NULL)
        *mismatched = n_mismatched;
    return ok;
}

uint64_t bw_state_hash(const void* data, size_t size, uint64_t seed) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = seed != 0u ? seed : 1469598103934665603ull;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
    return hash;
}
