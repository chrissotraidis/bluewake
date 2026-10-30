"""Synthetic checks for scripts/render_bench and render_bench.py. No disc, card or app:
tests/render_fixtures/fake_bluewake.py writes the log lines and frames instead."""

from pathlib import Path
import gzip
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import argparse

ROOT = Path(__file__).parents[1]
SCRIPTS = ROOT / 'scripts'
FAKE = ROOT / 'tests' / 'render_fixtures' / 'fake_bluewake.py'
sys.path.insert(0, str(SCRIPTS))
import render_bench
import rendered_run

try:
    import numpy  # noqa: F401
    import PIL  # noqa: F401
    HAVE_IMAGES = True
except ImportError:
    HAVE_IMAGES = False


def write_state(path, retrace):
    """A save state with only its header, as render_bench.state_retrace reads it."""
    with gzip.open(path, 'wb') as out:
        out.write(b'BWSTATE1' + bytes(4) + b'HEADER'.ljust(16, b'\0') + (40).to_bytes(8, 'little')
                  + bytes(32) + retrace.to_bytes(8, 'little'))


class Work(unittest.TestCase):
    def setUp(self):
        self.dir = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.dir)
        (self.dir / 'card').write_bytes(b'card')

    def app(self, name, commit='0123456789ab', file=None):
        path = self.dir / 'builds' / f'{name}-{commit}' / (file or f'{name}.AppImage')
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(FAKE, path)
        path.chmod(0o755)
        return path

    def bench(self, *builds, extra=(), cache=True, file=None):
        """collect then analyze, here; (exit of analyze, summary, report)."""
        out = self.dir / ('out-' + '-'.join(b for b in builds))
        if cache:
            (self.dir / 'cache').mkdir(exist_ok=True)
            if not (self.dir / 'cache' / 'tour.bwstate').exists():
                write_state(self.dir / 'cache' / 'tour.bwstate', 2391)
        apps = {name: self.app(build, commit, file) for name, build, commit in
                zip('AB', builds, ('aaaaaaaaaaaa', 'bbbbbbbbbbbb'))}
        cmd = [sys.executable, str(SCRIPTS / 'render_bench.py'), 'collect', str(out), '--card', str(self.dir / 'card'),
               '--cache', str(self.dir / 'cache'), '--display', 'desktop', *extra]
        for name, app in apps.items():
            cmd += ['--app', f'{name}={app}']
        done = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(done.returncode, 0, done.stdout[-3000:] + done.stderr)
        cmd = [sys.executable, str(SCRIPTS / 'render_bench.py'), 'analyze', str(out)]
        for name, app in apps.items():
            cmd += ['--build', f'{name}={app}']
        code = subprocess.run(cmd, capture_output=True, text=True).returncode
        return code, json.loads((out / 'summary.json').read_text()), (out / 'report.md').read_text()


