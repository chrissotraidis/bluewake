#!/usr/bin/env python3
"""Write a transformed copy of a generated chunk for a roofline ablation.

Why this exists. The emitted body costs about 30 host instructions per guest
cycle, and piecewise trimming has failed eleven times to move it. Those failures
were measured by rebuilding and re-running the route. This measures the same
question on one chunk in seconds, by deleting a named construct from the emitted
C and re-running the real-state sweep.

Each ablation is a price, not a patch: several of them are not semantically
safe, because the point is to bound what a leaner emitter could take, not to
propose an edit. The one exception is "restrict", which is safe - the generated
code's ctx genuinely does not alias the guest memory reached through it - and it
is the one that tests the register-file hypothesis, because it lets the compiler
hold ctx fields such as the guest registers across the memory helpers.

Ablations (comma separated):
  restrict     mark the ctx parameter __restrict in every emitted function
  no-pc        drop the per-instruction ctx->pc materialisation
  no-guard     drop the downcount budget tests (charges are kept)
  no-suffix    drop the cycle-observation suffix stores and reconciles
  prepaid      force cycle_block_prepaid true, removing the precharge call
  unchecked    replace the memory helpers with a bounds-only MEM1 fast path
  cheap-budget test downcount + cycle_budget <= 0 instead of the budget macro,
               equivalent while cycle_budget >= 1
  precharge-lean drop the redundant "remaining >= 0" inside can_precharge,
               equivalent for a block of one cycle or more
  prepaid-decision keep the block-entry precharge decision and force the body down
               the prepaid path anyway, which is the executed shape of the
               block-entry design minus its duplicated precise twin
  expect       hint the compiler that the charge and return paths are cold
  charge-helper move the per-instruction precise charge out of line, so the
               hot path keeps the test but not the dead charge body

Usage: ablate_chunk.py <source> <output> <ablation[,ablation...]|none>
"""

import os
import re
import sys

GUARD_TESTS = [
    re.compile(
        r"^    if \(ctx->downcount <= -\(s64\)DOLRECOMP_C_LOOP_CYCLE_BUDGET\) \{\n"
        r"        ctx->pc = 0x[0-9A-F]{8}u;\n"
        r"        return;\n"
        r"    \}\n",
        re.MULTILINE,
    ),
    re.compile(
        r"^        if \(ctx->downcount <= -\(s64\)DOLRECOMP_C_LOOP_CYCLE_BUDGET\) \{\n"
        r"            ctx->pc = 0x[0-9A-F]{8}u;\n"
        r"            return;\n"
        r"        \}\n",
        re.MULTILINE,
    ),
]

PREPAID_CALL = re.compile(
    r"cycle_block_prepaid = dolrecomp_block_can_precharge\(ctx, [0-9]+u\);",
)

# The block-entry design of docs/archive/GOAL_PROMPT_V52_2026-09-22.md, item 2: decide at
# entry whether no interrupt can fall inside the block, and if so drop every
# per-instruction charge test. What the design still pays that "prepaid" does not
# is the decision itself, so this keeps the call and forces the flag anyway - the
# executed shape, without the duplicated precise twin the design also emits.
PREPAID_KEEP_DECISION = re.compile(
    r"cycle_block_prepaid = dolrecomp_block_can_precharge\((ctx, [0-9]+u)\);",
)


# dolrecomp_loop_cycle_budget() is "cycle_budget > 0 ? cycle_budget : 256", so
# every budget test pays a load, a compare and a select. The host guarantees
# cycle_budget >= 1 (bounded_budget clamps to 1 and begin_turn sets 1), so the
# test is the same comparison against the field itself.
BUDGET_TEST = re.compile(
    r"ctx->downcount <= -\(s64\)DOLRECOMP_C_LOOP_CYCLE_BUDGET\)",
)

# can_precharge ends "remaining >= 0 && (u64)remaining >= (u64)block_cycles".
# For block_cycles >= 1 the first arm is implied by the second, so a signed
# compare against the block cost is the same test.
PRECHARGE = re.compile(
    r"return remaining >= 0 && \(u64\)remaining >= \(u64\)block_cycles;",
)

EXPECT_NEEDLES = [
    ("if (!cycle_block_prepaid) {", "if (__builtin_expect(!cycle_block_prepaid, 0)) {"),
    (
        "if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {",
        "if (__builtin_expect(ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET, 0)) {",
    ),
    (
        "if (ctx->downcount + (s64)ctx->cycle_budget <= 0) {",
        "if (__builtin_expect(ctx->downcount + (s64)ctx->cycle_budget <= 0, 0)) {",
    ),
]

# The non-leader charge site, in the shipping text. Moved out of line it still
# tests the flag on the hot path, but the six-instruction charge body is only
# reachable through a call.
CHARGE_SITE = re.compile(
    r"    if \(!cycle_block_prepaid\) \{\n"
    r"        if \(ctx->downcount <= -\(s64\)DOLRECOMP_C_LOOP_CYCLE_BUDGET\) \{\n"
    r"            ctx->pc = 0x([0-9A-F]{8})u;\n"
    r"            return;\n"
    r"        \}\n"
    r"        ctx->downcount -= ([0-9]+)u;\n"
    r"    \}\n",
)

CHARGE_HELPER = '''
__attribute__((noinline))
static bool bw_charge(CPUState* ctx, u32 cycles, u32 resume) {
    if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {
        ctx->pc = resume;
        return false;
    }
    ctx->downcount -= (s64)cycles;
    return true;
}
'''

PC_STORE = re.compile(r"^    ctx->pc = 0x[0-9A-F]{8}u;\n", re.MULTILINE)

