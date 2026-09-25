import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v2.pkl'); g = c['g'].copy()
g['grp'] = np.where(g.source.str.startswith('top'), 'top', g.source)
keep = ['top'] + [f'ours:{v}:lb' for v in ['vadim6', 'v12', 'w384', 'v11', 'majkel']]
g = g[g.grp.isin(keep)]
g['animals_bought'] = g.bought_goose + g.bought_cow + g.bought_sheep
g['d1_animals'] = g.d3_animals_goose + g.d3_animals_cow + g.d3_animals_sheep
g['d9_animals'] = g.d9_animals_goose + g.d9_animals_cow + g.d9_animals_sheep
g['d15_animals'] = g.d15_animals_goose + g.d15_animals_cow + g.d15_animals_sheep
g['melon_price'] = g.price_melon
g['wage_per_hire'] = g.hire_cost / g.hires
g['idle_share'] = g.act_idle / g.unit_turns
cols = ['win', 'margin', 'money', 'revenue', 'spend', 'd1_animals', 'd3_animals_sheep', 'd9_animals', 'd15_animals', 'animals_bought', 'd29_land',
        'planted_melon', 'sold_melon', 'melon_price', 'rev_melon', 'sold_fertilizer', 'rev_fertilizer', 'hire_cost', 'wage_per_hire', 'idle_share',
        'd0_workers', 'd3_workers', 'd3_money_dawn', 'd6_money_dawn', 'planted_carrot', 'rev_carrot', 'planted_strawberry', 'rev_strawberry', 'planted_tomato',
        'rev_milk', 'rev_wool', 'rev_egg', 'price_wool', 'price_milk', 'price_strawberry', 'sold_h18']
print(g.groupby('grp')[cols].mean().reindex(keep).T.round(2).to_string())
