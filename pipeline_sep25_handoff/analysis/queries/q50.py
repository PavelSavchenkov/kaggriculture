"""Mirror traces: ours minus opponent by product (revenue, units, price), wages, for base vs a probe."""
import sys
import pandas as pd
P = ['wheat','carrot','tomato','strawberry','melon','egg','milk','wool','fertilizer']
d = pd.read_csv('ledger/mtr_days.csv')
d['variant'] = d.group.str.split('/').str[0]
d['who'] = d.label.str.startswith('ours').map({True: 'ours', False: 'opp'})
agg = {f'rev_{p}': 'sum' for p in P} | {f'sold_{p}': 'sum' for p in P} | {'hire_cost': 'sum'}
g = d.groupby(['variant', 'trace', 'who']).agg(agg)
diff = g.xs('ours', level='who') - g.xs('opp', level='who')
m = diff.groupby('variant').mean()
pd.set_option('display.width', 250)
print('ours minus opponent, mean per game')
print(m[[f'rev_{p}' for p in P] + ['hire_cost']].round(0).to_string())
print(m[[f'sold_{p}' for p in P]].round(1).to_string())
s = pd.read_csv('ledger/mtr_sales.csv')
g2 = pd.read_csv('ledger/mtr_games.csv')
ours = {(r.trace, r.seat) for r in g2.itertuples() if r.label.startswith('ours')}
s['who'] = [('ours' if (t, p) in ours else 'opp') for t, p in zip(s.trace, s.seat)]
s['variant'] = s.trace.str.extract(r'mtr/(\w+)/')[0]
s = s[s.units > 0]
s['bucket'] = pd.cut(s.hour, [-1, 5, 11, 17, 19, 21, 23], labels=['0-5', '6-11', '12-17', '18-19', '20-21', '22-23'])
for q in [3, 6, 7, 4]:
    x = s[s['product'] == q]
    t = x.pivot_table(index=['variant', 'who'], columns='bucket', values='units', aggfunc='sum', observed=False) / 32
    pr = x.groupby(['variant', 'who']).apply(lambda y: y.revenue.sum() / y.units.sum())
    t['price'] = pr
    print(P[q]); print(t.round(1).to_string())
