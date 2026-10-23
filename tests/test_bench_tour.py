"""Synthetic checks for the benchmark tour. No disc, card, or app."""

from contextlib import contextmanager
import hashlib
import json
from pathlib import Path
import os
import re
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).parents[1] / 'scripts'))
import bench_tour


def checkpoint(retrace, cpu):
    return {
        'retrace': retrace,
        'cycle': retrace * 8100000,
        'cpu': cpu,
        'mem1': '2' * 32,
        'mem2': '3' * 32,
        'aliases': '4' * 32,
        'alias_spans': 2,
        'alias_bytes': 1024,
    }


def summary(fps, checkpoints, busy=80.0):
    return {
        'modes': {
            'rendered': {
                'stops': [{
                    'retrace': 2400,
                    'code': 'sea:44:0',
                    'name': 'Outset',
                    'loaded': True,
                    'game_frames_per_second': fps,
                    'busy_percent': busy,
                    'dip_causes': [],
                }],
                'checkpoints': checkpoints,
            },
        },
    }


def perf(retrace, rate, busy):
    return f'[perf] retrace={retrace} rate={rate:.1f} worst_ms=16.0 hitches=0 busy={busy:.0f}%\n'


def dip(retrace, cause):
    return (f'[fps-dip] retrace={retrace} shown=20.0 game=20 speed=67% interpolated=0/1 '
            f'draws/frame=10 rejected=0.0% unmatched=0.0% busy=90% waits: gx=0ms present=0ms '
            f'gpu=0ms workers: gx=0% interp=0% render=0% '
            f'stage=sea room=44 event=0 pos=0,0,0 target=30 pipelines=1 '
            f'cause={cause} reason=game below full speed\n')