class Plan(unittest.TestCase):
    def test_one_build_runs_and_pairs_interleave(self):
        args = argparse.Namespace(runs=3, pairs=2, warmup=1)
        self.assertEqual(render_bench.plan(['A'], args), [('A', False), ('A', True), ('A', True), ('A', True)])
        self.assertEqual([b for b, measured in render_bench.plan(['A', 'B'], args) if measured],
                         ['A', 'B', 'B', 'A', 'A', 'B', 'B', 'A'])
        self.assertEqual(render_bench.plan(['A', 'B'], args)[:2], [('A', False), ('B', False)])

    def test_the_commit_comes_from_build_script_paths(self):
        with tempfile.TemporaryDirectory() as d:
            for folder in ('artifacts/main-090/211abfdecf13', 'gpr-8d8587b37c36'):
                image = Path(d) / folder / 'BlueWake-x86_64.AppImage'
                image.parent.mkdir(parents=True)
                image.write_bytes(b'x')
                self.assertEqual(render_bench.build_info(image)['commit'], folder[-12:])
                self.assertEqual(render_bench.build_info(image.parent)['commit'], folder[-12:])
            loose = Path(d) / 'BlueWake.AppImage'
            loose.write_bytes(b'x')
            self.assertIsNone(render_bench.build_info(loose)['commit'])

    def test_a_mac_app_is_a_build_with_its_own_commit(self):
        with tempfile.TemporaryDirectory() as d:
            app = Path(d) / 'build' / 'macos' / 'packaged' / 'BlueWake.app'
            (app / 'Contents' / 'MacOS').mkdir(parents=True)
            (app / 'Contents' / 'Resources').mkdir()
            (app / 'Contents' / 'MacOS' / 'BlueWake').write_bytes(b'x')
            (app / 'Contents' / 'Resources' / 'BuilderProvenance.json').write_text(json.dumps(
                {'source_commit': '82371ebf15adaa2fdaefd5e3a69af004e378d2f6', 'source_modified': False,
                 'module_sha256': '29' * 32}))
            for given in (app, app / 'Contents' / 'MacOS' / 'BlueWake', app.parents[1]):
                info = render_bench.build_info(given)
                self.assertEqual((info['commit'], info['path'], info['module_sha256']),
                                 ('82371ebf15ad', str(app.resolve()), '29' * 32), given)

    def test_a_linux_folder_build_is_a_build_with_its_own_commit(self):
        with tempfile.TemporaryDirectory() as d:
            folder = Path(d) / 'build' / 'portable-new' / 'BlueWake'
            folder.mkdir(parents=True)
            (folder / 'bluewake').write_bytes(b'x')
            (folder / 'bluewake').chmod(0o755)
            (folder / 'BuilderProvenance.json').write_text(json.dumps(
                {'source_commit': 'b1bf26a0b8a5c1c6e86b9351d658806ff9eb3c1c', 'source_modified': False,
                 'module_sha256': '41' * 32}))
            for given in (folder, folder.parent, folder / 'bluewake'):
                info = render_bench.build_info(given)
                self.assertEqual((info['commit'], info['path'], info['module_sha256']),
                                 ('b1bf26a0b8a5', str(folder.resolve()), '41' * 32), given)
            # Without a provenance file, build_script's paths as for an AppImage.
            (folder / 'BuilderProvenance.json').unlink()
            plain = Path(d) / 'gpr-8d8587b37c36'
            folder.rename(plain)
            self.assertEqual(render_bench.build_info(plain)['commit'], '8d8587b37c36')

    def test_a_windows_build_is_a_build_with_its_own_commit(self):
        with tempfile.TemporaryDirectory() as d:
            folder = Path(d) / 'build' / 'gpr-main' / 'BlueWake'
            folder.mkdir(parents=True)
            (folder / 'BlueWake.exe').write_bytes(b'x')
            (folder / 'BuilderProvenance.json').write_text(json.dumps(
                {'profile': 'bluewake-windows', 'source_commit': 'a7731853c197cc9763a86d62e731d65ca0b25556',
                 'source_modified': False, 'module_sha256': 'd9' * 32}))
            # The build folder scripts/windows/build.py makes (--out), its BlueWake/, or its BlueWake.exe.
            for given in (folder.parent, folder, folder / 'BlueWake.exe'):
                info = render_bench.build_info(given)
                self.assertEqual((info['commit'], info['path'], info['module_sha256']),
                                 ('a7731853c197', str(folder.resolve()), 'd9' * 32), given)
                self.assertEqual(render_bench.build_image(given), (folder / 'BlueWake.exe').resolve(), given)

    def test_a_file_that_is_not_a_state_has_no_retrace(self):
        with tempfile.NamedTemporaryFile() as f:
            f.write(b's\n')
            f.flush()
            self.assertIsNone(render_bench.state_retrace(f.name))

    def test_perf_outputs_are_parsed_for_reports(self):
        with tempfile.TemporaryDirectory() as d:
            stat = Path(d) / 'perf-stat.csv'
            stat.write_text('# started on Thu Oct 29 12:09:32 2026\n\n'
                            'bluewake-100;992.49;msec;task-clock:u;992493015;100.00;;\n'
                            'bluewake-101;400.00;msec;task-clock:u;400000000;100.00;;\n'
                            'bluewake-100;3993204915;;cycles:u;992493265;100.00;;\n'
                            'bluewake-101;<not counted>;;cycles:u;0;100.00;;\n')
            self.assertEqual(rendered_run.per_thread_stat(stat),
                             {100: {'task-clock': 992.49, 'cycles': 3993204915.0}, 101: {'task-clock': 400.0}})
            report = Path(d) / 'perf-report.txt'
            report.write_text('# Overhead  Shared Object  Symbol\n'
                              '    12.60%  gGZLE01_recomp.so  [.] func_802456E0\n'
                              '     4.45%  bluewake           [.] GxCoreState::build_draw_plan_into(int) const\n')
            symbols = render_bench.perf_symbols(report)
            self.assertEqual(symbols[0], {'overhead': 12.6, 'dso': 'gGZLE01_recomp.so', 'symbol': 'func_802456E0'})
            self.assertEqual(symbols[1]['symbol'], 'GxCoreState::build_draw_plan_into(int) const')

    def test_the_retraces_a_profile_covered_are_interpolated(self):
        points = [(10.0, 1000), (11.0, 1060), (12.0, 1120)]
        self.assertEqual(rendered_run.retrace_at(points, 10.5), 1030)
        self.assertEqual(rendered_run.retrace_at(points, 12.5), 1150)  # past the last line: along the last two
        self.assertEqual(rendered_run.retrace_at(points, 9.0), 940)


