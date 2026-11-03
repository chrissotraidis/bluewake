#ifdef NDEBUG
#undef NDEBUG
#endif
#include "audio_dma_stereo.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint8_t data[] = {0x7F, 0xFF, 0x80, 0x00, 0, 1, 0xFF, 0xFF, 0xCC, 0xDD};
    const uint8_t original[] = {0x7F, 0xFF, 0x80, 0x00, 0, 1, 0xFF, 0xFF, 0xCC, 0xDD};
    const uint8_t expected[] = {0x80, 0, 0x7F, 0xFF, 0xFF, 0xFF, 0, 1, 0xCC, 0xDD};
    assert(bluewake_audio_dma_rl_to_lr(data, 8));
    assert(memcmp(data, expected, sizeof data) == 0);
    assert(bluewake_audio_dma_rl_to_lr(data, 8));
    assert(memcmp(data, original, sizeof data) == 0);
    assert(bluewake_audio_dma_rl_to_lr(data, 0));
    assert(!bluewake_audio_dma_rl_to_lr(NULL, 4));
    for (size_t size = 1; size < 8; ++size) {
        if (size == 4)
            continue;
        assert(!bluewake_audio_dma_rl_to_lr(data, size));
        assert(memcmp(data, original, sizeof data) == 0);
    }
    for (uint32_t right = 0; right <= UINT16_MAX; ++right) {
        const uint16_t left = (uint16_t)~right;
        uint8_t pair[] = {(uint8_t)(right >> 8), (uint8_t)right,
                          (uint8_t)(left >> 8), (uint8_t)left};
        assert(bluewake_audio_dma_rl_to_lr(pair, sizeof pair));
        assert(pair[0] == (uint8_t)(left >> 8) && pair[1] == (uint8_t)left);
        assert(pair[2] == (uint8_t)(right >> 8) && pair[3] == (uint8_t)right);
    }
    puts("PASS: DMA R,L to output L,R; every 16-bit value; signed extrema; canaries; malformed input");
    return 0;
}
