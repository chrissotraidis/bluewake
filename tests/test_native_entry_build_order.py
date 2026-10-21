#!/usr/bin/env python3
"""Keep native-entry certification after all translated-source rewrites."""

import ast
import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "native_entries", ROOT / "scripts/windows/native_entries.py")
native_entries = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(native_entries)


def prepare_blocks_source(builder):
    path = ROOT / builder
    source = path.read_text()
    tree = ast.parse(source)
    for node in ast.walk(tree):
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and node.name == "prepare_blocks":
            return ast.get_source_segment(source, node)
    raise AssertionError(f"prepare_blocks not found in {builder}")


class NativeEntryBuildOrderTest(unittest.TestCase):
    def test_groundcross_follows_timing_without_moving_existing_entries(self):
        for builder in ("scripts/linux/build.py", "scripts/windows/build.py"):
            with self.subTest(builder=builder):
                body = prepare_blocks_source(builder)
                timing = body.index('self.run("behaviour-timing"')
                entries = body.index('self.run("native-entries"')
                groundcross = body.index('self.run("native-groundcross"')
                digest = body.index('digest = tree_digest')
                self.assertLess(entries, timing)
                self.assertLess(timing, groundcross)
                self.assertLess(groundcross, digest)

    def test_only_mode_preserves_hooks_from_the_earlier_pass(self):
        first = 0x802D8BD8
        second = 0x802D8C58
        chunk = 0x802D56E0
        text = ('#include "../generated.h"\n' + native_entries.MARK +
                '#include "native_fifo.h"\n\n' +
                f'label_{first:08X}:\n' + native_entries.hook_text(first, chunk) +
                f'label_{second:08X}:\n' + native_entries.hook_text(second, chunk) +
                f'return_dispatch_{chunk:08X}:\n')
        converted, added = native_entries.transform(text, chunk, {first: "first"}, {first})
        self.assertEqual(added, 0)
        self.assertIn(native_entries.hook_text(first, chunk), converted)
        self.assertIn(native_entries.hook_text(second, chunk), converted)


if __name__ == "__main__":
    unittest.main()
