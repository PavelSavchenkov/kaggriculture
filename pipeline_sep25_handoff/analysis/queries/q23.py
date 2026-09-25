import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from compare import source
c = pd.concat([pd.read_csv('ledger/cohort_top.csv'), pd.read_csv('ledger/cohort_ours.csv')], ignore_index=True)
c['src'] = [source(l, o, g) for l, o, g in zip(c.label, c.opp_label, c.group)]
c = c[c.src.notna()]
c['grp'] = np.where(c.src.str.startswith('top'), 'top', np.where(c.src.str.contains('self'), 'ours_self', 'ours_lb'))
c['mst_per_tile'] = c.mst / c.n
c['size'] = pd.cut(c.n, [1, 3, 6, 10, 20, 100]).astype(str)
for kind in ['all_crops', 'wheat', 'carrot', 'strawberry', 'tomato', 'animals', 'harvested']:
    x = c[(c.kind == kind) & (c.day >= 3) & (c.day <= 27)]
    t = x.groupby(['size', 'grp']).agg(cohorts=('n', 'size'), n=('n', 'mean'), quadrants=('quadrants', 'mean'), mst_per_tile=('mst_per_tile', 'mean'), shed=('mean_shed_dist', 'mean')).unstack('grp')
    print(f'\n== {kind} cohorts (days 3-27), by cohort size'); print(t.round(2).to_string())
