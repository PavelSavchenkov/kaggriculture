"""Bounded common-seed C++ screen and operational checks for the new port."""
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
OUT = EXP / 'results/refresh_1112_screen'
RIVALS = ['shop_herd_guarded_001_best', 'teammate_shoprouter', 'king_rc4', 'binghua_116', 'public_terminal_router']


def run(job):
    name, a, b, games, threads, mode = job
    binary = EXP / 'build/refresh_1112' / ('debug_pair' if mode == 'debug' else 'generic') / 'arena'
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', a, '--b', b, '--games', str(games),
               '--threads', str(threads), '--seed-start', '1000', '--seat-mode', 'both', '--validate', '--output', str(OUT / f'{name}.json')]
    if mode == 'native':
        command.append('--native-shops')
    if mode == 'profile':
        command.append('--profile')
    result = subprocess.run(command, capture_output=True, text=True)
    (OUT / f'{name}.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    data = json.loads((OUT / f'{name}.json').read_text())
    g = data['games']
    summary = {'case': name, 'games': len(g), 'wins': sum(x['cash'] > x['opponent_cash'] for x in g),
               'ties': sum(x['cash'] == x['opponent_cash'] for x in g),
               'mean_margin': sum(x['cash'] - x['opponent_cash'] for x in g) / len(g)}
    print(json.dumps(summary), flush=True)
    return {'summary': summary, 'command': command}


def main():
    OUT.mkdir(exist_ok=True)
    jobs = [(f'{a}_vs_{b}', a, b, 64, 4, '') for a in ['public_router_v5', 'public_sixday'] for b in RIVALS]
    jobs += [('public_router_v5_vs_public_sixday', 'public_router_v5', 'public_sixday', 64, 4, ''),
             ('pass256', 'public_router_v5', 'pass', 128, 4, ''),
             ('self8', 'public_router_v5', 'public_router_v5', 4, 2, ''),
             ('debug16', 'public_router_v5', 'public_sixday', 8, 1, 'debug'),
             ('generic16', 'public_router_v5', 'public_sixday', 8, 16, ''),
             ('native_best128', 'public_router_v5', RIVALS[0], 64, 4, 'native'),
             ('profile_best32', 'public_router_v5', RIVALS[0], 16, 4, 'profile')]
    with ThreadPoolExecutor(max_workers=4) as pool:
        reports = list(pool.map(run, jobs))
    a = json.loads((OUT / 'debug16.json').read_text())['games']
    b = json.loads((OUT / 'generic16.json').read_text())['games']
    assert a == b
    (OUT / 'COMMANDS.json').write_text(json.dumps({'cases': reports, 'generic_debug_thread_records_identical': len(a)}, indent=2) + '\n')


if __name__ == '__main__':
    main()
