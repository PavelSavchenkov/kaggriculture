"""Action mix per day (days 10-28): ours vs top; watering vs need; moves per non-water action."""
import numpy as np, pandas as pd
CATS = ["move", "plant", "water", "harvest_crop", "harvest_animal", "fertilize", "dig", "build", "place_animal", "shed_io",
        "feed", "care", "collect_fert", "idle"]
cols = ['trace', 'seat', 'label', 'day', 'unit_turns', 'hires'] + [f'act_{c}' for c in CATS] + \
       [f'plants_{c}' for c in ['wheat', 'carrot', 'tomato', 'strawberry', 'melon']]
d = pd.read_csv('labor/l_days.csv', usecols=cols)
d['rank'] = d.label.str.extract(r'\|(\d+)$')[0].astype(float)
d['src'] = np.where(d.label.str.startswith('ours'), d.label, np.where(d['rank'] <= 10, 'top', None))
d = d[d.src.notna() & (d.day >= 10) & (d.day <= 28)]
d['plants'] = d[[c for c in cols if c.startswith('plants_')]].sum(axis=1)
m = d.groupby('src')[[f'act_{c}' for c in CATS] + ['unit_turns', 'plants']].mean()
m.columns = [c.replace('act_', '') for c in m.columns]
m['work'] = m[[c for c in CATS if c not in ('move', 'idle')]].sum(axis=1)
m['work_nowater'] = m.work - m.water
m['water/plant'] = m.water / m.plants
m['move/work'] = m.move / m.work
m['move/work_nowater'] = m.move / m.work_nowater
pd.set_option('display.width', 250)
print(m.round(2).T.to_string())
