#!/usr/bin/env python3
"""The committed widescreen mods are what the generator makes, and every preset is complete.

`scripts/mods/widescreen_aspect.py` derives each mod from the 16:9 code, but
nothing checked that the files in `mods/widescreen/` are still its output: a
preset could be edited by hand, or a new aspect could be added to `ASPECTS`
while the pool-constant tables were left without an entry for it, and only a
player on that monitor would see the HUD go to the wrong place. Both are
asserted here, and the generator is run through its own CLI so the shipped entry
point is what is tested.
"""
import importlib.util
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "scripts/mods/widescreen_aspect.py"
MODS = ROOT / "mods/widescreen"
SHIPPED = {
    "16:10": "GZLE01-16x10.gecko",
    "21:9": "GZLE01-21x9.gecko",
    "64:27": "GZLE01-64x27.gecko",
    "43:18": "GZLE01-43x18.gecko",
}
MINIMAP = 0x803E68E8


class WidescreenAspectTest(unittest.TestCase):
    def setUp(self):
        spec = importlib.util.spec_from_file_location("widescreen_aspect", GENERATOR)
        self.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)

    def generate(self, aspect):
        # Through the CLI, as the release's build does: `main()` reads the 16:9
        # source relative to sys.path[0], which only the script as __main__ has.
        done = subprocess.run([sys.executable, str(GENERATOR), aspect],
                              capture_output=True, text=True, check=True)
        return done.stdout

    def wide_aspects(self):
        return [name for name, ratio in self.module.ASPECTS.items() if ratio > 16.0 / 9.0 + 1e-9]

    def test_every_shipped_mod_is_what_the_generator_makes(self):
        for aspect, name in SHIPPED.items():
            with self.subTest(aspect=aspect):
                self.assertEqual(self.generate(aspect), (MODS / name).read_text())

    def test_every_shipped_mod_is_committed(self):
        for name in SHIPPED.values():
            with self.subTest(name=name):
                self.assertTrue((MODS / name).is_file())

    def test_every_aspect_wider_than_16_9_names_every_pool_constant(self):
        wide = self.wide_aspects()
        self.assertTrue(wide, "no aspect is wider than 16:9")
        for aspect in wide:
            for offset, swaps in self.module.POOL_SWAPS.items():
                with self.subTest(aspect=aspect, offset=offset):
                    self.assertIn(aspect, swaps)

    def test_every_wide_aspect_has_the_minimap_override(self):
        for aspect in self.wide_aspects():
            with self.subTest(aspect=aspect):
                self.assertIn(aspect, self.module.HALF_OVERRIDES[MINIMAP])

    def test_each_aspect_is_a_different_shape(self):
        ratios = list(self.module.ASPECTS.values())
        self.assertEqual(len(ratios), len(set(ratios)))


if __name__ == "__main__":
    unittest.main()
