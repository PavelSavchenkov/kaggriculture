"""Complete exact C++ games, paired fresh audits, and operational comparisons."""
import argparse
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from prepare import NAMES
from diagnose import REFERENCE, RIVALS

RUN = Path(__file__).resolve().parent
RETAIN = ['atakan_integrated_s1_margin', 'atakan_integrated_s64_margin']


def run(job):
    name, a, b, seed, count, mode = job
    results = RUN / 'results';results.mkdir(exist_ok=True)
    build = mode if mode in ('debug', 'masked') else 'generic'
    command = ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'build' / build / 'arena'), '--a', a, '--b', b,
               '--seed-start', str(seed), '--games', str(count), '--seat-mode', 'both', '--threads', '1' if build != 'generic' else '4',
               '--validate', '--output', str(results / f'{name}.json')]
    if mode == 'native':
        command.append('--native-shops')
    with (results / f'{name}.log').open('w') as log:
        subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
    return {'name': name, 'command': command}


def main():
    parser = argparse.ArgumentParser();parser.add_argument('phase', choices=['discovery', 'fresh', 'checks', 'native']);args = parser.parse_args()
    rivals = RIVALS+[REFERENCE]
    if args.phase == 'discovery':
        jobs = [(f'{a}_vs_{b}', a, b, 1000, 32, '') for a in NAMES for b in rivals]
    elif args.phase == 'fresh':
        candidates = RETAIN+['atakan_value_margin', 'atakan_demand']
        jobs = [(f'fresh_{a}_vs_{b}', a, b, 1450000, 256, '') for a in candidates for b in rivals]
        jobs += [(f'fresh_{a}_vs_{b}', a, b, 1450000, 256, '') for a in RETAIN for b in ['atakan_value_margin', 'atakan_demand']]
        jobs += [(f'fresh_{RETAIN[0]}_vs_{RETAIN[1]}', *RETAIN, 1450000, 256, '')]
    elif args.phase == 'native':
        jobs = [(f'native_{a}_vs_{b}', a, b, 1450000, 128, 'native') for a in RETAIN+['atakan_demand','atakan_value_margin'] for b in [REFERENCE, 'public_router_v5', 'teammate_shoprouter']]
    else:
        jobs = [(f'{a}_pass256', a, 'pass', 1000, 128, '') for a in RETAIN]
        jobs += [(f'{a}_self8', a, a, 1000, 4, '') for a in RETAIN]
        jobs += [(f'{a}_{mode}16', a, 'atakan_demand', 1000, 8, mode) for a in RETAIN for mode in ['', 'masked']]
        jobs += [(f'typed_{mode}16', *RETAIN, 1000, 8, mode) for mode in ['', 'debug']]
    with ThreadPoolExecutor(max_workers=4) as pool:
        records = list(pool.map(run, jobs))
    (RUN / f'{args.phase.upper()}_COMMANDS.json').write_text(json.dumps(records, indent=2)+'\n')
    print(f'{args.phase}: {sum(job[4]*2 for job in jobs)} games completed.')


if __name__ == '__main__':
    main()