class BenchTourTests(unittest.TestCase):
    def test_default_warp_and_tour_end(self):
        self.assertEqual(
            bench_tour.warp_string(bench_tour.DEFAULT_STOPS),
            '2400:sea:44:0,4200:sea:11:1,6000:M_NewD2:0:0,7800:sea:41:0,9600:sea:1:100',
        )
        self.assertEqual(bench_tour.tour_end(bench_tour.DEFAULT_STOPS), 11400)

    def test_untrained_string_is_outside_the_training_tour(self):
        self.assertEqual(
            bench_tour.warp_string(bench_tour.UNTRAINED_STOPS),
            '2400:M_Dai:0:0,4200:kaze:0:0',
        )
        build = Path(__file__).parents[1] / 'scripts' / 'windows' / 'build.py'
        match = re.search(r'TRAINING_TOUR = \[(.*?)\]', build.read_text(), re.S)
        self.assertIsNotNone(match)
        tour = match.group(0)
        self.assertNotIn('M_Dai', tour)
        self.assertNotIn('kaze', tour)

    def test_perf_inside_the_first_300_retraces_is_ignored(self):
        log = ''.join([
            '[load] test warp retrace=2400 to sea room 44 point 0 layer -1\n',
            '[load] retrace=2450 overlap=0x80000000 peek=1 fader=1 alpha=255 stage=sea cpu_us=1 wall_us=1\n',
            perf(2500, 10.0, 1),
            perf(2699, 10.0, 1),
            dip(2500, 'shader-compile'),
            perf(2700, 60.0, 40),
            perf(3000, 60.0, 60),
            dip(2800, 'game-thread'),
            perf(4200, 1000.0, 99),
            perf(4500, 40.0, 50),
            '[load] retrace=4300 overlap=0x80000000 peek=1 fader=1 alpha=255 stage=sea cpu_us=1 wall_us=1\n',
        ])
        body = bench_tour.summarize_log(log, bench_tour.DEFAULT_STOPS)
        outset = body['stops'][0]
        self.assertEqual(outset['game_frames_per_second'], 30.0)
        self.assertEqual(outset['busy_percent'], 50.0)
        self.assertEqual(outset['dip_causes'], ['game-thread'])
        self.assertTrue(outset['loaded'])
        windfall = body['stops'][1]
        self.assertEqual(windfall['game_frames_per_second'], 20.0)
        self.assertTrue(windfall['loaded'])

        warp_only = '[load] test warp retrace=2400 to sea room 44 point 0 layer -1\n'
        refused = bench_tour.summarize_log(warp_only, bench_tour.DEFAULT_STOPS[:1])
        self.assertFalse(refused['stops'][0]['loaded'])

    def test_equal_summaries_match(self):
        made = summary(30.0, [checkpoint(300, 'A' * 32), checkpoint(600, 'A' * 32)])
        code, text = bench_tour.compare_summaries(made, summary(30.0, [checkpoint(300, 'A' * 32), checkpoint(600, 'A' * 32)]))
        self.assertEqual(code, 0)
        self.assertIn('checkpoints match', text)

    def test_first_differing_checkpoint_retrace_is_the_later_one(self):
        left = summary(30.0, [checkpoint(300, 'A' * 32), checkpoint(600, 'A' * 32)])
        right = summary(30.0, [checkpoint(300, 'A' * 32), checkpoint(600, 'B' * 32)])
        code, text = bench_tour.compare_summaries(left, right)
        self.assertNotEqual(code, 0)
        self.assertIn('checkpoints differ at retrace 600', text)
        self.assertNotIn('retrace 300', text)

    def test_different_speeds_with_equal_checkpoints_match(self):
        points = [checkpoint(300, 'A' * 32), checkpoint(600, 'A' * 32)]
        code, text = bench_tour.compare_summaries(summary(30.0, points), summary(36.0, points, busy=70.0))
        self.assertEqual(code, 0)
        self.assertIn('checkpoints match', text)
        self.assertIn('30.00', text)
        self.assertIn('36.00', text)
        self.assertIn('80.0%', text)
        self.assertIn('70.0%', text)

    def test_measured_environment_has_phase1_and_not_probe_or_disc(self):
        scratch = '/tmp/bluewake-bench-scratch'
        card = scratch + '/GZLE01.card'
        base = {
            'PATH': '/usr/bin',
            'BLUEWAKE_PLAYER_PROBE': '1',
            'BLUEWAKE_DISC': '/tmp/game.iso',
            'BLUEWAKE_CARD_PATH': '/home/player/GZLE01.card',
            'DOL_AURORA_FRAME_INTERP': '1',
        }
        env = bench_tour.measurement_environment(
            base,
            mode='rendered',
            load_state=scratch + '/tour.bwstate',
            scratch=scratch,
            card=card,
            warp=bench_tour.warp_string(bench_tour.DEFAULT_STOPS),
            max_retraces=11400,
        )
        self.assertEqual(env['BLUEWAKE_MAX_RETRACES'], '11400')
        self.assertEqual(env['BLUEWAKE_WALL_PACE'], '0')
        self.assertEqual(env['BLUEWAKE_PERF_LOG'], '1')
        self.assertEqual(env['DOL_AURORA_FRAME_INTERP'], '0')
        self.assertEqual(env['BLUEWAKE_PAD_BUTTONS'], '0x0100')
        self.assertEqual(env['BLUEWAKE_PAD_PULSE_ON_TITLE_READY'], '1')
        self.assertEqual(env['BLUEWAKE_PAD_PULSE_LENGTH'], '2')
        self.assertEqual(env['BLUEWAKE_GUEST_CHECKPOINT_INTERVAL'], '300')
        self.assertEqual(env['BLUEWAKE_PAD_SCRIPT'], ','.join(f'{n}:0x0100:2' for n in range(340, 1601, 60)))
        self.assertEqual(env['BLUEWAKE_TEST_WARP'],
                         '2400:sea:44:0,4200:sea:11:1,6000:M_NewD2:0:0,7800:sea:41:0,9600:sea:1:100')
        self.assertEqual(env['BLUEWAKE_LOAD_STATE'], scratch + '/tour.bwstate')
        self.assertNotIn('BLUEWAKE_RENDERER', env)
        self.assertNotIn('BLUEWAKE_PLAYER_PROBE', env)
        self.assertNotIn('BLUEWAKE_DISC', env)
        self.assertEqual(env['PATH'], '/usr/bin')
        if sys.platform == 'win32':
            self.assertEqual(env['BLUEWAKE_DATA_DIR'], scratch)
            self.assertNotIn('BLUEWAKE_CARD_PATH', env)
        else:
            self.assertEqual(env['BLUEWAKE_CARD_PATH'], card)
            self.assertEqual(env['BLUEWAKE_SRAM'], scratch + '/sram.bin')
            self.assertEqual(env['BLUEWAKE_SETTINGS'], scratch + '/settings.ini')
            self.assertNotIn('BLUEWAKE_DATA_DIR', env)

    def test_state_sets_load_state_without_warp(self):
        env = bench_tour.measurement_environment(
            {'BLUEWAKE_TEST_WARP': '1:sea:44:0', 'BLUEWAKE_PLAYER_PROBE': '1'},
            mode='headless',
            load_state='scene.bwstate',
            scratch='/tmp/bluewake-bench-scratch',
            card='/tmp/bluewake-bench-scratch/GZLE01.card',
            warp=None,
            max_retraces=11400,
        )
        self.assertEqual(env['BLUEWAKE_LOAD_STATE'], 'scene.bwstate')
        self.assertNotIn('BLUEWAKE_TEST_WARP', env)
        self.assertEqual(env['BLUEWAKE_RENDERER'], 'headless')
        self.assertNotIn('BLUEWAKE_PLAYER_PROBE', env)

    def test_card_command_is_an_argument_list(self):
        argv = bench_tour.card_restart_argv('in.card', 'out.card')
        script = Path(__file__).parents[1] / 'scripts' / 'card_set_restart.py'
        self.assertEqual(argv, [sys.executable, str(script), 'in.card', 'out.card', 'sea', '44', '0'])