class Runs(Work):
    def test_a_hung_run_is_stopped_and_marked(self):
        result = rendered_run.run(self.app('hang'), None, self.dir / 'run', {}, self.dir / 'status.txt', 0,
                                  timeout=1)
        self.assertTrue(result['timed_out'])

    def test_an_empty_quest_log_ends_the_run_at_once(self):
        result = rendered_run.run(self.app('empty'), None, self.dir / 'run', {'BLUEWAKE_WARP': 'sea:44:0:0'},
                                  self.dir / 'status.txt', 0, timeout=60, stop_on='is empty; stopped pressing A')
        self.assertIn('is empty', (self.dir / 'run' / 'log.txt').read_text())
        self.assertFalse(result['timed_out'])
        self.assertLess(result['seconds'], 15)


@unittest.skipUnless(HAVE_IMAGES, 'needs numpy and Pillow')
class Tour(Work):
    def test_a_against_b_scene_by_scene(self):
        code, summary, report = self.bench('plain', 'plain-slow', extra=('--pairs', '1'))
        self.assertEqual(code, 0, report)
        self.assertEqual([s['name'] for s in summary['scenes']],
                         ['Outset', 'Windfall', 'Dragon Roost Cavern', 'Forest Haven', 'sea by the Fortress'])
        self.assertEqual(summary['runs_per_build'], {'A': 2, 'B': 2})
        self.assertEqual(summary['loaded_runs'], 4)
        self.assertEqual(summary['checkpoint_differences'], [])
        self.assertEqual(summary['overall']['A']['fps'], 60.0)
        self.assertEqual(summary['overall']['B']['fps'], 25.0)
        self.assertEqual(summary['overall']['change'], {'fps': 140.0, 'busy': 0.0, 'gpu_ms': -50.0})
        self.assertEqual({p['verdict'] for p in summary['pictures']}, {'identical'})
        self.assertEqual({p['images'] for p in summary['pictures']}, {5})
        self.assertEqual(summary['info']['A']['commit'], 'aaaaaaaaaaaa')
        for text in ('`aaaaaaaaaaaa` (A) against `bbbbbbbbbbbb` (B, the reference)', '| Outset | 25.000 | 60.000 |',
                     '**+140.00%**', 'Guest checkpoints identical', 'faster overall by **140.00%**'):
            self.assertIn(text, report)
        out = self.dir / 'out-plain-plain-slow'
        text = (out / 'report.txt').read_text()
        self.assertIn('aaaaaaaaaaaa was faster overall by 140.00%.', text)
        self.assertRegex(text, r'Outset +25\.000 +60\.000 +\+140\.00%')
        self.assertNotIn('**', text)
        page = (out / 'report.html').read_text()
        self.assertIn('<td class=num><strong>+140.00%</strong></td>', page)
        self.assertIn('<code>aaaaaaaaaaaa</code>', page)

    def test_frames_a_retrace_apart_are_the_same_capture(self):
        code, summary, _ = self.bench('plain', 'plain-late', extra=('--pairs', '1'))
        self.assertEqual(code, 0)
        self.assertEqual({p['verdict'] for p in summary['pictures']}, {'identical'})

    def test_checkpoints_that_differ_fail(self):
        code, summary, report = self.bench('plain', 'plain-diverge', extra=('--pairs', '1'))
        self.assertEqual(code, 1)
        self.assertIn('at retrace 2700', summary['checkpoint_differences'][0])
        self.assertIn('**Guest checkpoints differ**', report)

    def test_pictures_that_differ_fail(self):
        code, summary, _ = self.bench('bright', 'plain', extra=('--pairs', '1'))
        self.assertEqual(code, 1)
        self.assertEqual({p['verdict'] for p in summary['pictures']}, {'changed'})

    def test_one_build(self):
        code, summary, report = self.bench('plain', extra=('--runs', '2'))
        self.assertEqual(code, 0, report)
        self.assertEqual(summary['measured'], 2)
        self.assertEqual(summary['pictures'], [])
        self.assertIn('| Outset | 60.000 |', report)
        self.assertNotIn('Conclusion', report)
        self.assertIn('FPS is at 60 in Outset', report)  # the fake runs at 60: as a display's refresh would hold it

    def test_the_state_is_made_from_the_first_quest_log_with_a_save(self):
        code, summary, _ = self.bench('empty12', extra=('--runs', '1'), cache=False)
        self.assertEqual(code, 0)
        self.assertEqual(render_bench.state_retrace(self.dir / 'cache' / 'tour.bwstate'), 1021)