SUFFIX_STORE = re.compile(
    r"^\s*ctx->cycle_observation_suffix = [^;]*;\n", re.MULTILINE
)

RECONCILE = re.compile(
    r"^\s*if \(cycle_block_prepaid &&\n"
    r"\s*ctx->cycle_deadline_budget > 0 &&\n"
    r"\s*\(s64\)ctx->cycle_observation_suffix > ctx->cycle_deadline_budget\) \{\n"
    r"\s*ctx->downcount \+= \(s64\)ctx->cycle_observation_suffix;\n"
    r"\s*cycle_block_prepaid = false;\n"
    r"\s*\}\n",
    re.MULTILINE,
)

UNCHECKED_PRELUDE = '''
/* Ablation: bounds-only MEM1 fast path, no alias flag and no mirror test. */
#define BW_ABL_FAST(c, ea, size) \
    (((u32)((ea) - GC_RAM_BASE) <= (c)->ram_size - (size)) ? \
         ((c)->ram + ((ea) - GC_RAM_BASE)) : NULL)
static inline u32 bw_abl_read32(CPUState* c, u32 ea) {
    u8* p = BW_ABL_FAST(c, ea, 4u);
    return (p != NULL) ? read_be32(p) : mem_read32(c, ea);
}
static inline void bw_abl_write32(CPUState* c, u32 ea, u32 v) {
    u8* p = BW_ABL_FAST(c, ea, 4u);
    if (p != NULL) write_be32(p, v); else mem_write32(c, ea, v);
}
static inline u8 bw_abl_read8(CPUState* c, u32 ea) {
    u8* p = BW_ABL_FAST(c, ea, 1u);
    return (p != NULL) ? (u8)*p : mem_read8(c, ea);
}
static inline void bw_abl_write8(CPUState* c, u32 ea, u8 v) {
    u8* p = BW_ABL_FAST(c, ea, 1u);
    if (p != NULL) *p = v; else mem_write8(c, ea, v);
}
static inline u64 bw_abl_read64(CPUState* c, u32 ea) {
    u8* p = BW_ABL_FAST(c, ea, 8u);
    return (p != NULL) ? read_be64(p) : mem_read64(c, ea);
}
static inline void bw_abl_write64(CPUState* c, u32 ea, u64 v) {
    u8* p = BW_ABL_FAST(c, ea, 8u);
    if (p != NULL) write_be64(p, v); else mem_write64(c, ea, v);
}
'''

MEM_SWAPS = [
    ("mem_read32(", "bw_abl_read32("),
    ("mem_write32(", "bw_abl_write32("),
    ("mem_read8(", "bw_abl_read8("),
    ("mem_write8(", "bw_abl_write8("),
    ("mem_read64(", "bw_abl_read64("),
    ("mem_write64(", "bw_abl_write64("),
]


def apply(text: str, names: list) -> str:
    if "restrict" in names:
        text = text.replace("(CPUState* ctx) {", "(CPUState* __restrict ctx) {")
    if "no-pc" in names:
        text = PC_STORE.sub("", text)
    if "no-guard" in names:
        for pattern in GUARD_TESTS:
            text = pattern.sub("", text)
    if "no-suffix" in names:
        text = SUFFIX_STORE.sub("", text)
        text = RECONCILE.sub("", text)
    if "prepaid" in names:
        text = PREPAID_CALL.sub("cycle_block_prepaid = true;", text)
    if "prepaid-decision" in names:
        text = PREPAID_KEEP_DECISION.sub(
            r"(void)dolrecomp_block_can_precharge(\1);\n    cycle_block_prepaid = true;",
            text,
        )
    if "cheap-budget" in names:
        text = BUDGET_TEST.sub(
            "ctx->downcount + (s64)ctx->cycle_budget <= 0)", text
        )
    if "precharge-lean" in names:
        text = PRECHARGE.sub(
            "return ctx->cycle_deadline_budget + ctx->downcount >= (s64)block_cycles;",
            text,
        )
    if "expect" in names:
        for needle, replacement in EXPECT_NEEDLES:
            text = text.replace(needle, replacement)
    if "charge-helper" in names:
        anchor = '#include "../generated.h"\n'
        text = text.replace(anchor, anchor + CHARGE_HELPER, 1)
        text = CHARGE_SITE.sub(
            r"    if (!cycle_block_prepaid && !bw_charge(ctx, \2u, 0x\1u)) return;\n",
            text,
        )
    if "unchecked" in names:
        anchor = '#include "../generated.h"\n'
        if anchor not in text:
            raise SystemExit("ablate_chunk: generated.h include not found")
        text = text.replace(anchor, anchor + UNCHECKED_PRELUDE, 1)
        for src, dst in MEM_SWAPS:
            text = text.replace(src, dst)
    return text


def main() -> int:
    if len(sys.argv) != 4:
        sys.stderr.write(__doc__)
        return 2
    source_path, output_path, spec = sys.argv[1], sys.argv[2], sys.argv[3]
    names = [] if spec == "none" else [n for n in spec.split(",") if n]
    with open(source_path, "r", encoding="utf-8") as handle:
        text = handle.read()
    before = len(text)
    text = apply(text, names)
    # The transform writes to a temp directory, so the chunk's relative include
    # has to become absolute. This runs after apply(), which anchors on the
    # original relative include.
    generated = os.path.abspath(os.path.join(os.path.dirname(source_path), "..", "generated.h"))
    text = text.replace('#include "../generated.h"', '#include "%s"' % generated)
    with open(output_path, "w", encoding="utf-8") as handle:
        handle.write(text)
    sys.stderr.write(
        "ablate_chunk: %s -> %s (%d -> %d bytes)\n"
        % (spec, output_path, before, len(text))
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
