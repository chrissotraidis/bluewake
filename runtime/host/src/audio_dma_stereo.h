#ifndef BLUEWAKE_AUDIO_DMA_STEREO_H
#define BLUEWAKE_AUDIO_DMA_STEREO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* GameCube audio DMA is big-endian R,L; WAV and the platform sink expect L,R.
 * Swap whole 16-bit samples, leaving their byte order and values unchanged. */
static inline bool bluewake_audio_dma_rl_to_lr(uint8_t* data, size_t size) {
    if (data == NULL || size % 4 != 0)
        return false;
    for (size_t at = 0; at < size; at += 4) {
        const uint8_t hi = data[at], lo = data[at + 1];
        data[at] = data[at + 2];
        data[at + 1] = data[at + 3];
        data[at + 2] = hi;
        data[at + 3] = lo;
    }
    return true;
}

#endif