def perf_usable():
    if sys.platform != 'linux' or not shutil.which('perf'):
        return False
    try:
        return int(Path('/proc/sys/kernel/perf_event_paranoid').read_text()) <= 2
    except (OSError, ValueError):
        return False


@unittest.skipUnless(HAVE_IMAGES and perf_usable(), 'needs numpy, Pillow and a usable Linux perf')
class Profile(Work):
    def test_each_scene_is_profiled_only_over_its_window(self):
        states = self.dir / 'states'
        states.mkdir()
        write_state(states / '1-outset.bwstate', 3000)
        # Named bluewake, as the profiler looks for BlueWake's process by name.
        code, summary, report = self.bench('realtime', extra=('--runs', '1', '--states', str(states),
                                                               '--cpu-profile'), file='bluewake')
        self.assertEqual(code, 0, report)
        [profile] = summary['cpu_profiles']
        first, last = profile['retraces']
        # Asked for 3300 to 4560 (300 in, 240 before the end): perf attaches and detaches within a [perf] line or two.
        self.assertTrue(3300 <= first < 3500 and 4400 < last <= 4700, profile['retraces'])
        self.assertGreater(profile['game']['cycles_per_frame'] or 0, 0)
        self.assertIn('### CPU profiles', report)
        self.assertIn('Game thread — 1-outset (A)', report)
        self.assertEqual(summary['measured'], 1)  # the profiled run is not measured


class MakeState(Work):
    def test_a_state_anywhere_from_the_first_quest_log_with_a_save(self):
        state = self.dir / 'states' / '2-windfall.bwstate'
        state.parent.mkdir()
        done = subprocess.run([sys.executable, str(SCRIPTS / 'render_bench.py'), 'make-state', str(state),
                               '--app', str(self.app('empty12')), '--warp', 'sea:11:1:0', '--card',
                               str(self.dir / 'card'), '--display', 'desktop'], capture_output=True, text=True)
        self.assertEqual(done.returncode, 0, done.stdout[-2000:] + done.stderr)
        self.assertEqual(render_bench.state_retrace(state), 1021)
        self.assertIn('from quest log 3', done.stdout)
        self.assertEqual([p.name for p in state.parent.iterdir()], ['2-windfall.bwstate'])  # no work left behind


@unittest.skipUnless(HAVE_IMAGES, 'needs numpy and Pillow')
class States(Work):
    def test_each_state_is_a_scene_named_after_its_file_in_lexical_order(self):
        states = self.dir / 'states'
        states.mkdir()
        for name, retrace in (('2-cavern', 5000), ('10-sea', 7000), ('1-outset', 3000)):
            write_state(states / f'{name}.bwstate', retrace)
        (states / 'notes.txt').write_text('ignored')
        code, summary, report = self.bench('plain', 'plain', extra=('--pairs', '1', '--states', str(states),
                                                                     '--retraces', '900'))
        self.assertEqual(code, 0, report)
        self.assertEqual([s['name'] for s in summary['scenes']], ['1-outset', '10-sea', '2-cavern'])
        self.assertEqual(summary['states']['1-outset']['retrace'], 3000)
        self.assertEqual({p['verdict'] for p in summary['pictures']}, {'identical'})


def wrapper_env(**more):
    """The wrapper runs `python3` from PATH: put this Python first, the one with numpy and Pillow (a venv in CI)."""
    path = os.path.dirname(sys.executable) + os.pathsep + os.environ.get('PATH', '')
    return dict(os.environ, PATH=path, **more)


