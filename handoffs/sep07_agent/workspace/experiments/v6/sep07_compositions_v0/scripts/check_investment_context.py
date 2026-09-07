"""Complete-game checks for the two-context animal worker rebuild."""
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
OUT = EXP / 'results/investment_context_checks'
NEW = 'investment_context_guarded_001_best'
OLD = 'animal_adaptive_r1_c0_b0'
GENERIC = EXP / 'build/8dee9d8af503a3ed5b93/arena'
DEBUG = EXP / 'build/90ed0c310648e0b12674/arena'


def run(job):
    name, a, b, count, seed, threads, mode = job
    output = OUT / f'{name}.json'
    if output.exists():
        raise FileExistsError(output)
    command = ['conda', 'run', '-n', 'kaggriculture', str(DEBUG if mode == 'debug' else GENERIC),
               '--a', a, '--b', b, '--games', str(count), '--seed-start', str(seed),
               '--threads', str(threads), '--seat-mode', 'both', '--validate', '--output', str(output)]
    if mode == 'native':
        command.append('--native-shops')
    if mode == 'profile':
        command.append('--profile')
    result = subprocess.run(command, text=True, capture_output=True)
    (OUT / f'{name}.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
    data = json.loads(output.read_text())
    games = data.pop('games')
    data.update(games=len(games), wins=sum(g['cash'] > g['opponent_cash'] for g in games),
                ties=sum(g['cash'] == g['opponent_cash'] for g in games), command=command)
    print(name, data['wins'], '/', len(games), 'margin', data['mean_margin'], flush=True)
    return name, data


def main():
    OUT.mkdir(exist_ok=True)
    jobs = [('generic64', NEW, OLD, 32, 1000, 1, ''),
            ('debug64', NEW, OLD, 32, 1000, 1, 'debug'),
            ('thread64', NEW, OLD, 32, 1000, 16, '')]
    for a in [NEW, OLD]:
        jobs += [(f'{a}_pass256', a, 'pass', 128, 1000, 4, ''),
                 (f'{a}_self16', a, a, 8, 1000, 4, ''),
                 (f'{a}_profile64', a, 'public_router', 32, 1000, 4, 'profile')]
        jobs += [(f'{a}_native_{b}', a, b, 128, 14000, 4, 'native')
                 for b in [OLD, 'teammate_shoprouter', 'king_rc4', 'public_router', 'public_router_v5']]
    with ThreadPoolExecutor(max_workers=4) as pool:
        report = dict(pool.map(run, jobs))
    records = [json.loads((OUT / f'{name}64.json').read_text())['games']
               for name in ['generic', 'debug', 'thread']]
    assert records[0] == records[1] == records[2]
    report['generic_debug_thread_full_records_equal'] = len(records[0])
    report['physical_context_regression'] = 'runs/investment_context_guarded_001/single_context_regression.json'
    (OUT / 'CHECKS.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
