"""Idle, moves and shed visits by hour (days 10-28): ours vs top."""
import numpy as np, pandas as pd
h = pd.read_csv('labor/m_hours.csv')
h['rank'] = h.label.str.extract(r'\|(\d+)$')[0].astype(float)
h['src'] = np.where(h.label.str.startswith('ours'), h.label, np.where(h['rank'] <= 10, 'top', None))
h = h[h.src.notna() & (h.day >= 10) & (h.day <= 28)]
t = h.groupby(['src', 'hour'])[['units', 'work', 'moves', 'idle', 'shed_io']].mean()
pd.set_option('display.width', 250)
for c in ['idle', 'moves', 'work', 'shed_io', 'units']:
    print(c); print(t[c].unstack(0).round(1).T.to_string())
