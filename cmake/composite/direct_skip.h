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

#include <stdbool.h>
#include <stdint.h>

typedef struct BwHostSkipFast {
    uint32_t simple;
    uint32_t single[2];      /* addresses the host observes; 0 is none */
    uint32_t range_first[2]; /* ranges it observes, first .. first + span; */
    uint32_t range_span[2];  /* an empty range is first 0xFFFFFFFF, span 0 */
} BwHostSkipFast;

/* The gate (bluewake_composite_direct_gate): one host word the module reads
 * inline before a direct call to a constant address in the main executable's
 * code, outside both ranges. Zero means every host-side condition of the plain
 * check holds, so only the depth, an exception and the turn's budget remain:
 *   - the interrupt sources are clean and no decrementer or PI interrupt is
 *     pending (the guest's MSR[EE] then cannot matter);
 *   - the address-only answer is on and simple, and neither single address lies
 *     in the main executable's code, so a constant target there cannot be one.
 * The first two bytes are the host's own flags: main.c keeps them here, so
 * every write to either updates the word. The host sets the other two where
 * their inputs change. Any non-zero byte sends the call to the out-of-line
 * check, which is exact; a stale non-zero byte only costs that call. */
typedef union BwHostGate {
    uint32_t word;
    struct {
        bool sources_dirty;       /* host_refresh_interrupt_sources still to run */
        bool decrementer_pending; /* a decrementer exception the guest would take */
        uint8_t closed;           /* inline answer off or not simple, or a single in main code */
        uint8_t pi_pending;       /* pi_cause & pi_mask, as of the last refresh or dispatch */
    } bits;
} BwHostGate;
_Static_assert(sizeof(bool) == 1u && sizeof(BwHostGate) == 4u,
               "the gate is one 32-bit word of four bytes");

/* The main executable's code (bw_edge_unwatched's main_code). */
#define BW_GATE_MAIN_FIRST 0x80003100u
#define BW_GATE_MAIN_END 0x80400000u

/* The two ranges, fixed guest addresses (the host's draw_tags.h; main.c checks
 * they agree). A direct call whose target the build knows lies outside both
 * reads only the singles (direct_calls.h, bw_direct_call_ready_plain). */
#define BW_SKIP_RANGE0_FIRST 0x80260BACu /* particle draws */
#define BW_SKIP_RANGE0_LAST 0x80264C4Cu
#define BW_SKIP_RANGE1_FIRST 0x8007E484u /* wake draws */
#define BW_SKIP_RANGE1_LAST 0x8007F3BCu

#endif
