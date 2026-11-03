#include "dispatch_loop.h"

/* The exported entry point is the inline loop's body; see dispatch_loop.h. */
int bluewake_composite_dispatch_until_boundary(
    CPUState* ctx, u32 address, BluewakeCompositeDispatchFn dispatch,
    BluewakeEdgeServiceFn edge_service, void* service_user) {
    return bluewake_chassis_dispatch_loop(ctx, address, dispatch, edge_service,
                                          service_user);
}
