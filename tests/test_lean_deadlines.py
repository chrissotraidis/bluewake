#!/usr/bin/env python3
"""scripts/windows/lean_deadlines.py on a prepaid copy written as lean_memory.py leaves it."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts/windows"))
import lean_deadlines  # noqa: E402

HEAD = ("/* bluewake: lean prepaid block copies (scripts/windows/fast_blocks.py --lean) */\n"
        + lean_deadlines.LEAN_MEMORY_MARK)


def access_test(suffix, pc, target):
    return [
        "    if (ctx->cycle_deadline_budget > 0 &&",
        f"        (s64){suffix}u > ctx->cycle_deadline_budget) {{",
        f"        ctx->pc = 0x{pc}u;",
        f"        ctx->cycle_observation_suffix = {suffix}u;",
        f"        ctx->downcount += (s64){suffix}u;",
        "        cycle_block_prepaid = false;",
        f"        goto {target};",
        "    }",
    ]


STORED_TEST = lean_deadlines.STORED_TEST + ["        goto bwslow_0_3;", "    }"]


def copy(*middle):
    lines = ["bwfast_0:"]
    for part in middle:
        lines.extend(part)
    lines.append("    goto bwend_0;")
    return HEAD + "\n".join(lines) + "\n"


STORE = ["    // 80001000: stw     r0, 20(r1)",
         "    {",
         "        u32 ea = ctx->gpr[1] + (u32)(s32)(20);",
         "        bw_write32_at(ctx, 0x80001000u, 4u, ea, (u32)ctx->gpr[0]);",
         "    }"] + access_test(4, "80001000", "bwslow_0_1")
MFLR = ["    ctx->cycle_observation_suffix = 3u;",
        "    // 80001004: mflr    r0",
        "    ctx->gpr[0] = ctx->lr;",
        "",
        "    ctx->pc = 0x80001004u;"] + STORED_TEST
LOAD = ["    // 8000100C: lwz     r3, 8(r1)",
        "    {",
        "        u32 ea = ctx->gpr[1] + (u32)(s32)(8);",
        "        ctx->gpr[3] = bw_read32_at(ctx, 0x8000100Cu, 1u, ea);",
        "    }"] + access_test(1, "8000100C", "bwslow_0_4")
CALL = ["    ctx->pc = 0x80001008u;",
        "    // 80001008: dcbf    0, r3",
        "    ppc_dcbf(ctx, 0u, 3u);"]


class LeanDeadlines(unittest.TestCase):
    def test_access_test_only_out_of_line(self):
        out, count = lean_deadlines.transform(copy(STORE))
        self.assertEqual(count, 1)
        self.assertIn("bw_write32_ato(&bw_ool, ctx, 0x80001000u, 4u, ea,", out)
        self.assertIn("    if (__builtin_expect(bw_ool, 0) &&\n        ctx->cycle_deadline_budget > 0 &&\n"
                      "        (s64)4u > ctx->cycle_deadline_budget) {", out)
        self.assertIn(lean_deadlines.MARK, out)

    def test_register_move_drops_its_test(self):
        out, count = lean_deadlines.transform(copy(STORE, MFLR))
        self.assertEqual(count, 2)
        self.assertNotIn("ctx->cycle_observation_suffix = 3u;", out)
        self.assertNotIn("(s64)ctx->cycle_observation_suffix > ctx->cycle_deadline_budget", out)
        self.assertIn("    ctx->gpr[0] = ctx->lr;", out)
        self.assertIn("    ctx->pc = 0x80001004u;", out)  # the pc store stays

    def test_a_call_keeps_the_next_test(self):
        out, count = lean_deadlines.transform(copy(STORE, CALL, MFLR, LOAD))
        # The store is lean; the call may move the deadline, so the mflr keeps its
        # test, which then vouches for the load after it.
        self.assertEqual(count, 2)
        self.assertIn("ctx->cycle_observation_suffix = 3u;", out)
        self.assertIn("(s64)ctx->cycle_observation_suffix > ctx->cycle_deadline_budget", out)
        self.assertIn("bw_read32_ato(&bw_ool, ctx, 0x8000100Cu, 1u, ea)", out)

    def test_a_call_keeps_an_access_test(self):
        out, count = lean_deadlines.transform(copy(CALL, LOAD))
        self.assertEqual(count, 0)
        self.assertNotIn("_ato(", out)

    def test_repeatable_and_needs_lean_memory(self):
        once, _ = lean_deadlines.transform(copy(STORE, MFLR, LOAD))
        self.assertEqual(lean_deadlines.transform(once), (once, 0))
        plain = copy(STORE).replace(lean_deadlines.LEAN_MEMORY_MARK, "")
        self.assertEqual(lean_deadlines.transform(plain), (plain, 0))


if __name__ == "__main__":
    unittest.main()
