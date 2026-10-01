"""Seed ensemble offline: averages the predicted band totals of several fc_eval.py outputs (same rows, same order), as the C++ runtime
averages <model>.forecast_tf, .forecast_tf.2, ... hourly rates. usage: fc_avg.py <out.csv> <eval_1.csv> <eval_2.csv> ..."""
import sys
import pandas as pd

ds = [pd.read_csv(f) for f in sys.argv[2:]]
key = ['trace', 'target', 'day', 'product', 'decision']
for d in ds[1:]:
    assert (d[key].values == ds[0][key].values).all(), 'row order differs'
out = ds[0].copy()
for k in range(4):
    out[f'pred_b{k}'] = sum(d[f'pred_b{k}'] for d in ds) / len(ds)
out.to_csv(sys.argv[1], index=False)
print(f'{len(ds)} models averaged -> {sys.argv[1]}')
