"""Frozen candidate, prior broad panel and unused seeds; no policy fitting here."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
old_spec = json.loads((EXP / 'results/opening_market_validation.json').read_text())['preregistered']
opponents = sorted(set(old_spec['opponents'] + ['opening_q32_b13_v1', 'late_goose_wheat_context']))
agents = ['late_value_s32_t0_r05', 'opening_q32_b13_v1']
spec = {'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': agents[0], 'baseline': agents[1],
        'seed_start': 1850000, 'seeds': 2048, 'seat_mode': 'both', 'opponents': opponents,
        'grouping': old_spec['grouping'], 'historical_grouping': old_spec['historical_grouping'],
        'direct_parent_separate_from_groups': True,
        'gates': {'current_group_utility_gain_95_lower': 0,
                  'historical_group_utility_gain_95_lower': -0.0025,
                  'minimum_individual_utility_gain': -0.02,
                  'minimum_paired_mean_margin_gain': 0,
                  'direct_parent_utility': 0.5,
                  'direct_parent_mean_margin': 0},
        'scope': 'Independent2048-seed confirmation of unchanged candidate after first512-seed panel left the current-group95%lower bound unresolved. No fitting on1830000; same gates, one new confirmation panel.',
        'candidate_files_sha256': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest()
                                   for p in (EXP / 'runs/late_portfolio_001/proposals/late_value_s32_t0_r05').rglob('*') if p.is_file()}}
prereg = RUN / 'FRESH_PREREGISTERED.json'
assert not prereg.exists()
prereg.write_text(json.dumps(spec, indent=2) + '\n')
build_command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/build_arena.py'), '--agents', *sorted(set(agents + opponents))]
build = subprocess.run(build_command, capture_output=True, text=True, check=True)
(RUN / 'fresh_build.log').write_text(build.stdout + build.stderr)
binary = Path(build.stdout.strip().splitlines()[-1])
assert binary.exists()
(RUN / 'FRESH_BUILD.json').write_text(json.dumps({'command': build_command, 'binary': str(binary.relative_to(ROOT))}, indent=2) + '\n')
output = RUN / 'fresh'
output.mkdir(exist_ok=False)


def run(job):
    agent, opponent = job
    path = output / f'{agent}_vs_{opponent}.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', agent, '--b', opponent,
               '--games', str(spec['seeds']), '--seed-start', str(spec['seed_start']), '--seat-mode', 'both',
               '--threads', '6', '--validate', '--output', str(path)]
    with path.with_suffix('.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    print(agent, 'vs', opponent, 'complete', flush=True)
    return {'agent': agent, 'opponent': opponent, 'command': command, 'returncode': 0}


jobs = [(agent, opponent) for opponent in opponents for agent in agents]
with ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(run, jobs))
(RUN / 'FRESH_PROCESS_RESULTS.json').write_text(json.dumps(results, indent=2) + '\n')