FAKE_APP = r"""#!/usr/bin/env python3
import hashlib
import os
from pathlib import Path
import sys

here = Path(__file__).resolve().parent
counter = here / "invocations.txt"
count = int(counter.read_text().strip() or 0) if counter.exists() else 0
count += 1
counter.write_text(str(count))

save = os.environ.get("BLUEWAKE_SAVE_STATE")
if save:
    path, _, at = save.rpartition("@")
    Path(path).write_bytes(b"setup-%d" % count)
    print("[boot-milestone] title-ready retrace=328 blocks=1")
    print("[boot-milestone] file-select retrace=454 blocks=1")
    print("[boot-milestone] play-scene retrace=609 blocks=1")
    print("[state] saved %s retrace=%s" % (path, at))
    sys.exit(0)

state = Path(os.environ["BLUEWAKE_LOAD_STATE"]).read_bytes()
cpu = hashlib.md5(state).hexdigest().upper()

warp = os.environ.get("BLUEWAKE_TEST_WARP", "")
stops = []
for part in warp.split(","):
    if not part:
        continue
    at, stage = part.split(":")[0], part.split(":")[1]
    stops.append((int(at), stage))
end = (stops[-1][0] + 1800) if stops else 4200
stage_at = dict(stops)
for retrace in range(2400, end + 1, 100):
    print("[perf] retrace=%d rate=60.0 worst_ms=16.0 hitches=0 busy=99%%" % retrace)
    if retrace in stage_at:
        print("[load] retrace=%d overlap=0 stage=%s cpu_us=1 wall_us=1" % (retrace, stage_at[retrace]))
    if retrace % 300 == 0 and retrace >= 2400:
        print("[guest-checkpoint] version=1 retrace=%d cycle=%d cpu=%s mem1=%s mem2=%s "
              "aliases=%s alias_spans=1 alias_bytes=8"
              % (retrace, retrace * 1000, cpu, "1" * 32, "2" * 32, "3" * 32))
"""


FAKE_APP_NAME_ENTRY = r"""#!/usr/bin/env python3
import os
from pathlib import Path
import sys

save = os.environ.get("BLUEWAKE_SAVE_STATE")
if not save:
    # The setup pass was accepted, so a measurement pass ran: that is the bug.
    sys.exit(3)
path, _, at = save.rpartition("@")
Path(path).write_bytes(b"setup-name-entry")
print("[boot-milestone] title-ready retrace=328 blocks=1")
print("[boot-milestone] file-select retrace=454 blocks=1")
print("[boot-milestone] name-scene-create retrace=632 blocks=1")
print("[boot-milestone] name-input-complete retrace=700 blocks=1")
print("[state] saved %s retrace=%s" % (path, at))
sys.exit(0)
"""


@contextmanager
def working_directory(path):
    previous = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(previous)


