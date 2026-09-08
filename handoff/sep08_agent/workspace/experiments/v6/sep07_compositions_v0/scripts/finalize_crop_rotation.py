"""Analyze frozen paired crop-policy evidence; fail if its promotion gate fails."""
import argparse
import hashlib
import json
import math
from datetime import datetime, timezone
from pathlib import Path

import numpy as np

EXP = Path(__file__).resolve().parents[1]


def load(path):
    return json.loads(path.read_text())


def metrics(data):
    games = data['games']
    margin = np.array([g['cash'] - g['opponent_cash'] for g in games])
    utility = (margin > 0).astype(float) + .5 * (margin == 0)
    return {'games': len(games), 'wins': int((margin > 0).sum()), 'ties': int((margin == 0).sum()),
            'losses': int((margin < 0).sum()), 'utility': float(utility.mean()),
            'mean_margin': float(margin.mean()),
            'margin_cvar10': float(np.sort(margin)[:math.ceil(len(margin) / 10)].mean()),
            'pass_J': data['pass_J']}, utility


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('run')
    parser.add_argument('--checks', required=True)
    parser.add_argument('--output', default='crop_rotation_berry_validation.json')
    parser.add_argument('--output-delta', type=int, nargs='+', default=[-36, 0, 24])
    args = parser.parse_args()
    run, checks = [EXP / 'runs' / name for name in [args.run, args.checks]]
    spec = load(run / 'PREREGISTERED.json')
    new, old = spec['candidate'], spec['baseline']
    records = {new: {}, old: {}}
    utilities, paired, output_cases = {}, {}, {}
    file_hashes = {}
    for opponent in spec['opponents']:
        data = {}
        for agent in [new, old]:
            path = run / 'fresh' / f'{agent}_vs_{opponent}.json'
            data[agent] = load(path)
            records[agent][opponent], utilities[agent, opponent] = metrics(data[agent])
            file_hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
        pairs = list(zip(data[old]['games'], data[new]['games']))
        assert len(pairs) == 2 * spec['seeds']
        expected_keys = [(seed, seat) for seed in range(spec['seed_start'], spec['seed_start'] + spec['seeds']) for seat in range(2)]
        assert [(a['seed'], a['seat']) for a, b in pairs] == expected_keys
        assert all((a['seed'], a['seat']) == (b['seed'], b['seat']) for a, b in pairs)
        cases = sorted({tuple(y - x for x, y in zip(a['produced'], b['produced'])) for a, b in pairs})
        expected_delta = tuple(args.output_delta)
        assert all(row == (0,) * len(row) or row == expected_delta + (0,) * (len(row) - len(expected_delta)) for row in cases)
        output_cases[opponent] = {'cases': cases, 'changed_production': sum(a['produced'] != b['produced'] for a, b in pairs)}
        paired[opponent] = {key + '_gain': records[new][opponent][key] - records[old][opponent][key]
                            for key in ['utility', 'mean_margin', 'margin_cvar10']}
        paired[opponent]['own_cash_gain'] = sum(b['cash'] - a['cash'] for a, b in pairs) / len(pairs)
        paired[opponent]['rival_cash_gain'] = sum(b['opponent_cash'] - a['opponent_cash'] for a, b in pairs) / len(pairs)
        paired[opponent]['unit_faults_gain'] = sum(b['unit_faults'] - a['unit_faults'] for a, b in pairs) / len(pairs)
        paired[opponent]['discards_gain'] = sum(sum(b['discarded']) - sum(a['discarded']) for a, b in pairs) / len(pairs)
    grouped = {agent: np.mean([np.mean([utilities[agent, opponent] for opponent in opponents], axis=0)
                               for opponents in spec['grouping'].values()], axis=0) for agent in [new, old]}
    seed_gain = (grouped[new] - grouped[old]).reshape(-1, 2).mean(axis=1)
    rng = np.random.default_rng(4121)
    draws = rng.integers(0, len(seed_gain), (10000, len(seed_gain)))
    interval = np.quantile(seed_gain[draws].mean(axis=1), [.025, .975]).tolist()
    operational = load(checks / 'CHECKS.json')
    assert operational['generic_pair_debug_thread_all_game_records_equal']
    native = {}
    for opponent in [old, 'teammate_shoprouter', 'king_rc4', 'public_router', 'public_router_v5']:
        native[opponent] = {agent: metrics(load(checks / f'{agent}_native_{opponent}.json'))[0] for agent in [new, old]}
    pass_results = {agent: metrics(load(checks / f'{agent}_pass256.json'))[0] for agent in [new, old]}
    passed = (interval[0] > 0 and records[new][old]['utility'] > .5 and
              records[new][old]['mean_margin'] > 0 and
              all(row['utility_gain'] >= 0 and row['mean_margin_gain'] > 0 for row in paired.values()) and
              all(row[new]['utility'] >= row[old]['utility'] and row[new]['mean_margin'] >= row[old]['mean_margin'] for row in native.values()))
    report = {'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': new, 'parent': old,
              'status': 'Promotion gates passed' if passed else 'Not promoted: at least one comparison gate failed',
              'fresh_scope': spec, 'group_objective': {agent: float(grouped[agent].mean()) for agent in [new, old]},
              'seed_cluster_bootstrap_gain_95pct': interval, 'fresh': records, 'paired': paired,
              'native': native, 'pass': pass_results, 'production_closure': output_cases,
              'operational_checks': str((checks / 'CHECKS.json').relative_to(EXP)),
              'causal_profiles': 'runs/crop_rotation_berry_gate_001/profiles/CAUSAL.json',
              'frozen_sources': str((run / 'FROZEN.json').relative_to(EXP)), 'files_sha256': file_hashes,
              'limits': ['One fixed three-tile family replacement selected by observed shops, not a general placement compiler.',
                         'Exact physical entry required; financing depends on future real prices.',
                         'A mean or win gain does not imply every individual scenario improves; lower tails are reported separately.',
                         'CPU local policy; no Kaggle adapter or additional submission for this version.']}
    (EXP / 'results' / args.output).write_text(json.dumps(report, indent=2) + '\n')
    print(report['status'], report['group_objective'], interval)
    print('Direct parent:', records[new][old])
    print('Native paired:', {opponent: {key: row[new][key] - row[old][key] for key in ['utility', 'mean_margin', 'margin_cvar10']} for opponent, row in native.items()})
    print('PASS J:', {agent: row['pass_J'] for agent, row in pass_results.items()})
    assert passed


if __name__ == '__main__':
    main()
