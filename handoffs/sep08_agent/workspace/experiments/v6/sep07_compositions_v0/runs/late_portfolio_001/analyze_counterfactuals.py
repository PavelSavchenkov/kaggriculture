"""Diagnose the C++ value model and score simple selectors from exact outcomes."""
from pathlib import Path
from datetime import datetime, timezone
import csv
import itertools
import json
import numpy as np

RUN = Path(__file__).resolve().parent
data = {}
diagnostics = []
for opponent in ['opening_q32_b13_v1', 'public_router']:
    games = [json.loads((RUN / f'counterfactuals/force{c}_vs_{opponent}.json').read_text())['games'] for c in range(4)]
    rows = [list(csv.DictReader((RUN / f'counterfactuals/force{c}_vs_{opponent}.json.predictions.csv').open())) for c in range(4)]
    assert all(len(g) == 1024 for g in games)
    for c in range(1, 4):
        for a, b in zip(rows[0], rows[c]):
            assert {k: v for k, v in a.items() if k not in ['choice', 'matched_days', 'estimate_seconds']} == {k: v for k, v in b.items() if k not in ['choice', 'matched_days', 'estimate_seconds']}
    eligible = np.array([[int(row[f'eligible_{c}']) for c in range(4)] for row in rows[0]], dtype=bool)
    predicted = {metric: np.array([[float(row[f'{metric}_{c}']) for c in range(4)] for row in rows[0]])
                 for metric in ['own', 'margin', 'deviation', 'positive_fraction']}
    margins = np.array([[games[c][i]['cash'] - games[c][i]['opponent_cash'] for c in range(4)] for i in range(1024)])
    gains = margins - margins[:, :1]
    utility = (margins > 0) + 0.5 * (margins == 0)
    data[opponent] = eligible, predicted, margins, utility
    for c, species in enumerate(['wait', 'goose', 'cow', 'sheep']):
        if c == 0:
            continue
        selected = eligible[:, c]
        estimate = predicted['margin'][selected, c]
        realized = gains[selected, c]
        residual = estimate - realized
        diagnostics.append({'opponent': opponent, 'species': species, 'eligible_games': int(selected.sum()),
                            'estimate_mean': float(estimate.mean()), 'realized_mean': float(realized.mean()),
                            'mean_overestimate': float(residual.mean()), 'mean_absolute_error': float(np.abs(residual).mean()),
                            'correlation': float(np.corrcoef(estimate, realized)[0, 1]),
                            'predicted_profitable': int((estimate > 0).sum()),
                            'predicted_profitable_but_lost': int(((estimate > 0) & (realized < 0)).sum())})
    seconds = np.array([float(row['estimate_seconds']) for row in rows[0]])
    print(opponent, 'estimation microseconds eligible median', np.median(seconds[eligible[:, 1:].any(axis=1)]) * 1e6)

results = []
for threshold, risk, own_weight in itertools.product([0, 50, 100, 150, 200, 300, 400], [0, .5, 1], [0, 1]):
    report = {'threshold': threshold, 'risk': risk, 'own_weight': own_weight, 'opponents': {}}
    for opponent, (eligible, predicted, margins, utility) in data.items():
        scores = (1 - own_weight) * predicted['margin'] + own_weight * predicted['own'] - risk * predicted['deviation']
        scores[~eligible] = -1e20
        scores[:, 0] = threshold
        choice = scores.argmax(axis=1)
        row = np.arange(len(choice))
        selected_utility = utility[row, choice]
        report['opponents'][opponent] = {'utility': float(selected_utility.mean()), 'utility_gain': float((selected_utility - utility[:, 0]).mean()),
                                         'margin': float(margins[row, choice].mean()),
                                         'margin_gain': float((margins[row, choice] - margins[:, 0]).mean()),
                                         'choices': np.bincount(choice, minlength=4).tolist()}
    report['mean_utility_gain'] = float(np.mean([v['utility_gain'] for v in report['opponents'].values()]))
    report['mean_margin_gain'] = float(np.mean([v['margin_gain'] for v in report['opponents'].values()]))
    results.append(report)
results.sort(key=lambda row: (row['mean_utility_gain'], row['mean_margin_gain']), reverse=True)
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': 8192,
          'scope': 'Previously used1790000 diagnosis panel. Counterfactual selection preserves the exact shared observed prefix. Grid is diagnostic; selected actual C++ policy and unused seeds still required.',
          'prediction_prefixes_equal': True, 'diagnostics': diagnostics, 'selectors': results}
(RUN / 'COUNTERFACTUAL_ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
print('diagnostics', diagnostics)
print('top selectors', json.dumps(results[:5], indent=2))
