#include "actor_search_budget.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>
#include <stdio.h>

// The original nine leader checks plus the following chassis boundary,
// evaluated with wider arithmetic so extreme test inputs cannot overflow.
static bool reference(int64_t downcount, int64_t budget, int64_t deadline) {
    const unsigned blocks[] = {10,4,2,5,2,3,2,2,5};
    __int128 d = downcount;
    if (budget <= 0) return false;
    for (unsigned i=0; i<sizeof(blocks)/sizeof(*blocks); ++i) {
        if (d <= -(__int128)budget) return false;
        if (deadline > 0 && (__int128)deadline+d < blocks[i]) return false;
        d -= blocks[i];
    }
    return d > -(__int128)budget;
}

int main(void) {
    for (int64_t budget=1; budget<100; ++budget)
        for (int64_t deadline=-1; deadline<100; ++deadline)
            for (int64_t d=-110; d<=40; ++d)
                assert(bluewake_actor_search_iteration_fits(d,budget,deadline) ==
                       reference(d,budget,deadline));
    const int64_t edges[] = {INT64_MIN, INT64_MIN+1, -100, -1, 0, 1, 34, 35, 36,
                             INT64_MAX-35, INT64_MAX-1, INT64_MAX};
    for (unsigned i=0; i<sizeof(edges)/sizeof(*edges); ++i)
        for (unsigned j=0; j<sizeof(edges)/sizeof(*edges); ++j)
            for (unsigned k=0; k<sizeof(edges)/sizeof(*edges); ++k)
                assert(bluewake_actor_search_iteration_fits(edges[i],edges[j],edges[k]) ==
                       reference(edges[i],edges[j],edges[k]));
    puts("actor search: collapsed deadline checks match all original boundaries");
}
