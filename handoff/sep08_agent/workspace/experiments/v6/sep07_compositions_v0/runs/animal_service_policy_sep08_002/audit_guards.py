"""Trace every missed guard in completed broad match files, retaining exact controls."""
import csv
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

RUN = Path(__file__).resolve().parent
BROAD = RUN / 'discovery_2390000'
OUT = RUN / 'guard_audit_results'
OUT.mkdir(exist_ok=True)
assert (RUN / 'GUARD_AUDIT_BUILD.json').exists()
protocol = json.loads((BROAD / 'PROTOCOL.json').read_text())
jobs = []
for job in protocol['jobs']:
    prefix = 'native_' if job['native'] else ''
    name = f"{prefix}{job['a']}_vs_{job['b']}"
    source = BROAD / (name + '.json')
    diagnostic = BROAD / (name + '.json.diagnostics.json')
    if not source.exists() or not diagnostic.exists():
        continue
    # The writer closes valid JSON before its completed job notification.
    text = diagnostic.read_text()
    if not text.rstrip().endswith(']}'):
        continue
    records = json.loads(text)['games']
    seeds = sorted({g['seed'] for g in records if g['missed_days']})
    if seeds:
        jobs.append((name, job, seeds, source))


def run(entry):
    name, job, seeds, source = entry
    output = OUT / (name + '.json')
    seed_file = OUT / (name + '.seeds.txt')
    seed_text = ''.join(f'{s}\n' for s in seeds)
    if output.exists():
        assert seed_file.read_text() == seed_text
    else:
        seed_file.write_text(seed_text)
        command = ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'guard_audit'), '--a', job['a'], '--b', job['b'],
            '--seed-file', str(seed_file), '--seat-mode', 'both', '--threads', '2', '--validate', '--profile', '--output', str(output)]
        if job['native']:
            command.append('--native-shops')
        with (OUT / (name + '.log')).open('w') as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    expected = {(g['seed'], g['seat']): g for g in json.loads(source.read_text())['games']}
    actual = json.loads(output.read_text())['games']
    assert all(g == expected[g['seed'], g['seat']] for g in actual), name
    differences = list(csv.DictReader(Path(str(output) + '.differences.csv').open()))
    result = {'name': name, 'games': len(actual), 'exact_full_records_equal': True, 'differences': differences}
    print(name, len(actual), 'games,', len(differences), 'field differences', flush=True)
    return result


with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(OUT / 'AUDIT.json').write_text(json.dumps({'completed_matchups': len(results), 'games': sum(r['games'] for r in results), 'results': results}, indent=2) + '\n')
print('Guard traces complete for all currently completed match files; manual interpretation remains required.')
