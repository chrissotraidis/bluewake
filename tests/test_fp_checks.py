#!/usr/bin/env python3
"""scripts/windows/fp_checks.py on prepaid copies as fast_blocks.py writes them."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts/windows"))
import fp_checks  # noqa: E402
import lean_memory  # noqa: E402

HEAD = "/* bluewake: lean prepaid block copies (scripts/windows/fast_blocks.py --lean) */\n"


def test_line(pc):
    return f"    if (!ppc_fp_available_inline(ctx, 0x{pc}u)) return;"


def fadds(pc, d):
    return [f"    ctx->pc = 0x{pc}u;",
            f"    // {pc}: fadds   f{d}, f1, f2",
            test_line(pc),
            f"    ppc_fadds(ctx, {d}, 1, 2);",
            ""]


def lfs(pc, d, suffix, resume):
    return [f"    ctx->pc = 0x{pc}u;",
            f"    ctx->cycle_observation_suffix = {suffix}u;",
            f"    // {pc}: lfs     f{d}, 8(r3)",
            test_line(pc),
            "    {",
            "        u32 ea = ctx->gpr[3] + (u32)(s32)(8);",
            "        f64 value = dolrecomp_f32_from_bits(mem_read32(ctx, ea));",
            f"        ctx->fpr[{d}] = value;",
            f"        ctx->ps1[{d}] = value;",
            "    }",
            "",
            "    if (ctx->cycle_deadline_budget > 0 &&",
            "        (s64)ctx->cycle_observation_suffix > ctx->cycle_deadline_budget) {",
            "        ctx->downcount += (s64)ctx->cycle_observation_suffix;",
            "        cycle_block_prepaid = false;",
            f"        goto {resume};",
            "    }"]


def copy(*parts, n=0):
    lines = [f"bwfast_{n}:"]
    for part in parts:
        lines.extend(part)
    lines.append(f"    goto bwend_{n};")
    return lines


def chunk(*copies, original=()):
    body = ["void func_80001000(CPUState* ctx) {", *original, "    return;"]
    for c in copies:
        body.extend(c)
    body.append("}")
    return HEAD + "\n".join(body) + "\n"


class FpChecks(unittest.TestCase):
    def test_first_test_stays_later_ones_go(self):
        text = chunk(copy(fadds("80001000", 3), fadds("80001004", 4), fadds("80001008", 5)))
        out, removed = fp_checks.transform(text)
        self.assertEqual(removed, 2)
        self.assertIn(test_line("80001000"), out)
        self.assertNotIn(test_line("80001004"), out)
        self.assertNotIn(test_line("80001008"), out)
        # Everything else, the pc stores included, is the copy's own text.
        self.assertEqual(out.count("ctx->pc = 0x8000100"), 3)
        self.assertIn(fp_checks.MARK, out)

    def test_a_call_that_can_run_guest_code_starts_a_new_stretch(self):
        for call in ("    if (bw_direct_call_ready(ctx, 0x80004000u)) {",
                     "    loop_80001010(ctx);",
                     "    ppc_rfi(ctx);",
                     "    if (bluewake_native_game_math_try(ctx)) return;"):
            with self.subTest(call=call):
                text = chunk(copy(fadds("80001000", 3), [call], fadds("80001004", 4), fadds("80001008", 5)))
                out, removed = fp_checks.transform(text)
                self.assertEqual(removed, 1)
                self.assertIn(test_line("80001000"), out)
                self.assertIn(test_line("80001004"), out)
                self.assertNotIn(test_line("80001008"), out)

    def test_memory_and_fp_helpers_do_not(self):
        text = chunk(copy(fadds("80001000", 3),
                          ["        ctx->gpr[5] = bw_read32_ato(&bw_ool, ctx, 0x80001004u, 3u, ea);",
                           "    ppc_fcmp(ctx, 0, ctx->fpr[1], ctx->fpr[2], true);",
                           "    if (!ppc_psq_load_inline(ctx, 2u, ea, false, 0u, false, 0x80001004u)) {}"],
                          fadds("80001008", 4)))
        _, removed = fp_checks.transform(text)
        self.assertEqual(removed, 1)

    def test_original_blocks_and_other_copies_keep_their_tests(self):
        original = fadds("80001000", 3) + fadds("80001004", 4)
        text = chunk(copy(fadds("80001000", 3)), copy(fadds("80001004", 4), n=1), original=original)
        out, removed = fp_checks.transform(text)
        self.assertEqual(removed, 0)  # each copy's only test is its first
        self.assertEqual(out, text)

    def test_repeatable_and_needs_prepaid_copies(self):
        text = chunk(copy(fadds("80001000", 3), fadds("80001004", 4)))
        once, _ = fp_checks.transform(text)
        twice, removed = fp_checks.transform(once)
        self.assertEqual((twice, removed), (once, 0))
        plain = text.replace(HEAD, "")
        self.assertEqual(fp_checks.transform(plain), (plain, 0))

    def test_unsafe_names(self):
        for name in ("ppc_fallback_instruction", "ppc_fp_available", "ppc_rfi", "ppc_mtmsr",
                     "bw_direct_call_ready", "bw_call_translated", "bw_native_call", "loop_80001010"):
            self.assertFalse(fp_checks.safe(name), name)
        for name in ("ppc_fadds", "ppc_fmul", "ppc_fcmp", "ppc_fctiw", "ppc_frsqrte", "ppc_ps_madds0",
                     "ppc_psq_load_inline", "bw_read32_ato", "mem_write64", "dolrecomp_f32_from_bits"):
            self.assertTrue(fp_checks.safe(name), name)

    def test_a_later_fp_load_becomes_lean(self):
        text = chunk(copy(lfs("80001000", 1, 4, "bwslow_0_1"), lfs("80001004", 2, 3, "bwslow_0_2"),
                          fadds("80001008", 3)))
        prepared, removed = fp_checks.transform(text)
        self.assertEqual(removed, 2)
        lean, accesses = lean_memory.transform(prepared)
        self.assertEqual(accesses, 1)
        # The first load keeps its test and its plain access; the second is lean.
        self.assertIn(test_line("80001000"), lean)
        self.assertIn("dolrecomp_f32_from_bits(mem_read32(ctx, ea))", lean)
        self.assertIn("dolrecomp_f32_from_bits(bw_read32_at(ctx, 0x80001004u, 3u, ea))", lean)
        # Without fp_checks.py neither load qualifies.
        _, before = lean_memory.transform(text)
        self.assertEqual(before, 0)


if __name__ == "__main__":
    unittest.main()
