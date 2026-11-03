#!/usr/bin/env python3
"""Emit the open-addressed table for the edge intercept fast reject.

Why this exists. bluewake_edge_requires_host runs once per boundary-loop
dispatch, and almost every call is a miss, so its whole cost on a miss is the
four switches it consults - about fourteen comparisons. The intercept set is
fixed and small, so it is also a tiny hash table, and a miss that hashes to an
empty slot is rejected in one load and one compare. The switches stay the
authority; this only skips them, and because the table is built from the same
case lists it cannot produce a false negative.

Run it after adding a case to any of the switches below. It writes
runtime/host/src/edge_intercept_table.h directly.

It writes the file rather than printing a block to paste, because the paste is
the step that broke. The header in the tree on 2026-09-21 declared multiplier
668265263 and shift 24 while holding a table placed with different parameters,
so three of the fifty-two keys - 0x80181634, 0x8030F5A4 (the DVD fast open) and
0x8031D2E0 - hashed to the wrong slot. The fast reject then answered false for
addresses the switches match, so the chassis never returned to the host for the
DVD fast-open intercept and the guest never reached the title screen:
title_ready=0 and a stop at pc 0x80301510 instead of 0x80307EF4. The route
caught it; the test could not, because the test's key list is hand-written. The
header now carries the full key set as well, so a test can assert that the
lookup finds every key, which is the check that would have caught this.

Usage: scripts/gen_edge_intercept_table.py
"""

import re
import sys
from pathlib import Path

EDGE = "runtime/host/src/edge_intercepts.c"
# bluewake_edge_requires_host moved into its own header so the boundary loop can
# inline it (it runs once per dispatch and almost every call is a miss). Its
# case list is part of the intercept set like the two in the .c file, so the
# generator has to read it too: the first run after the move built the table
# from the .c file alone and silently dropped the four keys that only the header
# lists (0x80303A50, 0x80240EE8, 0x80241178, 0x802411F8). The fast reject then
# answered false for addresses the switches still matched - the same failure
# this file's docstring records for a misplaced key, and the same one the route
# catches as a stop before the title screen, because those four are the
# chassis's own re-entry addresses.
EDGE_HEADER = "runtime/host/src/edge_intercepts.h"
CARD = "runtime/host/src/card_runtime.c"
EDGE_FUNCS = (
    "bluewake_edge_address_requires_host",
    "bluewake_edge_observation_requires_host",
    "bluewake_edge_requires_host",
)
CARD_FUNCS = ("bluewake_card_runtime_intercepts", "card_handler")

# Candidate multipliers, tried with every shift that lands in range.
MULTIPLIERS = (668265263, 2654435761, 2246822519, 3266489917, 374761393, 1103515245)
SLOTS = 256
# A miss is the common case and pays the probe chain, so the placement is
# chosen for the longest probe over all keys.
MAX_PROBE = 4


def cases(text: str, func: str):
    start = text.find(func)
    if start < 0:
        return None
    end = text.find("\n}", start)
    body = text[start:end]
    return [int(m, 16) for m in re.findall(r"case (0x[0-9A-Fa-f]+)u:", body)]


def collect():
    keys = set()
    edge = open(EDGE, encoding="utf-8").read()
    header = open(EDGE_HEADER, encoding="utf-8").read()
    card = open(CARD, encoding="utf-8").read()
    for fn in EDGE_FUNCS:
        # A name can appear in one file and be defined in the other: the
        # inlined predicate is named in a comment in the .c file and defined in
        # the header. Take the file that actually carries the case list, and
        # refuse to run if neither does.
        found = []
        for text in (edge, header):
            got = cases(text, fn)
            if got:
                found = got
                break
        if not found:
            sys.exit(
                "gen_edge_intercept_table: %s has no case list in %s or %s"
                % (fn, EDGE, EDGE_HEADER)
            )
        keys.update(found)
    for fn in CARD_FUNCS:
        keys.update(cases(card, fn))
    return sorted(k for k in keys if k != 0)


def build(keys, multiplier, shift):
    """Place every key with linear probing, then verify the lookup finds it.

    The lookup is a short probe chain that ends at the first empty slot, so it
    cannot answer false for a key the switches match: an empty slot means the
    key was never placed. A single-probe lookup would need a perfect hash, and
    there is none over these fifty-two addresses in 256 slots - the first
    version of this function claimed otherwise because its accounting loop
    never wrote the key it had just placed, so it always saw an empty table and
    always reported zero displacements. The placement loop then moved three
    keys - 0x80181634, 0x8030F5A4 (the DVD fast open) and 0x8031D2E0 - one slot
    past their hash, and the emitted header answered false for addresses the
    switches match. The guest never reached the title screen. The verification
    pass below is the check that makes that impossible.
    """
    table = [0] * SLOTS
    farthest = 0
    for key in keys:
        slot = ((key * multiplier) & 0xFFFFFFFF) >> shift
        if slot >= SLOTS:
            return None
        probe = 0
        while table[slot] not in (0, key):
            slot = (slot + 1) % SLOTS
            probe += 1
            if probe > MAX_PROBE:
                return None
        table[slot] = key
        farthest = max(farthest, probe)
    for key in keys:
        slot = ((key * multiplier) & 0xFFFFFFFF) >> shift
        while table[slot] not in (0, key):
            slot = (slot + 1) % SLOTS
        if table[slot] != key:
            return None
    return table, farthest


