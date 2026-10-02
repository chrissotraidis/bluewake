#!/usr/bin/env python3
"""Synthetic Windows build-cache checks; no disc or translated game source."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("windows_builder", REPO / "scripts/windows/build.py")
bw = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bw)
fast_spec = importlib.util.spec_from_file_location("fast_blocks", REPO / "scripts/windows/fast_blocks.py")
fast = importlib.util.module_from_spec(fast_spec)
fast_spec.loader.exec_module(fast)

# A small invented instruction block with the translator's public charge form.
CHUNK = '''#include "../generated.h"
void synthetic(CPUState* ctx) {
    bool cycle_block_prepaid;
    ctx->pc = 0x80001000u;
    cycle_block_prepaid = dolrecomp_block_can_precharge(ctx, 2u);
    if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {
        ctx->pc = 0x80001000u;
        return;
    }
    ctx->downcount -= cycle_block_prepaid ? 2u : 1u;
    ctx->gpr[3] += 1u;
    ctx->pc = 0x80001004u;
    if (!cycle_block_prepaid && !dolrecomp_charge_precise(ctx, 1u, 0x80001004u)) return;
    ctx->gpr[4] += 1u;
    ctx->pc = ctx->lr;
    return;
}
'''
MARK = "bluewake: prepaid block copies"


class PreparedBlockSelectionTest(unittest.TestCase):
    def test_retains_pc_and_prepaid_observation_suffix(self):
        source = CHUNK.replace("    ctx->gpr[4] += 1u;",
                               "    ctx->cycle_observation_suffix = cycle_block_prepaid ? 1u : 0u;\n"
                               "    ctx->gpr[4] += 1u;")
        converted, count = fast.transform(source)
        self.assertEqual(count, 1)
        copy = converted.split("bwfast_0:\n", 1)[1]
        self.assertIn("ctx->pc = 0x80001004u;", copy)
        self.assertIn("ctx->cycle_observation_suffix = 1u;", copy)
        self.assertNotIn("dolrecomp_charge_precise", copy)
        self.assertEqual(fast.transform(converted), (converted, 0))

    def test_refund_and_unknown_prepaid_forms_keep_original_body(self):
        refund = """    if (cycle_block_prepaid &&
        ctx->cycle_deadline_budget > 0 &&
        (s64)ctx->cycle_observation_suffix > ctx->cycle_deadline_budget) {
        ctx->downcount += (s64)ctx->cycle_observation_suffix;
        cycle_block_prepaid = false;
    }
