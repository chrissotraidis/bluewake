#!/usr/bin/env python3
"""Run the register save and restore routines inline at their call sites.

  inline_save_restore_gpr.py COMPOSITE_SRC

Almost every function the game's compiler wrote saves its non-volatile
registers by calling _savegpr_N and restores them by calling _restgpr_N: two
straight runs of stw and lwz off r11 at 0x80328F04 and 0x80328F50, ending in
blr. They are in one chunk and nearly all of their 6,901 callers are in
others, so each call leaves the caller's chunk for the chassis loop, enters
the routine's chunk, and leaves it again to come back: two block boundaries,
two edge-service calls and two chunk entries for at most 18 stores. Over
Outset play they are 10 percent of all block boundaries (the boundary census,
BLUEWAKE_BOUNDARY_CENSUS_BY_ADDRESS).

At each such call this writes the routine's stores (or loads) where the call
was and continues at the return address in the caller's chunk, as the
translator already does for a call into the same chunk. It does so only when
the result is the one the translated routine gives:

- cycles: entered anywhere but its first instruction, the routine charges each
  instruction on its own (dolrecomp_charge_precise), which stops at the loop
  budget. The inline form runs when none of those charges would stop and
  charges the same total; entered at r14 it is a block that prepays 19 cycles,
  and runs inline when that prepayment is certain and no deadline refund could
  happen inside it (no deadline, or one of 18 cycles or more).
- the observation suffix the routine leaves: 0, or 1 after the prepaid block.
- the chassis's own stop after the return (an exception, or the turn's budget
  spent) ends the turn at the return address, as the chassis would.
Otherwise the call goes out as before. A call whose return address the host
watches (any guest address named in runtime/host/src or windows/src, in
either mirror form) is left alone, so every host hook still sees its boundary.

The change is repeatable (a prepared chunk is left as it is) and keeps LF line
ends. Run it after scripts/windows/global_guest_cpu.py and before
scripts/mods/prepare_simulation_60hz.py, whose manifest hashes the chunks as
they finally are.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MARK = "/* bluewake: _savegpr/_restgpr inline (scripts/windows/inline_save_restore_gpr.py) */\n"
SAVE, RESTORE = 0x80328F04, 0x80328F50
CALL = re.compile(
    r"    // ([0-9A-F]{8}): bl      0x(80328F[0-9A-F]{2})\n"
    r"    \{\n"
    r"            ctx->lr = 0x([0-9A-F]{8})u;\n"
    r"            ctx->pc = 0x\2u;\n"
    r"            return;\n"
    r"    \}\n")
FUNCTION = re.compile(r"^(?:static )?void \w+\(CPUState\* ctx_param\) \{$", re.M)
BUDGET = "-(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET"


def watched_addresses():
    """Guest addresses the host names: a boundary at any of them may matter.
    Both mirror forms of each: the edge service tests a boundary's address
    with the 0x40000000 bit cleared (host_canonical_linked_pc), so a REL
    chunk's 0xC1E01B88 is its 0x81E01B88."""
    found = set()
    for folder in ("runtime/host/src", "windows/src"):
        for path in (ROOT / folder).rglob("*"):
            if path.suffix in (".c", ".h", ".cpp", ".mm", ".m"):
                for m in re.finditer(r"0x([8C][0-9A-Fa-f]{7})u?\b", path.read_text(errors="replace")):
                    address = int(m.group(1), 16)
                    found.update((address, address | 0x40000000, address & ~0x40000000))
    return found


def inline_body(target, ret):
    """The routine entered at `target`, then its blr to `ret`, as one statement."""
    save = SAVE <= target < SAVE + 18 * 4
    first = 14 + (target - (SAVE if save else RESTORE)) // 4
    count = 32 - first + 1  # the stores or loads, and the blr
    lines = [f"            {MARK.strip()}"]
    if first == 14:
        # The routine's first instruction leads a block that prepays it all.
        lines.append(f"            if (ctx->downcount > {BUDGET} &&")
        lines.append("                (ctx->cycle_deadline_budget <= 0 ||")
        lines.append("                 (ctx->cycle_deadline_budget >= 18 &&")
        lines.append(f"                  dolrecomp_block_can_precharge(ctx, {count}u)))) {{")
        suffix = 1
    else:
        lines.append(f"            if (ctx->downcount - {count - 1} > {BUDGET}) {{")
        suffix = 0
    lines.append(f"                ctx->downcount -= {count};")
    lines.append("                const u32 frame = ctx->gpr[11];")
    for reg in range(first, 32):
        offset = 4 * reg - 128
        if save:
            lines.append(f"                mem_write32(ctx, frame + (u32)(s32)({offset}), (u32)ctx->gpr[{reg}]);")
        else:
            lines.append(f"                ctx->gpr[{reg}] = mem_read32(ctx, frame + (u32)(s32)({offset}));")
    lines.append(f"                ctx->cycle_observation_suffix = {suffix}u;")
    lines.append(f"                ctx->pc = 0x{ret:08X}u;")
    # The routine's return dispatch, then the chassis's checks at the boundary.
    lines.append(f"                if (ctx->downcount <= {BUDGET} || ctx->exception != 0u ||")
    lines.append("                    (ctx->cycle_budget > 0 && ctx->downcount <= -ctx->cycle_budget))")
    lines.append("                    return;")
    lines.append(f"                goto label_{ret:08X};")
    lines.append("            }")
    return "\n".join(lines) + "\n"


def transform(text, watched):
    if MARK in text:
        return text, 0
    starts = [m.start() for m in FUNCTION.finditer(text)] + [len(text)]
    out, done, last = [], 0, 0
    for begin, end in zip(starts, starts[1:]):
        body = text[begin:end]
        pieces, cursor = [], 0
        for m in CALL.finditer(body):
            site, target, ret = int(m.group(1), 16), int(m.group(2), 16), int(m.group(3), 16)
            routine = SAVE <= target < SAVE + 18 * 4 or RESTORE <= target < RESTORE + 18 * 4
            if (not routine or ret != site + 4 or ret in watched or site in watched
                    or f"\nlabel_{ret:08X}:\n" not in body):
                continue
            pieces.append(body[cursor:m.start()])
            pieces.append(
                f"    // {m.group(1)}: bl      0x{m.group(2)}\n"
                "    {\n"
                f"            ctx->lr = 0x{m.group(3)}u;\n"
                + inline_body(target, ret) +
                f"            ctx->pc = 0x{m.group(2)}u;\n"
                "            return;\n"
                "    }\n")
            cursor = m.end()
            done += 1
        pieces.append(body[cursor:])
        out.append(text[last:begin])
        out.append("".join(pieces))
        last = end
    out.append(text[last:])
    return "".join(out), done


def main():
    root = Path(sys.argv[1])
    chunks = sorted(root.glob("chunks_*/*.c"))
    if not chunks:
        sys.exit(f"no chunks under {root}")
    watched = watched_addresses()
    sites = files = 0
    for path in chunks:
        with open(path, encoding="utf-8", newline="") as file:
            original = file.read()
        converted, count = transform(original, watched)
        if count:
            temporary = path.with_suffix(".c.tmp")
            with open(temporary, "w", encoding="utf-8", newline="") as file:
                file.write(converted)
            temporary.replace(path)
            sites += count
            files += 1
    print(f"register save and restore inline: {sites} calls in {files} chunks")


if __name__ == "__main__":
    main()
