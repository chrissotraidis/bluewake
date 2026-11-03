#!/usr/bin/env python3
"""Session-log regressions use synthetic metadata, never game files or player paths."""
import contextlib
import importlib.util
import io
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('triage', Path(__file__).resolve().parents[1] / 'scripts/triage_session_log.py')
triage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(triage)


def line(retrace, state, track='1tale.afc', sound_id='0xC0000024'):
    return f'retrace={retrace} path="Audiores/Stream/{track}" id={sound_id} state={state} muted=0'


class MusicEvidenceTest(unittest.TestCase):
    def test_reporter_sequence_is_a_two_retrace_playback(self):
        changes = [line(1979, 3), line(2081, 4), line(2083, 0)]
        self.assertEqual(triage.music_playback_spans(changes), [('Audiores/Stream/1tale.afc', 2, 'state=0')])
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'session.log'
            path.write_text('\n'.join('[music-stream] ' + v for v in changes))
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                triage.report(path)
            self.assertIn('playing for 2 retraces, then state=0', out.getvalue())
            self.assertIn('not a diagnosed failure', out.getvalue())

    def test_state_four_changes_do_not_reset_duration(self):
        changes = [line(100, 4), line(120, 4) + ' decoded=4096 playback_samples=64000', line(220, 0)]
        self.assertEqual(triage.music_playback_spans(changes)[0][1], 120)

    def test_track_switch_and_truncated_log(self):
        self.assertEqual(triage.music_playback_spans([line(10, 4)]), [])
        self.assertEqual(triage.music_playback_spans([line(10, 4), line(20, 4, 'title.afc')]),
                         [('Audiores/Stream/1tale.afc', 10, 'track changed')])

    def test_rewound_counter_and_missing_diagnostics(self):
        self.assertEqual(triage.music_playback_spans([line(1000, 4), line(5, 0)]), [])
        self.assertEqual(triage.music_playback_spans(['no diagnostics', 'state=4']), [])
        self.assertEqual(triage.music_playback_spans([line(1000, 4), line(5, 4), line(9, 0)])[0][1], 4)
        # A partial rewind can stay above the original start and still invalidate it.
        self.assertEqual(triage.music_playback_spans([line(100, 4), line(200, 4), line(150, 0)]), [])


class FatalEvidenceTest(unittest.TestCase):
    def render(self, text):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'session.log'
            path.write_text(text)
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                triage.report(path)
            return out.getvalue()

    def test_shutdown_abort_after_performance_summary(self):
        out = self.render('18:05:03.521 [perf-summary] exit watched_s=434 below_target_s=1\n'
                          '18:05:03.526 double free or corruption (!prev)\n')
        self.assertIn('fatal lines: 1', out)
        self.assertIn('double free or corruption (!prev)', out)
        self.assertIn('last [perf-summary]: exit watched_s=434 below_target_s=1', out)

    def test_untagged_allocator_and_console_failures(self):
        for failure in ('free(): double free detected in tcache 2',
                        'malloc(): invalid size (unsorted)',
                        'realloc(): invalid pointer', 'fatal: failed to initialize'):
            with self.subTest(failure=failure):
                out = self.render(failure + '\n')
                self.assertIn('fatal lines: 1', out)
                self.assertIn(failure, out)

    def test_tagged_failure_counted_once_and_benign_teardown_ignored(self):
        out = self.render('[crash] access violation\n'
                          '[crash] reports=/example/reports\n'
                          '[gpu] Device lost: Device was destroyed\n'
                          '[panic] vcall-after trace\n'
                          'allocator: 42 free blocks\n')
        self.assertIn('fatal lines: 1', out)


if __name__ == '__main__':
    unittest.main()
