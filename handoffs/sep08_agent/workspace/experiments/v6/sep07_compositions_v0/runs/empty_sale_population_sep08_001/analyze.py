from datetime import datetime, timezone
from pathlib import Path
import argparse
import hashlib
import json
import math
import numpy as np

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('panel', choices=['fresh', 'native'])
args = parser.parse_args()
spec = json.loads((RUN / 'PREREGISTERED.json').read_text())
candidate, baseline = spec['candidate'], spec['baseline']
panel = spec[args.panel]
expected = [(s, seat) for s in range(panel['seed_start'], panel['seed_start']+panel['seeds']) for seat in range(2)]
metrics, values, contrasts, file_hashes = {}, {}, {}, {}
games_by_agent = {}


def metric(games):
    cash = np.array([g['cash'] for g in games])
    margin = cash - np.array([g['opponent_cash'] for g in games])
    utility = (margin > 0) + .5*(margin == 0)
    count = math.ceil(len(games)/10)
    return {'games': len(games), 'wins': int((margin > 0).sum()), 'ties': int((margin == 0).sum()),
        'losses': int((margin < 0).sum()), 'utility': float(utility.mean()), 'mean_margin': float(margin.mean()),
        'mean_cash': float(cash.mean()), 'cvar10_margin': float(np.sort(margin)[:count].mean()),
        'cvar10_cash': float(np.sort(cash)[:count].mean())}, {'margin': margin, 'utility': utility}


for agent in [candidate, baseline]:
    metrics[agent], games_by_agent[agent] = {}, {}
    for opponent in panel['opponents']:
        path = RUN / args.panel / f'{agent}_vs_{opponent}.json'
        data = json.loads(path.read_text())
        games = data['games']
        assert data['validated']
        assert [(g['seed'], g['seat']) for g in games] == expected
        assert all(g['turns'] == 719 for g in games)
        metrics[agent][opponent], values[agent, opponent] = metric(games)
        games_by_agent[agent][opponent] = games
        file_hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
for opponent in panel['opponents']:
    ma, mb = metrics[candidate][opponent], metrics[baseline][opponent]
    a, b = games_by_agent[candidate][opponent], games_by_agent[baseline][opponent]
    c = {k+'_gain': ma[k]-mb[k] for k in ['utility', 'mean_margin', 'mean_cash', 'cvar10_margin', 'cvar10_cash']}
    delta = values[candidate, opponent]['margin']-values[baseline, opponent]['margin']
    c['paired_margin_regressions'] = int((delta < 0).sum())
    c['minimum_paired_margin_gain'] = float(delta.min())
    c['changed_own_actions'] = sum(x['action_hash'] != y['action_hash'] for x, y in zip(a, b))
    c['changed_rival_actions'] = sum(x['opponent_action_hash'] != y['opponent_action_hash'] for x, y in zip(a, b))
    for key in ['produced', 'sold', 'discarded']:
        c[key+'_gain'] = (np.array([g[key] for g in a])-np.array([g[key] for g in b])).mean(axis=0).tolist()
        c[key+'_changed_games'] = sum(x[key] != y[key] for x, y in zip(a, b))
    for key in ['unit_faults', 'worker_days']:
        c[key+'_gain'] = float(np.mean([x[key]-y[key] for x, y in zip(a, b)]))
    contrasts[opponent] = c

limits = spec['gates']
groups = {}
if args.panel == 'fresh':
    draws = np.random.default_rng(spec['bootstrap']['seed']).integers(0, panel['seeds'], (spec['bootstrap']['resamples'], panel['seeds']))
    for label, key in [('primary', 'grouping'), ('historical', 'historical_grouping')]:
        group = {}
        for kind in ['utility', 'margin']:
            array = {a: np.mean([np.mean([values[a, o][kind] for o in opponents], axis=0) for opponents in spec[key].values()], axis=0) for a in [candidate, baseline]}
            gain = (array[candidate]-array[baseline]).reshape(-1, 2).mean(axis=1)
            group[kind] = {'means': {a: float(v.mean()) for a, v in array.items()}, 'gain': float(gain.mean()),
                'gain_95pct': np.quantile(gain[draws].mean(axis=1), [.025, .975]).tolist()}
        groups[label] = group
    primary = set(o for opponents in spec['grouping'].values() for o in opponents)
    gates = {
        'primary_utility_noninferior': groups['primary']['utility']['gain_95pct'][0] >= limits['fresh_primary_utility_95pct_lower_min'],
        'primary_mean_margin_positive': groups['primary']['margin']['gain_95pct'][0] > 0,
        'historical_utility_noninferior': groups['historical']['utility']['gain_95pct'][0] >= limits['fresh_historical_utility_95pct_lower_min'],
        'individual_primary_utility': all(contrasts[o]['utility_gain'] >= limits['fresh_individual_primary_utility_gain_min'] for o in primary),
        'individual_primary_mean_margin': all(contrasts[o]['mean_margin_gain'] >= limits['fresh_individual_primary_mean_margin_gain_min'] for o in primary),
        'individual_primary_tail': all(contrasts[o]['cvar10_margin_gain'] >= limits['fresh_individual_primary_cvar10_margin_gain_min'] for o in primary),
    }
else:
    field = [o for o in panel['opponents'] if o != 'pass']
    gates = {
        'individual_native_utility': all(contrasts[o]['utility_gain'] >= limits['native_individual_utility_gain_min'] for o in field),
        'individual_native_mean_margin': all(contrasts[o]['mean_margin_gain'] >= limits['native_individual_mean_margin_gain_min'] for o in field),
        'individual_native_tail': all(contrasts[o]['cvar10_margin_gain'] >= limits['native_individual_cvar10_margin_gain_min'] for o in field),
        'pass_mean_cash': contrasts['pass']['mean_cash_gain'] >= limits['native_pass_mean_cash_gain_min'],
        'pass_tail_cash': contrasts['pass']['cvar10_cash_gain'] >= limits['native_pass_cvar10_cash_gain_min'],
    }
gates['direct_guard_utility'] = metrics[candidate][baseline]['utility'] > limits['direct_guard_utility_strictly_above']
gates['direct_guard_mean_margin'] = metrics[candidate][baseline]['mean_margin'] > limits['direct_guard_mean_margin_strictly_above']
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'candidate': candidate, 'baseline': baseline,
    'panel': args.panel, 'games': sum(m['games'] for a in metrics.values() for m in a.values()),
    'gates': gates, 'metrics': metrics, 'groups': groups, 'contrasts': contrasts, 'files_sha256': file_hashes,
    'protocol_sha256': hashlib.sha256((RUN / 'PREREGISTERED.json').read_bytes()).hexdigest(),
    'analysis_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    'status': 'All panel gates passed; combine with the other panel and unchanged operational/source evidence before selection.' if all(gates.values()) else 'Panel gate failed; no promotion.'}
(RUN / f'{args.panel.upper()}_ANALYSIS.json').write_text(json.dumps(report, indent=2)+'\n')
print(args.panel, report['games'], 'gates', gates)
print('groups', groups)
print('direct guard', metrics[candidate][baseline])
for o, c in contrasts.items():
    print(o, 'utility', c['utility_gain'], 'mean margin', c['mean_margin_gain'], 'tail', c['cvar10_margin_gain'], 'paired losses', c['paired_margin_regressions'])
