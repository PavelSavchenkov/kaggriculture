"""Separate cow execution savings from repriced animal choices on paired leagues."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
assert json.loads((RUN / 'FIXTURE_CHECKS.json').read_text())['games'] == 16
assert json.loads((RUN / 'OPERATIONAL_CHECKS.json').read_text())['games'] == 32
generic = Path(json.loads((RUN / 'OPERATIONAL_BINARIES.json').read_text())['generic']['binary'])
OUT = RUN / 'discovery_2390000'
OUT.mkdir(exist_ok=False)
agents = ['animal_repair_q24_premium_m2', 'cow_service_retained_fixed_choices_m2', 'cow_service_retained_q24_premium_m2']
primary = ['teammate_shoprouter', 'public_router', 'public_router_v52', 'ahmed_v23',
    'ahmed_v24', 'ahmed_v25', 'junghoon_wool_sales', 'king_rc4', 'yusuke_sep08_m2']
opponents = [*primary, 'empty_sale_slots_m2', 'animal_repair_q24_premium_m2', 'pass']
jobs = [dict(a=a, b=b, native=native, seeds=64 if native else 128,
    seed_start=2391000 if native else 2390000) for native in (False, True) for b in opponents for a in agents]
protocol = {'agents': agents, 'primary': primary, 'opponents': opponents, 'jobs': jobs,
    'games': sum(job['seeds']*2 for job in jobs), 'seats': 'both',
    'questions': ['Do cheaper exact cow courses improve whole games while keeping original investment choices?',
        'Does repricing their marginal labor cost improve observed-shop animal selection?',
        'Which extra activations, output, trade, guard and tail changes explain the result?'],
    'scope': 'Discovery only, no automatic promotion. All native and independent-shop panels remain separate.',
    'advance': 'Consider fresh confirmation only if primary mean win score is nondecreasing and paired primary margin gain has a positive95% seed-cluster lower bound in both panels. Explain all per-opponent regressions and all missed guards; never relax the accepted-reference promotion protocol based on this screen.'}
(OUT / 'PROTOCOL.json').write_text(json.dumps(protocol, indent=2) + '\n')
metadata = json.loads((generic.parent / 'build.json').read_text())
build = RUN / 'diagnostics_build'
build.mkdir(exist_ok=False)
command = metadata['command'][:]
command[command.index(str(EXP / 'src/arena.cpp'))] = str(RUN / 'source/diagnostics.cpp')
command[-1] = str(build / 'arena')
(build / 'build.json').write_text(json.dumps(dict(metadata, command=command), indent=2) + '\n')
with (build / 'build.log').open('w') as log:
    subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
binary = build / 'arena'
subprocess.run(['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/freeze_arena_inputs.py'),
    str(binary), str(OUT / 'freeze')], check=True)
frozen = json.loads((OUT / 'freeze/FROZEN.json').read_text())['files_sha256']

def invoke(job, path, runner):
    command = ['conda', 'run', '-n', 'kaggriculture', str(runner), '--a', job['a'], '--b', job['b'],
        '--games', str(job['seeds']), '--seed-start', str(job['seed_start']), '--seat-mode', 'both',
        '--threads', '3', '--budget-expansions', '100000', '--validate', '--profile', '--output', str(path)]
    if job['native']:
        command.append('--native-shops')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    records = json.loads(path.read_text())['games']
    assert len(records) == job['seeds']*2 and all(row['turns'] == 719 for row in records)
    return command, records

for agent in agents:
    job = dict(a=agent, b='king_rc4', seeds=2, seed_start=1014, native=False)
    records = [invoke(job, OUT / f'control_{agent}_{label}.json', runner)[1]
        for label, runner in (('generic', generic), ('telemetry', binary))]
    assert records[0] == records[1]
(OUT / 'TELEMETRY_PARITY.json').write_text(json.dumps({'games': 24, 'full_records_equal': 12}) + '\n')
print('Starting', protocol['games'], 'paired discovery games.', flush=True)

def run(job):
    name = ('native_' if job['native'] else '') + job['a'] + '_vs_' + job['b']
    command, records = invoke(job, OUT / (name + '.json'), binary)
    row = dict(job, command=command, games=len(records),
        wtl=[sum(g['cash'] > g['opponent_cash'] for g in records),
             sum(g['cash'] == g['opponent_cash'] for g in records),
             sum(g['cash'] < g['opponent_cash'] for g in records)],
        mean_margin=sum(g['cash'] - g['opponent_cash'] for g in records)/len(records))
    print(name, row['wtl'], round(row['mean_margin'], 2), flush=True)
    return row

with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
assert all(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == digest for path, digest in frozen.items())
(OUT / 'EXECUTION.json').write_text(json.dumps({'games': protocol['games'], 'sources_unchanged': True,
    'results': results}, indent=2) + '\n')
