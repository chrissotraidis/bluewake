#!/usr/bin/env python3
"""Synthetic inline-register preparation and helper certification checks."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('gpr', ROOT / 'scripts/windows/inline_save_restore_gpr.py')
gpr = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gpr)
CALL = '''#include "../generated.h"
void synthetic(CPUState* ctx) {
    // 80004000: bl      0x80328F84
    {
            ctx->lr = 0x80004004u;
            ctx->pc = 0x80328F84u;
            return;
    }
label_80004004:
    return;
}
'''


class InlineGprTests(unittest.TestCase):
    def test_watched_target_cannot_bypass_a_host_observation(self):
        entry = CALL.replace('0x80328F84', '0x80328F88').replace('0x80328F84u', '0x80328F88u')
        for source, watched in ((entry, {0x80328F88}), (CALL, {0x80004000}), (CALL, {0x80004004})):
            self.assertEqual(gpr.transform(source, watched), (source, 0))

    def test_host_names_a_return_keyed_entry_only_with_its_return(self):
        # RETURN_KEYED inlines every caller of these entries except where the
        # return is watched. That holds only while every host hook on the entry
        # also checks that return: a hook on the entry alone (any lr) would be
        # skipped by the inline form. The intercept table and its case list
        # name addresses without hooking them.
        listing = {'edge_intercept_table.h', 'edge_intercepts.c'}
        watched = gpr.watched_addresses()
        for entry, returns in gpr.RETURN_KEYED.items():
            for ret in returns:
                self.assertIn(ret, watched)
            forms = (f'0X{entry:08X}', f'0X{entry | 0x40000000:08X}')
            for folder in ('runtime/host/src', 'windows/src'):
                for path in sorted((ROOT / folder).rglob('*')):
                    if path.suffix not in ('.c', '.h', '.cpp', '.mm', '.m') or path.name in listing:
                        continue
                    lines = [l.split('//')[0].upper() for l in path.read_text(errors='replace').splitlines()]
                    for number, code in enumerate(lines):
                        # The condition may run on over the next two lines.
                        near = ' '.join(lines[number:number + 3])
                        if any(f in code for f in forms) and \
                                not any(f'0X{ret:08X}' in near for ret in returns):
                            self.fail(f'{path.relative_to(ROOT)}:{number + 1} names 0x{entry:08X} '
                                      'without one of its returns; recheck RETURN_KEYED')

    def test_return_keyed_entry_stays_out_only_where_its_return_is_watched(self):
        # The GroundCross check at _restgpr_27 is keyed on its one return: that
        # call site stays out, and any other caller of the entry is inlined.
        self.assertEqual(gpr.transform(CALL, {0x80328F84, 0x80004004}), (CALL, 0))
        result, count = gpr.transform(CALL, {0x80328F84})
        self.assertEqual(count, 1)
        self.assertIn('bw_direct_call_ready(ctx, 0x80328F84u)', result)

    def test_rel_call_through_the_mirror_is_inlined_by_its_canonical_entry(self):
        rel = (CALL.replace('80004000', 'C0DA0254').replace('80004004', 'C0DA0258')
               .replace('0x80328F84', '0xC0328F84'))
        result, count = gpr.transform(rel, set())
        self.assertEqual(count, 1)
        self.assertIn('bw_direct_call_ready(ctx, 0x80328F84u)', result)
        self.assertIn('bw_inline_gpr_memory_ready(ctx, 27u)', result)
        self.assertIn('bw_direct_call_ready(ctx, 0xC0DA0258u)', result)
        self.assertIn('goto label_C0DA0258;', result)
        # Declined, it still goes out through the mirror address it called.
        self.assertIn('ctx->pc = 0xC0328F84u;\n            return;', result)

    def test_plain_and_fixed_cpu_rewrites_keep_memory_and_host_guards(self):
        for source in (CALL, CALL.replace('CPUState* ctx)', 'CPUState* ctx_param)')):
            result, count = gpr.transform(source, set())
            self.assertEqual(count, 1)
            self.assertIn('#include "inline_gpr.h"', result)
            self.assertIn('bw_inline_gpr_memory_ready(ctx, 27u)', result)
            self.assertIn('bw_direct_call_ready(ctx, 0x80328F84u)', result)
            self.assertIn('bw_direct_call_ready(ctx, 0x80004004u)', result)
            self.assertLess(result.index('ctx->pc = 0x80328F84u'), result.index('bw_direct_call_ready'))
            self.assertEqual(gpr.transform(result, set()), (result, 0))

    def test_unaligned_and_missing_continuation_stay_ordinary(self):
        for source in (CALL.replace('80328F84', '80328F85'),
                       CALL.replace('label_80004004:', 'label_80004008:')):
            self.assertEqual(gpr.transform(source, set()), (source, 0))

    def test_changed_helper_is_rejected_before_caller_rewriting(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            helper = root / 'synthetic_803256E0.c'
            body = '\nlabel_80328F04:\n    return;\n'
            helper.write_text(body + '\nlabel_80328F50:\n')
            digest = hashlib.sha256(' '.join(body.split()).encode()).hexdigest()
            with patch.object(gpr, 'LEAVES', ((0x80328F04, 0x80328F50, digest),)):
                gpr.validate_helpers(root)
                helper.write_text(body.replace('return;', 'different();') + '\nlabel_80328F50:\n')
                with self.assertRaisesRegex(ValueError, 'changed GPR helper'):
                    gpr.validate_helpers(root)
            with self.assertRaises(ValueError):
                gpr.validate_helpers(root / 'missing')


if __name__ == '__main__':
    unittest.main()
