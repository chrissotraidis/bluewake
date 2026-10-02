"""Synthetic certification tests; no translated game source is required."""
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts/windows'))
import native_game_math as native

class CertificationTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / 'chunks_dol').mkdir()
        (self.root / 'generated.h').write_text('/* synthetic header */\n')
        self.original = native.FRAGMENTS, native.ENTRIES, native.watched_addresses
        self.head = '\nlabel_80000100:\n    ctx->pc = 0x80000104u;\n'
        self.tail = '\nlabel_80000200:\n    ctx->gpr[3] = 1;\n'
        digest = lambda s: hashlib.sha256(native.canonical(s).encode()).hexdigest()
        native.FRAGMENTS = {
            'head': (0x80000100, 0x80000100, 0x80000108, digest(self.head)),
            'tail': (0x80000200, 0x80000200, 0x80000208, digest(self.tail)),
        }
        native.ENTRIES = {0x80000100: ('head', 'tail')}
        native.watched_addresses = lambda: set()
        self.a = self.root / 'chunks_dol/chunk_80000100.c'
        self.b = self.root / 'chunks_dol/chunk_80000200.c'
        self.a.write_text(native.INCLUDE + self.head + '\nlabel_80000108:\n\nreturn_dispatch_80000100:\n')
        self.b.write_text(native.INCLUDE + self.tail + '\nlabel_80000208:\n\nreturn_dispatch_80000200:\n')

    def tearDown(self):
        native.FRAGMENTS, native.ENTRIES, native.watched_addresses = self.original
        self.tmp.cleanup()

    def test_repeat_and_mod_variants(self):
        folder = self.root / 'chunks_mod_test'; folder.mkdir()
        variant = folder / 'variant_80000100.c'; variant.write_text(self.a.read_text())
        native.prepare(self.root)
        before = {p: (p.read_bytes(), p.stat().st_mtime_ns) for p in self.root.rglob('*') if p.is_file()}
        native.prepare(self.root)
        self.assertEqual(before, {p: (p.read_bytes(), p.stat().st_mtime_ns) for p in before})
        self.assertIn('bluewake_native_game_math_try', variant.read_text())

    def test_dependency_change_preserves_prior_files(self):
        native.prepare(self.root)
        original = self.a.read_bytes(); manifest = (self.root / 'native_game_math.json').read_bytes()
        self.b.write_text(self.b.read_text().replace('= 1;', '= 2;'))
        with self.assertRaises(ValueError): native.prepare(self.root)
        self.assertEqual(original, self.a.read_bytes())
        self.assertEqual(manifest, (self.root / 'native_game_math.json').read_bytes())

    def test_missing_cross_chunk_tail(self):
        self.b.unlink()
        with self.assertRaises(ValueError): native.prepare(self.root)
        self.assertNotIn('native_game_math_try', self.a.read_text())
        self.assertFalse((self.root / 'native_game_math.json').exists())

    def test_changed_mod_variant(self):
        folder = self.root / 'chunks_mod_test'; folder.mkdir()
        (folder / 'variant_80000200.c').write_text(self.b.read_text().replace('= 1;', '= 2;'))
        with self.assertRaises(ValueError): native.prepare(self.root)
        self.assertNotIn('native_game_math_try', self.a.read_text())

    def test_watched_internal_boundary(self):
        native.watched_addresses = lambda: {0x80000104}
        with self.assertRaises(ValueError): native.prepare(self.root)
        self.assertFalse((self.root / 'native_game_math.json').exists())

    def test_changed_hook(self):
        native.prepare(self.root)
        self.a.write_text(self.a.read_text().replace('try(ctx, 0x80000100u)', 'try(ctx, 0x80000104u)'))
        with self.assertRaises(ValueError): native.prepare(self.root)

if __name__ == '__main__': unittest.main()
