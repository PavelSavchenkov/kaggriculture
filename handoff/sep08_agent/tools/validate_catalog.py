"""Validate the catalog against full session records and typed/instance controls."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import argparse
import hashlib
import json
import subprocess

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
ENV = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture']
parser = argparse.ArgumentParser()
parser.add_argument('binary', type=Path)
parser.add_argument('--reference-root', type=Path, default=HERE / '_work/session')
parser.add_argument('--output', type=Path, default=HERE / '_work/catalog_validation')
args = parser.parse_args()
REFERENCE = args.reference_root.resolve()
EXP = REFERENCE / 'experiments/v6/sep07_compositions_v0'
binary = args.binary.resolve()
out = args.output.resolve()
out.mkdir(exist_ok=False)
packages = json.loads((HERE / 'evidence/catalog.json').read_text())
names = [p['name'] for p in packages]
commands = []


def run(label, a, b, seeds=2, start=1014, native=False, threads=2, typed=False):
    path = out / (label + '.json')
    command = ENV + [str(binary), '--a', a, '--b', b, '--games', str(seeds), '--seed-start', str(start),
        '--seat-mode', 'both', '--threads', str(threads), '--budget-expansions', '100000', '--validate', '--output', str(path)]
    if native: command.append('--native-shops')
    if typed: command.append('--typed-pass')
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    commands.append(command)
    records = json.loads(path.read_text())['games']
    assert len(records) == seeds * 2 and all(g['turns'] == 719 for g in records)
    return records


def check(name):
    generic = run(name + '_pass', name, 'pass')
    typed = run(name + '_typed_pass', name, 'pass', typed=True)
    one = run(name + '_one_thread', name, 'pass', threads=1)
    assert generic == typed == one, name
    run(name + '_self', name, name)
    return {'name': name, 'games': 16, 'typed_generic_thread_records_equal': 4, 'self_pass_complete': True}


with ThreadPoolExecutor(max_workers=4) as pool:
    operational = list(pool.map(check, names))
print('All45 catalog agents pass720 operational matches.', flush=True)

keys = ['seed', 'seat', 'cash', 'opponent_cash', 'turns', 'action_hash', 'opponent_action_hash', 'unit_faults', 'opponent_unit_faults', 'worker_days', 'max_workers', 'produced', 'sold', 'shops']


def compare(records, expected, label):
    expected = {(g['seed'], g['seat']): g for g in expected}
    for g in records:
        old = expected[g['seed'], g['seat']]
        assert {k: g[k] for k in keys} == {k: old[k] for k in keys}, (label, g['seed'], g['seat'])


broad = EXP / 'runs/animal_repair_sep08_001/broad_2360000'
protocol = json.loads((broad / 'PROTOCOL.json').read_text())


def parity(b):
    a = 'empty_sale_slots_m2'
    original = broad / f'{a}_vs_{b}.json'
    records = run('parity_' + b, a, b, seeds=4, start=2360000)
    compare(records, json.loads(original.read_text())['games'], b)
    return {'a': a, 'b': b, 'games': 8, 'reference': str(original.relative_to(ROOT)),
            'reference_sha256': hashlib.sha256(original.read_bytes()).hexdigest()}


with ThreadPoolExecutor(max_workers=4) as pool:
    parity_results = list(pool.map(parity, protocol['opponents']))
for a in ['animal_repair_premium_m2', 'animal_repair_q24_premium_m2']:
    original = broad / f'{a}_vs_king_rc4.json'
    records = run('parity_' + a, a, 'king_rc4', seeds=8, start=2360000)
    compare(records, json.loads(original.read_text())['games'], a)
    parity_results.append({'a': a, 'b': 'king_rc4', 'games': 16, 'reference': str(original.relative_to(ROOT))})
cow = EXP / 'runs/animal_service_policy_sep08_002/discovery_2390000'
for a, b in [('cow_service_retained_q24_premium_m2', 'ahmed_v25'), ('animal_repair_q24_premium_m2', 'ahmed_v25')]:
    original = cow / f'{a}_vs_{b}.json'
    records = run('parity_' + a + '_' + b, a, b, seeds=8, start=2390000)
    compare(records, json.loads(original.read_text())['games'], a)
    parity_results.append({'a': a, 'b': b, 'games': 16, 'reference': str(original.relative_to(ROOT))})
print('Matched384 complete session records.', flush=True)

direct = []
for b, file in [('teammate_shoprouter', 'teammate4096.json'), ('investment_context_guarded_001_best', 'prior4096.json')]:
    a = 'animal_repair_q24_premium_m2'
    records = run('submitted_' + b, a, b, seeds=2048, start=2370000, native=True, threads=4)
    original = REFERENCE / 'submissions/sep8-composition-adaptive-v1' / file
    compare(records, json.loads(original.read_text())['games'], b)
    direct.append({'opponent': b, 'games': len(records), 'wins': sum(g['cash'] > g['opponent_cash'] for g in records),
                   'ties': sum(g['cash'] == g['opponent_cash'] for g in records),
                   'mean_margin': sum(g['cash'] - g['opponent_cash'] for g in records) / len(records),
                   'exact_frozen_match_records_equal': len(records)})
report = {'binary': str(binary.relative_to(ROOT)), 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
          'operational': operational, 'session_parity': parity_results, 'parity_fields': keys,
          'direct': direct, 'games': sum(r['games'] for r in operational + parity_results + direct),
          'commands': commands,
          'scope': 'Catalog generic/typed-PASS/thread equality, independent self/PASS instances, per-action interface validation; complete source-session action/reward/output parity. Debug and isolated rebuild reports are separate.'}
(HERE / 'evidence/CATALOG_VALIDATION.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({'games': report['games'], 'direct': direct}, indent=2), flush=True)
