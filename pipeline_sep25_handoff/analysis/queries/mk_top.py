import csv, collections, os
E = '../../experiments/v10/sep24_BC_opus'
lb = list(csv.DictReader(open(E + '/data/leaderboard/kaggriculture-publicleaderboard-2026-09-24T22:13:19.csv', encoding='utf-8-sig')))
rank = {r['TeamName']: int(r['Rank']) for r in lb}
rows = list(csv.DictReader(open(E + '/data/daily_perspectives.csv')))
ep = collections.defaultdict(dict)
for r in rows:
    if r['date'] >= '2026-09-18':
        ep[r['episode']][int(r['seat'])] = r
out, n = [], collections.Counter()
for e, s in ep.items():
    if len(s) != 2:
        continue
    t = [s[0]['team'], s[1]['team']]
    rk = [rank.get(x, 999) for x in t]
    if min(rk) > 10:
        continue
    path = f"{E}/data/traces/{e}.txt"
    if not os.path.exists(path):
        continue
    lab = [f"{x.replace(' ', '_')}|{r}" for x, r in zip(t, rk)]
    out.append(f"{path} {lab[0]} {lab[1]} top")
    for x, r in zip(t, rk):
        if r <= 10:
            n[x] += 1
open('top_list.txt', 'w').write('\n'.join(out) + '\n')
print(len(out), n.most_common())
