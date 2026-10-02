#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include "gx_flush_metrics.h"
int main(void) {
    assert(bw_gx_flush_start(NULL) == 13800);
    assert(bw_gx_flush_start("0") == 0);
    assert(bw_gx_flush_start("123") == 123);
    assert(bw_gx_flush_start("-1") == 13800 && bw_gx_flush_start("12x") == 13800);
    assert(bw_gx_flush_start("9999999999999999999999") == 13800);
    struct timespec a = {4,999980000}, b = {5,19000};
    assert(bw_elapsed_us(a,b) == 39);
    assert(bw_elapsed_us(b,a) == 0 && bw_elapsed_us(a,a) == 0);
}
