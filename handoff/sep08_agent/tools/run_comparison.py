"""Run a new paired discovery screen. Never promotes or uploads an agent."""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import argparse
import hashlib
import json
import subprocess
import numpy as np

HERE = Path(__file__).resolve().parents[1]
ENV = ['conda', 'run', '--no-capture-output', '-n', 'kaggriculture']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--parent', default='empty_sale_slots_m2')
    parser.add_argument('--opponents', nargs='+', default=['teammate_shoprouter', 'public_router', 'public_router_v52',
        'ahmed_v23', 'ahmed_v24', 'ahmed_v25', 'junghoon_wool_sales', 'king_rc4', 'yusuke_sep08_m2'])
    parser.add_argument('--seed-start', type=int, required=True)
    parser.add_argument('--seeds', type=int, default=32)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--registry', type=Path)
    parser.add_argument('--binary', type=Path, help='Optional already built arena containing every requested agent.')
    args = parser.parse_args()
    assert args.candidate != args.parent and len(set(args.opponents)) == len(args.opponents)
    assert args.seeds > 0 and args.seed_start >= 0
    args.output.mkdir(parents=True, exist_ok=False)
    protocol = {'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': args.candidate,
        'parent': args.parent, 'opponents': args.opponents, 'seeds': args.seeds,
        'independent_seed_start': args.seed_start, 'native_seed_start': args.seed_start + 10000,
        'seat_mode': 'both', 'opponent_weights': 'equal', 'purpose': 'Discovery only; not an unseen or promotion claim.',
        'advance': 'Explain branch execution and all per-opponent regressions before considering a newly frozen confirmation. This script never updates the accepted reference.'}
    (args.output / 'PROTOCOL.json').write_text(json.dumps(protocol, indent=2) + '\n')
    if args.binary:
        binary = args.binary.resolve()
    else:
        command = ENV + ['python', str(HERE / 'tools/build_catalog.py'), '--agents', args.candidate, args.parent, *args.opponents]
        if args.registry:
            command += ['--registry', str(args.registry.resolve())]
        built = subprocess.check_output(command, text=True)
        binary = Path(built.strip().splitlines()[-1])
    (args.output / 'BUILD.json').write_text(json.dumps({'binary': str(binary),
        'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()}, indent=2) + '\n')
    jobs = [(native, a, b) for native in (False, True) for b in args.opponents for a in (args.parent, args.candidate)]

    def run(job):
        native, a, b = job
        path = args.output / (('native_' if native else 'fresh_') + a + '_vs_' + b + '.json')
        command = ENV + [str(binary), '--a', a, '--b', b, '--games', str(args.seeds),
            '--seed-start', str(args.seed_start + 10000 * native), '--seat-mode', 'both', '--threads', '2',
            '--budget-expansions', '100000', '--validate', '--profile', '--output', str(path)]
        if native: command.append('--native-shops')
        with path.with_suffix('.log').open('w') as log:
            subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
        result = json.loads(path.read_text())
        assert len(result['games']) == args.seeds * 2 and all(g['turns'] == 719 for g in result['games'])
        return job, result, command

    with ThreadPoolExecutor(max_workers=2) as pool:
        completed = list(pool.map(run, jobs))
    results = {job: data for job, data, _ in completed}
    def values(data):
        return np.array([[float(g['cash'] > g['opponent_cash']) + .5 * (g['cash'] == g['opponent_cash']),
                          g['cash'], g['cash'] - g['opponent_cash']] for g in data['games']])
    rows, groups = [], []
    for native in (False, True):
        all_delta = []
        for b in args.opponents:
            candidate, parent = [results[native, a, b] for a in (args.candidate, args.parent)]
            assert [(g['seed'], g['seat']) for g in candidate['games']] == [(g['seed'], g['seat']) for g in parent['games']]
            x, y = values(candidate), values(parent)
            all_delta.append((x - y).reshape(args.seeds, 2, 3).mean(axis=1))
            tail = max(1, int(np.ceil(len(x) / 10)))
            rows.append({'native': native, 'opponent': b, 'candidate_means': x.mean(axis=0).tolist(),
                'parent_means': y.mean(axis=0).tolist(), 'gain': (x-y).mean(axis=0).tolist(),
                'candidate_cash_margin_tail10': np.sort(x[:, 1:], axis=0)[:tail].mean(axis=0).tolist(),
                'parent_cash_margin_tail10': np.sort(y[:, 1:], axis=0)[:tail].mean(axis=0).tolist()})
        paired = np.stack(all_delta).mean(axis=0)
        rng = np.random.default_rng(args.seed_start)
        sample = paired[rng.integers(0, args.seeds, (4000, args.seeds))].mean(axis=1)
        groups.append({'native': native, 'gain': paired.mean(axis=0).tolist(),
                       'gain95': np.quantile(sample, [.025, .975], axis=0).tolist()})
    report = {'games': len(jobs) * args.seeds * 2, 'columns': ['win_utility', 'own_cash', 'margin'],
              'rows': rows, 'groups': groups, 'promoted': False,
              'commands': [command for _, _, command in completed],
              'limitations': 'Does not audit every branch guard or confer promotion. Seed blocks are chosen by the caller and are not certified unused.'}
    (args.output / 'RESULTS.json').write_text(json.dumps(report, indent=2) + '\n')
    body = '# Paired discovery result\n\nNo promotion. Read PROTOCOL.json and all per-opponent rows in RESULTS.json.\n\n'
    body += '| Panel | Win gain pp | Own cash gain | Margin gain | Margin95% |\n| --- | ---: | ---: | ---: | --- |\n'
    for g in groups:
        body += f'| {"native" if g["native"] else "independent shops"} | {100*g["gain"][0]:+.3f} | {g["gain"][1]:+.2f} | {g["gain"][2]:+.2f} | [{g["gain95"][0][2]:+.2f}, {g["gain95"][1][2]:+.2f}] |\n'
    (args.output / 'RESULTS.md').write_text(body)
    print(body)


if __name__ == '__main__':
    main()
