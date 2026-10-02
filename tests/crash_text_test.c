#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>
#include "../windows/src/crash_text.h"
int main(void) {
    char output[19]; output[18] = '!';
    assert(bw_crash_hex(output, UINT64_MAX) == 18);
    assert(memcmp(output, "0xffffffffffffffff!", 19) == 0);
    bw_crash_hex(output, 0); assert(memcmp(output, "0x0000000000000000!", 19) == 0);
    bw_crash_hex(output, 0xABCDEF); assert(memcmp(output, "0x0000000000abcdef!", 19) == 0);
}
