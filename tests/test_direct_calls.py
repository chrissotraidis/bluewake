#!/usr/bin/env python3
"""Synthetic preparation checks; no game input or generated game source."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('direct', ROOT / 'scripts/windows/direct_calls.py')
direct = importlib.util.module_from_spec(spec)
spec.loader.exec_module(direct)

CALL = '''#include "../generated.h"
void synthetic(CPUState* ctx) {
    // 80004000: bl      0x80006000
    {
            ctx->lr = 0x80004004u;
            ctx->pc = 0x80006000u;
            return;
    }
label_80004004:
    ctx->gpr[3] += 1;
}
'''

INDIRECT = '''#include "../generated.h"
void synthetic(CPUState* ctx) {
    // 80004000: bctrl
    {
        u32 target = ctx->ctr & ~3u;
        bool ctr_ok = true;
        bool cr_ok = true;
        if (ctr_ok && cr_ok) {
            ctx->lr = 0x80004004u;
            ctx->pc = target;
            return;
        }
    }
label_80004004:
    ctx->gpr[3] += 1;
}
'''

FALLBACK = '''#include "../generated.h"
void synthetic(CPUState* ctx) {
    // 80004000: synthetic interpreter instruction
    ppc_fallback_instruction(ctx, 0x00000000u, 0x80004000u);
    return;
label_80004004:
    ctx->gpr[3] += 1;
}
'''


def convert(source, watched=()):
    return direct.transform(source, 0x80004000, [0x80004000, 0x80006000],
                            {0x80004000: 0, 0x80006000: 1}, set(watched))


class DirectPreparationTests(unittest.TestCase):
    def test_direct_and_fixed_cpu_calls_keep_both_boundary_queries(self):
        for source in (CALL, CALL.replace('CPUState* ctx)', 'CPUState* ctx_param)')):
            with self.subTest(fixed='ctx_param' in source):
                result, count = convert(source)
                self.assertEqual(count, 1)
                self.assertIn('bw_direct_call_ready(ctx, 0x80006000u)', result)
                self.assertIn('bw_direct_call_ready(ctx, 0x80004004u)', result)
                self.assertIn('bw_chunk_fns[1](ctx)', result)
                self.assertNotIn('bw_native_call', result)
                self.assertEqual(convert(result), (result, 0))

    def test_watched_and_missing_continuations_keep_original_dispatch(self):
        for watched in ({0x80004000}, {0x80004004}, {0x80006000}):
            self.assertEqual(convert(CALL, watched), (CALL, 0))
        for source in (CALL.replace('label_80004004:', 'label_80004008:'),
                       CALL.replace('0x80006000', '0x70006000')):
            self.assertEqual(convert(source), (source, 0))

    def test_mirror_uses_dispatcher_canonical_pc(self):
        source = CALL.replace('0x80006000', '0xC0006000')
        result, count = convert(source)
        self.assertEqual(count, 1)
        self.assertIn('ctx->pc = 0x80006000u;', result)
        self.assertEqual(convert(source, {0x80006000}), (source, 0))
        self.assertEqual(convert(source, {0xC0006000}), (source, 0))

    def test_indirect_and_interpreter_continuations_query_the_host(self):
        result, count = direct.transform_indirect(INDIRECT, set())
        self.assertEqual(count, 1)
        self.assertIn('bw_call_translated(ctx, target)', result)
        self.assertIn('bw_direct_call_ready(ctx, target)', result)
        self.assertIn('bw_direct_call_ready(ctx, 0x80004004u)', result)
        self.assertEqual(direct.transform_indirect(result, set()), (result, 0))
        result, count = direct.transform_fallback(FALLBACK, set())
        self.assertEqual(count, 1)
        self.assertIn('ctx->pc == 0x80004004u && bw_direct_call_ready(ctx, 0x80004004u)', result)
        self.assertEqual(direct.transform_fallback(result, set()), (result, 0))
        for convert_fn, source in ((direct.transform_indirect, INDIRECT),
                                   (direct.transform_fallback, FALLBACK)):
            for watched in ({0x80004000}, {0x80004004}):
                self.assertEqual(convert_fn(source, watched), (source, 0))

    def test_watch_list_collapses_mirrors_and_is_repeatable(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(direct.write_watch_list(root, {0x80004000, 0xC0004000}), 1)
            before = (root / 'bw_edge_watch.inc').read_bytes()
            direct.write_watch_list(root, {0xC0004000, 0x80004000})
            self.assertEqual(before, (root / 'bw_edge_watch.inc').read_bytes())


if __name__ == '__main__':
    unittest.main()
