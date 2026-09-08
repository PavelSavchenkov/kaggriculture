from pathlib import Path
from collections import defaultdict
import csv
import json
import numpy as np

RUN = Path(__file__).resolve().parent
opponents = ['late_value_s32_t0_r05', 'public_router_v52', 'public_router']
rules = {
    'any_yarn': lambda first, second: first == 7 or second == 7,
    'early_yarn': lambda first, second: first == 7,
    'early_yarn_no_pet': lambda first, second: first == 7 and second != 4,
    'early_or_late_milk': lambda first, second: first == 7 or (second == 7 and first in [3, 5, 6]),
    'early_or_late_pizza_smoothie': lambda first, second: first == 7 or (second == 7 and first in [5, 6]),
    'no_pet_early_or_late_pizza_smoothie': lambda first, second: (first == 7 and second != 4) or (second == 7 and first in [5, 6]),
}
rows = []
parity = []
for opponent in opponents:
    files = {a: RUN / f'counterfactuals/{a}_vs_{opponent}.json' for a in ['baseline', 'force']}
    games = {a: json.loads(p.read_text())['games'] for a, p in files.items()}
    features = {a: list(csv.DictReader(Path(str(p) + '.features.csv').open())) for a, p in files.items()}
    for index, (base, force, a, b) in enumerate(zip(games['baseline'], games['force'], features['baseline'], features['force'])):
        assert (base['seed'], base['seat']) == (force['seed'], force['seat'])
        assert a['prefix_hash'] == b['prefix_hash']
        assert all(a[f'f{i}'] == b[f'f{i}'] for i in range(26))
        margin_base = base['cash'] - base['opponent_cash']
        margin_force = force['cash'] - force['opponent_cash']
        rows.append({'opponent': opponent, 'seed': base['seed'], 'seat': base['seat'], 'first': int(float(a['f0'])),
                     'second': int(float(a['f1'])), 'eligible': int(b['selected']) == 1,
                     'base_margin': margin_base, 'gain': margin_force - margin_base,
                     'base_utility': float(margin_base > 0) + 0.5 * (margin_base == 0),
                     'force_utility': float(margin_force > 0) + 0.5 * (margin_force == 0),
                     'own_gain': force['cash'] - base['cash']})
    for policy, folder, name in [('baseline', 'transfer_discovery', 'late_value_s32_t0_r05'),
                                 ('force', 'optimized_discovery', 'v52_transfer_wool0_h18')]:
        original = json.loads((RUN / f'{folder}/{name}_vs_{opponent}.json').read_text())['games']
        assert games[policy][:len(original)] == original
        parity.append({'opponent': opponent, 'policy': policy, 'full_records': len(original)})


def metrics(group, rule):
    choices = np.array([r['eligible'] and rule(r['first'], r['second']) for r in group])
    gains = np.array([r['gain'] for r in group]) * choices
    base = np.array([r['base_utility'] for r in group])
    changes = np.array([r['force_utility'] - r['base_utility'] for r in group]) * choices
    return {'games': len(group), 'activated': int(choices.sum()), 'mean_margin_gain': float(gains.mean()),
            'baseline_utility': float(base.mean()), 'utility': float((base + changes).mean()),
            'utility_gain': float(changes.mean()), 'own_cash_gain': float(np.mean([r['own_gain'] * c for r, c in zip(group, choices)]))}


report = {'scope': 'All data are diagnosis, including the reported half splits. No untouched validation or promotion is claimed.',
          'games': len(rows) * 2, 'identical_observation_and_action_prefix_pairs': len(rows), 'generic_full_record_parity': parity,
          'rules': {}, 'context_means': {}, 'hindsight_two_course_mean_margin_gain': {}}
for name, rule in rules.items():
    result = {opponent: metrics([r for r in rows if r['opponent'] == opponent], rule) for opponent in opponents}
    result['pooled'] = metrics(rows, rule)
    result['diagnosis_first512'] = metrics([r for r in rows if r['seed'] < 1512], rule)
    result['diagnosis_second512'] = metrics([r for r in rows if r['seed'] >= 1512], rule)
    report['rules'][name] = result
for opponent in opponents:
    groups = defaultdict(list)
    subset = [r for r in rows if r['opponent'] == opponent]
    for row in subset:
        if row['eligible'] and 7 in [row['first'], row['second']]:
            groups[f"{row['first']},{row['second']}"].append(row['gain'])
    report['context_means'][opponent] = {k: {'games': len(v), 'mean_gain': float(np.mean(v)), 'sd': float(np.std(v))} for k, v in sorted(groups.items())}
    report['hindsight_two_course_mean_margin_gain'][opponent] = float(np.mean([max(0, r['gain']) for r in subset]))
(RUN / 'COUNTERFACTUAL_ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
(RUN / 'counterfactual_rows.json').write_text(json.dumps(rows, separators=(',', ':')) + '\n')
for name, values in report['rules'].items():
    print(name, 'pooled', values['pooled'])
    for opponent in opponents:
        print(' ', opponent, 'margin', round(values[opponent]['mean_margin_gain'], 2), 'utility_gain', round(values[opponent]['utility_gain'], 5))
print('hindsight mean margin', report['hindsight_two_course_mean_margin_gain'])
print('All', len(rows), 'pairs have identical observed entry features and action prefixes.')
