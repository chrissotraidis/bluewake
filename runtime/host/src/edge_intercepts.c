#include "edge_intercepts.h"

#include "card_runtime.h"
#include "edge_intercept_table.h"

// The pause menu's execute and the collect screen it creates. Counting them here
// rather than in the per-turn body is what makes the count exact: this function
// only reaches those cases when the address matched the table, so a miss pays
// nothing for the counter, and it runs with the address the chassis is about to
// dispatch, not with whatever the program counter happened to hold at the
// retrace boundary.
static u64 g_menu_execute_calls;
static u64 g_menu_collect_calls;

// The two menu addresses are observed only when the save route is driving the
// pause menu. They stay in the switch list either way - the fast-reject table is
// built from these lists and must not answer false for an address the switches
// match - but with the flag clear they return false, which is what the chassis
// did before they were added. That matters: returning true costs a host turn at
// that block, and a host turn is what the route's turn count and the certified
// stops are quoted in. Adding them unconditionally moved the certified 14,100
// stop from 32,203,791 turns to 32,203,876 (measured, digest unchanged).
static bool g_menu_path_observation;

void bluewake_edge_set_menu_path_observation(bool enabled) {
    g_menu_path_observation = enabled;
}

u64 bluewake_edge_menu_execute_calls(void) { return g_menu_execute_calls; }
u64 bluewake_edge_menu_collect_calls(void) { return g_menu_collect_calls; }

bool bluewake_edge_address_requires_host(u32 address, u32 module1_raw_base) {
    // This runs once per boundary-loop dispatch and almost every call is a miss,
    // so the switches below are the whole cost of a miss. The set they match is
    // fixed and small, so it is also a perfect hash: one multiply, one load and
    // one compare rejects anything that cannot match, and a hit falls through to
    // the switches, which stay the authority. The module-1 alias is dynamic, so
    // it is checked explicitly rather than through the table.
    const bool module1_alias =
        module1_raw_base != 0u && address == module1_raw_base + 0xD4u;
    if (!module1_alias && !bluewake_edge_maybe_intercept(address))
        return false;
    if (bluewake_card_runtime_intercepts(address))
        return true;

    switch (address) {
    case 0x803193ACu: // JAudio DSP task boot handshake.
    case 0x80308A9Cu: // GX draw-done return publishes PE finish.
    case 0x80322B20u: // GXSetDrawSync flush publishes the PE token.
    case 0x80322BC8u: // GXSetDrawDone return publishes the async PE finish.
    case 0x8030F2B0u: // DVD path to entry.
    case 0x8030F618u: // DVD open.
    case 0x8030F5A4u: // DVD fast open.
    case 0x80017FD8u: // Archive path to entry.
    case 0x8030FADCu: // DVD async read.
    case 0x8030FBCCu: // DVD synchronous read.
    case 0x80240744u: // Archived REL materialization.
    case 0x81E000D4u: // Legacy module-1 raw entry alias.
        return true;
    default:
        return module1_raw_base != 0u &&
               address == module1_raw_base + 0xD4u;
    }
}

bool bluewake_edge_observation_requires_host(u32 canonical_address) {
    if (!bluewake_edge_maybe_intercept(canonical_address))
        return false;
    switch (canonical_address) {
    case 0x81E01B88u:
    case 0x81E01BA4u:
    case 0x802315A8u:
    case 0x8022F9FCu:
    case 0x802305E0u:
    case 0x80230A14u:
    case 0x8017E798u:
    case 0x8017E86Cu:
    case 0x80181634u:
    case 0x80182A90u:
    case 0x80120188u:
    case 0x800A0B60u:
    case 0x800D8DB8u:
    case 0x8015E3F0u:
    case 0x8015EA5Cu:
    case 0x8015D80Cu:
        return true;
    // Keep this as an explicit case: the table generator collects cases.
    // GroundCross is observed by the full edge service without ending the turn.
    case 0x80328F84u:
        return false;
    // The pause menu's own execute (d_menu_window.cpp's dMs_Execute) and the
    // collect screen it creates. They are the two addresses that say whether
    // the START press reached the menu at all, which is the question P4
    // milestone 9 stalls on: the guest sees the button and nothing opens.
    case 0x801DD960u:
        if (!g_menu_path_observation)
            return false;
        g_menu_execute_calls++;
        return true;
    case 0x801DBA58u:
        if (!g_menu_path_observation)
            return false;
        g_menu_collect_calls++;
        return true;
    default:
        return false;
    }
}

// bluewake_edge_requires_host is defined in the header: it runs once per block
// boundary and the call frame is a measurable part of a miss.
