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

/* v4 extends, rather than changes, the v3 ABI. `simple` keeps its meaning for
 * v3 modules (non-zero: the inline path may answer), and a v4 module also reads
 * the stage select's five fixed hooks from its bits 1..5 (BW_SKIP_KNOWN_*), so
 * no direct call loads anything more: with the stage select off `simple` is 1
 * and the hooks cost one compare on a value already loaded. Addresses are
 * checked with the 0x40000000 code-mirror bit removed, matching the edge-watch
 * table. The host sets the bits only for a v4 module (a v3 module keeps the
 * host query while the stage select is live) and statically asserts its
 * stage_select.h addresses match (main.c). A host that needs a watch the
 * module doesn't know turns the inline path off (`simple` = 0): exact. */
#define BW_SKIP_KNOWN_CHANGE 0x80029E6Cu       /* fopScnM_ChangeReq, bit 1 */
#define BW_SKIP_KNOWN_DATA_SELECT 0x80181634u  /* dFile_select_c::dataSelect, bit 2 */
#define BW_SKIP_KNOWN_EXECUTE 0x8022ED50u      /* dScnMenu_Execute, bit 3 */
#define BW_SKIP_KNOWN_PLAY_EXECUTE 0x80234FD0u /* dScnPly_Execute, bit 4 */
#define BW_SKIP_KNOWN_MENU_DELETE 0x8022F320u  /* dScnMenu_Delete, bit 5 */
#define BW_SKIP_KNOWN_ALL 0x3Eu                /* all five, in `simple` */
typedef struct BwHostSkipFastV4 {
    BwHostSkipFast base;
} BwHostSkipFastV4;

#endif
