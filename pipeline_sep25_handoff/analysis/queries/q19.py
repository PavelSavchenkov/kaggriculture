import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from analyze import load, perspectives, P
from compare import source
parts = [load('ledger/exp45'), load('ledger/exp12'), load('ledger/ours?')]
data = {k: pd.concat([x[k] for x in parts], ignore_index=True) for k in parts[0]}
g, d = perspectives(data, source)
keep = ['ours:vadim6:lb', 'ours:x1_d0:lb', 'ours:x2_melon:lb', 'ours:x4_d0_melon:lb', 'ours:x5_return_wage:lb']
g = g[g.source.isin(keep)]
g['disc'] = g[[f'discarded_{p}' for p in P]].sum(axis=1)
g['animals15'] = sum(g[f'd15_animals_{a}'] for a in ['goose', 'cow', 'sheep'])
cols = ['margin', 'money', 'opp_money', 'revenue', 'hire_cost', 'hires', 'night_carried', 'disc', 'sold_h0', 'sold_h18', 'animals15', 'price_melon', 'rev_melon',
        'price_milk', 'price_wool', 'price_strawberry', 'unsold_value']
print(g.groupby('source')[cols].mean().reindex(keep).T.round(1).to_string())
