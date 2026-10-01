#include "save_state.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

static void write_bytes(const char* path, const void* bytes, size_t size) {
    gzFile file = gzopen(path, "wb1");
    assert(file != NULL);
    assert(gzwrite(file, bytes, (unsigned)size) == (int)size);
    assert(gzclose(file) == Z_OK);
}

int main(void) {
    char path[256];
    snprintf(path, sizeof path, "bluewake-state-test-%lu.bwstate", (unsigned long)getpid());
    uint32_t a = 42, b = 17;
    BwStateField fields[] = {{"a", &a, sizeof a}, {"b", &b, sizeof b}};
    uint8_t* blob;
    uint64_t size;
    assert(bw_state_fields_pack(fields, 2, &blob, &size));
    assert(bw_state_fields_valid(blob, size));
    a = 0; b = 0;
    assert(bw_state_fields_unpack(fields, 2, blob, size, NULL, NULL, NULL));
    assert(a == 42 && b == 17);
    // A valid first field followed by a truncated second must change nothing.
    a = 9; b = 8;
    assert(!bw_state_fields_unpack(fields, 2, blob, size - 1, NULL, NULL, NULL));
    assert(a == 9 && b == 8);
    assert(!bw_state_fields_valid(NULL, 4));
    assert(!bw_state_fields_valid(blob, 3));
    BwStateWriter* writer = bw_state_writer_open(path);
    assert(writer != NULL);
    assert(bw_state_write_chunk(writer, "FIELDS", blob, size));
    assert(bw_state_writer_close(writer));
    writer = bw_state_writer_open(path);
    assert(writer != NULL);
    assert(!bw_state_write_chunk(writer, "INVALID", NULL, 1));
    assert(!bw_state_writer_close(writer));
    BwStateReader reader;
    assert(bw_state_reader_open(&reader, path));
    const BwStateChunk* chunk = bw_state_find(&reader, "FIELDS");
    assert(chunk != NULL && chunk->size == size);
    assert(memcmp(chunk->data, blob, size) == 0);
    assert(bw_state_find(&reader, "MISSING") == NULL);
    // The parser rejects missing END, unsupported versions and trailing bytes.
    const size_t raw_size = reader.buffer_size;
    uint8_t* raw = malloc(raw_size + 1);
    assert(raw != NULL);
    memcpy(raw, reader.buffer, raw_size);
    bw_state_reader_close(&reader);
    write_bytes(path, raw, raw_size - 1);
    assert(!bw_state_reader_open(&reader, path));
    raw[8] = BW_STATE_VERSION + 1;
    write_bytes(path, raw, raw_size);
    assert(!bw_state_reader_open(&reader, path));
    raw[8] = BW_STATE_VERSION;
    raw[raw_size] = 1;
    write_bytes(path, raw, raw_size + 1);
    assert(!bw_state_reader_open(&reader, path));
    free(raw);
    writer = bw_state_writer_open(path);
    assert(bw_state_write_chunk(writer, "FIELDS", blob, size));
    assert(bw_state_write_chunk(writer, "FIELDS", blob, size));
    assert(bw_state_writer_close(writer));
    assert(!bw_state_reader_open(&reader, path));
    writer = bw_state_writer_open(path);
    assert(bw_state_write_chunk(writer, "FIELDS", blob, size));
    assert(bw_state_writer_close(writer));
    FILE* compressed = fopen(path, "rb+");
    assert(compressed != NULL);
    assert(fseek(compressed, 0, SEEK_END) == 0);
    const long compressed_size = ftell(compressed);
    assert(compressed_size > 8);
#ifdef _WIN32
    assert(_chsize_s(_fileno(compressed), compressed_size - 8) == 0);
#else
    assert(ftruncate(fileno(compressed), compressed_size - 8) == 0);
#endif
    fclose(compressed);
    assert(!bw_state_reader_open(&reader, path));
    free(blob);
    unlink(path);
    return 0;
}
