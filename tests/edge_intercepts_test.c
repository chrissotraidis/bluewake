#ifdef NDEBUG
#undef NDEBUG
#endif
#include "edge_intercepts.h"
#include "edge_intercept_table.h"

#include <assert.h>
#include <stddef.h>

int main(void) {
    // The fast reject must never answer false for an address the switches
    // match: it is a skip, not an authority. Its table and its hash parameters
    // are generated separately, and on 2026-09-21 they had drifted apart - the
    // header declared multiplier 668265263 and shift 24 while carrying a table
    // placed with different parameters, so three keys hashed to the wrong slot
    // and the guest never reached the title screen. The route caught it; this
    // is the check that would have caught it without a full boot.
    for (size_t i = 0u; i < BLUEWAKE_EDGE_KEY_COUNT; i++)
        assert(bluewake_edge_maybe_intercept(g_edge_keys_all[i]));

    static const u32 semantic_addresses[] = {
        0x803193ACu, 0x80308A9Cu, 0x80322B20u, 0x80322BC8u,
        0x8030F2B0u, 0x8030F618u, 0x8030F5A4u, 0x80017FD8u,
        0x8030FADCu, 0x8030FBCCu,
        0x80240744u, 0x81E000D4u,
        0x8031A6E8u, 0x8031CD50u, 0x8031D2E0u, 0x8031D400u,
        0x8031D438u, 0x8031DAFCu, 0x8031DC9Cu, 0x8031DD80u,
        0x8031E5C8u, 0x8031E74Cu, 0x8031E8C4u, 0x8031EA48u,
        0x8031EC68u, 0x8031EF98u, 0x8031F0E0u, 0x8031F348u,
        0x8031F45Cu, 0x8031F69Cu, 0x8031F7C8u, 0x8031F93Cu,
        0x8031F984u,
    };

    for (size_t i = 0u;
         i < sizeof semantic_addresses / sizeof semantic_addresses[0]; i++)
        assert(bluewake_edge_address_requires_host(
            semantic_addresses[i], 0u));

    const u32 raw_base = 0x81234000u;
    assert(bluewake_edge_address_requires_host(raw_base + 0xD4u, raw_base));
    assert(!bluewake_edge_address_requires_host(raw_base + 0xD0u, raw_base));
    // CARD callback return lives outside generated code and must miss back to
    // the host callback service rather than masquerade as an address bridge.
    assert(!bluewake_edge_address_requires_host(0x7FFF0000u, raw_base));
    assert(!bluewake_edge_address_requires_host(0x80003140u, raw_base));
    assert(!bluewake_edge_address_requires_host(0x803193B0u, raw_base));

    static const u32 observation_addresses[] = {
        0x81E01B88u, 0x81E01BA4u,
        0x802315A8u, 0x8022F9FCu, 0x802305E0u, 0x80230A14u,
        0x8017E798u, 0x8017E86Cu, 0x80181634u, 0x80182A90u,
        0x80120188u, 0x800A0B60u,
        0x800D8DB8u, 0x8015E3F0u, 0x8015EA5Cu, 0x8015D80Cu,
    };
    for (size_t i = 0u;
         i < sizeof observation_addresses / sizeof observation_addresses[0];
         i++)
        assert(bluewake_edge_observation_requires_host(
            observation_addresses[i]));
    assert(!bluewake_edge_observation_requires_host(0x80003140u));
    // GroundCross is still handed to the full edge service, but does not
    // unconditionally end the host turn.
    assert(bluewake_edge_maybe_intercept(0x80328F84u));
    assert(!bluewake_edge_observation_requires_host(0x80328F84u));
    assert(!bluewake_edge_requires_host(0x80328F84u, 0x80328F84u, raw_base));
    // Menu observations are explicitly enabled; counters count only enabled visits.
    bluewake_edge_set_menu_path_observation(false);
    const u64 executes = bluewake_edge_menu_execute_calls();
    const u64 collects = bluewake_edge_menu_collect_calls();
    assert(!bluewake_edge_observation_requires_host(0x801DD960u));
    assert(!bluewake_edge_observation_requires_host(0x801DBA58u));
    assert(bluewake_edge_menu_execute_calls() == executes);
    assert(bluewake_edge_menu_collect_calls() == collects);
    bluewake_edge_set_menu_path_observation(true);
    assert(bluewake_edge_observation_requires_host(0x801DD960u));
    assert(bluewake_edge_observation_requires_host(0x801DBA58u));
    assert(bluewake_edge_menu_execute_calls() == executes + 1u);
    assert(bluewake_edge_menu_collect_calls() == collects + 1u);
    bluewake_edge_set_menu_path_observation(false);

    static const u32 canonical_semantic_addresses[] = {
        0x80303A50u, 0x80240EE8u, 0x80241178u, 0x802411F8u,
    };
    for (size_t i = 0u;
         i < sizeof canonical_semantic_addresses /
                 sizeof canonical_semantic_addresses[0];
         i++) {
        const u32 canonical = canonical_semantic_addresses[i];
        assert(bluewake_edge_requires_host(canonical, canonical, raw_base));
        assert(bluewake_edge_requires_host(
            canonical | 0x40000000u, canonical, raw_base));
    }
    assert(bluewake_edge_requires_host(
        0xC1E01B88u, 0x81E01B88u, raw_base));
    assert(!bluewake_edge_requires_host(
        0x80003140u, 0x80003140u, raw_base));
    return 0;
}
