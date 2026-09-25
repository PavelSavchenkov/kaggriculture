import warnings; warnings.filterwarnings("ignore")
import pandas as pd, numpy as np
from analyze import load, perspectives, P
data = load('ledger/rs2')
g, d = perspectives(data, lambda l, o, gr: l)
g['tag'] = g.group.str.split('/').str[0]
g['side'] = np.where(g.label.str.startswith('ours'), 'ours', 'opp')
g['key'] = g.trace.str.extract(r'/([^/]+)_seat\d\.txt')[0] + '_s' + g.seat.astype(str)
g['game'] = g.trace.str.extract(r'/([^/]+_\d+_seat\d)\.txt')[0]
cols = [f'rev_{p}' for p in P]
t = g.groupby(['side', 'tag'])[['money', 'revenue'] + cols].mean().T
t['ours_diff'] = t[('ours', 'rs')] - t[('ours', 'x10')]
t['opp_diff'] = t[('opp', 'rs')] - t[('opp', 'x10')]
print('Mean per game (32 LB games), route search (rs) minus X10:')
print(t[['ours_diff', 'opp_diff']].round(0).to_string())
pr = g.groupby(['side', 'tag'])[[f'price_{p}' for p in P] + [f'sold_{p}' for p in P]].mean().T
print('\nopponent price and units by product:'); print(pr[[('opp', 'x10'), ('opp', 'rs')]].round(1).to_string())
s = pd.read_csv('ledger/rs2_sales.csv')
lab = {}
for line in open('ledger/rs_list.txt'):
    f = line.split(); lab[f[0]] = (0 if f[1].startswith('ours') else 1, f[3].split('/')[0])
s['ours'] = s.seat == s.trace.map(lambda t: lab[t][0]); s['tag'] = s.trace.map(lambda t: lab[t][1])
o = s[s.ours]
o['bucket'] = pd.cut(o.hour, [-1, 5, 11, 17, 23], labels=['h0-5', 'h6-11', 'h12-17', 'h18-23'])
print('\nour units sold by product and hour bucket (per game):')
print((o.pivot_table(index=['product'], columns=['tag', 'bucket'], values='units', aggfunc='sum') / 32).round(1).rename(index=dict(enumerate(P))).to_string())

print('\n--- per game: opponent money and strawberry price, rs vs x10')
w = g[g.side == 'opp'].pivot_table(index='game', columns='tag', values=['opp_money', 'money', 'price_strawberry', 'rev_strawberry'])
diff = pd.DataFrame({'opp_money_diff': w[('money', 'rs')] - w[('money', 'x10')], 'opp_straw_price_diff': w[('price_strawberry', 'rs')] - w[('price_strawberry', 'x10')],
                     'opp_straw_rev_diff': w[('rev_strawberry', 'rs')] - w[('rev_strawberry', 'x10')]})
print('games where the opponent is richer:', (diff.opp_money_diff > 0).sum(), 'of', len(diff))
print('games where the opponent strawberry price is higher:', (diff.opp_straw_price_diff > 0).sum(), 'lower:', (diff.opp_straw_price_diff < 0).sum())
print(diff.describe().round(0).to_string())
print('correlation opp money diff with its strawberry revenue diff:', round(diff.opp_money_diff.corr(diff.opp_straw_rev_diff), 2))
