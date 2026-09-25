import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from analyze import load, perspectives
from compare import source
sh = pd.read_csv('ledger/shops.csv')
s12 = sh[sh.day == 12].drop(columns='day').set_index('trace')
def src(label, opp, group):
    if label.startswith('ours:'):
        v = group.split('/')[0]
        return f'ours:{v}:{"lb" if "/lb/" in group else "zoo" if group.count("/") == 1 else "cpp"}'
    return source(label, opp, group)
parts = [load(p) for p in ['ledger/top0', 'ledger/top1', 'ledger/top2', 'ledger/top3', 'ledger/ours0', 'ledger/ours1', 'ledger/ours2', 'ledger/ours3', 'ledger/ours4', 'ledger/ours5', 'ledger/x10lb', 'ledger/zoo0', 'ledger/zoo1', 'ledger/zoo2', 'ledger/zoo3']]
data = {k: pd.concat([x[k] for x in parts], ignore_index=True) for k in ['games', 'days', 'crops', 'animals']}
g, d = perspectives(data, src)
g['grp'] = np.where(g.source.str.startswith('top'), 'top', g.source)
g = g.join(s12, on='trace')
# late plantings (days 9-25) per crop, from the days table
late = d[(d.day >= 9) & (d.day <= 25)].groupby(['trace', 'seat'])[[f'planted_{c}' for c in ['wheat', 'carrot', 'tomato', 'strawberry']]].sum().add_prefix('late_')
g = g.join(late, on=['trace', 'seat'])
tests = {
    'sheep d15 ~ yarn': ('d15_animals_sheep', ['yarn']),
    'geese d15 ~ bakery+brunch': ('d15_animals_goose', ['bakery', 'brunch']),
    'cows d15 ~ pizza+icecream+smoothie': ('d15_animals_cow', ['pizza', 'icecream', 'smoothie']),
    'carrots planted d9-25 ~ petcafe x2 + farmers': ('late_planted_carrot', ['petcafe', 'petcafe', 'farmers']),
    'tomatoes planted d9-25 ~ pizza+farmers': ('late_planted_tomato', ['pizza', 'farmers']),
    'strawberries planted d9-25 ~ brunch+icecream+smoothie+farmers': ('late_planted_strawberry', ['brunch', 'icecream', 'smoothie', 'farmers']),
    'wheat planted d9-25 ~ bakery+pizza+brunch+icecream+farmers': ('late_planted_wheat', ['bakery', 'pizza', 'brunch', 'icecream', 'farmers']),
}
groups = ['top', 'ours:vadim6:lb', 'ours:x10:lb', 'ours:v12:lb', 'ours:w384:lb', 'ours:x10:zoo', 'ours:base:zoo']
rows = []
for name, (y, xs) in tests.items():
    for grp in groups:
        x = g[g.grp == grp]
        if len(x) < 50: continue
        demand = sum(x[c] for c in xs)
        slope = np.polyfit(demand, x[y], 1)[0]
        rows.append(dict(test=name, group=grp, n=len(x), mean=x[y].mean(), slope_per_shop=slope, rel=slope / max(0.1, x[y].mean())))
r = pd.DataFrame(rows).set_index(['test', 'group'])
print('Response of own production to shop demand unlocked by day 12 (OLS slope per extra shop unit)')
print(r.round(2).to_string())
