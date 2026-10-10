"""scripts/windows/return_ranges.py: the return dispatch's range test."""
import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("return_ranges", ROOT / "scripts/windows/return_ranges.py")
return_ranges = importlib.util.module_from_spec(spec)
spec.loader.exec_module(return_ranges)

CHUNK = "\n".join([
    '#include "../generated.h"',
    "void chunk(CPUState* ctx) {",
    "label_80001000:",
    "    ctx->gpr[3] = 1;",
    "label_80001010:",
    "    return;",
    "return_dispatch_80001000:",
    "    if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) return;",
    "    switch (ctx->pc) {",
    "    case 0x80001010u: goto label_80001010;",
    "    case 0x80001000u: goto label_80001000;",
    "    default: return;",
    "    }",
    "}",
    "",
])


class ReturnRangesTest(unittest.TestCase):
    def test_range_test_before_the_switch(self):
        text, count = return_ranges.transform(CHUNK)
        self.assertEqual(count, 1)
        lines = text.split("\n")
        at = lines.index("    if ((u32)(ctx->pc - 0x80001000u) > 0x00000010u) return;")
        self.assertEqual(lines[at - 1], "    if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) return;")
        self.assertEqual(lines[at + 1], "    switch (ctx->pc) {")
        self.assertIn(return_ranges.MARK, text)
        # Everything else is unchanged.
        self.assertEqual(text.replace(return_ranges.MARK, "").replace(lines[at] + "\n", ""), CHUNK)

    def test_repeatable(self):
        once, _ = return_ranges.transform(CHUNK)
        self.assertEqual(return_ranges.transform(once), (once, 0))

    def test_a_dispatch_it_does_not_recognise_is_left_alone(self):
        odd = CHUNK.replace("    default: return;", "    default: ctx->pc = 0; return;")
        self.assertEqual(return_ranges.transform(odd), (odd, 0))


if __name__ == "__main__":
    unittest.main()

