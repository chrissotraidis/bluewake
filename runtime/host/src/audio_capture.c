#include "audio_capture.h"

#include <string.h>

static bool write_u16_le(FILE* file, u16 value) {
    const u8 bytes[2] = {(u8)value, (u8)(value >> 8)};
    return fwrite(bytes, 1u, sizeof(bytes), file) == sizeof(bytes);
}

static bool write_u32_le(FILE* file, u32 value) {
    const u8 bytes[4] = {
        (u8)value, (u8)(value >> 8), (u8)(value >> 16), (u8)(value >> 24),
    };
    return fwrite(bytes, 1u, sizeof(bytes), file) == sizeof(bytes);
}

static bool write_header(FILE* file, u32 sample_rate, u32 data_size) {
    return fwrite("RIFF", 1u, 4u, file) == 4u &&
           write_u32_le(file, 36u + data_size) &&
           fwrite("WAVEfmt ", 1u, 8u, file) == 8u &&
           write_u32_le(file, 16u) && write_u16_le(file, 1u) &&
           write_u16_le(file, 2u) && write_u32_le(file, sample_rate) &&
           write_u32_le(file, sample_rate * 4u) && write_u16_le(file, 4u) &&
           write_u16_le(file, 16u) &&
           fwrite("data", 1u, 4u, file) == 4u &&
           write_u32_le(file, data_size);
}

void bluewake_audio_capture_init(BluewakeAudioCapture* capture,
                                 const char* path) {
    if (capture == NULL)
        return;
    memset(capture, 0, sizeof(*capture));
    capture->path = path;
    capture->sample_hash = 2166136261u;
}

static bool open_capture(BluewakeAudioCapture* capture, u32 sample_rate) {
    capture->file = fopen(capture->path, "wb");
    if (capture->file == NULL || !write_header(capture->file, sample_rate, 0u)) {
        if (capture->file != NULL)
            fclose(capture->file);
        capture->file = NULL;
        capture->failed = true;
        return false;
    }
    capture->sample_rate = sample_rate;
    return true;
}

bool bluewake_audio_capture_append_be16_stereo(BluewakeAudioCapture* capture,
                                               const u8* data, u32 size,
                                               u32 sample_rate) {
    if (capture == NULL || capture->path == NULL || capture->path[0] == '\0')
        return true;
    if (capture->failed || data == NULL || size == 0u || (size % 4u) != 0u ||
        (sample_rate != 32000u && sample_rate != 48000u)) {
        if (capture != NULL)
            capture->failed = true;
        return false;
    }
    if (capture->file == NULL && !open_capture(capture, sample_rate))
        return false;
    if (capture->sample_rate != sample_rate || size > 0xFFFFFFFFu - 36u ||
        capture->frames * 4u > 0xFFFFFFFFu - 36u - size) {
        capture->failed = true;
        return false;
    }

    for (u32 offset = 0u; offset < size; offset += 2u) {
        const u16 bits = (u16)(((u16)data[offset] << 8) | data[offset + 1u]);
        const s32 sample = (s16)bits;
        const s32 magnitude = sample < 0 ? -sample : sample;
        if (!write_u16_le(capture->file, bits)) {
            capture->failed = true;
            return false;
        }
        if (sample != 0)
            capture->nonzero_samples++;
        if (magnitude > capture->peak_sample)
            capture->peak_sample = magnitude;
        capture->sample_hash ^= bits;
        capture->sample_hash *= 16777619u;
    }
    capture->frames += size / 4u;
    return true;
}

bool bluewake_audio_capture_close(BluewakeAudioCapture* capture) {
    if (capture == NULL || capture->file == NULL)
        return capture == NULL || !capture->failed;
    const u64 data_size_64 = capture->frames * 4u;
    bool ok = !capture->failed && data_size_64 <= 0xFFFFFFFFu - 36u;
    if (ok && fseek(capture->file, 4, SEEK_SET) == 0)
        ok = write_u32_le(capture->file, 36u + (u32)data_size_64);
    else
        ok = false;
    if (ok && fseek(capture->file, 40, SEEK_SET) == 0)
        ok = write_u32_le(capture->file, (u32)data_size_64);
    else
        ok = false;
    if (fclose(capture->file) != 0)
        ok = false;
    capture->file = NULL;
    capture->failed = !ok;
    return ok;
}