@unittest.skipUnless(HAVE_IMAGES, 'needs numpy and Pillow')
class Wrapper(Work):
    def test_one_folder_a_run_with_the_reports_flat_in_it(self):
        home = self.dir / 'home'
        data = home / ('Library/Application Support/BlueWake' if sys.platform == 'darwin' else '.local/share/BlueWake')
        data.mkdir(parents=True)
        (data / 'GZLE01.card').write_bytes(b'card')  # the tour's state is made from it
        here = self.dir / 'here'
        here.mkdir()
        done = subprocess.run(['bash', str(SCRIPTS / 'render_bench'), str(self.app('bright')),
                               str(self.app('plain')), '--pairs', '1', '--display', 'desktop'],
                              cwd=here, env=wrapper_env(HOME=str(home)), capture_output=True, text=True)
        self.assertEqual(done.returncode, 1, done.stdout[-2000:] + done.stderr)  # the pictures changed
        [folder] = list(here.iterdir())
        self.assertRegex(folder.name, r'^\d{4}_\d{2}_\d{2}-\d{6}$')
        self.assertEqual(sorted(p.name for p in folder.iterdir() if not p.name.startswith('worst-')),
                         ['raw', 'report.html', 'report.md', 'report.txt', 'summary.json'])
        self.assertEqual(len(list(folder.glob('worst-*.png'))), 5)
        self.assertIn('worst-Outset.png', (folder / 'report.md').read_text())
        self.assertIn('BlueWake A/B benchmark', done.stdout)  # report.txt, printed at the end


    def test_linux_folder_builds_are_builds_to_the_wrapper(self):
        folders = []
        for name in ('bright', 'plain'):
            folder = self.dir / name / 'BlueWake'
            shutil.copytree(self.app(name).parent, folder)
            (folder / f'{name}.AppImage').rename(folder / 'bluewake')
            folders.append(str(folder.parent))  # the portable builder's folder, holding BlueWake/
        here = self.dir / 'here'
        here.mkdir()
        done = subprocess.run(['bash', str(SCRIPTS / 'render_bench'), *folders, '--pairs', '1', '--display', 'desktop',
                               '--card', str(self.dir / 'card')], cwd=here, env=wrapper_env(HOME=str(self.dir)),
                              capture_output=True, text=True)
        self.assertEqual(done.returncode, 1, done.stdout[-2000:] + done.stderr)  # the pictures changed
        self.assertIn('A: ' + str((self.dir / 'bright/BlueWake/bluewake').resolve()), done.stdout)

    def test_mac_apps_are_builds_to_the_wrapper(self):
        apps = []
        for name in ('bright', 'plain'):
            app = self.dir / name / 'packaged' / 'BlueWake.app'
            shutil.copytree(self.app(name).parent, app / 'Contents' / 'MacOS')
            (app / 'Contents' / 'MacOS' / f'{name}.AppImage').rename(app / 'Contents' / 'MacOS' / 'BlueWake')
            apps.append(str(app.parents[1]))  # the build folder, as build/macos is
        here = self.dir / 'here'
        here.mkdir()
        done = subprocess.run(['bash', str(SCRIPTS / 'render_bench'), *apps, '--pairs', '1', '--display', 'desktop',
                               '--card', str(self.dir / 'card')], cwd=here, env=wrapper_env(HOME=str(self.dir)),
                              capture_output=True, text=True)
        self.assertEqual(done.returncode, 1, done.stdout[-2000:] + done.stderr)  # the pictures changed
        self.assertIn('A: ' + str((self.dir / 'bright/packaged/BlueWake.app/Contents/MacOS/BlueWake').resolve()),
                      done.stdout)


class Arguments(unittest.TestCase):
    def test_help_and_bad_arguments(self):
        script = SCRIPTS / 'render_bench'
        self.assertEqual(subprocess.run(['bash', str(script), '--help'], capture_output=True).returncode, 0)
        for bad in (['a', 'b', 'c'], ['a', '--pairs', 'x'], ['a', '--colour']):
            self.assertEqual(subprocess.run(['bash', str(script), *bad], capture_output=True).returncode, 2, bad)
        help_text = subprocess.run(['bash', str(script), '--help'], capture_output=True, text=True).stdout
        self.assertIn('--cpu-profile', help_text)


if __name__ == '__main__':
    unittest.main()
