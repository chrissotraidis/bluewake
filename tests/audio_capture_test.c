#ifdef NDEBUG
#undef NDEBUG
#endif
#include "audio_capture.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static u32 read_u32_le(const u8* data) {
    return (u32)data[0] | ((u32)data[1] << 8) | ((u32)data[2] << 16) |
           ((u32)data[3] << 24);
}

int main(void) {
    char path[] = "/tmp/bluewake-audio-capture-XXXXXX";
    const int fd = mkstemp(path);
    assert(fd >= 0);
    close(fd);

    const u8 source[] = {
        0x00, 0x01, 0xFF, 0xFE, 0x7F, 0xFF, 0x80, 0x00,
        0x00, 0x00, 0x12, 0x34, 0xED, 0xCC, 0x00, 0x00,
    };
    const u8 expected[] = {
        0x01, 0x00, 0xFE, 0xFF, 0xFF, 0x7F, 0x00, 0x80,
        0x00, 0x00, 0x34, 0x12, 0xCC, 0xED, 0x00, 0x00,
    };

    BluewakeAudioCapture capture;
    bluewake_audio_capture_init(&capture, path);
    assert(bluewake_audio_capture_append_be16_stereo(
        &capture, source, sizeof(source), 32000u));
    assert(capture.frames == 4u);
    assert(capture.nonzero_samples == 6u);
    assert(capture.peak_sample == 32768);
    assert(bluewake_audio_capture_close(&capture));

    u8 file_data[60];
    FILE* file = fopen(path, "rb");
    assert(file != NULL);
    assert(fread(file_data, 1u, sizeof(file_data), file) == sizeof(file_data));
    assert(fgetc(file) == EOF);
    assert(fclose(file) == 0);
    assert(memcmp(file_data, "RIFF", 4u) == 0);
    assert(read_u32_le(file_data + 4u) == 52u);
    assert(memcmp(file_data + 8u, "WAVEfmt ", 8u) == 0);
    assert(read_u32_le(file_data + 24u) == 32000u);
    assert(read_u32_le(file_data + 28u) == 128000u);
    assert(memcmp(file_data + 36u, "data", 4u) == 0);
    assert(read_u32_le(file_data + 40u) == sizeof(expected));
    assert(memcmp(file_data + 44u, expected, sizeof(expected)) == 0);
    assert(unlink(path) == 0);
    return 0;
}
