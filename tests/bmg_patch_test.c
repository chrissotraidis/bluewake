#ifdef NDEBUG
#undef NDEBUG
#endif
#include "bmg_patch.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Fixture {
    uint8_t data[256];
    unsigned writes;
} Fixture;

static void put32(uint8_t* p, uint32_t n) {
    p[0] = (uint8_t)(n >> 24); p[1] = (uint8_t)(n >> 16);
    p[2] = (uint8_t)(n >> 8); p[3] = (uint8_t)n;
}
static void put16(uint8_t* p, uint16_t n) {
    p[0] = (uint8_t)(n >> 8); p[1] = (uint8_t)n;
}
static Fixture fixture(void) {
    Fixture f = {0};
    memset(f.data + 128, 0xCC, 128); // canaries outside the declared file
    memcpy(f.data, "MESGbmg1", 8);
    put32(f.data + 8, 4); put32(f.data + 12, 2);
    memcpy(f.data + 32, "INF1", 4); put32(f.data + 36, 64);
    put16(f.data + 40, 2); put16(f.data + 42, 16);
    put32(f.data + 48, 1); put32(f.data + 64, 12);
    f.data[61] = f.data[77] = 0x99;
    memcpy(f.data + 96, "DAT1", 4); put32(f.data + 100, 32);
    const uint8_t wait[] = {'A', 0x1A, 7, 0, 0, 7, 0, 9, 0};
    const uint8_t prompt[] = {'B', 0x1A, 7, 0, 0, 3, 0, 0, 0};
    memcpy(f.data + 105, wait, sizeof wait);
    memcpy(f.data + 116, prompt, sizeof prompt);
    return f;
}
static void write_byte(void* user, size_t offset, uint8_t value) {
    Fixture* f = user;
    assert(offset < 128); // never touch even one canary byte
    ++f->writes;
    f->data[offset] = value;
}
static void rejected(Fixture f) {
    const Fixture before = f;
    uint32_t messages = 0x11111111, waits = 0x22222222;
    assert(!bluewake_bmg_patch(f.data, sizeof f.data, write_byte, &f, &messages, &waits));
    assert(f.writes == 0 && memcmp(f.data, before.data, sizeof f.data) == 0);
    assert(messages == 0x11111111 && waits == 0x22222222);
}

int main(void) {
    Fixture f = fixture(), expected = f;
    expected.data[61] = expected.data[77] = 1;
    expected.data[110] = expected.data[121] = 7;
    expected.data[111] = expected.data[112] = expected.data[122] = expected.data[123] = 0;
    uint32_t messages, waits;
    assert(bluewake_bmg_patch(f.data, sizeof f.data, write_byte, &f, &messages, &waits));
    assert(messages == 2 && waits == 2 && f.writes == 8);
    assert(memcmp(f.data, expected.data, sizeof f.data) == 0);
    expected = f;
    assert(bluewake_bmg_patch(f.data, sizeof f.data, write_byte, &f, &messages, &waits));
    assert(messages == 2 && waits == 0 && f.writes == 10);
    assert(memcmp(f.data, expected.data, sizeof f.data) == 0);

    /* Exact-size allocations exercise every truncation under ASan, rather
     * than relying only on a larger synthetic RAM span to hide overreads. */
    f = fixture();
    for (size_t n = 0; n < 128; ++n) {
        uint8_t* truncated = malloc(n == 0 ? 1 : n);
        assert(truncated != NULL);
        memcpy(truncated, f.data, n);
        messages = 11; waits = 22;
        assert(!bluewake_bmg_patch(truncated, n, write_byte, &f, &messages, &waits));
        assert(f.writes == 0 && messages == 11 && waits == 22);
        free(truncated);
    }
    f = fixture(); put32(f.data + 8, UINT32_MAX); rejected(f);
    f = fixture(); put32(f.data + 12, 3); rejected(f); // missing section header
    f = fixture(); put32(f.data + 36, UINT32_MAX); rejected(f); // wrapping section size
    f = fixture(); put32(f.data + 36, 8); rejected(f); // short INF1
    f = fixture(); put16(f.data + 40, 4); rejected(f); // entries beyond INF1
    f = fixture(); put16(f.data + 42, 13); rejected(f); // draw type outside entry
    f = fixture(); f.data[16] = 2; rejected(f); // unsupported UTF-16
    f = fixture(); put32(f.data + 64, UINT32_MAX); rejected(f); // invalid later offset
    f = fixture(); put32(f.data + 64, 24); rejected(f); // exactly DAT1's end
    f = fixture(); put32(f.data + 64, 23); f.data[127] = 0x1A; rejected(f);
    f = fixture(); put32(f.data + 64, 21);
    f.data[125] = 0x1A; f.data[126] = 7; f.data[127] = 0;
    f.data[128] = 0; f.data[129] = 3; f.data[130] = 9; f.data[131] = 9;
    rejected(f); // the old length==7 path wrote beyond DAT1 into these canaries
    f = fixture(); f.data[118] = 0; rejected(f);
    f = fixture(); f.data[118] = 1; rejected(f);
    f = fixture(); memset(f.data + 116, 'Z', 12); rejected(f); // no NUL

    /* Sections are located by their tags, and entries are bounded by INF1,
     * not by an assumed DAT1 position. */
    f = fixture();
    uint8_t inf[64], dat[32];
    memcpy(inf, f.data + 32, sizeof inf); memcpy(dat, f.data + 96, sizeof dat);
    memcpy(f.data + 32, dat, sizeof dat); memcpy(f.data + 64, inf, sizeof inf);
    assert(bluewake_bmg_patch(f.data, sizeof f.data, write_byte, &f, &messages, &waits));
    assert(messages == 2 && waits == 2 && f.writes == 8);

    /* Every single-byte mutation either succeeds with bounded writes or
     * rejects without partially applying an earlier valid message. */
    for (size_t at = 0; at < 128; ++at) {
        for (unsigned byte = 0; byte < 256; ++byte) {
            f = fixture(); f.data[at] = (uint8_t)byte;
            const Fixture before = f;
            if (!bluewake_bmg_patch(f.data, sizeof f.data, write_byte, &f, &messages, &waits))
                assert(f.writes == 0 && memcmp(f.data, before.data, sizeof f.data) == 0);
            assert(memcmp(f.data + 128, before.data + 128, 128) == 0);
        }
    }
    puts("BMG patch: valid/idempotent writes, 128 exact truncations, malformed tables/commands and 32768 mutations pass");
    return 0;
}
