import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
class FrameTailTest(unittest.TestCase):
    def test_distinct_streams(self):
        with tempfile.NamedTemporaryFile(mode='w') as log:
            log.write('[frame-timing] retrace=1 us=0\n[frame-timing] retrace=2 us=16000\n'
                      '[frame-timing] retrace=3 us=33000\n[frame-timing] retrace=5 us=99000\n'
                      '12:30:01 [display-timing] present=1 us=0\n[display-timing] present=2 us=33000\n'
                      '[display-timing] present=3 us=66000\n[display-timing] present=4 us=170000\n')
            log.flush()
            retrace = subprocess.check_output(['python3', str(ROOT/'scripts/frame_tail.py'), log.name], text=True)
            display = subprocess.check_output(['python3', str(ROOT/'scripts/frame_tail.py'), log.name, '--kind', 'display'], text=True)
            self.assertIn('2 retrace intervals', retrace)
            self.assertIn('p50 16.00', retrace)
            self.assertIn('3 display intervals', display)
            self.assertIn('p50 33.00', display)
            self.assertIn('>100ms=1', display)
            self.assertNotIn('rendered gates', retrace)
    def test_invalid_kind(self):
        self.assertEqual(subprocess.run(['python3', str(ROOT/'scripts/frame_tail.py'), 'none', '--kind', 'game']).returncode, 2)
if __name__ == '__main__': unittest.main()
