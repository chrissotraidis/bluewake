#!/usr/bin/env python3
"""Focused source-shape tests for the shared native replay generator."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts/windows"))
import native_gx_gen  # noqa: E402


class LeanDeadlineAccesses(unittest.TestCase):
    def test_normalizes_store_wrapper(self):
        source = """    {
    bool bw_ool = false;
    // 8032EC1C: stwu     r1, -32(r1)
    {
        u32 ea = ctx->gpr[1] + (u32)(s32)(-32);
        bw_write32_ato(&bw_ool, ctx, 0x8032EC1Cu, 11u, ea, (u32)ctx->gpr[1]);
        ctx->gpr[1] = ea;
    }
    if (__builtin_expect(bw_ool, 0) &&
        ctx->cycle_deadline_budget > 0 &&
        (s64)11u > ctx->cycle_deadline_budget) {
        ctx->pc = 0x8032EC1Cu;
        ctx->cycle_observation_suffix = 11u;
        ctx->downcount += (s64)11u;
        cycle_block_prepaid = false;
        goto bwslow_379_1;
    }
    }
"""
        out = native_gx_gen.normalize_lean_deadline_accesses(source)
        self.assertIn("bw_write32_at(ctx, 0x8032EC1Cu, 11u, ea,", out)
        self.assertNotIn("bw_ool", out)
        self.assertNotIn("bwslow_379_1", out)

    def test_leaves_regular_access_unchanged(self):
        source = "    value = bw_read16_at(ctx, 0x80001000u, 2u, ea);\n"
        self.assertEqual(native_gx_gen.normalize_lean_deadline_accesses(source), source)

    def test_rejects_unknown_wrapper(self):
        source = "    bool bw_ool = false;\n    bw_read32_ato(&bw_ool, ctx, 0x80001000u, 1u, ea);\n"
        with self.assertRaises(SystemExit):
            native_gx_gen.normalize_lean_deadline_accesses(source)


class PreparedBlockLeaders(unittest.TestCase):
    def test_recognizes_precharge_and_always_prepaid_forms(self):
        precharge = """
label_80001000:
    ctx->pc = 0x80001000u;
    cycle_block_prepaid = dolrecomp_block_can_precharge(ctx, 12u);
"""
        timed = """
label_80001000:
    if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {
        ctx->pc = 0x80001000u;
        return;
    }
    cycle_block_prepaid = true;
    ctx->downcount -= 12u;
"""
        hooked = """
label_80001000:
    /* bluewake: certified native hook */
    if (bluewake_native_vec_try(ctx, 0x80001000u))
        goto return_dispatch_80000000;
bwend_16: ;
    ctx->pc = 0x80001000u;
    cycle_block_prepaid = dolrecomp_block_can_precharge(ctx, 12u);
"""
        for source in (precharge, timed, hooked):
            match = native_gx_gen.LEADER.search(source)
            self.assertIsNotNone(match)
            self.assertEqual(int(match.group(2) or match.group(3)), 12)

    def test_always_prepaid_leader_selects_its_copy(self):
        source = """
void func_800016E0(CPUState* ctx_param) {
label_800016E0:
    cycle_block_prepaid = true;
    ctx->downcount -= 2u;
    goto bwfast_1;
    ctx->gpr[3] = 0u;
return_dispatch_800016E0:
    switch (ctx->pc) {
    default: return;
    }
bwfast_1:
    ctx->gpr[3] = 1u;
}
"""
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "chunks_dol").mkdir()
            (root / "chunks_dol/chunk_test_800016E0.c").write_text(source)
            chunk = native_gx_gen.Chunk(root, 0x800016E0)
        self.assertEqual(chunk.blocks[0x800016E0]["copy"], "bwfast_1")


class DirectCallGuards(unittest.TestCase):
    def test_normalizes_known_and_dynamic_edges(self):
        source = ("bw_direct_call_ready(ctx, 0x80330E34u) && "
                  "bw_direct_call_ready(ctx, target)")
        self.assertEqual(native_gx_gen.normalize_direct_call_guards(source),
                         "bw_direct_call_ready(ctx) && bw_direct_call_ready(ctx)")

    def test_normalizes_guarded_inline_save(self):
        source = """            ctx->lr = 0x80247CE8u;
            ctx->pc = 0x80328F34u;
            /* bluewake: _savegpr/_restgpr inline (scripts/windows/inline_save_restore_gpr.py) */
            if (bw_direct_call_ready(ctx) &&
                bw_inline_gpr_memory_ready(ctx, 26u)) {
            if (ctx->downcount - 6 > -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET) {
                ctx->downcount -= 7;
                ctx->cycle_observation_suffix = 0u;
                ctx->pc = 0x80247CE8u;
                if (ctx->downcount <= -(s64)DOLRECOMP_C_LOOP_CYCLE_BUDGET || ctx->exception != 0u ||
                    (ctx->cycle_budget > 0 && ctx->downcount <= -ctx->cycle_budget) ||
                    !bw_direct_call_ready(ctx))
                    return;
                goto label_80247CE8;
            }
            }
            ctx->pc = 0x80328F34u;
            return;
"""
        out = native_gx_gen.normalize_inline_gpr_guards(source)
        self.assertEqual(native_gx_gen.inline_gpr_edges(source), {0x80247CE8: 0x80328F34})
        self.assertNotIn("bw_inline_gpr_memory_ready", out)
        self.assertNotIn("!bw_direct_call_ready", out)
        self.assertEqual(out.count("            }\n"), 1)


if __name__ == "__main__":
    unittest.main()
