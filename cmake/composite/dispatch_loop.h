#ifndef BLUEWAKE_COMPOSITE_DISPATCH_LOOP_H
#define BLUEWAKE_COMPOSITE_DISPATCH_LOOP_H

#include "edge_intercept_abi.h"

typedef int (*BluewakeCompositeDispatchFn)(CPUState* ctx, u32 address);

/* The boundary loop, in the header and always inlined.
 *
 * Why it is not only the out-of-line function below. The composite dispatches
 * one guest block per iteration of this loop, and the dispatch arrives as a
 * function pointer, so passing it through the entry point of another
 * translation unit left the compiler with an indirect call it could not see
 * through: the pointer was reloaded from its stack slot at every guest edge and
 * neither selected_dispatch nor the chunk lookup beneath it could be inlined
 * into the loop. Called with a static function from the same translation unit
 * the call devirtualises, the lookup folds into the loop, and the loop's own
 * state survives an edge in registers instead of being rebuilt.
 *
 * The exported form and the unit test keep the out-of-line entry point; it is
 * this body, so the two cannot drift.
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((always_inline))
#endif
static inline int bluewake_chassis_dispatch_loop(
    CPUState* ctx, u32 address, BluewakeCompositeDispatchFn dispatch,
    BluewakeEdgeServiceFn edge_service, void* service_user) {
    if (ctx == NULL || dispatch == NULL)
        return 0;

    s64 prior_downcount = ctx->downcount;
    int dispatched = dispatch(ctx, address);
    if (!dispatched || edge_service == NULL)
        return dispatched;
    if (ctx->downcount >= prior_downcount)
        return 1;

    unsigned zero_charge_run = 0u;
    for (;;) {
        if (ctx->exception != 0u ||
            (ctx->cycle_budget > 0 &&
             ctx->downcount <= -ctx->cycle_budget))
            return 1;

        address = ctx->pc;
        if (edge_service(service_user, ctx, address))
            return 1;

        prior_downcount = ctx->downcount;
        dispatched = dispatch(ctx, address);
        if (!dispatched)
            return 1;
        if (ctx->downcount >= prior_downcount) {
            /* Eight non-advancing successors are allowed; the ninth yields
             * so a stuck guest always returns to the host. Progress resets
             * the run, and a new turn or CPU gets an independent allowance. */
            if (++zero_charge_run <= 8u)
                continue;
            return 1;
        }
        zero_charge_run = 0u;
    }
}

int bluewake_composite_dispatch_until_boundary(
    CPUState* ctx, u32 address, BluewakeCompositeDispatchFn dispatch,
    BluewakeEdgeServiceFn edge_service, void* service_user);

#endif
