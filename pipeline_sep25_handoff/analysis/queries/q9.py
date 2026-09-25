import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
c = pd.read_pickle('cache_v1.pkl'); g = c['g'].copy()
g['grp'] = np.where(g.source.str.startswith('top'), 'top', g.source)
keep = ['top', 'ours:vadim6:lb', 'ours:v12:lb', 'ours:vadim6:cpp', 'ours:vadim6:self', 'opp:arsgorynich-herd-safe-v3']
g = g[g.grp.isin(keep)]
g['animal_nights'] = g.fed + g.unfed
g['fert_per_animal_night'] = g.harvested_fertilizer / g.animal_nights
g['fert_used'] = g.act_fertilize
g['fert_sold_share'] = g.sold_fertilizer / g.harvested_fertilizer
g['feed_rate'] = g.fed / g.animal_nights
g['care_rate'] = g.cared / g.fed
g['wheat_net_sold'] = g.sold_wheat - g.bought_wheat
cols = ['animal_nights', 'harvested_fertilizer', 'fert_per_animal_night', 'fert_missed', 'fert_used', 'bought_fertilizer', 'sold_fertilizer', 'price_fertilizer',
        'feed_rate', 'care_rate', 'harvested_wheat', 'sold_wheat', 'bought_wheat', 'wheat_net_sold', 'act_feed', 'planted_wheat',
        'hires', 'hire_cost', 'unit_turns', 'act_idle', 'unsold_value', 'held_value', 'carried_units', 'discarded_wheat', 'night_carried',
        'escapes_goose', 'escapes_cow', 'escapes_sheep', 'bonus_lost', 'cap_lost_crop', 'cap_lost_cow', 'cap_lost_sheep', 'cap_lost_goose']
print(g.groupby('grp')[cols].mean().reindex(keep).T.round(2).to_string())
