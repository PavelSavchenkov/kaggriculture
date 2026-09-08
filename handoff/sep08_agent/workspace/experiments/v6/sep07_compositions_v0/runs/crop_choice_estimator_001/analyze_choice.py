"""Evaluate observed-state branch predictions against paired complete games."""
import argparse
import json
from pathlib import Path

import numpy as np
import pandas as pd

RUN = Path(__file__).resolve().parent


def utility(margin):
    return (margin > 0).astype(float) + .5 * (margin == 0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    directory = args.directory.resolve()
    assert directory.is_relative_to(RUN)
    data = pd.read_csv(directory / 'scores.csv')
    exact = pd.read_csv(directory / 'exact.csv')
    context = ['opponent', 'seed', 'seat']
    count = len(exact.drop_duplicates(context))
    eligible = len(data.drop_duplicates(context))
    assert len(data) == eligible * 16 and len(exact) == count * 18
    assert not data.duplicated(context + ['model']).any()
    data['wheat_margin'] = data.wheat_own - data.wheat_rival
    data['tomato_margin'] = data.tomato_own - data.tomato_rival
    data['real_delta'] = data.tomato_margin - data.wheat_margin
    data['real_own_delta'] = data.tomato_own - data.wheat_own
    data['pred_own_delta'] = data.pred_tomato_own - data.pred_wheat_own
    data['pred_rival_delta'] = data.pred_tomato_rival - data.pred_wheat_rival
    data['pred_delta'] = data.pred_own_delta - data.pred_rival_delta
    data['current_margin'] = np.where(data.tomato_shops >= 2, data.tomato_margin, data.wheat_margin)
    data['current_own'] = np.where(data.tomato_shops >= 2, data.tomato_own, data.wheat_own)
    rows = []
    for model, group in data.groupby('model'):
        for objective in ('own', 'margin'):
            selected = group.pred_own_delta > 0 if objective == 'own' else group.pred_delta > 0
            margin = np.where(selected, group.tomato_margin, group.wheat_margin)
            own = np.where(selected, group.tomato_own, group.wheat_own)
            gain = margin - group.current_margin
            own_gain = own - group.current_own
            win_gain = utility(margin) - utility(group.current_margin)
            row = {
                'model': int(model), 'crop_mode': int(model // 4), 'subtract_feed': bool(model & 2),
                'animal_fertilizer': bool(model & 1), 'objective': objective,
                'tomato_choices': int(selected.sum()),
                'changes_from_current': int((selected != (group.tomato_shops >= 2)).sum()),
                'mean_margin_gain_all_contexts': float(gain.sum() / count),
                'mean_own_gain_all_contexts': float(own_gain.sum() / count),
                'mean_win_utility_gain_all_contexts': float(win_gain.sum() / count),
                'eligible_mean_margin_regret': float((np.maximum(group.wheat_margin, group.tomato_margin) - margin).mean()),
                'eligible_prediction_delta_mae': float((group.pred_delta - group.real_delta).abs().mean()),
                'eligible_prediction_delta_bias': float((group.pred_delta - group.real_delta).mean()),
                'eligible_own_delta_mae': float((group.pred_own_delta - group.real_own_delta).abs().mean()),
                'eligible_rival_delta_mae': float((group.pred_rival_delta - (group.tomato_rival - group.wheat_rival)).abs().mean()),
                'forecast_microseconds_median': float(group.microseconds.median()),
                'forecast_microseconds_p95': float(group.microseconds.quantile(.95)),
                'per_opponent': []
            }
            for opponent in group.opponent.unique():
                mask = group.opponent == opponent
                n = len(exact.loc[exact.opponent == opponent].drop_duplicates(context))
                row['per_opponent'].append({'opponent': opponent, 'games': n,
                    'mean_margin_gain': float(gain[mask].sum() / n),
                    'mean_own_gain': float(own_gain[mask].sum() / n),
                    'win_utility_gain': float(win_gain[mask].sum() / n)})
            rows.append(row)
    reference = data[data.model == 0]
    report = {'scope': 'Discovery only; no promotion. Ineligible runtime contexts retain wheat and contribute zero difference.',
              'contexts': count, 'eligible': eligible, 'models': 16,
              'current_eligible_mean_margin_regret': float((np.maximum(reference.wheat_margin, reference.tomato_margin) - reference.current_margin).mean()),
              'results': rows}
    (directory / 'REPORT.json').write_text(json.dumps(report, indent=2) + '\n')
    for row in sorted(rows, key=lambda r: r['mean_margin_gain_all_contexts'], reverse=True):
        print(row['model'], row['objective'], 'margin gain', round(row['mean_margin_gain_all_contexts'], 3),
              'win gain', round(row['mean_win_utility_gain_all_contexts'], 5),
              'changed', row['changes_from_current'], 'delta MAE', round(row['eligible_prediction_delta_mae'], 2))


if __name__ == '__main__':
    main()
