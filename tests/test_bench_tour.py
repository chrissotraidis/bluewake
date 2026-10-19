"""Synthetic checks for the benchmark tour. No disc, card, or app."""

from pathlib import Path
import copy
import json
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


def qualification_summary(modes=('rendered', 'headless')):
    stops = [{'code': 'sea:44:0', 'loaded': True}]
    points = [checkpoint(300, 'A' * 32), checkpoint(600, 'B' * 32)]
    return {
        'requested_modes': list(modes),
        'expected_stop_codes': ['sea:44:0'],
        'modes': {mode: {'stops': copy.deepcopy(stops),
                         'checkpoints': copy.deepcopy(points)} for mode in modes},
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

    def test_worker_override_is_explicit_and_applied_after_sanitizing(self):
        env = bench_tour.measurement_environment(
            {'BLUEWAKE_NATIVE_WORKERS': '7', 'BLUEWAKE_GAMETHREAD_DIAGNOSTICS': '0'},
            mode='rendered', load_state='state', scratch='/tmp/scratch',
            card='/tmp/scratch/card', warp=None, max_retraces=10,
            overrides={'BLUEWAKE_NATIVE_WORKERS': 2,
                       'BLUEWAKE_GAMETHREAD_DIAGNOSTICS': 1},
        )
        self.assertEqual(env['BLUEWAKE_NATIVE_WORKERS'], '2')
        self.assertEqual(env['BLUEWAKE_GAMETHREAD_DIAGNOSTICS'], '1')
        with self.assertRaises(ValueError):
            bench_tour.measurement_environment(
                {}, mode='rendered', load_state='state', scratch='/tmp/scratch',
                card='/tmp/scratch/card', warp=None, max_retraces=10,
                overrides={'UNRELATED': 1},
            )

    def test_qualification_requires_complete_comparable_evidence(self):
        complete = qualification_summary()
        self.assertEqual(bench_tour.qualification_errors(complete), [])

        missing_mode = qualification_summary()
        del missing_mode['modes']['headless']
        self.assertTrue(bench_tour.qualification_errors(missing_mode))

        unloaded = qualification_summary()
        unloaded['modes']['rendered']['stops'][0]['loaded'] = False
        self.assertTrue(bench_tour.qualification_errors(unloaded))

        wrong_grid = qualification_summary()
        wrong_grid['modes']['headless']['checkpoints'][1]['retrace'] = 900
        self.assertTrue(bench_tour.qualification_errors(wrong_grid))

        failed = qualification_summary()
        failed['modes']['rendered']['checkpoints'][0]['failed'] = True
        self.assertTrue(bench_tour.qualification_errors(failed))

    def test_state_qualification_requires_one_loaded_stop(self):
        body = qualification_summary(('rendered',))
        body['expected_stop_codes'] = None
        self.assertEqual(bench_tour.qualification_errors(body), [])
        body['modes']['rendered']['stops'].append({'code': 'sea:11:1', 'loaded': True})
        self.assertTrue(bench_tour.qualification_errors(body))

    def test_qualification_compare_uses_distinct_incomplete_exit(self):
        body = qualification_summary()
        body['modes']['rendered']['checkpoints'] = []
        with tempfile.TemporaryDirectory() as directory:
            left = Path(directory) / 'left.json'
            right = Path(directory) / 'right.json'
            left.write_text(json.dumps(body), encoding='utf-8')
            right.write_text(json.dumps(qualification_summary()), encoding='utf-8')
            self.assertEqual(
                bench_tour._compare_main(['--qualification', str(left), str(right)]),
                3,
            )

    def test_card_command_is_an_argument_list(self):
        argv = bench_tour.card_restart_argv('in.card', 'out.card')
        script = Path(__file__).parents[1] / 'scripts' / 'card_set_restart.py'
        self.assertEqual(argv, [sys.executable, str(script), 'in.card', 'out.card', 'sea', '44', '0'])


if __name__ == '__main__':
    unittest.main()
