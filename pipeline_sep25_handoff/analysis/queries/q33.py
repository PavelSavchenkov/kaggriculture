import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np, glob
from analyze import load, perspectives, P, C, A
from compare import source
def src(label, opp, group):
    if label.startswith('ours:'):
        v = group.split('/')[0]
        return f'ours:{v}:{"lb" if "/lb/" in group else "zoo" if group.count("/") == 1 else "cpp"}'
    return source(label, opp, group)
files = ['ledger/top0', 'ledger/top1', 'ledger/top2', 'ledger/top3', 'ledger/ours0', 'ledger/ours1', 'ledger/ours2', 'ledger/ours3', 'ledger/ours4', 'ledger/ours5', 'ledger/x10lb', 'ledger/zoo0', 'ledger/zoo1', 'ledger/zoo2', 'ledger/zoo3']
files += sorted({f[:-10] for f in glob.glob('ledger/mirror*_games.csv')})
parts = [load(p) for p in files]
data = {k: pd.concat([x[k] for x in parts], ignore_index=True) for k in ['games', 'days', 'crops', 'animals']}
g, d = perspectives(data, src)
g['grp'] = np.where(g.source.str.startswith('top'), 'top', g.source)
groups = ['top', 'ours:vadim6:lb', 'ours:x10:lb', 'ours:x10:zoo', 'ours:x10mirror:zoo', 'ours:base:zoo']
groups = [x for x in groups if x in set(g.grp)]
for day in (1, 6, 9, 15, 25):
    g[f'animals_d{day}'] = sum(g[f'd{day}_animals_{a}'] for a in A) if f'd{day}_animals_goose' in g else np.nan
g['animals_bought'] = g[[f'bought_{a}' for a in A]].sum(axis=1)
g['idle_share'] = g.act_idle / g.unit_turns
g['work_per_turn'] = (g.unit_turns - g.act_idle - g.act_move) / g.unit_turns
g['moves_per_work'] = g.act_move / (g.unit_turns - g.act_idle - g.act_move)
late = d[d.day >= 24].groupby(['trace', 'seat']).agg(late_rev=('revenue', 'sum'), late_seed=('spend_seeds', 'sum'), late_animals=('spend_animals', 'sum'), fed_late=('fed', 'sum'))
g = g.join(late, on=['trace', 'seat'])
shed16 = d[d.day == 16].set_index(['trace', 'seat']).shed_dawn
g['shed_d16'] = g.set_index(['trace', 'seat']).index.map(shed16)
cols = ['n', 'win', 'margin', 'money', 'opp_money', 'revenue', 'spend',
        'animals_d1', 'animals_d6', 'animals_d9', 'animals_d15', 'animals_bought',
        'planted_wheat', 'bought_wheat', 'sold_wheat', 'planted_melon', 'price_melon', 'rev_melon',
        'rev_strawberry', 'rev_milk', 'rev_wool', 'rev_egg', 'rev_fertilizer', 'rev_carrot', 'rev_tomato', 'rev_wheat',
        'hire_cost', 'idle_share', 'moves_per_work', 'sold_h0', 'sold_h18', 'shed_d16', 'night_carried',
        'late_rev', 'late_seed', 'late_animals', 'fed_late', 'escapes_sheep', 'escapes_cow', 'held_value', 'unsold_value']
g['n'] = 1
t = g.groupby('grp').agg({c: ('sum' if c == 'n' else 'mean') for c in cols}).reindex(groups).T
print(t.round(2).to_string())
