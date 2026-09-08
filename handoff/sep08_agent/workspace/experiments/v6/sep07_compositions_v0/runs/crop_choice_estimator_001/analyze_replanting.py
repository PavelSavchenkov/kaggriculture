"""Measure public-state crop quantity and date errors against full exact games."""
import json
from pathlib import Path

import pandas as pd

RUN = Path(__file__).resolve().parent


def main():
    data = pd.read_csv(RUN / 'replanting_audit.csv')
    context = ['opponent', 'seed', 'seat']
    assert len(data) == 384 * 4 * 5
    assert not data.duplicated(context + ['mode', 'product']).any()
    columns = ['predicted', 'actual', 'absolute_quantity_error', 'dated_absolute_error']
    timings = data.drop_duplicates(context).forecast_microseconds
    totals = data.groupby('mode')[columns].sum()
    report = {
        'scope': 'Day-12 observations in 384 exact games, 32 discovery seeds and both seats against six opponents; crops only.',
        'modes': ['no crops', 'current crop lives', 'current crops plus repeated wheat/carrot', 'replanting plus final carrot'],
        'product_order': ['WHEAT', 'CARROT', 'TOMATO', 'STRAWBERRY', 'MELON'],
        'total_errors': totals.reset_index().to_dict('records'),
        'mean_per_product': data.groupby(['mode', 'product'])[columns].mean().reset_index().to_dict('records'),
        'mean_per_opponent_product': data.groupby(['opponent', 'mode'])[columns].mean().reset_index().to_dict('records'),
        'four_forecasts_microseconds': {str(q): float(timings.quantile(q)) for q in (.5, .95, 1)},
        'limitations': [
            'Production forecast only; no evidence yet about economic ranking or policy strength.',
            'Dated error remains large even where total crop quantities are close.',
            'Final carrot improves pooled errors but worsens the teammate forecast.',
            'No future opponent family insertion, expansion, private inventory, service failures or exact deposit delay.',
            'The financial model has not yet been calibrated; rival fixed costs and current-crop fertilizer use are incomplete.'
        ]
    }
    (RUN / 'REPLANTING_AUDIT.json').write_text(json.dumps(report, indent=2) + '\n')
    print(totals.to_string())
    print('Four forecast timing quantiles, us:', report['four_forecasts_microseconds'])


if __name__ == '__main__':
    main()
