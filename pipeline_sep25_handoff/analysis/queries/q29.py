import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from analyze import load
d = pd.concat([load(f'ledger/top{i}')['days'][['trace', 'seat', 'day', 'money_dawn', 'label']] for i in range(4)], ignore_index=True)
g = pd.concat([load(f'ledger/top{i}')['games'][['trace', 'seat', 'money', 'opp_money']] for i in range(4)], ignore_index=True)
o = d.rename(columns={'money_dawn': 'opp_money_dawn', 'seat': 'oseat'})[['trace', 'oseat', 'day', 'opp_money_dawn']]
d['oseat'] = 1 - d.seat
d = d.merge(o, on=['trace', 'oseat', 'day']).merge(g, on=['trace', 'seat'])
d['final'] = d.money - d.opp_money
d['now'] = d.money_dawn - d.opp_money_dawn
d['win'] = (d.final > 0).astype(float)
rows = []
for day, x in d.groupby('day'):
    r2 = np.corrcoef(x.now, x.final)[0, 1] ** 2
    # win accuracy of sign(now)
    acc = ((x.now > 0) == (x.final > 0)).mean()
    rows.append(dict(day=day, n=len(x), std_final=x.final.std(), r2_current_margin=r2, win_acc_current_sign=acc, std_togo=(x.final - x.now).std()))
print(pd.DataFrame(rows).set_index('day').loc[[0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 29]].round(3).to_string())
