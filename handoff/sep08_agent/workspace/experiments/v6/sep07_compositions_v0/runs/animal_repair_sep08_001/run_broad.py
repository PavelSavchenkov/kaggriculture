"""Register and run a broad independent population audit, retaining telemetry."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
assert json.loads((RUN / 'OPERATIONAL_CHECKS.json').read_text())['games'] == 56
assert json.loads((RUN / 'FIXTURE_CHECKS.json').read_text())['games'] == 16
assert json.loads((RUN / 'discovery/EXECUTION.json').read_text())['sources_unchanged']
out = RUN / 'broad_2360000'
out.mkdir(exist_ok=False)
old = json.loads((EXP / 'results/empty_sale_slots_m2_population_validation.json').read_text())
opponents = list(old['fresh']['metrics']['empty_sale_slots_m2'])
opponents += ['yusuke_sep08_m2', 'ahmed_v24', 'arlene_v4_m31', 'salem_sep08_m3', 'titan_frontier']
assert len(opponents) == len(set(opponents)) == 40
agents = ['empty_sale_slots_m2', 'animal_repair_premium_m2', 'animal_repair_q24_premium_m2']
primary = ['teammate_shoprouter', 'public_router', 'public_router_v52', 'ahmed_v23',
    'junghoon_wool_sales', 'king_rc4', 'yusuke_sep08_m2', 'ahmed_v24']
historical = [b for b in opponents if b not in primary and b != 'empty_sale_slots_m2']
native = [*primary, 'public_router_v5', 'arlene_v4_m31', 'salem_sep08_m3', 'titan_frontier', 'pass']
jobs = [{'a': a, 'b': b, 'seeds': 512, 'seed_start': 2360000, 'native': False} for b in opponents for a in agents]
jobs += [{'a': a, 'b': b, 'seeds': 128, 'seed_start': 2364000, 'native': True} for b in native for a in agents]
protocol = {'created_utc': datetime.now(timezone.utc).isoformat(), 'agents': agents, 'opponents': opponents,
    'primary': primary, 'historical': historical, 'native_opponents': native, 'jobs': jobs,
    'games': sum(j['seeds'] * 2 for j in jobs), 'seats': 'both',
    'selection': 'Compare each candidate with accepted parent on identical seeds and both seats. Seed-cluster intervals resample opponents jointly. Select eligible highest primary win score, then primary mean margin. No automatic promotion.',
    'eligibility': [
        'All required operations, active repair checks, source freezing and full719-action validation pass.',
        'Fresh primary utility gain and mean-margin gain have positive95% lower bounds.',
        'Fresh historical utility and mean-margin gains have nonnegative95% lower bounds.',
        'Native primary utility and mean-margin gains have positive95% lower bounds.',
        'Candidate point win score exceeds50% against every primary opponent in fresh and native panels.',
        'Every activated missed day guard is investigated; no unexplained animal-service loss or invalid runtime behavior remains.',
        'Inspect and report every-opponent win/cash/margin/tail regressions and all candidate-parent comparisons before deciding.'
    ],
    'tradeoffs': 'This new independent population audit permits individual losing games and opponent mean regressions. It targets broad match strength. It does not retroactively pass the earlier q24 positive-margin failure or any earlier strict gate. King regression remains explicit.',
    'scope': 'Broad evidence for strongest complete agent; frozen rebuild and any unresolved execution audit remain required before promotion. Final900000 unused.'}
(out / 'PROTOCOL.json').write_text(json.dumps(protocol, indent=2) + '\n')
command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py'),
    '--agents', *sorted(set(agents + opponents + ['pass']))]
built = subprocess.run(command, capture_output=True, text=True)
(out / 'build_generic.log').write_text(built.stdout + built.stderr)
built.check_returncode()
generic = Path(built.stdout.strip().splitlines()[-1])
build = RUN / 'broad_build'
build.mkdir(exist_ok=False)
metadata = json.loads((generic.parent / 'build.json').read_text())
compile_command = metadata['command'][:]
compile_command[compile_command.index(str(EXP / 'src/arena.cpp'))] = str(RUN / 'source/broad.cpp')
binary = build / 'arena'
compile_command[-1] = str(binary)
(build / 'build.json').write_text(json.dumps(dict(metadata, command=compile_command), indent=2) + '\n')
with (out / 'build_telemetry.log').open('x') as log:
    subprocess.run(compile_command, stdout=log, stderr=subprocess.STDOUT, check=True)
subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/freeze_arena_inputs.py'), str(binary), str(out / 'freeze')], check=True)
frozen = json.loads((out / 'freeze/FROZEN.json').read_text())['files_sha256']

def run(a, b, seeds, seed_start, native, path, runner):
    command = ['conda', 'run', '-n', 'kaggriculture', str(runner), '--a', a, '--b', b, '--games', str(seeds),
        '--seed-start', str(seed_start), '--seat-mode', 'both', '--threads', '4', '--budget-expansions', '100000',
        '--validate', '--profile', '--output', str(path)]
    if native:
        command.append('--native-shops')
    with path.with_suffix('.log').open('x') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    return command

# Check all three diagnostic adapter paths against the ordinary generic runner.
controls = []
for a in agents:
    paths = [out / f'control_{a}_{kind}.json' for kind in ('generic', 'telemetry')]
    for path, runner in zip(paths, (generic, binary)):
        controls.append(run(a, 'king_rc4', 2, 1014, False, path, runner))
    assert json.loads(paths[0].read_text())['games'] == json.loads(paths[1].read_text())['games']
(out / 'TELEMETRY_PARITY.json').write_text(json.dumps({'commands': controls, 'games': 24, 'full_records_equal': 12}, indent=2) + '\n')
print('Broad build and telemetry parity complete; starting', protocol['games'], 'games.', flush=True)

def match(job):
    prefix = 'native_' if job['native'] else ''
    path = out / f"{prefix}{job['a']}_vs_{job['b']}.json"
    command = run(job['a'], job['b'], job['seeds'], job['seed_start'], job['native'], path, binary)
    print(path.stem, 'complete', flush=True)
    return command

with ThreadPoolExecutor(max_workers=3) as pool:
    commands = list(pool.map(match, jobs))
assert all(hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == h for p, h in frozen.items())
(out / 'EXECUTION.json').write_text(json.dumps({'commands': commands, 'games': protocol['games'],
    'sources_unchanged': True, 'telemetry_parity_games': 24}, indent=2) + '\n')
print('Broad comparison complete. Promotion requires the declared evidence review.')
