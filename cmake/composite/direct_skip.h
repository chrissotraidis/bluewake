#ifndef BLUEWAKE_COMPOSITE_DIRECT_SKIP_H
#define BLUEWAKE_COMPOSITE_DIRECT_SKIP_H

/* What the host hands the module (bluewake_composite_direct_calls_v3) so that a
 * direct call answers the host's per-call question (host_direct_can_skip)
 * itself, without a call into the host. Shared by the host and the module.
 *
 * Only with the lazy overlap watch (on by default, BLUEWAKE_LAZY_OVERLAP=0 turns it
 * off; main.c), when the
 * host's answer depends on the address alone. simple is nonzero only while no
 * diagnostic is on and the jump button is not armed; the host refreshes it
 * where either can change, and with it off the module asks the host as before.
 * The addresses are those the host observes whether or not their features are
 * on, so the answer can only be more careful than the host's.
 * BLUEWAKE_DIRECT_SKIP_VERIFY=1 asks the host as well and counts disagreements. */

#include <stdint.h>

typedef struct BwHostSkipFast {
    uint32_t simple;
    uint32_t single[2];      /* addresses the host observes; 0 is none */
    uint32_t range_first[2]; /* ranges it observes, first .. first + span; */
    uint32_t range_span[2];  /* an empty range is first 0xFFFFFFFF, span 0 */
} BwHostSkipFast;

#endif
