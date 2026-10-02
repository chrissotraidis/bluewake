from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).parents[1] / 'scripts'))
from guest_checkpoints import compare, read_checkpoints


def checkpoint(retrace: int) -> str:
    return (f'[guest-checkpoint] version=1 retrace={retrace} cycle={retrace * 8100000} '
            f'cpu={"1" * 32} mem1={"2" * 32} mem2={"3" * 32} aliases={"4" * 32} '
            'alias_spans=2 alias_bytes=1024\n')


def route() -> str:
    return ('unrelated diagnostics\n' + checkpoint(1000) + checkpoint(2000) + checkpoint(3000)
            + '[clock] summary cycles=24300000000 retraces=3000\n'
            + '[run] stopped: normal after 100 blocks at pc=0x80000000\n')


class CheckpointTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.first = Path(self.directory.name) / 'first'
        self.second = Path(self.directory.name) / 'second'
        self.first.write_text(route())

    def test_matching_coverage(self):
        self.second.write_text(route().replace('100 blocks', '200 blocks'))
        self.assertEqual(compare([self.first, self.second], 1000, 3000), 3)

    def test_rejects_incomplete_or_invalid_evidence(self):
        bad_routes = [
            route().replace(checkpoint(2000), ''),
            route().replace(checkpoint(3000), ''),
            route().replace(checkpoint(1000), checkpoint(1000) * 2),
            route().replace(checkpoint(2000), checkpoint(3000)),
            route().replace('version=1', 'version=2'),
            route().replace(checkpoint(2000), '[guest-checkpoint] failed retrace=2000\n'),
            route().replace('cycle=16200000000', 'cycle=8100000000'),
            route().replace('retraces=3000', 'retraces=2000'),
            route().replace('[run] stopped: normal', '[run] stopped: failure'),
            route().split('[run]')[0],
            route() * 2,
            '',
        ]
        for bad in bad_routes:
            with self.subTest(bad=bad):
                self.first.write_text(bad)
                with self.assertRaises(ValueError):
                    read_checkpoints(self.first, 1000, 3000)

    def test_rejects_state_and_timing_differences(self):
        changes = [
            ('cpu=' + '1' * 32, 'cpu=' + 'F' * 32),
            ('mem1=' + '2' * 32, 'mem1=' + 'F' * 32),
            ('mem2=' + '3' * 32, 'mem2=' + 'F' * 32),
            ('aliases=' + '4' * 32, 'aliases=' + 'F' * 32),
            ('alias_spans=2', 'alias_spans=3'),
            ('alias_bytes=1024', 'alias_bytes=2048'),
            ('cycle=16200000000', 'cycle=16200000001'),
        ]
        for old, new in changes:
            with self.subTest(field=old):
                self.second.write_text(route().replace(old, new))
                with self.assertRaisesRegex(ValueError, 'differs in'):
                    compare([self.first, self.second], 1000, 3000)

    def test_rejects_empty_coverage(self):
        for interval, through in [(0, 3000), (-1, 3000), (4000, 3000)]:
            with self.subTest(interval=interval, through=through):
                with self.assertRaises(ValueError):
                    read_checkpoints(self.first, interval, through)
        with self.assertRaises(ValueError):
            compare([self.first], 1000, 3000)


if __name__ == '__main__':
    unittest.main()
