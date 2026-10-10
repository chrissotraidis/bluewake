#!/usr/bin/env python3
"""Synthetic preparation checks; no game input or generated game source."""
import importlib.util
from pathlib import Path
import re
import shutil
import subprocess
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


# Every address a feature hook observes, for every combination of the features'
# switches, is one the builder's watch list holds or one host_direct_can_skip
# still checks itself (draw tags' ranges, Forest Water's load-time address).
SKIP_HARNESS = r"""
#include <assert.h>
#include <stdio.h>
#include "feature_dispatch.h"
#include "bw_edge_watch.inc"
bool bluewake_climb_on, bluewake_quick_doors_armed, bluewake_draw_tags_enabled;
bool bluewake_forest_water_enabled;
u32 bluewake_forest_water_tree_timer_check;
static unsigned char watched[0x01800000u / 4u];
int main(void) {
    for (unsigned i = 0; i < sizeof bw_edge_watch_list / sizeof bw_edge_watch_list[0]; ++i)
        if (bw_edge_watch_list[i] - 0x80000000u < 0x01800000u)
            watched[(bw_edge_watch_list[i] - 0x80000000u) / 4u] = 1;
    unsigned long checked = 0;
    for (unsigned flags = 0; flags < 16; ++flags) {
        bluewake_climb_on = flags & 1;
        bluewake_quick_doors_armed = (flags & 2) != 0;
        bluewake_draw_tags_enabled = (flags & 4) != 0;
        bluewake_forest_water_enabled = (flags & 8) != 0;
        /* Anywhere in MEM1, so a timer address the list lacks is covered too. */
        bluewake_forest_water_tree_timer_check = 0x80001234u;
        for (u32 address = 0x80000000u; address < 0x81800000u; address += 4) {
            if (!bluewake_feature_observes(address))
                continue;
            const bool host_checks = bluewake_draw_tags_observes_range(address) ||
                (bluewake_forest_water_enabled && address == bluewake_forest_water_tree_timer_check);
            if (!watched[(address - 0x80000000u) / 4u] && !host_checks) {
                fprintf(stderr, "0x%08X observed but neither watched nor checked\n", (unsigned)address);
                return 1;
            }
            ++checked;
        }
    }
    printf("%lu\n", checked);
    return 0;
}
"""


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


    @unittest.skipUnless(shutil.which('cc'), 'needs a C compiler')
    def test_direct_call_skip_sees_every_feature_address(self):
        src = ROOT / 'runtime/host/src'
        main_c = (src / 'main.c').read_text()
        watched = direct.watched_addresses()
        # Checks host_direct_can_skip leaves to the watch list.
        judge = int(re.search(r'#define BW_SEARCH_JUDGE_FILTER (0x[0-9A-Fa-f]+)u', main_c).group(1), 16)
        self.assertIn(judge, watched)
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            direct.write_watch_list(root, watched)
            # The feature headers only pass these types through.
            (root / 'core').mkdir()
            (root / 'core/cpu.h').write_text(
                '#include <stdint.h>\ntypedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;\n'
                'typedef uint64_t u64; typedef struct CPUState CPUState;\n')
            (root / 'gxruntime').mkdir()
            (root / 'gxruntime/platform.h').write_text('typedef struct DolPadState DolPadState;\n')
            (root / 'harness.c').write_text(SKIP_HARNESS)
            subprocess.run(['cc', '-std=c11', '-O1', '-I', str(root), '-I', str(src), '-o',
                            str(root / 'harness'), str(root / 'harness.c')], check=True)
            run = subprocess.run([str(root / 'harness')], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            self.assertGreater(int(run.stdout), 0)


if __name__ == '__main__':
    unittest.main()
