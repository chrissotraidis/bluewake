"""Native routing must reject changed bodies and hooks before modifying variants."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / 'scripts/mods/prepare_native_j3d.py'
spec = importlib.util.spec_from_file_location('native_j3d_prepare', SCRIPT)
prepare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prepare)


class CertificationTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.body = '\nlabel_00000100:\n    ctx->gpr[3] = 1;\n'
        digest = hashlib.sha256(' '.join(self.body.split()).encode()).hexdigest()
        self.old_leaves = prepare.LEAVES
        prepare.LEAVES = ((0x100, 0x104, {digest}),)
        self.source = prepare.GENERATED_INCLUDE + self.body + '\nlabel_00000104:\nreturn_dispatch_802D96E0:\n'
        self.file = self.root / 'chunk_802D96E0.c'
        self.file.write_text(self.source)

    def tearDown(self):
        prepare.LEAVES = self.old_leaves
        self.temp.cleanup()

    def test_repeat_and_variants(self):
        variant = self.root / 'chunk_802D96E0_mod.c'
        variant.write_text(self.source)
        prepare.prepare(self.root)
        before = {p.name: p.read_bytes() for p in self.root.iterdir()}
        prepare.prepare(self.root)
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.root.iterdir()})
        self.assertIn(prepare.hook(0x100), self.file.read_text())

    def test_modified_body_does_not_partially_route(self):
        variant = self.root / 'chunk_802D96E0_mod.c'
        variant.write_text(self.source.replace('= 1;', '= 2;'))
        with self.assertRaises(ValueError):
            prepare.prepare(self.root)
        self.assertEqual(self.file.read_text(), self.source)
        self.assertFalse((self.root / 'native_j3d.json').exists())

    def test_modified_hook_and_missing_function(self):
        prepare.prepare(self.root)
        manifest = (self.root / 'native_j3d.json').read_bytes()
        self.file.write_text(self.file.read_text().replace('0x00000100u)', '0x00000104u)'))
        with self.assertRaises(ValueError):
            prepare.prepare(self.root)
        self.assertEqual((self.root / 'native_j3d.json').read_bytes(), manifest)
        self.file.write_text(prepare.GENERATED_INCLUDE + 'return_dispatch_802D96E0:\n')
        with self.assertRaises(ValueError):
            prepare.prepare(self.root)


if __name__ == '__main__':
    unittest.main()
