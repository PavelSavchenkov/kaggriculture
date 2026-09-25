import warnings; warnings.filterwarnings("ignore")
import sys
import pandas as pd
from analyze import load, perspectives
g, d = perspectives(load(sys.argv[1]), lambda l, o, gr: l if l.startswith('ours') else None)
g['work'] = g.unit_turns - g.act_idle - g.act_move
print(g.groupby('source')[['margin', 'money', 'opp_money', 'hire_cost', 'hires', 'unit_turns', 'act_move', 'work', 'act_idle', 'revenue', 'sold_h0', 'sold_h18', 'night_carried']].mean().round(0).T.to_string())