"""
        for unsupported in (refund, "    cycle_block_prepaid = false;\n",
                            "    synthetic_observe(cycle_block_prepaid);\n"):
            with self.subTest(form=unsupported):
                source = CHUNK.replace("    ctx->gpr[4] += 1u;",
                                       unsupported + "    ctx->gpr[4] += 1u;")
                self.assertEqual(fast.transform(source), (source, 0))


class PreparedCacheTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for script in ("scripts/ios/composite_manifest.py", "scripts/windows/fast_blocks.py",
                       "scripts/windows/global_guest_cpu.py", "scripts/windows/chunk_headers.py",
                       "cmake/composite/inline_fp.h"):
            dst = self.root / script
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(REPO / script, dst)
        self.base = self.root / "synthetic-base"
        (self.base / "chunks_dol").mkdir(parents=True)
        (self.base / "generated.h").write_text("/* synthetic fixture */\n")
        for name in ("a.c", "b.c"):
            (self.base / "chunks_dol" / name).write_text(CHUNK)
        self.out = self.root / "build"
        self.out.mkdir()
        self.args = SimpleNamespace(out=self.out, accept_new_composite=False, prepared_blocks=False, fixed_cpu=False, fixed_mem1=False, inline_fp=False)
        self.builder = bw.Builder(self.args)
        self.builder.mods = False
        self.builder.composite = lambda *args: shutil.copytree(self.base, args[-2])
        self.addCleanup(patch.stopall)
        patch.object(bw, "ROOT", self.root).start()
        self.digest = bw.tree_digest(self.base)
        patch.object(bw, "profile_value", lambda name: self.digest).start()

    def cycle(self):
        self.builder.generate()
        self.builder.prepare_blocks()

    def chunk(self, name="a.c"):
        return self.out / "composite-src/chunks_dol" / name

    def test_explicit_enable_reuse_and_disable(self):
        self.cycle()
        self.assertNotIn(MARK, self.chunk().read_text())
        self.args.prepared_blocks = True
        self.cycle()
        self.assertIn(MARK, self.chunk().read_text())
        before = self.chunk().read_bytes(), self.chunk().stat().st_mtime_ns
        self.cycle()
        self.assertEqual(before, (self.chunk().read_bytes(), self.chunk().stat().st_mtime_ns))
        receipt = json.loads((self.out / "prepared-blocks.json").read_text())
        self.assertTrue(receipt["enabled"])
        self.assertEqual(receipt["final_digest"], bw.tree_digest(self.out / "composite-src"))
        self.args.prepared_blocks = False
        self.cycle()
        self.assertEqual(self.chunk().read_text(), CHUNK)
        self.assertFalse(json.loads((self.out / "prepared-blocks.json").read_text())["enabled"])

    def test_fixed_cpu_can_be_selected_combined_reused_and_disabled(self):
        self.cycle()
        self.args.fixed_cpu = True
        self.cycle()
        self.assertIn("#define ctx (&bw_guest_cpu)", self.chunk().read_text())
        self.assertNotIn(MARK, self.chunk().read_text())
        self.args.prepared_blocks = True
        self.cycle()
        self.assertIn("#define ctx (&bw_guest_cpu)", self.chunk().read_text())
        self.assertIn(MARK, self.chunk().read_text())
        before = self.chunk().read_bytes(), self.chunk().stat().st_mtime_ns
        self.cycle()
        self.assertEqual(before, (self.chunk().read_bytes(), self.chunk().stat().st_mtime_ns))
        self.assertTrue(json.loads((self.out / "prepared-blocks.json").read_text())["fixed_cpu"])
        script = self.root / "scripts/windows/global_guest_cpu.py"
        script.write_text(script.read_text() + "\n# synthetic CPU transform revision\n")
        self.builder.generate()
        self.assertEqual(self.chunk().read_text(), CHUNK)
        self.builder.prepare_blocks()
        self.assertIn("#define ctx (&bw_guest_cpu)", self.chunk().read_text())
        self.args.fixed_cpu = False
        self.cycle()
        self.assertNotIn("bw_guest_cpu", self.chunk().read_text())
        self.assertIn(MARK, self.chunk().read_text())
        self.args.prepared_blocks = False
        self.cycle()
        self.assertEqual(self.chunk().read_text(), CHUNK)

    def test_fixed_mem1_selection_is_recorded_and_invalidates_inputs(self):
        self.args.fixed_cpu = True
        self.cycle()
        prior = (self.out / "composite-inputs.digest").read_text()
        self.args.fixed_mem1 = True
        self.cycle()
        self.assertNotEqual(prior, (self.out / "composite-inputs.digest").read_text())
        self.assertTrue(json.loads((self.out / "prepared-blocks.json").read_text())["fixed_mem1"])
        before = self.chunk().read_bytes(), self.chunk().stat().st_mtime_ns
        self.cycle()
        self.assertEqual(before, (self.chunk().read_bytes(), self.chunk().stat().st_mtime_ns))
        self.args.fixed_mem1 = False
        self.cycle()
        self.assertFalse(json.loads((self.out / "prepared-blocks.json").read_text())["fixed_mem1"])
        self.assertIn("#define ctx (&bw_guest_cpu)", self.chunk().read_text())

    def test_inline_fp_independent_combined_reuse_disable_and_helper_changes(self):
        self.cycle()
        self.assertNotIn('"inline_fp.h"', self.chunk().read_text())
        self.args.inline_fp = True
        self.cycle()
        self.assertIn('#include "../generated.h"\n#include "inline_fp.h"', self.chunk().read_text())
        self.assertNotIn('gather_pipe', self.chunk().read_text())
        before = self.chunk().read_bytes(), self.chunk().stat().st_mtime_ns
        self.cycle()
        self.assertEqual(before, (self.chunk().read_bytes(), self.chunk().stat().st_mtime_ns))
        for helper in ("scripts/windows/chunk_headers.py", "cmake/composite/inline_fp.h"):
            path = self.root / helper
            path.write_text(path.read_text() + "\n")
            self.builder.generate()
            self.assertEqual(self.chunk().read_text(), CHUNK)
            self.builder.prepare_blocks()
            receipt = json.loads((self.out / "prepared-blocks.json").read_text())
            self.assertTrue(receipt["inline_fp"])
            self.assertEqual(receipt["inline_fp_header_sha256"], bw.sha256_file(self.root / "cmake/composite/inline_fp.h"))
        self.args.fixed_cpu = self.args.prepared_blocks = True
        self.cycle()
        self.assertIn('"inline_fp.h"', self.chunk().read_text())
        self.assertIn("#define ctx (&bw_guest_cpu)", self.chunk().read_text())
        self.assertIn(MARK, self.chunk().read_text())
        self.args.inline_fp = False
        self.cycle()
        self.assertNotIn('"inline_fp.h"', self.chunk().read_text())
        self.assertIn(MARK, self.chunk().read_text())

    def test_interrupted_preparation_is_not_reused(self):
        self.args.prepared_blocks = True
        self.builder.generate()
        # Emulate interruption after one atomic chunk rewrite, before receipt.
        self.chunk().write_text(self.chunk().read_text() + "/* partial preparation */\n")
        self.builder.generate()
        self.assertEqual(self.chunk().read_text(), CHUNK)
        self.builder.prepare_blocks()
        self.assertIn(MARK, self.chunk("a.c").read_text())
        self.assertIn(MARK, self.chunk("b.c").read_text())

    def test_transform_change_invalidates_prepared_cache(self):
        self.args.prepared_blocks = True
        self.cycle()
        script = self.root / "scripts/windows/fast_blocks.py"
        script.write_text(script.read_text() + "\n# synthetic revision change\n")
        self.builder.generate()
        self.assertNotIn(MARK, self.chunk().read_text())
        self.builder.prepare_blocks()
        self.assertIn(MARK, self.chunk().read_text())


if __name__ == "__main__":
    unittest.main()
