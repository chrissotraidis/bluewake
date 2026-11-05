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

/* v4 extends, rather than changes, the v3 ABI. The host may name fixed guest
 * addresses added after a game module was built. They are checked with the
 * 0x40000000 code-mirror bit removed, matching the edge-watch table. */
#define BW_HOST_SKIP_WATCH_MAX 5u
/* The stage select's five fixed hooks, known to the module at build time so a
 * direct call compares its (usually constant) target against constants: the
 * compiler folds every comparison away except at these five addresses. The host
 * turns them on with `known` (one bit each, in this order) and checks that its
 * own stage_select.h addresses match (main.c). watched[] stays for addresses a
 * host adds after a module was built; it is normally empty. */
#define BW_SKIP_KNOWN_CHANGE 0x80029E6Cu       /* fopScnM_ChangeReq */
#define BW_SKIP_KNOWN_DATA_SELECT 0x80181634u  /* dFile_select_c::dataSelect */
#define BW_SKIP_KNOWN_EXECUTE 0x8022ED50u      /* dScnMenu_Execute */
#define BW_SKIP_KNOWN_PLAY_EXECUTE 0x80234FD0u /* dScnPly_Execute */
#define BW_SKIP_KNOWN_MENU_DELETE 0x8022F320u  /* dScnMenu_Delete */
#define BW_SKIP_KNOWN_ALL 0x1Fu
typedef struct BwHostSkipFastV4 {
    BwHostSkipFast base;
    uint32_t watched_count;
    uint32_t watched[BW_HOST_SKIP_WATCH_MAX];
    uint32_t known; /* BW_SKIP_KNOWN_* bits the host observes */
} BwHostSkipFastV4;

#endif
