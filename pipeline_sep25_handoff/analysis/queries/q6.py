import re, glob, collections
import pandas as pd
rows = []
for f in glob.glob('reports/*/*.txt'):
    run = f.split('/')[1]
    for line in open(f):
        m = re.match(r'd(\d+) status (\d+) fb (\d+) cash (-?\d+) \| new (\d+) (\d+) (\d+) (\d+) (\d+) \| animals (\d+)\+(\d+) (\d+)\+(\d+) (\d+)\+(\d+) \| land (\d) .*?\|(.*)$', line.strip())
        if not m: continue
        v = list(map(int, m.groups()[:16]))
        reason = m.group(17)
        tr = re.search(r'trimmed (\d+) new entities', reason)
        rows.append(dict(run=run, variant=run.split('_')[0], day=v[0], status=v[1], fb=v[2], cash=v[3], crops=sum(v[4:9]), melon=v[8],
                         goose=v[9], cow=v[11], sheep=v[13], land=v[15], trimmed=int(tr.group(1)) if tr else 0,
                         first=reason.split(';')[0][:60]))
r = pd.DataFrame(rows)
print(len(r), 'dawns')
print('\nshare of dawns with trims and mean trimmed units, by day (both variants):')
t = r.groupby(['variant', 'day']).agg(trim_share=('trimmed', lambda s: (s > 0).mean()), trimmed=('trimmed', 'mean'), fb=('fb', 'mean'),
                                     cash=('cash', 'median'), goose=('goose', 'mean'), cow=('cow', 'mean'), sheep=('sheep', 'mean'), land=('land', 'mean'), crops=('crops', 'mean'))
print(t.unstack(0).loc[:14].round(2).to_string())
print('\nfirst failure reason on trimmed dawns (counts):')
x = r[r.trimmed > 0].copy(); x['why'] = x.first.str.replace(r'\d+', 'N', regex=True)
print(x.groupby(['variant', 'why']).size().sort_values(ascending=False).head(20))
r.to_pickle('reports_days.pkl')
