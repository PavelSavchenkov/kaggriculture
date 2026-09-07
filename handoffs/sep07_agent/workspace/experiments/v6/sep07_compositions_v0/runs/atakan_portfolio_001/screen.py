"""Run common discovery games and independent diagnostics in C++."""
import argparse
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

RUN = Path(__file__).resolve().parent
NAMES = ['atakan_cow', 'atakan_sheep', 'atakan_goose', 'atakan_demand', 'atakan_quotes', 'atakan_value_own', 'atakan_value_margin']
RIVALS = ['animal_adaptive_r1_c0_b0', 'teammate_shoprouter', 'public_router_v5', 'king_rc4', 'public_router']


def run(job):
    name, a, b, count, mode = job
    directory = RUN / 'results';directory.mkdir(exist_ok=True)
    kind = mode if mode in ('debug', 'masked') else 'generic'
    binary = RUN / 'build' / kind / ('diagnostics' if mode == 'trace' else 'arena')
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b, '--games', str(count), '--threads', '1' if mode in ('debug', 'masked') else '4', '--seed-start', '1000', '--seat-mode', 'both', '--validate', '--output', str(directory / f'{name}.json')]
    if mode == 'profile':
        command.append('--profile')
    result = subprocess.run(command, capture_output=True, text=True)
    (directory / f'{name}.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    if mode != 'trace':
        data = json.loads((directory / f'{name}.json').read_text());g = data['games']
        print(json.dumps({'case': name, 'games': len(g), 'wins': sum(x['cash'] > x['opponent_cash'] for x in g), 'margin': sum(x['cash'] - x['opponent_cash'] for x in g) / len(g)}), flush=True)
    return {'case': name, 'command': command}


def main():
    parser = argparse.ArgumentParser();parser.add_argument('--checks', action='store_true');args = parser.parse_args()
    if args.checks:
        jobs = [(f'{a}_pass256', a, 'pass', 128, '') for a in NAMES]
        jobs += [(f'{a}_self8', a, a, 4, '') for a in NAMES]
        jobs += [(f'{a}_masked16', a, 'atakan_goose', 8, 'masked') for a in NAMES]
        jobs += [(f'{a}_generic16', a, 'atakan_goose', 8, '') for a in NAMES]
        jobs += [('typed_debug16', 'atakan_demand', 'atakan_value_margin', 8, 'debug'), ('typed_generic16', 'atakan_demand', 'atakan_value_margin', 8, '')]
    else:
        jobs = [(f'{a}_vs_{b}', a, b, 32, 'profile' if a in NAMES[:3] else '') for a in NAMES for b in RIVALS]
        jobs += [(f'trace_{b}', 'atakan_cow', b, 32, 'trace') for b in RIVALS]
    with ThreadPoolExecutor(max_workers=4) as pool:
        commands = list(pool.map(run, jobs))
    (RUN / ('CHECK_COMMANDS.json' if args.checks else 'SCREEN_COMMANDS.json')).write_text(json.dumps(commands, indent=2) + '\n')


if __name__ == '__main__':
    main()