def main() -> int:
    keys = collect()
    masked = [k for k in keys if (k & 0x40000000) != 0]
    if masked:
        sys.exit(
            "gen_edge_intercept_table: %d key(s) carry the 0x40000000 mirror bit, "
            "so a caller that canonicalises before testing would need its own "
            "lookup: %s" % (len(masked), ", ".join("0x%08X" % k for k in masked[:4]))
        )
    best = None
    for multiplier in MULTIPLIERS:
        for shift in range(20, 33):
            built = build(keys, multiplier, shift)
            if built is None:
                continue
            table, farthest = built
            if best is None or farthest < best[0]:
                best = (farthest, multiplier, shift, table)
    if best is None:
        sys.exit(
            "gen_edge_intercept_table: no placement over %d keys in %d slots "
            "within %d probes; widen MULTIPLIERS or SLOTS."
            % (len(keys), SLOTS, MAX_PROBE)
        )
    farthest, multiplier, shift, table = best
    out = []
    out.append("/* Generated by scripts/gen_edge_intercept_table.py - do not edit.")
    out.append(" *")
    out.append(" * An open-addressed table over the edge intercept addresses. The lookup")
    out.append(" * walks a short probe chain and stops at the first empty slot, which is")
    out.append(" * why it cannot answer false for a key the switches match - an empty slot")
    out.append(" * means the key was never placed. It is built from the same case lists the")
    out.append(" * switches use; a hit only skips the switches, which stay the authority.")
    out.append(" * The placement is chosen for the longest probe, and the generator")
    out.append(" * verifies every key is found again after placing it. */")
    out.append("#ifndef BLUEWAKE_EDGE_INTERCEPT_TABLE_H")
    out.append("#define BLUEWAKE_EDGE_INTERCEPT_TABLE_H")
    out.append("")
    out.append("#include \"core/cpu.h\"")
    out.append("")
    out.append("#define BLUEWAKE_EDGE_KEY_COUNT %du" % len(keys))
    out.append("#define BLUEWAKE_EDGE_KEY_MULT %uu" % multiplier)
    out.append("#define BLUEWAKE_EDGE_KEY_SHIFT %du" % shift)
    out.append("#define BLUEWAKE_EDGE_KEY_SLOTS %du" % SLOTS)
    out.append("static const u32 g_edge_keys[BLUEWAKE_EDGE_KEY_SLOTS] = {")
    for row in range(0, SLOTS, 8):
        cells = ["0x%08Xu" % v if v else "0u" for v in table[row:row + 8]]
        out.append("    " + ", ".join(cells) + ",")
    out.append("};")
    out.append("")
    out.append("/* The key set the table is a permutation of, in ascending order.")
    out.append(" * A test asserts the lookup finds every one of these, which is the")
    out.append(" * check that catches a header whose table and hash parameters")
    out.append(" * disagree: a fast reject that answers false for a key the switches")
    out.append(" * would match is a boot failure, not a slowdown. */")
    out.append("static const u32 g_edge_keys_all[BLUEWAKE_EDGE_KEY_COUNT] = {")
    for row in range(0, len(keys), 8):
        out.append("    " + ", ".join("0x%08Xu" % v for v in keys[row:row + 8]) + ",")
    out.append("};")
    out.append("")
    out.append("/* Probe until the address is found or an empty slot ends the chain. */")
    out.append("static inline bool bluewake_edge_maybe_intercept(u32 address) {")
    out.append("    u32 slot = (address * BLUEWAKE_EDGE_KEY_MULT) >>")
    out.append("               BLUEWAKE_EDGE_KEY_SHIFT;")
    out.append("    for (;;) {")
    out.append("        const u32 key = g_edge_keys[slot];")
    out.append("        if (key == address)")
    out.append("            return true;")
    out.append("        if (key == 0u)")
    out.append("            return false;")
    out.append("        slot = (slot + 1u) % BLUEWAKE_EDGE_KEY_SLOTS;")
    out.append("    }")
    out.append("}")
    out.append("")
    out.append("#endif")
    Path(EDGE).with_name("edge_intercept_table.h").write_text(
        "\n".join(out) + "\n", encoding="utf-8")
    sys.stderr.write(
        "gen_edge_intercept_table: wrote edge_intercept_table.h with %d keys, "
        "mult %u shift %d, longest probe %d\n"
        % (len(keys), multiplier, shift, farthest)
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
