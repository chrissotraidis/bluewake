#ifndef BLUEWAKE_AUDIO_CAPTURE_H
#define BLUEWAKE_AUDIO_CAPTURE_H

#include "core/types.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct BluewakeAudioCapture {
    const char* path;
    FILE* file;
    u64 frames;
    u64 nonzero_samples;
    u32 sample_rate;
    u32 sample_hash;
    s32 peak_sample;
    bool failed;
} BluewakeAudioCapture;

void bluewake_audio_capture_init(BluewakeAudioCapture* capture,
                                 const char* path);
bool bluewake_audio_capture_append_be16_stereo(BluewakeAudioCapture* capture,
                                               const u8* data, u32 size,
                                               u32 sample_rate);
bool bluewake_audio_capture_close(BluewakeAudioCapture* capture);

#endif
