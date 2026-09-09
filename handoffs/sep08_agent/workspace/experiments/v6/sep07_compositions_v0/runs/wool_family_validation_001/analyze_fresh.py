from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import math
import numpy as np

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
spec = json.loads((RUN / 'FRESH_PREREGISTERED.json').read_text())
candidate, baseline = spec['candidate'], spec['baseline']
expected_order = [(seed, seat) for seed in range(spec['seed_start'], spec['seed_start'] + spec['seeds']) for seat in range(2)]


def metrics(games):
    margins = np.array([g['cash'] - g['opponent_cash'] for g in games])
    utility = (margins > 0) + 0.5 * (margins == 0)
    return {'games': len(games), 'wins': int((margins > 0).sum()), 'ties': int((margins == 0).sum()),
            'losses': int((margins < 0).sum()), 'utility': float(utility.mean()),
            'mean_margin': float(margins.mean()), 'cvar10_margin': float(np.sort(margins)[:math.ceil(len(games) / 10)].mean())}, utility


records, utilities, contrasts, hashes = {}, {}, {}, {}
for agent in [candidate, baseline]:
    records[agent] = {}
    for opponent in spec['opponents']:
        path = RUN / f'fresh/{agent}_vs_{opponent}.json'
        games = json.loads(path.read_text())['games']
        assert [(g['seed'], g['seat']) for g in games] == expected_order
        assert all(g['turns'] == 719 for g in games)
        records[agent][opponent], utilities[agent, opponent] = metrics(games)
        hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
for opponent in spec['opponents']:
    contrasts[opponent] = {key + '_gain': records[candidate][opponent][key] - records[baseline][opponent][key]
                          for key in ['utility', 'mean_margin', 'cvar10_margin']}
draws = np.random.default_rng(18701907).integers(0, spec['seeds'], (10000, spec['seeds']))
groups = {}
for label, key in [('current', 'grouping'), ('historical', 'historical_grouping')]:
    values = {a: np.mean([np.mean([utilities[a, o] for o in opponents], axis=0)
                          for opponents in spec[key].values()], axis=0) for a in [candidate, baseline]}
    gain = (values[candidate] - values[baseline]).reshape(-1, 2).mean(axis=1)
    groups[label] = {'utilities': {a: float(v.mean()) for a, v in values.items()},
                     'gain_95pct': np.quantile(gain[draws].mean(axis=1), [.025, .975]).tolist()}
gates = {'current_group_positive_95pct': groups['current']['gain_95pct'][0] > 0,
         'historical_group_noninferiority': groups['historical']['gain_95pct'][0] > -.0025,
         'individual_utility_regression_limit': min(v['utility_gain'] for v in contrasts.values()) >= -.02,
         'all_paired_mean_margins_nonnegative': min(v['mean_margin_gain'] for v in contrasts.values()) >= 0,
         'direct_parent_positive': records[candidate][baseline]['utility'] > .5 and records[candidate][baseline]['mean_margin'] > 0}
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'candidate': candidate, 'baseline': baseline,
          'games': sum(v['games'] for a in records.values() for v in a.values()),
          'status': 'Fresh numeric gates passed; operational/native/frozen audits still required' if all(gates.values()) else 'Not promoted: fresh numeric gate failed',
          'gates': gates, 'groups': groups, 'metrics': records, 'contrasts': contrasts,
          'tail_regressions': {o: c['cvar10_margin_gain'] for o, c in contrasts.items() if c['cvar10_margin_gain'] < 0},
          'files_sha256': hashes,
          'limitations': ['Independent sheep-heavy full-farm suffix; not unrestricted composition construction.',
                          'Context rule selected from six simple rules on reused diagnosis games.',
                          'Actual guards and funding can reject entry; native/causal/frozen operational checks remain separate.']}
(RUN / 'FRESH_ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
print(report['status'], 'games', report['games'])
print('gates', gates)
print('groups', groups)
print('direct', records[candidate][baseline])
for opponent, values in contrasts.items():print(opponent, values)
