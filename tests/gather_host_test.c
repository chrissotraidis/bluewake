#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include "gather_pipe.h"
#include "gather_pipe_bridge.h"

void bluewake_composite_set_gather_pipe(BwGatherPipeWrite write);
void bluewake_composite_set_gather_pipe_bytes(BwGatherPipeBytes bytes);
static unsigned words, batches, delivered;
static void word(u64 value, u8 size) {
    assert(value == 0x12345678u && size == 4);
    words++;
    delivered += size;
}
static void bytes(const u8* data, u32 size) {
    assert(size == 4 && memcmp(data, "\x12\x34\x56\x78", 4) == 0);
    batches++;
    delivered += size;
}
static BluewakeGatherMode configure(const char* direct, const char* batch, bool trace) {
    return bluewake_gather_pipe_configure(bluewake_composite_set_gather_pipe,
        bluewake_composite_set_gather_pipe_bytes, word, bytes, direct, batch, trace);
}
int main(void) {
    const char* disabled[] = {NULL, "", "0", "false", "true", "01", "1x"};
    for (unsigned i = 0; i < sizeof disabled / sizeof disabled[0]; ++i) {
        assert(configure(disabled[i], "1", false) == BLUEWAKE_GATHER_OFF);
        assert(bw_gather_pipe_write == NULL && bw_gather_pipe_bytes == NULL);
    }
    assert(configure("1", NULL, false) == BLUEWAKE_GATHER_DIRECT);
    bw_gather_pipe_put(0x12345678u, 4);
    assert(words == 1 && batches == 0 && delivered == 4);
    assert(configure("1", "1", false) == BLUEWAKE_GATHER_BATCH);
    bw_gather_pipe_put(0x12345678u, 4);
    assert(delivered == 4 && bw_gather_pipe_length == 4);
    /* Turning diagnostics on delivers queued bytes through the old writer
     * before disconnecting it; later writes must take ordinary MMIO. */
    assert(configure("1", "1", true) == BLUEWAKE_GATHER_OFF);
    assert(delivered == 8 && batches == 1 && bw_gather_pipe_length == 0);
    assert(bw_gather_pipe_write == NULL && bw_gather_pipe_bytes == NULL);
    assert(configure("1", "1", false) == BLUEWAKE_GATHER_BATCH);
    bw_gather_pipe_put(0x12345678u, 4);
    assert(configure("1", "0", false) == BLUEWAKE_GATHER_DIRECT);
    assert(delivered == 12 && batches == 2 && bw_gather_pipe_length == 0);
    bw_gather_pipe_put(0x12345678u, 4);
    assert(delivered == 16 && words == 2);
    assert(configure(NULL, NULL, false) == BLUEWAKE_GATHER_OFF);
    assert(bluewake_gather_pipe_configure(NULL, NULL, word, bytes, "1", "1", false)
           == BLUEWAKE_GATHER_OFF);
    assert(bluewake_gather_pipe_configure(bluewake_composite_set_gather_pipe,
        NULL, word, bytes, "1", "1", false) == BLUEWAKE_GATHER_DIRECT);
    assert(bw_gather_pipe_bytes == NULL);
    assert(bluewake_gather_pipe_configure(bluewake_composite_set_gather_pipe,
        bluewake_composite_set_gather_pipe_bytes, word, NULL, "1", "1", false)
        == BLUEWAKE_GATHER_DIRECT);
    assert(bw_gather_pipe_bytes == NULL);
    assert(configure(NULL, NULL, false) == BLUEWAKE_GATHER_OFF);
    puts("host gather selection and buffered transitions passed");
    return 0;
}
