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
        for watched in ({0x80328F84}, {0x80004000}, {0x80004004}):
            self.assertEqual(gpr.transform(CALL, watched), (CALL, 0))

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
