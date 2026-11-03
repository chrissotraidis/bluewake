#ifndef BLUEWAKE_ACTOR_SEARCH_BUDGET_H
#define BLUEWAKE_ACTOR_SEARCH_BUDGET_H

#include <stdbool.h>
#include <stdint.h>

// The native no-match iteration visits blocks of 10,4,2,5,2,3,2,2,5
// instructions. All costs are positive: fitting the last deadline and the
// next chassis boundary proves that every intervening leader also fits.
// Compare before subtracting, including at the signed counter limits.
static inline bool bluewake_actor_search_iteration_fits(
    int64_t downcount, int64_t budget, int64_t deadline) {
    return budget > 0 && downcount > -budget + 35 &&
           (deadline <= 0 || downcount >= -deadline + 35);
}

#endif
