import pandas as pd
r = pd.read_pickle('reports_days.pkl')
r['cost_new'] = r.goose * 300 + r.cow * 400 + r.sheep * 500 + r.land * 1000
t = r.groupby('day').agg(dawns=('trimmed', 'size'), trimmed_share=('trimmed', lambda s: (s > 0).mean()), trimmed_units=('trimmed', 'mean'),
                         cash=('cash', 'median'), asked_animal_land_cost=('cost_new', 'mean'))
print(t.loc[[0, 3, 6, 7, 8, 9, 10, 11, 12, 13]].round(2).to_string())
print('\nshare of all trimmed units on days 0, 6, 7, 9:', round(r[r.day.isin([0, 6, 7, 9])].trimmed.sum() / r.trimmed.sum(), 3))