@unittest.skipIf(sys.platform == 'win32', 'the fake app is a POSIX script')
class SetupStateTests(unittest.TestCase):
    """--setup-state: one card route, then the same guest state for every run."""

    def setUp(self):
        # A temporary directory the fake app can be executed from: some
        # sandboxes mount /tmp noexec.
        self.directory = tempfile.TemporaryDirectory(dir=Path.home())
        self.root = Path(self.directory.name)
        self.app = self.root / 'fake_app.py'
        self.app.write_text(FAKE_APP)
        self.app.chmod(0o755)
        self.state = self.root / 'tour.bwstate'
        self.card_argv = bench_tour.card_restart_argv
        bench_tour.card_restart_argv = lambda source, dest: [
            sys.executable, '-c', 'import sys; sys.exit(0)']

    def tearDown(self):
        bench_tour.card_restart_argv = self.card_argv
        self.directory.cleanup()

    def invocations(self):
        counter = self.root / 'invocations.txt'
        return int(counter.read_text().strip()) if counter.exists() else 0

    def run_twice(self, setup_state):
        for name in ('run1', 'run2'):
            (self.root / name).mkdir()
            with working_directory(self.root / name):
                code = bench_tour.run_tour(
                    'card', str(self.app), stop_retraces=600, untrained=False,
                    mode='headless', state=None, setup_state=setup_state,
                )
                self.assertEqual(code, 0)
        left = json.loads((self.root / 'run1' / 'summary.json').read_text())
        right = json.loads((self.root / 'run2' / 'summary.json').read_text())
        return left, right

    def test_a_kept_state_serves_the_later_run(self):
        left, right = self.run_twice(str(self.state))
        self.assertTrue(self.state.is_file())
        self.assertEqual(self.invocations(), 3)
        code, text = bench_tour.compare_summaries(left, right)
        self.assertEqual(code, 0, text)
        self.assertIn('checkpoints match', text)

    def test_without_it_each_run_plays_the_card_route(self):
        left, right = self.run_twice(None)
        self.assertEqual(self.invocations(), 4)
        code, text = bench_tour.compare_summaries(left, right)
        self.assertNotEqual(code, 0)
        self.assertIn('checkpoints differ', text)

    def make_app(self, text):
        app = self.root / 'fake_app.py'
        app.write_text(text)
        app.chmod(0o755)
        return app

    def test_a_setup_pass_that_never_plays_is_refused(self):
        app = self.make_app(FAKE_APP_NAME_ENTRY)
        (self.root / 'run').mkdir()
        with working_directory(self.root / 'run'):
            code = bench_tour.run_tour(
                'card', str(app), stop_retraces=600, untrained=False,
                mode='headless', state=None, setup_state=str(self.state),
            )
        self.assertNotEqual(code, 0)
        self.assertFalse((self.root / 'run' / 'summary.json').exists(),
                         'no measurement may run from that state')
        self.assertFalse(self.state.is_file(),
                         'a refused state must not be left for a later run')

    def test_the_refusal_names_the_last_milestone(self):
        state = self.root / 's.bwstate'
        state.write_bytes(b'name-entry')
        log = ('[boot-milestone] file-select retrace=454 blocks=1 scene=0x00000000\n'
               '[boot-milestone] name-scene-create retrace=632 blocks=1\n'
               '[boot-milestone] name-input-complete retrace=700 blocks=1\n')
        code, problem = bench_tour.setup_verdict(0, state, log)
        self.assertEqual(code, 1)
        self.assertIn('never reached gameplay', problem)
        self.assertIn('name-input-complete', problem)

    def test_a_setup_pass_that_plays_is_accepted(self):
        state = self.root / 's.bwstate'
        state.write_bytes(b'play')
        log = ('[boot-milestone] file-select retrace=454 blocks=1 scene=0x00000000\n'
               '[boot-milestone] play-scene retrace=609 blocks=4\n')
        self.assertEqual(bench_tour.setup_verdict(0, state, log), (0, ''))

    def test_the_summary_line_alone_counts_as_gameplay(self):
        state = self.root / 's.bwstate'
        state.write_bytes(b'play')
        log = ('[boot-milestone] summary title_ready=1 title_retrace=328 '
               'play_scene=1 play_scene_retrace=609\n')
        self.assertEqual(bench_tour.setup_verdict(0, state, log), (0, ''))

    def test_a_setup_pass_that_wrote_no_state_is_refused(self):
        code, problem = bench_tour.setup_verdict(0, self.root / 'missing.bwstate',
                                                 '[boot-milestone] play-scene retrace=609 blocks=4\n')
        self.assertEqual(code, 1)
        self.assertIn('wrote no state', problem)


if __name__ == '__main__':
    unittest.main()
