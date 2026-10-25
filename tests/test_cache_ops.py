#!/usr/bin/env python3
"""scripts/windows/cache_ops.py: the four cache opcodes become the pc store."""
import contextlib
import importlib.util
import io
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cache_ops", ROOT / "scripts/windows/cache_ops.py")
cache_ops = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cache_ops)


def opcode(primary, xo):
    return (primary << 26) | (xo << 1)


class CacheOpsTest(unittest.TestCase):
    def test_four_cache_opcodes_become_the_pc_store_and_a_second_run_changes_nothing(self):
        sites = (
            (54, 0x80001000, "dcbst"),
            (86, 0x80001020, "dcbf"),
            (470, 0x80001040, "dcbi"),
            (982, 0x80001060, "icbi"),
        )
        other = opcode(31, 266)  # add: opcode 31, not one of the four
        lines = ['#include "../generated.h"', "void chunk(CPUState* ctx) {"]
        for xo, cia, _name in sites:
            lines.append(f"    ppc_fallback_instruction(ctx, 0x{opcode(31, xo):08X}u, 0x{cia:08X}u);")
        lines.append(f"    ppc_fallback_instruction(ctx, 0x{other:08X}u, 0x80001080u);")
        lines += ["}", ""]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "chunks_dol" / "chunk_0000_text1_80001000.c"
            path.parent.mkdir()
            path.write_text("\n".join(lines), newline="\n")
            import sys
            argv = sys.argv
            sys.argv = ["cache_ops.py", str(root)]
            try:
                first = io.StringIO()
                with contextlib.redirect_stdout(first):
                    cache_ops.main()
                once = path.read_bytes()
                second = io.StringIO()
                with contextlib.redirect_stdout(second):
                    cache_ops.main()
                twice = path.read_bytes()
            finally:
                sys.argv = argv
        text = once.decode()
        self.assertIn("4 in 1 chunks", first.getvalue())
        self.assertIn("0 in 0 chunks", second.getvalue())
        self.assertEqual(once, twice)
        for _xo, cia, name in sites:
            self.assertIn(
                f"ctx->pc = 0x{cia + 4:08X}u; /* {name}: the host's fallback only steps past it */",
                text)
        self.assertIn(f"ppc_fallback_instruction(ctx, 0x{other:08X}u, 0x80001080u);", text)
        self.assertEqual(text.count("ppc_fallback_instruction"), 1)


if __name__ == "__main__":
    unittest.main()
