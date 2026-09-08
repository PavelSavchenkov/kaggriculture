"""Apply the preregistered opening-component gate to complete exact evidence."""
import hashlib
import json
import math
from datetime import datetime, timezone
from pathlib import Path

import numpy as np
import pandas as pd

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
CHECKS = EXP / 'runs/bohann_opening_checks_001'


def read(path):
    return json.loads(path.read_text())


def metrics(data):
    games = data['games']
    margins = np.array([g['cash'] - g['opponent_cash'] for g in games])
    utility = (margins > 0).astype(float) + .5 * (margins == 0)
    return {'games': len(games), 'wins': int((margins > 0).sum()), 'ties': int((margins == 0).sum()),
            'losses': int((margins < 0).sum()), 'utility': float(utility.mean()),
            'mean_margin': float(margins.mean()),
            'margin_cvar10': float(np.sort(margins)[:math.ceil(len(games) / 10)].mean()),
            'pass_J': data['pass_J']}, utility


def main():
    spec = read(RUN / 'PREREGISTERED.json')
    new, old = spec['candidate'], spec['baseline']
    records = {new: {}, old: {}}
    utilities, paired, hashes = {}, {}, {}
    keys = [(seed, seat) for seed in range(spec['seed_start'], spec['seed_start'] + spec['seeds']) for seat in range(2)]
    for opponent in spec['opponents']:
        data = {}
        for agent in [new, old]:
            path = RUN / f'fresh/{agent}_vs_{opponent}.json'
            data[agent] = read(path)
            assert [(g['seed'], g['seat']) for g in data[agent]['games']] == keys
            assert all(g['turns'] == 719 for g in data[agent]['games'])
            records[agent][opponent], utilities[agent, opponent] = metrics(data[agent])
            hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
        games = list(zip(data[new]['games'], data[old]['games']))
        row = {key + '_gain': records[new][opponent][key] - records[old][opponent][key]
               for key in ('utility', 'mean_margin', 'margin_cvar10')}
        for key in ('cash', 'opponent_cash', 'unit_faults', 'worker_days'):
            row[key + '_gain'] = sum(a[key] - b[key] for a, b in games) / len(games)
        for key in ('produced', 'sold', 'discarded'):
            row[key + '_gain'] = [sum(a[key][i] - b[key][i] for a, b in games) / len(games) for i in range(9)]
        row['changed_rival_actions'] = sum(a['opponent_action_hash'] != b['opponent_action_hash'] for a, b in games)
        paired[opponent] = row
    grouped = {agent: np.mean([np.mean([utilities[agent, opponent] for opponent in opponents], axis=0)
                               for opponents in spec['grouping'].values()], axis=0) for agent in [new, old]}
    gain = (grouped[new] - grouped[old]).reshape(-1, 2).mean(axis=1)
    random = np.random.default_rng(17504121)
    draws = random.integers(0, len(gain), (10000, len(gain)))
    interval = np.quantile(gain[draws].mean(axis=1), [.025, .975]).tolist()
    checks = read(CHECKS / 'CHECKS.json')
    assert checks['generic_pair_debug_thread_all_game_records_equal']
    assert read(RUN / 'COMPACT_PARITY.json')['complete_records_exact'] == 768
    frozen = read(RUN / 'frozen/FROZEN.json')
    native = {}
    for opponent in [old, 'teammate_shoprouter', 'king_rc4', 'public_router', 'public_router_v5']:
        native[opponent] = {agent: metrics(read(CHECKS / f'{agent}_native_{opponent}.json'))[0] for agent in [new, old]}
    passes = {agent: metrics(read(CHECKS / f'{agent}_pass256.json'))[0] for agent in [new, old]}
    profiles = {}
    for opponent in [old, 'teammate_shoprouter', 'king_rc4', 'public_router', 'public_router_v5']:
        a, b = [read(RUN / f'profiles/{agent}_vs_{opponent}.json')['games'] for agent in [new, old]]
        assert len(a) == len(b) == 64
        row = {}
        for key in ('hires', 'hire_cost', 'land', 'land_cost', 'weed_digs'):
            row[key + '_gain'] = sum(x['profile'][key] - y['profile'][key] for x, y in zip(a, b)) / 64
        for key in ('cash', 'opponent_cash', 'revenue', 'spend', 'unit_faults'):
            row[key + '_gain'] = sum(x[key] - y[key] for x, y in zip(a, b)) / 64
        for key in ('produced', 'sold', 'discarded'):
            row[key + '_gain'] = [sum(x[key][i] - y[key][i] for x, y in zip(a, b)) / 64 for i in range(9)]
        row['buy_quantity_gain'] = [sum(x['profile']['buys'][i] - y['profile']['buys'][i] for x, y in zip(a, b)) / 64 for i in range(12)]
        profiles[opponent] = row
    trace = pd.read_csv(RUN / 'opening_trace/states.csv')
    actions = pd.read_csv(RUN / 'opening_trace/actions.csv', dtype={'workers': str, 'markets': str})
    trace_report = {'scope': 'One seed1000 seat0, four opponents, complete paired exact trajectories; not a cohort-wide causal decomposition.', 'opponents': {}}
    for opponent in actions.opponent.unique():
        row = {}
        for player in (0, 1):
            current, previous = [actions[(actions.agent == agent) & (actions.opponent == opponent) & (actions.player == player)].set_index('step') for agent in (new, old)]
            row[str(player)] = {kind + '_first_changes': current.index[current[kind] != previous[kind]].tolist()[:8] for kind in ('workers', 'markets')}
        trace_report['opponents'][opponent] = row
    king = trace[(trace.opponent == 'king_rc4') & (trace.player == 1) & (trace.step == 24)]
    trace_report['king_day1_start_cash'] = dict(zip(king.agent, king.cash))
    (RUN / 'profiles/CAUSAL.json').write_text(json.dumps(profiles, indent=2) + '\n')
    (RUN / 'opening_trace/REPORT.json').write_text(json.dumps(trace_report, indent=2) + '\n')
    passed = (interval[0] > 0 and records[new][old]['utility'] > .5 and records[new][old]['mean_margin'] > 0
              and min(row['utility_gain'] for row in paired.values()) >= -.02
              and native[old][new]['utility'] > .5 and native[old][new]['mean_margin'] > 0
              and min(row[new]['utility'] - row[old]['utility'] for row in native.values()) >= -.02)
    report = {'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': new, 'parent': old,
              'status': 'Promotion gates passed' if passed else 'Not promoted: gate failed',
              'preregistered': spec, 'group_objective': {agent: float(grouped[agent].mean()) for agent in (new, old)},
              'seed_cluster_bootstrap_gain_95pct': interval, 'fresh': records, 'paired': paired,
              'native': native, 'pass': passes, 'operational_checks': str((CHECKS / 'CHECKS.json').relative_to(EXP)),
              'compact_complete_records_exact': 768, 'frozen_sources': str((RUN / 'frozen/FROZEN.json').relative_to(EXP)),
              'frozen_dependency_count': len(frozen['files_sha256']), 'causal_profiles': profiles,
              'trace': trace_report, 'files_sha256': hashes,
              'limitations': [
                  'The opening improves aggregate league utility, not every opponent mean or tail. V5 loses two fresh wins; native public_router loses one.',
                  'Gross wheat buys and sales rise through a round trip; they are not increased crop production.',
                  'Copied hiring orders add workers relative to the optimized parent; preserving parent hires is a useful untested follow-up.',
                  'Large King gains involve changed opponent actions and market flows; a fixed-rival economic forecast would miss this response.',
                  'The historical four-group objective omits some newer audited opponents. All19matchups are reported separately.',
                  'The new component is a fixed opening on an existing adaptive crop/animal policy. It does not solve general composition construction.',
                  'No Kaggle package or additional submission exists for this candidate.'
              ]}
    (EXP / 'results/bohann_opening_validation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(report['status'], report['group_objective'], '95% gain', interval)
    print('Direct parent', records[new][old])
    print('King', records[new]['king_rc4'])
    assert passed


if __name__ == '__main__':
    main()
